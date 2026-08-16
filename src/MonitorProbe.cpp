#include "MonitorProbe.h"
#include "Translator.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QTimer>

namespace {

// PCI vendor id (hex without "0x") -> display name
const QHash<QString, QString> kVendors = {
    { QStringLiteral("8086"), QStringLiteral("Intel") },
    { QStringLiteral("10de"), QStringLiteral("NVIDIA") },
    { QStringLiteral("1002"), QStringLiteral("AMD") },
    { QStringLiteral("1af4"), QStringLiteral("Red Hat") },
    { QStringLiteral("1b36"), QStringLiteral("Red Hat") },
    { QStringLiteral("15ad"), QStringLiteral("VMware") },
    { QStringLiteral("1234"), QStringLiteral("Cirrus Logic") },
    { QStringLiteral("1ae0"), QStringLiteral("Google") },
};

// Kernel driver -> translation key (reuses the DeviceManager driver table keys)
const QHash<QString, QString> kDriverNames = {
    { QStringLiteral("i915"), QStringLiteral("intelIntegratedGraphics") },
    { QStringLiteral("amdgpu"), QStringLiteral("amdGraphics") },
    { QStringLiteral("radeon"), QStringLiteral("amdGraphics") },
    { QStringLiteral("nvidia"), QStringLiteral("nvidiaGraphics") },
    { QStringLiteral("nouveau"), QStringLiteral("nvidiaGraphics") },
    { QStringLiteral("vmwgfx"), QStringLiteral("vmwareGraphics") },
    { QStringLiteral("virtio_gpu"), QStringLiteral("virtioGraphics") },
    { QStringLiteral("qxl"), QStringLiteral("qxlGraphics") },
    { QStringLiteral("simpledrm"), QStringLiteral("simpleFramebuffer") },
    { QStringLiteral("efifb"), QStringLiteral("simpleFramebuffer") },
};

} // namespace

MonitorProbe::MonitorProbe(QObject *parent)
    : QObject(parent)
{
    m_gpuTimer = new QTimer(this);
    m_gpuTimer->setInterval(1000);
    connect(m_gpuTimer, &QTimer::timeout, this, &MonitorProbe::sampleGpu);

    m_netTimer = new QTimer(this);
    m_netTimer->setInterval(1000);
    connect(m_netTimer, &QTimer::timeout, this, &MonitorProbe::sampleNet);

    detectGpus();
    updateNetInterfaces();
}

MonitorProbe::~MonitorProbe()
{
    teardownIntelTopProcess();
}

QString MonitorProbe::readFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return QString();
    return QString::fromUtf8(f.readAll()).trimmed();
}

QString MonitorProbe::canonicalPath(const QString &path)
{
    return QFileInfo(path).canonicalFilePath();
}

QString MonitorProbe::driverOf(const QString &anchor)
{
    QFileInfo fi(anchor + QStringLiteral("/driver"));
    if (!fi.isSymLink())
        return QString();
    return QFileInfo(fi.symLinkTarget()).fileName();
}

QString MonitorProbe::gpuNameFor(const QString &vendorHex, const QString &driver)
{
    const QString key = kDriverNames.value(driver);
    if (!key.isEmpty())
        return Translator::translate(key);
    if (!vendorHex.isEmpty())
        return kVendors.value(vendorHex, vendorHex);
    return driver.isEmpty() ? QStringLiteral("GPU") : driver;
}

// ---------------------------------------------------------------------------
// GPU detection
// ---------------------------------------------------------------------------

