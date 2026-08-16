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

#include <algorithm>
#include <cstddef>

#include <arpa/inet.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <linux/inet_diag.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/sock_diag.h>
#include <linux/tcp.h>

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

    // per-process state: remember which GPU we are watching
    const GpuSource &src = m_gpuSources.first();
    m_gpuPdev = src.id.section(QLatin1Char('/'), -1).toLower();
    m_drmNodes = drmDeviceNodesOf(src.id);
    m_nvidiaIndex = -1;
    if (src.nvidiaSmi)
        m_nvidiaIndex = nvidiaIndexOf(m_gpuPdev);
    m_gpuEngineLast.clear();
    m_gpuProcesses.clear();
    m_gpuProcLastMsec = 0;
    emit gpuProcessesChanged();

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
    m_gpuProcesses.clear();
    m_gpuEngineLast.clear();
    m_gpuProcLastMsec = 0;
    emit gpuProcessesChanged();
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
    sampleGpuProcesses(s, vramTotal);
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
    m_netProcLast.clear();
    m_netProcesses.clear();
    m_netProcLastMsec = 0;
    emit netProcessesChanged();
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
    m_netProcesses.clear();
    m_netProcLast.clear();
    m_netProcLastMsec = 0;
    emit netProcessesChanged();
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
    sampleNetProcesses();
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

// ---------------------------------------------------------------------------
// Per-process usage
// ---------------------------------------------------------------------------

bool MonitorProbe::processesLimited() const
{
    return ::geteuid() != 0;
}

QString MonitorProbe::fdLinkTarget(const QString &path)
{
    const QByteArray p = QFile::encodeName(path);
    char buf[256];
    const ssize_t n = ::readlink(p.constData(), buf, sizeof(buf) - 1);
    if (n <= 0)
        return QString();
    buf[n] = '\0';
    return QString::fromLatin1(buf);
}

bool MonitorProbe::isLoopbackAddress(int family, const quint32 *addr)
{
    if (family == AF_INET)
        return (ntohl(addr[0]) >> 24) == 127;
    return addr[0] == 0 && addr[1] == 0 && addr[2] == 0 && ntohl(addr[3]) == 1;
}

bool MonitorProbe::isZeroAddress(int family, const quint32 *addr)
{
    if (family == AF_INET)
        return addr[0] == 0;
    return addr[0] == 0 && addr[1] == 0 && addr[2] == 0 && addr[3] == 0;
}

// Dumps one protocol/family table through netlink INET_DIAG (what `ss` uses).
// TCP entries carry the per-socket byte counters from tcp_info; UDP sockets
// come back with zero counters (the kernel has none) but still count as open.
QHash<qint64, MonitorProbe::SocketBytes> MonitorProbe::inetDiagQuery(int protocol, int family)
{
    QHash<qint64, SocketBytes> out;

    const int nl = ::socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_INET_DIAG);
    if (nl < 0)
        return out;
    timeval tv = { 0, 150000 };
    ::setsockopt(nl, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    char reqBuf[NLMSG_SPACE(sizeof(inet_diag_req_v2))] = {};
    nlmsghdr *nh = reinterpret_cast<nlmsghdr *>(reqBuf);
    nh->nlmsg_len = NLMSG_LENGTH(sizeof(inet_diag_req_v2));
    nh->nlmsg_type = SOCK_DIAG_BY_FAMILY;
    nh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    nh->nlmsg_seq = 1;
    inet_diag_req_v2 *req = reinterpret_cast<inet_diag_req_v2 *>(NLMSG_DATA(nh));
    req->sdiag_family = quint8(family);
    req->sdiag_protocol = quint8(protocol);
    req->idiag_states = 0xffffffff;
    // ask for INET_DIAG_INFO, otherwise the kernel omits tcp_info
    req->idiag_ext = 1 << (INET_DIAG_INFO - 1);

    sockaddr_nl kernel = {};
    kernel.nl_family = AF_NETLINK;
    if (::sendto(nl, reqBuf, nh->nlmsg_len, 0,
                 reinterpret_cast<sockaddr *>(&kernel), sizeof(kernel)) < 0) {
        ::close(nl);
        return out;
    }

    char buf[65536];
    bool done = false;
    while (!done) {
        const ssize_t n = ::recv(nl, buf, sizeof(buf), 0);
        if (n <= 0)
            break;
        unsigned int len = unsigned(n);
        for (nlmsghdr *m = reinterpret_cast<nlmsghdr *>(buf);
             NLMSG_OK(m, len); m = NLMSG_NEXT(m, len)) {
            if (m->nlmsg_type == NLMSG_DONE || m->nlmsg_type == NLMSG_ERROR) {
                done = true;
                break;
            }
            const inet_diag_msg *diag =
                reinterpret_cast<const inet_diag_msg *>(NLMSG_DATA(m));
            if (diag->idiag_inode == 0)
                continue;
            // loopback-only peers never touch the wire; skip them so local
            // IPC does not drown out real interface traffic
            if (isLoopbackAddress(family, diag->id.idiag_dst))
                continue;
            if (isZeroAddress(family, diag->id.idiag_dst)
                && isLoopbackAddress(family, diag->id.idiag_src)) {
                continue;
            }
            SocketBytes bytes;
            int attrLen = int(m->nlmsg_len) - int(NLMSG_LENGTH(sizeof(inet_diag_msg)));
            if (protocol == IPPROTO_TCP && attrLen > 0) {
                const rtattr *a = reinterpret_cast<const rtattr *>(
                    reinterpret_cast<const char *>(NLMSG_DATA(m))
                    + NLMSG_ALIGN(sizeof(inet_diag_msg)));
                for (; RTA_OK(a, attrLen); a = RTA_NEXT(a, attrLen)) {
                    if (a->rta_type != INET_DIAG_INFO)
                        continue;
                    const tcp_info *ti =
                        reinterpret_cast<const tcp_info *>(RTA_DATA(a));
                    const size_t plen = RTA_PAYLOAD(a);
                    if (plen >= offsetof(tcp_info, tcpi_bytes_received) + sizeof(quint64))
                        bytes.received = qint64(ti->tcpi_bytes_received);
                    if (plen >= offsetof(tcp_info, tcpi_bytes_sent) + sizeof(quint64))
                        bytes.sent = qint64(ti->tcpi_bytes_sent);
                }
            }
            out.insert(qint64(diag->idiag_inode), bytes);
        }
    }
    ::close(nl);
    return out;
}

// Maps socket inodes to owning PIDs through the /proc/<pid>/fd symlinks.
// Foreign processes are not readable without root; their sockets simply stay
// unmapped and end up in the aggregated "other processes" row.
QHash<qint64, int> MonitorProbe::socketOwners(QHash<int, QString> *names)
{
    QHash<qint64, int> out;
    DIR *proc = ::opendir("/proc");
    if (!proc)
        return out;
    struct dirent *de;
    while ((de = ::readdir(proc))) {
        const int pid = QByteArray(de->d_name).toInt();
        if (pid <= 0)
            continue;
        const QByteArray fdDir = "/proc/" + QByteArray::number(pid) + "/fd";
        DIR *d = ::opendir(fdDir.constData());
        if (!d)
            continue;
        bool named = false;
        struct dirent *fe;
        while ((fe = ::readdir(d))) {
            if (fe->d_name[0] == '.')
                continue;
            const QByteArray link = fdDir + '/' + fe->d_name;
            char buf[128];
            const ssize_t n = ::readlink(link.constData(), buf, sizeof(buf) - 1);
            if (n <= 10) // shorter than "socket:[0]"
                continue;
            buf[n] = '\0';
            if (qstrncmp(buf, "socket:[", 8) != 0)
                continue;
            buf[n - 1] = '\0'; // strip the trailing ']'
            const qint64 inode =
                QByteArray::fromRawData(buf + 8, int(n) - 9).toLongLong();
            if (inode <= 0 || out.contains(inode))
                continue;
            out.insert(inode, pid);
            if (!named && names && !names->contains(pid)) {
                names->insert(pid,
                    readFile(QStringLiteral("/proc/") + QString::number(pid)
                             + QStringLiteral("/comm")));
                named = true;
            }
        }
        ::closedir(d);
    }
    ::closedir(proc);
    return out;
}

QStringList MonitorProbe::drmDeviceNodesOf(const QString &anchor) const
{
    QStringList nodes;
    if (anchor.isEmpty())
        return nodes;
    const QDir drm(QStringLiteral("/sys/class/drm"));
    const QStringList names = drm.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &n : names) {
        if (n.contains(QLatin1Char('-')))
            continue; // "card0", "renderD128", not "card0-DP-1"
        if (canonicalPath(drm.absoluteFilePath(n) + QStringLiteral("/device")) == anchor)
            nodes << QStringLiteral("/dev/dri/") + n;
    }
    return nodes;
}