void MonitorProbe::detectGpus()
{
    m_gpus.clear();
    QSet<QString> seen;

    const QDir drm(QStringLiteral("/sys/class/drm"));
    const QStringList names = drm.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &n : names) {
        if (!n.startsWith(QStringLiteral("card")) || n.contains(QLatin1Char('-')))
            continue; // "card0", not "card0-DP-1"
        const QString anchor = canonicalPath(drm.absoluteFilePath(n) + QStringLiteral("/device"));
        if (anchor.isEmpty() || seen.contains(anchor))
            continue;
        seen.insert(anchor);

        const QString driver = driverOf(anchor);
        QString vendorHex = readFile(anchor + QStringLiteral("/vendor"));
        vendorHex.remove(QLatin1String("0x"));
        vendorHex = vendorHex.toLower();

        GpuSource s;
        s.id = anchor;
        s.name = gpuNameFor(vendorHex, driver);
        s.driver = driver;

        // amdgpu / radeon expose utilization and VRAM through sysfs
        const QString busy = anchor + QStringLiteral("/gpu_busy_percent");
        if (QFile::exists(busy)) {
            s.busySupported = true;
            s.busyPath = busy;
        }
        const QString vramUsed = anchor + QStringLiteral("/mem_info_vram_used");
        const QString vramTotal = anchor + QStringLiteral("/mem_info_vram_total");
        if (QFile::exists(vramUsed) && QFile::exists(vramTotal)) {
            s.vramSupported = true;
            s.vramUsedPath = vramUsed;
            s.vramTotalPath = vramTotal;
        }

        // Intel i915 needs the intel_gpu_top tool; NVIDIA needs nvidia-smi
        if (s.driver == QLatin1String("i915") || s.driver == QLatin1String("xe")) {
            if (!QStandardPaths::findExecutable(QStringLiteral("intel_gpu_top")).isEmpty()) {
                s.busySupported = true;
                s.intelTop = true;
            }
        } else if (s.driver == QLatin1String("nvidia")) {
            if (!QStandardPaths::findExecutable(QStringLiteral("nvidia-smi")).isEmpty()) {
                s.busySupported = true;
                s.vramSupported = true;
                s.nvidiaSmi = true;
            }
        }

        const bool monitorable = s.busySupported || s.vramSupported;
        QVariantMap m;
        m.insert(QStringLiteral("id"), s.id);
        m.insert(QStringLiteral("name"), s.name);
        m.insert(QStringLiteral("driver"), s.driver);
        m.insert(QStringLiteral("busySupported"), s.busySupported);
        m.insert(QStringLiteral("vramSupported"), s.vramSupported);
        m.insert(QStringLiteral("monitorable"), monitorable);
        m_gpus.append(m);
    }
}

bool MonitorProbe::supportsGpuMonitoring(const QVariantMap &device) const
{
    // match the (cheap) cached detection list instead of probing files per call
    const QString anchor = device.value(QStringLiteral("sysfsPath")).toString();
    for (const QVariant &gv : m_gpus) {
        const QVariantMap g = gv.toMap();
        if (g.value(QStringLiteral("id")).toString() == anchor)
            return g.value(QStringLiteral("monitorable")).toBool();
    }
    return false;
}

// ---------------------------------------------------------------------------
// GPU sampling
// ---------------------------------------------------------------------------

void MonitorProbe::setupGpuSources(const QVariantMap &device)
{
    m_gpuSources.clear();
    const QString anchor = device.value(QStringLiteral("sysfsPath")).toString();
    if (anchor.isEmpty())
        return;

    // The DRM card whose /device symlink resolves to `anchor`.
    const QDir drm(QStringLiteral("/sys/class/drm"));
    const QStringList names = drm.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    QString cardAnchor;
    for (const QString &n : names) {
        if (!n.startsWith(QStringLiteral("card")) || n.contains(QLatin1Char('-')))
            continue;
        const QString a = canonicalPath(drm.absoluteFilePath(n) + QStringLiteral("/device"));
        if (a == anchor) {
            cardAnchor = a;
            break;
        }
    }
    if (cardAnchor.isEmpty())
        return;

    GpuSource s;
    s.id = cardAnchor;
    s.driver = driverOf(cardAnchor);
    s.name = gpuNameFor(readFile(cardAnchor + QStringLiteral("/vendor")), s.driver);

    const QString busy = cardAnchor + QStringLiteral("/gpu_busy_percent");
    if (QFile::exists(busy)) {
        s.busySupported = true;
        s.busyPath = busy;
    }
    const QString vramUsed = cardAnchor + QStringLiteral("/mem_info_vram_used");
    const QString vramTotal = cardAnchor + QStringLiteral("/mem_info_vram_total");
    if (QFile::exists(vramUsed) && QFile::exists(vramTotal)) {
        s.vramSupported = true;
        s.vramUsedPath = vramUsed;
        s.vramTotalPath = vramTotal;
    }

    if (s.driver == QLatin1String("i915") || s.driver == QLatin1String("xe")) {
        if (!QStandardPaths::findExecutable(QStringLiteral("intel_gpu_top")).isEmpty()) {
            s.busySupported = true;
            s.intelTop = true;
        }
    } else if (s.driver == QLatin1String("nvidia")) {
        if (!QStandardPaths::findExecutable(QStringLiteral("nvidia-smi")).isEmpty()) {
            s.busySupported = true;
            s.vramSupported = true;
            s.nvidiaSmi = true;
        }
    }

    m_gpuSources.append(s);
}

void MonitorProbe::ensureIntelTopProcess()
{
    if (m_intelTop)
        return;
    m_intelTop = new QProcess(this);
    connect(m_intelTop, &QProcess::readyReadStandardOutput, this, [this]() {
        m_intelBuffer += m_intelTop->readAllStandardOutput();
        int nl = -1;
        while ((nl = m_intelBuffer.indexOf('\n')) >= 0) {
            const QByteArray line = m_intelBuffer.left(nl).trimmed();
            m_intelBuffer.remove(0, nl + 1);
            if (line.isEmpty())
                continue;
            const QJsonDocument doc = QJsonDocument::fromJson(line);
            if (!doc.isObject())
                continue;
            const QJsonArray engines = doc.object().value(QStringLiteral("engines")).toArray();
            double busy = 0.0;
            for (const QJsonValue &ev : engines) {
                const double b = ev.toObject().value(QStringLiteral("busy")).toDouble();
                if (b > busy)
                    busy = b;
            }
            m_intelBusy = busy;
        }
    });
    connect(m_intelTop, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        m_intelBusy = qQNaN();
    });
    m_intelTop->start(QStringLiteral("intel_gpu_top"), { QStringLiteral("-J"), QStringLiteral("-o"), QStringLiteral("-") });
}

void MonitorProbe::teardownIntelTopProcess()
{
    if (!m_intelTop)
        return;
    m_intelTop->kill();
    m_intelTop->waitForFinished(500);
    m_intelTop->deleteLater();
    m_intelTop = nullptr;
    m_intelBuffer.clear();
    m_intelBusy = qQNaN();
}

void MonitorProbe::startGpu(const QVariantMap &device)
{
    stopGpu();
    setupGpuSources(device);
    m_gpuSamples.clear();
    m_gpuHistory.clear();

    if (m_gpuSources.isEmpty() || !(m_gpuSources.first().busySupported || m_gpuSources.first().vramSupported))
        return;

    if (m_gpuSources.first().intelTop)
        ensureIntelTopProcess();

    m_gpuRunning = true;
    emit gpuRunningChanged();
    m_gpuTimer->start();
    sampleGpu(); // grab the first point right away
}

void MonitorProbe::stopGpu()
{
    m_gpuTimer->stop();
    teardownIntelTopProcess();
    if (m_gpuRunning) {
        m_gpuRunning = false;
        emit gpuRunningChanged();
    }
}

void MonitorProbe::clearGpu()
{
    m_gpuSamples.clear();
    m_gpuHistory.clear();
    emit gpuDataChanged();
}

void MonitorProbe::sampleGpu()
{
    if (m_gpuSources.isEmpty())
        return;
    const GpuSource &s = m_gpuSources.first();

    double usage = qQNaN();
    double vramUsed = qQNaN();
    double vramTotal = qQNaN();

    if (s.nvidiaSmi) {
        QProcess p;
        p.start(QStringLiteral("nvidia-smi"),
                { QStringLiteral("--query-gpu=utilization.gpu,memory.used,memory.total"),
                  QStringLiteral("--format=csv,noheader,nounits") });
        if (p.waitForFinished(2000)) {
            const QString out = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
            const QStringList parts = out.split(QLatin1Char(','), Qt::SkipEmptyParts);
            if (parts.size() >= 3) {
                bool ok = false;
                const double u = parts.at(0).trimmed().toDouble(&ok);
                if (ok)
                    usage = u;
                const double used = parts.at(1).trimmed().toDouble(&ok);
                const double total = parts.at(2).trimmed().toDouble(&ok);
                if (ok) {
                    vramUsed = used * 1024.0 * 1024.0;   // nvidia-smi reports MiB
                    vramTotal = total * 1024.0 * 1024.0;
                }
            }
        }
    } else {
        if (s.busySupported) {
            if (s.intelTop) {
                usage = m_intelBusy;
            } else if (!s.busyPath.isEmpty()) {
                bool ok = false;
                const double v = readFile(s.busyPath).toDouble(&ok);
                usage = ok ? v : qQNaN();
            }
        }
        if (s.vramSupported) {
            bool ok = false;
            const double used = readFile(s.vramUsedPath).toLongLong(&ok);
            vramUsed = ok ? double(used) : qQNaN();
            ok = false;
            const double total = readFile(s.vramTotalPath).toLongLong(&ok);
            vramTotal = ok ? double(total) : qQNaN();
        }
    }

    QVector<double> sample;
    sample << usage << vramUsed << vramTotal;
    m_gpuSamples.append(sample);
    while (m_gpuSamples.size() > kMaxSamples)
        m_gpuSamples.removeFirst();

    rebuildGpuHistory();
    emit gpuDataChanged();
}