// Matches the monitored PCI address against nvidia-smi's GPU index so pmon
// rows can be filtered on multi-GPU systems.
int MonitorProbe::nvidiaIndexOf(const QString &pdev) const
{
    if (pdev.isEmpty())
        return -1;
    QProcess p;
    p.start(QStringLiteral("nvidia-smi"),
            { QStringLiteral("--query-gpu=index,pci.bus_id"),
              QStringLiteral("--format=csv,noheader,nounits") });
    if (!p.waitForFinished(2000))
        return -1;
    const QStringList lines =
        QString::fromUtf8(p.readAllStandardOutput()).split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        const QStringList parts = line.split(QLatin1Char(','), Qt::SkipEmptyParts);
        if (parts.size() < 2)
            continue;
        const QString bus = parts.at(1).trimmed().toLower();
        if (bus.endsWith(pdev.right(7))) // "0000:01:00.0" vs "00000000:01:00.0"
            return parts.at(0).trimmed().toInt();
    }
    return -1;
}

void MonitorProbe::sampleGpuProcesses(const GpuSource &s, double vramTotal)
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    QVariantList rows;

    // fdinfo values look like "3522050 ns" — parse the leading number
    const auto leadingInt = [](const QString &v) {
        return v.section(QLatin1Char(' '), 0, 0).toLongLong();
    };

    if (s.nvidiaSmi) {
        // "gpu pid type sm mem enc dec command"; sm/mem are percentages and
        // '-' when the process did not touch that unit in the sample window
        QProcess p;
        p.start(QStringLiteral("nvidia-smi"),
                { QStringLiteral("pmon"), QStringLiteral("-c"), QStringLiteral("1"),
                  QStringLiteral("-s"), QStringLiteral("um") });
        if (p.waitForFinished(2000)) {
            const QStringList lines =
                QString::fromUtf8(p.readAllStandardOutput()).split(QLatin1Char('\n'));
            QHash<QString, int> col;
            for (const QString &line : lines) {
                const QString t = line.trimmed();
                if (!t.startsWith(QLatin1Char('#')))
                    continue;
                const QStringList h = t.mid(1).split(
                    QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
                if (!h.contains(QStringLiteral("pid"))
                    || !h.contains(QStringLiteral("command"))) {
                    continue;
                }
                for (int i = 0; i < h.size(); ++i)
                    col.insert(h.at(i), i);
                break;
            }
            if (!col.isEmpty()) {
                const int need = qMax(qMax(col.value(QStringLiteral("gpu"), 0),
                                           col.value(QStringLiteral("pid"), 0)),
                                      qMax(col.value(QStringLiteral("sm"), 0),
                                           qMax(col.value(QStringLiteral("mem"), 0),
                                                col.value(QStringLiteral("command"), 0))));
                for (const QString &line : lines) {
                    const QString t = line.trimmed();
                    if (t.isEmpty() || t.startsWith(QLatin1Char('#')))
                        continue;
                    const QStringList f = t.split(
                        QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
                    if (f.size() <= need)
                        continue;
                    const int pid = f.at(col.value(QStringLiteral("pid"))).toInt();
                    if (pid <= 0)
                        continue;
                    if (m_nvidiaIndex >= 0
                        && f.at(col.value(QStringLiteral("gpu"))).toInt() != m_nvidiaIndex) {
                        continue;
                    }
                    const QString smField = f.at(col.value(QStringLiteral("sm"), 0));
                    const QString memField = f.at(col.value(QStringLiteral("mem"), 0));
                    const double sm = smField == QLatin1String("-")
                        ? 0.0 : smField.toDouble();
                    const double memPct = memField == QLatin1String("-")
                        ? 0.0 : memField.toDouble();
                    QVariantMap m;
                    m.insert(QStringLiteral("pid"), pid);
                    m.insert(QStringLiteral("name"),
                             f.at(col.value(QStringLiteral("command"))));
                    m.insert(QStringLiteral("usage"), sm);
                    m.insert(QStringLiteral("mem"),
                             vramTotal > 0.0 ? memPct / 100.0 * vramTotal : -1.0);
                    rows.append(m);
                }
            }
        }
        std::sort(rows.begin(), rows.end(), [](const QVariant &l, const QVariant &r) {
            const QVariantMap lm = l.toMap(), rm = r.toMap();
            if (lm.value(QStringLiteral("usage")).toDouble()
                != rm.value(QStringLiteral("usage")).toDouble()) {
                return lm.value(QStringLiteral("usage")).toDouble()
                    > rm.value(QStringLiteral("usage")).toDouble();
            }
            return lm.value(QStringLiteral("mem")).toDouble()
                > rm.value(QStringLiteral("mem")).toDouble();
        });
        while (rows.size() > kMaxProcRows)
            rows.removeLast();
        m_gpuProcesses = rows;
        emit gpuProcessesChanged();
        return;
    }

    // DRM fdinfo scan: each fd of a process that points into /dev/dri carries
    // "drm-engine-<engine>: <ns>" counters (busy deltas -> utilization) and
    // "drm-total-<region>: <bytes>" memory stats. Filter clients to the
    // monitored GPU by the fdinfo "drm-pdev" PCI address when present,
    // otherwise by the /dev/dri node of the fd.
    struct ProcAcc
    {
        qint64 engineDelta = 0;
        qint64 mem = 0;
        bool hasEngine = false;
        bool hasMem = false;
    };
    QHash<int, ProcAcc> acc;
    QHash<int, QString> names;
    QHash<QString, qint64> engineNow;

    const bool filterByPdev = !m_gpuPdev.isEmpty();
    const bool filterByNodes = !m_drmNodes.isEmpty();

    DIR *proc = ::opendir("/proc");
    if (proc) {
        struct dirent *de;
        while ((de = ::readdir(proc))) {
            const int pid = QByteArray(de->d_name).toInt();
            if (pid <= 0)
                continue;
            const QString pidStr = QString::number(pid);
            const QByteArray fdDirB = "/proc/" + QByteArray::number(pid) + "/fd";
            DIR *d = ::opendir(fdDirB.constData());
            if (!d)
                continue;
            struct dirent *fe;
            while ((fe = ::readdir(d))) {
                if (fe->d_name[0] == '.')
                    continue;
                const QString fdTarget = fdLinkTarget(
                    QString::fromLatin1(fdDirB + '/' + fe->d_name));
                if (!fdTarget.startsWith(QStringLiteral("/dev/dri/")))
                    continue;

                const QString fdName = QString::fromLatin1(fe->d_name);
                const QStringList info = readFile(QStringLiteral("/proc/") + pidStr
                    + QStringLiteral("/fdinfo/") + fdName)
                    .split(QLatin1Char('\n'));

                QString pdev;
                qint64 engineNs = 0;
                qint64 vramBytes = 0;
                qint64 allMem = 0;
                bool hasEngine = false, hasMem = false, hasVramKey = false;
                for (const QString &ln : info) {
                    const int colon = ln.indexOf(QLatin1Char(':'));
                    if (colon < 0)
                        continue;
                    const QString key = ln.left(colon).trimmed();
                    const QString val = ln.mid(colon + 1).trimmed();
                    if (key == QLatin1String("drm-pdev")) {
                        pdev = val.toLower();
                    } else if (key.startsWith(QLatin1String("drm-engine-"))) {
                        engineNs += leadingInt(val);
                        hasEngine = true;
                    } else if (key.startsWith(QLatin1String("drm-memory-"))
                               || key.startsWith(QLatin1String("drm-total-"))) {
                        const qint64 v = leadingInt(val);
                        allMem += v;
                        hasMem = true;
                        // dedicated VRAM regions if the driver has them,
                        // otherwise fall back to all memory regions
                        if (key.contains(QLatin1String("vram"))
                            || key.contains(QLatin1String("local"))) {
                            vramBytes += v;
                            hasVramKey = true;
                        }
                    }
                }
                if (!hasEngine && !hasMem)
                    continue; // e.g. a mode-setting-only fd without stats
                if (filterByPdev) {
                    if (!pdev.isEmpty()) {
                        if (pdev != m_gpuPdev)
                            continue;
                    } else if (filterByNodes && !m_drmNodes.contains(fdTarget)) {
                        continue;
                    }
                }

                ProcAcc &a = acc[pid];
                a.hasEngine = a.hasEngine || hasEngine;
                if (hasMem) {
                    a.hasMem = true;
                    a.mem += hasVramKey ? vramBytes : allMem;
                }
                const QString key = pidStr + QLatin1Char(':') + fdName;
                const qint64 prev = m_gpuEngineLast.value(key, -1);
                if (hasEngine && prev >= 0)
                    a.engineDelta += qMax<qint64>(0, engineNs - prev);
                if (hasEngine)
                    engineNow.insert(key, engineNs);
                if (!names.contains(pid)) {
                    names.insert(pid, readFile(QStringLiteral("/proc/") + pidStr
                                               + QStringLiteral("/comm")));
                }
            }
            ::closedir(d);
        }
        ::closedir(proc);
    }

    const double dt = m_gpuProcLastMsec > 0
        ? (now - m_gpuProcLastMsec) / 1000.0 : 0.0;
    for (auto it = acc.begin(); it != acc.end(); ++it) {
        const ProcAcc &a = it.value();
        if (!a.hasEngine && a.mem <= 0)
            continue;
        const double usage = dt > 0.0
            ? qMin(100.0, a.engineDelta / 1000000000.0 / dt * 100.0) : 0.0;
        const QString name = names.value(it.key());
        QVariantMap m;
        m.insert(QStringLiteral("pid"), it.key());
        m.insert(QStringLiteral("name"),
                 name.isEmpty() ? QString::number(it.key()) : name);
        m.insert(QStringLiteral("usage"), usage);
        m.insert(QStringLiteral("mem"), a.hasMem ? double(a.mem) : -1.0);
        rows.append(m);
    }
    std::sort(rows.begin(), rows.end(), [](const QVariant &l, const QVariant &r) {
        const QVariantMap lm = l.toMap(), rm = r.toMap();
        if (lm.value(QStringLiteral("usage")).toDouble()
            != rm.value(QStringLiteral("usage")).toDouble()) {
            return lm.value(QStringLiteral("usage")).toDouble()
                > rm.value(QStringLiteral("usage")).toDouble();
        }
        return lm.value(QStringLiteral("mem")).toDouble()
            > rm.value(QStringLiteral("mem")).toDouble();
    });
    while (rows.size() > kMaxProcRows)
        rows.removeLast();

    m_gpuProcesses = rows;
    emit gpuProcessesChanged();
    m_gpuEngineLast = engineNow;
    m_gpuProcLastMsec = now;
}

void MonitorProbe::sampleNetProcesses()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    QHash<int, QString> names;
    const QHash<qint64, int> owners = socketOwners(&names);

    QHash<qint64, SocketBytes> sockets;
    const int families[2] = { AF_INET, AF_INET6 };
    for (int family : families) {
        const QHash<qint64, SocketBytes> tcp = inetDiagQuery(IPPROTO_TCP, family);
        for (auto it = tcp.begin(); it != tcp.end(); ++it)
            sockets.insert(it.key(), it.value());
        const QHash<qint64, SocketBytes> udp = inetDiagQuery(IPPROTO_UDP, family);
        for (auto it = udp.begin(); it != udp.end(); ++it)
            sockets.insert(it.key(), it.value());
    }

    struct NetAcc
    {
        qint64 inB = 0;
        qint64 outB = 0;
        int count = 0;
    };
    QHash<int, NetAcc> acc; // pid; -1 aggregates sockets of unreadable PIDs
    for (auto it = sockets.begin(); it != sockets.end(); ++it) {
        NetAcc &a = acc[owners.value(it.key(), -1)];
        a.inB += it.value().received;
        a.outB += it.value().sent;
        ++a.count;
    }

    const double dt = m_netProcLastMsec > 0
        ? (now - m_netProcLastMsec) / 1000.0 : 0.0;
    QVariantList rows;
    QHash<int, QPair<qint64, qint64>> snapshot;
    for (auto it = acc.begin(); it != acc.end(); ++it) {
        const NetAcc &a = it.value();
        const QPair<qint64, qint64> prev =
            m_netProcLast.value(it.key(), qMakePair<qint64, qint64>(0, 0));
        const double rx = dt > 0.0
            ? qMax(0.0, (a.inB - prev.first) / dt) : 0.0;
        const double tx = dt > 0.0
            ? qMax(0.0, (a.outB - prev.second) / dt) : 0.0;
        const int pid = it.key();
        const QString name = pid == -1
            ? Translator::translate(QStringLiteral("otherProcesses"))
            : names.value(pid);
        QVariantMap m;
        m.insert(QStringLiteral("pid"), pid);
        m.insert(QStringLiteral("name"),
                 name.isEmpty() ? QString::number(pid) : name);
        m.insert(QStringLiteral("rx"), rx);
        m.insert(QStringLiteral("tx"), tx);
        m.insert(QStringLiteral("sockets"), a.count);
        rows.append(m);
        snapshot.insert(pid, qMakePair(a.inB, a.outB));
    }
    std::sort(rows.begin(), rows.end(), [](const QVariant &l, const QVariant &r) {
        const QVariantMap lm = l.toMap(), rm = r.toMap();
        const double lRate = lm.value(QStringLiteral("rx")).toDouble()
            + lm.value(QStringLiteral("tx")).toDouble();
        const double rRate = rm.value(QStringLiteral("rx")).toDouble()
            + rm.value(QStringLiteral("tx")).toDouble();
        if (lRate != rRate)
            return lRate > rRate;
        return lm.value(QStringLiteral("sockets")).toInt()
            > rm.value(QStringLiteral("sockets")).toInt();
    });
    while (rows.size() > kMaxProcRows)
        rows.removeLast();

    m_netProcesses = rows;
    emit netProcessesChanged();
    m_netProcLast = snapshot;
    m_netProcLastMsec = now;
}