void MonitorProbe::rebuildGpuHistory()
{
    m_gpuHistory.clear();
    const int n = m_gpuSamples.size();
    for (int i = 0; i < n; ++i) {
        QVariantMap m;
        m.insert(QStringLiteral("time"), i - (n - 1));
        m.insert(QStringLiteral("usage"), m_gpuSamples[i][0]);
        m.insert(QStringLiteral("vramUsed"), m_gpuSamples[i][1]);
        m.insert(QStringLiteral("vramTotal"), m_gpuSamples[i][2]);
        m_gpuHistory.append(m);
    }
}

// ---------------------------------------------------------------------------
// Network
// ---------------------------------------------------------------------------

QHash<QString, MonitorProbe::NetCounters> MonitorProbe::readProcNetDev()
{
    QHash<QString, NetCounters> out;
    QFile f(QStringLiteral("/proc/net/dev"));
    if (!f.open(QIODevice::ReadOnly))
        return out;
    const QStringList lines = QString::fromUtf8(f.readAll()).split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        const int colon = line.indexOf(QLatin1Char(':'));
        if (colon < 0)
            continue;
        const QString name = line.left(colon).trimmed();
        const QStringList parts = line.mid(colon + 1).trimmed().split(QRegularExpression(QStringLiteral("\\s+")));
        if (parts.size() < 10)
            continue;
        bool rxOk = false, txOk = false;
        NetCounters c;
        c.rx = parts.at(0).toLongLong(&rxOk);
        c.tx = parts.at(8).toLongLong(&txOk);
        if (rxOk && txOk)
            out.insert(name, c);
    }
    return out;
}

void MonitorProbe::updateNetInterfaces()
{
    m_netInterfaces.clear();
    const QHash<QString, NetCounters> cur = readProcNetDev();
    QStringList names = cur.keys();
    names.sort();
    for (const QString &name : names) {
        const NetCounters c = cur.value(name);
        QVariantMap m;
        m.insert(QStringLiteral("name"), name);
        m.insert(QStringLiteral("rxTotal"), double(c.rx));
        m.insert(QStringLiteral("txTotal"), double(c.tx));
        m_netInterfaces.append(m);
    }
    m_netCounters = cur;
}

bool MonitorProbe::supportsNetMonitoring(const QVariantMap &device) const
{
    const QString iface = device.value(QStringLiteral("entryName")).toString();
    return !iface.isEmpty() && m_netCounters.contains(iface);
}

void MonitorProbe::startNet(const QString &iface)
{
    stopNet();
    m_netIface = iface;

    updateNetInterfaces();
    m_netLast = m_netCounters;
    m_netSamples.clear();
    m_netHistory.clear();

    if (!m_netCounters.contains(iface)) {
        emit netDataChanged();
        return;
    }

    m_netLastMsec = QDateTime::currentMSecsSinceEpoch();
    m_netRunning = true;
    emit netRunningChanged();
    m_netTimer->start();
    sampleNet(); // grab the first point right away
}

void MonitorProbe::stopNet()
{
    m_netTimer->stop();
    if (m_netRunning) {
        m_netRunning = false;
        emit netRunningChanged();
    }
}

void MonitorProbe::clearNet()
{
    m_netSamples.clear();
    m_netHistory.clear();
    emit netDataChanged();
}

void MonitorProbe::sampleNet()
{
    if (m_netIface.isEmpty())
        return;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const double dt = (now - m_netLastMsec) / 1000.0;
    m_netLastMsec = now;

    updateNetInterfaces();
    const NetCounters cur = m_netCounters.value(m_netIface);
    const NetCounters last = m_netLast.value(m_netIface);
    m_netLast = m_netCounters;

    double rx = 0.0, tx = 0.0;
    if (dt > 0.0) {
        rx = qMax(0.0, double(cur.rx - last.rx) / dt);
        tx = qMax(0.0, double(cur.tx - last.tx) / dt);
    }

    QVariantMap sample;
    sample.insert(QStringLiteral("time"), 0); // filled in rebuildNetHistory
    sample.insert(QStringLiteral("rx"), rx);
    sample.insert(QStringLiteral("tx"), tx);
    sample.insert(QStringLiteral("rxTotal"), double(cur.rx));
    sample.insert(QStringLiteral("txTotal"), double(cur.tx));
    m_netSamples.append(sample);
    while (m_netSamples.size() > kMaxSamples)
        m_netSamples.removeFirst();

    rebuildNetHistory();
    emit netDataChanged();
}

void MonitorProbe::rebuildNetHistory()
{
    m_netHistory.clear();
    const int n = m_netSamples.size();
    for (int i = 0; i < n; ++i) {
        QVariantMap m = m_netSamples[i];
        m.insert(QStringLiteral("time"), i - (n - 1));
        m_netHistory.append(m);
    }
}