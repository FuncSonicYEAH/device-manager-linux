#include "DriverHelper.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTimer>

#include <algorithm>

// ---------------------------------------------------------------------------
// hwdata (pci.ids / usb.ids) lookup: vendor and device display names
// ---------------------------------------------------------------------------

namespace {

struct HwIdsDb
{
    QHash<QString, QString> pciVendors, pciDevices;   // devices keyed "vendor:device"
    QHash<QString, QString> usbVendors, usbDevices;
    bool loaded = false;

    static HwIdsDb &instance()
    {
        static HwIdsDb db;
        if (!db.loaded)
            db.load();
        return db;
    }

    void load()
    {
        loaded = true;
        parse(QStringLiteral("pci.ids"), &pciVendors, &pciDevices);
        parse(QStringLiteral("usb.ids"), &usbVendors, &usbDevices);
    }

    static QString firstExisting(const QString &name)
    {
        const QStringList bases = {
            QStringLiteral("/usr/share/hwdata/") + name,
            QStringLiteral("/usr/share/misc/") + name,
        };
        for (const QString &b : bases)
            if (QFileInfo::exists(b))
                return b;
        return QString();
    }

    void parse(const QString &fileName, QHash<QString, QString> *vendors,
               QHash<QString, QString> *devices)
    {
        const QString path = firstExisting(fileName);
        if (path.isEmpty())
            return;
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly))
            return;

        QString vendor;
        bool inClassSection = false;
        while (!f.atEnd()) {
            const QString line = QString::fromLatin1(f.readLine());
            if (line.startsWith(QLatin1Char('#')))
                continue;
            // plain "idxx  name" lines are vendors; "C xx ..." starts a
            // class-code section whose sub-lines must not be read as devices
            if (!line.startsWith(QLatin1Char('\t')) && !line.startsWith(QLatin1Char(' '))) {
                // ids files separate id and name with two spaces
                const QStringList parts =
                    line.trimmed().split(QLatin1Char(' '), Qt::SkipEmptyParts);
                if (parts.size() >= 2 && parts.at(0).size() == 4) {
                    vendor = parts.at(0).toLower();
                    vendors->insert(vendor, parts.mid(1).join(QLatin1Char(' ')));
                    inClassSection = false;
                } else {
                    inClassSection = true;
                }
                continue;
            }
            if (inClassSection)
                continue;
            // exactly one tab + "idxx  name" = device of the current vendor
            if (line.startsWith(QLatin1Char('\t')) && !line.startsWith(QLatin1String("\t\t"))
                && !vendor.isEmpty()) {
                const QStringList parts =
                    line.trimmed().split(QLatin1Char(' '), Qt::SkipEmptyParts);
                if (parts.size() >= 2 && parts.at(0).size() == 4)
                    devices->insert(vendor + QLatin1Char(':') + parts.at(0).toLower(),
                                    parts.mid(1).join(QLatin1Char(' ')));
            }
        }
    }
};

QString readFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return QString();
    return QString::fromUtf8(f.readAll()).trimmed();
}

// fnmatch-style glob used by modules.alias patterns: '*' any run, '?' one
// char, '[...]' character class with '!'/'^' negation and a-z ranges.
bool globMatch(const QString &p, int pi, const QString &s, int si)
{
    const int pn = p.size(), sn = s.size();
    while (pi < pn) {
        const QChar c = p.at(pi);
        if (c == QLatin1Char('*')) {
            while (pi < pn && p.at(pi) == QLatin1Char('*'))
                ++pi;
            if (pi >= pn)
                return true;
            for (int i = si; i <= sn; ++i)
                if (globMatch(p, pi, s, i))
                    return true;
            return false;
        }
        if (si >= sn)
            return false;
        if (c == QLatin1Char('?')) {
            ++pi;
            ++si;
            continue;
        }
        if (c == QLatin1Char('[')) {
            int end = pi + 1;
            bool negate = false;
            if (end < pn && (p.at(end) == QLatin1Char('!') || p.at(end) == QLatin1Char('^'))) {
                negate = true;
                ++end;
            }
            bool matched = false;
            bool first = true;
            while (end < pn && (first || p.at(end) != QLatin1Char(']'))) {
                first = false;
                if (end + 2 < pn && p.at(end + 1) == QLatin1Char('-')
                    && p.at(end + 2) != QLatin1Char(']')) {
                    if (s.at(si) >= p.at(end) && s.at(si) <= p.at(end + 2))
                        matched = true;
                    end += 3;
                } else {
                    if (s.at(si) == p.at(end))
                        matched = true;
                    ++end;
                }
            }
            if (end >= pn) // unterminated class
                return false;
            if (matched == negate)
                return false;
            pi = end + 1;
            ++si;
            continue;
        }
        if (s.at(si) != c)
            return false;
        ++pi;
        ++si;
    }
    return si == sn;
}

QString symLinkName(const QString &path)
{
    const QFileInfo fi(path);
    if (!fi.isSymLink())
        return QString();
    return QFileInfo(fi.symLinkTarget()).fileName();
}

QString readKernelRelease()
{
    QString rel = readFile(QStringLiteral("/proc/sys/kernel/osrelease"));
    if (rel.isEmpty())
        rel = QSysInfo::kernelVersion();
    return rel;
}

QString humanSize(qint64 bytes)
{
    if (bytes >= 1024 * 1024)
        return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + QStringLiteral(" MB");
    if (bytes >= 1024)
        return QString::number(bytes / 1024.0, 'f', 0) + QStringLiteral(" KB");
    return QString::number(bytes) + QStringLiteral(" B");
}

} // namespace

// ---------------------------------------------------------------------------
// DriverHelper
// ---------------------------------------------------------------------------

DriverHelper::DriverHelper(QObject *parent)
    : QObject(parent)
{
    detectBackend();
}

void DriverHelper::detectBackend()
{
    struct Cand {
        const char *exe;
        const char *id;
        const char *name;
    };
    static const QList<Cand> cands = {
        { "dnf5", "dnf", "DNF" },
        { "dnf", "dnf", "DNF" },
        { "apt", "apt", "APT" },
        { "apt-get", "apt", "APT" },
        { "pacman", "pacman", "pacman" },
        { "zypper", "zypper", "ZYpp" },
        { "apk", "apk", "APK" },
        { "emerge", "emerge", "Portage" },
    };
    for (const Cand &c : cands) {
        const QString path = QStandardPaths::findExecutable(QLatin1String(c.exe));
        if (!path.isEmpty()) {
            m_backend = QLatin1String(c.id);
            m_backendName = QLatin1String(c.name);
            m_backendPath = path;
            break;
        }
    }
    if (m_backendPath.isEmpty()) {
        m_backend = QStringLiteral("unknown");
        m_backendName = QString();
    }
    // closed-source driver helpers
    m_yayPath = QStandardPaths::findExecutable(QStringLiteral("yay"));
    m_ubuntuDriversPath = QStandardPaths::findExecutable(QStringLiteral("ubuntu-drivers"));
}

// ---------------------------------------------------------------------------
// scan
// ---------------------------------------------------------------------------

void DriverHelper::scan()
{
    m_kernelRelease = readKernelRelease();
    m_lastError.clear();
    m_missing.clear();
    m_modules.clear();

    // module alias database -------------------------------------------------
    QList<AliasEntry> aliases;
    QSet<QString> builtin;
    const QString modBase = QStringLiteral("/lib/modules/") + m_kernelRelease;

    // /proc and /sys files report size 0, so atEnd() lies — always readAll()
    const QString aliasData = readFile(modBase + QStringLiteral("/modules.alias"));
    if (!aliasData.isEmpty()) {
        for (const QString &line : aliasData.split(QLatin1Char('\n'))) {
            const QStringList parts = line.simplified().split(QLatin1Char(' '));
            if (parts.size() >= 3 && parts.at(0) == QLatin1String("alias"))
                aliases.append({ parts.at(1), parts.at(2) });
        }
    } else {
        m_lastError = QStringLiteral("modules.alias not found for kernel %1").arg(m_kernelRelease);
    }

    const QString builtinData = readFile(modBase + QStringLiteral("/modules.builtin"));
    for (const QString &raw : builtinData.split(QLatin1Char('\n'))) {
        const QString name = raw.simplified().section(QLatin1Char('/'), -1);
        const int ko = name.indexOf(QStringLiteral(".ko"));
        if (ko > 0)
            builtin.insert(name.left(ko));
    }

    // loaded modules from /proc/modules --------------------------------------
    QSet<QString> loaded;
    const QString modsData = readFile(QStringLiteral("/proc/modules"));
    for (const QString &raw : modsData.split(QLatin1Char('\n'))) {
        const QStringList parts = raw.simplified().split(QLatin1Char(' '));
        if (parts.isEmpty() || parts.at(0).isEmpty())
            continue;
        loaded.insert(parts.at(0));
        QVariantMap m;
        m.insert(QStringLiteral("name"), parts.at(0));
        m.insert(QStringLiteral("size"), parts.size() > 1
                      ? humanSize(parts.at(1).toLongLong()) : QString());
        m.insert(QStringLiteral("usedCount"), parts.size() > 2 ? parts.at(2).toInt() : 0);
        m.insert(QStringLiteral("usedBy"), parts.size() > 3 && parts.at(3) != QLatin1String("-")
                     ? parts.at(3) : QString());
        m.insert(QStringLiteral("state"), parts.size() > 4 ? parts.at(4) : QString());
        m_modules.append(m);
    }
    std::sort(m_modules.begin(), m_modules.end(),
              [](const QVariant &a, const QVariant &b) {
                  return a.toMap().value(QStringLiteral("name")).toString()
                       < b.toMap().value(QStringLiteral("name")).toString();
              });

    scanBusDevices(QStringLiteral("pci"), aliases, builtin, loaded, &m_missing);
    scanBusDevices(QStringLiteral("usb"), aliases, builtin, loaded, &m_missing);

    emit scanChanged();
}

void DriverHelper::scanBusDevices(const QString &bus, const QList<AliasEntry> &aliases,
                                  const QSet<QString> &builtin, const QSet<QString> &loaded,
                                  QVariantList *out)
{
    const QDir dir(QStringLiteral("/sys/bus/") + bus + QStringLiteral("/devices"));
    const QStringList names = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    const HwIdsDb &ids = HwIdsDb::instance();

    for (const QString &name : names) {
        const QString entryPath = dir.filePath(name);
        const QString anchor = QFileInfo(entryPath).canonicalFilePath();
        if (anchor.isEmpty())
            continue;

        // USB interface nodes (3-1:1.0) are functions, root hubs (usb1) are
        // handled by the usb core; neither is a "missing driver" candidate.
        if (bus == QLatin1String("usb")
            && (name.contains(QLatin1Char(':')) || name.startsWith(QStringLiteral("usb"))))
            continue;

        // bound? (the device itself, or for USB any interface node)
        QString driver = symLinkName(anchor + QStringLiteral("/driver"));
        if (driver == QLatin1String("usb") || driver == QLatin1String("hub"))
            driver.clear();
        QStringList modaliases;
        const QString devAlias = readFile(anchor + QStringLiteral("/modalias"));
        if (!devAlias.isEmpty())
            modaliases.append(devAlias);
        if (bus == QLatin1String("usb") && driver.isEmpty()) {
            const QStringList subs = QDir(entryPath)
                                         .entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
            for (const QString &s : subs) {
                if (!s.contains(QLatin1Char(':')))
                    continue;
                if (driver.isEmpty())
                    driver = symLinkName(entryPath + QLatin1Char('/') + s
                                             + QStringLiteral("/driver"));
                const QString ifaceAlias = readFile(entryPath + QLatin1Char('/') + s
                                                    + QStringLiteral("/modalias"));
                if (!ifaceAlias.isEmpty())
                    modaliases.append(ifaceAlias);
            }
        }
        if (!driver.isEmpty() || modaliases.isEmpty())
            continue;

        if (bus == QLatin1String("pci")) {
            // bridges (class 06) operate without a driver; not a problem
            const QString cls = readFile(anchor + QStringLiteral("/class"));
            if (cls.startsWith(QStringLiteral("0x06")))
                continue;
        }

        // display names ------------------------------------------------------
        QString deviceName, vendorName;
        QString vendorHex, deviceHex;
        if (bus == QLatin1String("pci")) {
            vendorHex = readFile(anchor + QStringLiteral("/vendor")).toLower();   // 0x8086
            deviceHex = readFile(anchor + QStringLiteral("/device")).toLower();
            const QString v = vendorHex.mid(2), d = deviceHex.mid(2);
            vendorName = ids.pciVendors.value(v);
            if (vendorName.isEmpty() && v.size() == 4)
                vendorName = v.toUpper();
            deviceName = ids.pciDevices.value(v + QLatin1Char(':') + d);
        } else {
            vendorHex = readFile(anchor + QStringLiteral("/idVendor"));
            deviceHex = readFile(anchor + QStringLiteral("/idProduct"));
            vendorName = readFile(anchor + QStringLiteral("/manufacturer"));
            if (vendorName.isEmpty())
                vendorName = ids.usbVendors.value(vendorHex.toLower());
            deviceName = readFile(anchor + QStringLiteral("/product"));
        }
        if (deviceName.isEmpty()) {
            deviceName = QStringLiteral("%1 %2:%3")
                             .arg(vendorName.isEmpty() ? bus.toUpper() : vendorName,
                                  vendorHex.mid(2).toUpper(), deviceHex.mid(2).toUpper());
        }

        // candidate modules ----------------------------------------------------
        QStringList candidates;
        for (const QString &alias : modaliases) {
            for (const AliasEntry &a : aliases) {
                if (globMatch(a.pattern, 0, alias, 0) && !candidates.contains(a.module))
                    candidates.append(a.module);
            }
        }
        QVariantList candidateList;
        for (const QString &mod : candidates) {
            QVariantMap cm;
            cm.insert(QStringLiteral("module"), mod);
            cm.insert(QStringLiteral("state"),
                      builtin.contains(mod) ? QStringLiteral("builtin")
                                            : loaded.contains(mod) ? QStringLiteral("loaded")
                                                                   : QStringLiteral("available"));
            candidateList.append(cm);
        }

        QVariantMap dm;
        dm.insert(QStringLiteral("id"), anchor);
        dm.insert(QStringLiteral("sysfsPath"), anchor);
        dm.insert(QStringLiteral("bus"), bus);
        dm.insert(QStringLiteral("name"), deviceName);
        dm.insert(QStringLiteral("vendor"), vendorName);
        dm.insert(QStringLiteral("modalias"), modaliases.first());
        dm.insert(QStringLiteral("candidates"), candidateList);
        out->append(dm);
    }
}

// ---------------------------------------------------------------------------
// privileged module/device actions
// ---------------------------------------------------------------------------

void DriverHelper::runAction(const QStringList &pkexecArgs, const QString &target,
                             const QString &action)
{
    if (m_actionProcess) {
        emit actionFinished(target, action, false, QStringLiteral("busy"));
        return;
    }

    auto *p = new QProcess(this);
    m_actionProcess = p;
    emit busyChanged();
    p->setProgram(QStringLiteral("pkexec"));
    p->setArguments(pkexecArgs);

    // the polkit prompt can stay up for a while; generous timeout
    QTimer::singleShot(60000, p, [p]() {
        if (p->state() != QProcess::NotRunning)
            p->kill();
    });

    connect(p, &QProcess::finished, this,
            [this, p, target, action](int exitCode, QProcess::ExitStatus) {
        m_actionProcess = nullptr;
        emit busyChanged();
        const QString message = QString::fromUtf8(p->readAllStandardError()).trimmed();
        emit actionFinished(target, action, exitCode == 0,
                            exitCode == 0 ? QStringLiteral("ok") : message);
        p->deleteLater();
    });
    connect(p, &QProcess::errorOccurred, this,
            [this, p, target, action](QProcess::ProcessError err) {
        if (err != QProcess::FailedToStart)
            return;
        m_actionProcess = nullptr;
        emit busyChanged();
        emit actionFinished(target, action, false, QStringLiteral("pkexec not found"));
        p->deleteLater();
    });
    p->start();
}

void DriverHelper::loadModule(const QString &module)
{
    runAction({ QStringLiteral("modprobe"), module }, module, QStringLiteral("load"));
}

void DriverHelper::unloadModule(const QString &module)
{
    runAction({ QStringLiteral("rmmod"), module }, module, QStringLiteral("unload"));
}

void DriverHelper::probeDevice(const QVariantMap &device)
{
    const QString sysfs = device.value(QStringLiteral("sysfsPath")).toString();
    const QString bus = device.value(QStringLiteral("bus")).toString();
    const QString devName = QFileInfo(sysfs).fileName();
    const QString id = device.value(QStringLiteral("id")).toString();
    if (sysfs.isEmpty() || bus.isEmpty() || devName.isEmpty()) {
        emit actionFinished(id, QStringLiteral("probe"), false, QStringLiteral("unsupported"));
        return;
    }
    const QString cmd =
        QStringLiteral("echo '%1' > '/sys/bus/%2/drivers_probe'").arg(devName, bus);
    runAction({ QStringLiteral("sh"), QStringLiteral("-c"), cmd }, id, QStringLiteral("probe"));
}

void DriverHelper::bindDevice(const QVariantMap &device, const QString &driver)
{
    const QString sysfs = device.value(QStringLiteral("sysfsPath")).toString();
    const QString bus = device.value(QStringLiteral("bus")).toString();
    const QString devName = QFileInfo(sysfs).fileName();
    const QString id = device.value(QStringLiteral("id")).toString();
    if (sysfs.isEmpty() || bus.isEmpty() || driver.isEmpty() || devName.isEmpty()) {
        emit actionFinished(id, QStringLiteral("bind"), false, QStringLiteral("unsupported"));
        return;
    }
    const QString cmd = QStringLiteral("echo '%1' > '/sys/bus/%2/drivers/%3/bind'")
                            .arg(devName, bus, driver);
    runAction({ QStringLiteral("sh"), QStringLiteral("-c"), cmd }, id, QStringLiteral("bind"));
}

// ---------------------------------------------------------------------------
// modinfo
// ---------------------------------------------------------------------------

void DriverHelper::moduleInfo(const QString &module)
{
    auto *p = new QProcess(this);
    p->setProgram(QStringLiteral("modinfo"));
    p->setArguments({ module });
    QTimer::singleShot(10000, p, [p]() {
        if (p->state() != QProcess::NotRunning)
            p->kill();
    });
    connect(p, &QProcess::finished, this, [this, p, module](int exitCode, QProcess::ExitStatus) {
        QVariantMap info;
        if (exitCode != 0) {
            info.insert(QStringLiteral("error"),
                        QString::fromUtf8(p->readAllStandardError()).trimmed());
        } else {
            QStringList firmwares;
            int aliasCount = 0;
            const QStringList lines = QString::fromUtf8(p->readAllStandardOutput())
                                          .split(QLatin1Char('\n'));
            for (const QString &line : lines) {
                const int colon = line.indexOf(QLatin1Char(':'));
                if (colon < 0)
                    continue;
                const QString key = line.left(colon).trimmed();
                const QString value = line.mid(colon + 1).trimmed();
                if (key == QLatin1String("alias")) {
                    ++aliasCount;
                } else if (key == QLatin1String("firmware")) {
                    if (!value.isEmpty())
                        firmwares.append(value);
                } else if (key == QLatin1String("filename")
                           || key == QLatin1String("description")
                           || key == QLatin1String("version") || key == QLatin1String("author")
                           || key == QLatin1String("license") || key == QLatin1String("vermagic")
                           || key == QLatin1String("depends") || key == QLatin1String("intree")
                           || key == QLatin1String("signer") || key == QLatin1String("sig_key")
                           || key == QLatin1String("sig_hashalgo")) {
                    info.insert(key, value);
                }
            }
            info.insert(QStringLiteral("firmware"), firmwares.join(QStringLiteral(", ")));
            info.insert(QStringLiteral("aliasCount"), aliasCount);
        }
        emit moduleInfoReady(module, info);
        p->deleteLater();
    });
    connect(p, &QProcess::errorOccurred, this, [this, p, module](QProcess::ProcessError err) {
        if (err != QProcess::FailedToStart)
            return;
        QVariantMap info;
        info.insert(QStringLiteral("error"), QStringLiteral("modinfo not found"));
        emit moduleInfoReady(module, info);
        p->deleteLater();
    });
    p->start();
}

// ---------------------------------------------------------------------------
// package layer: curated suggestions
// ---------------------------------------------------------------------------

QStringList DriverHelper::curatedPackages(const QVariantMap &device, QString *searchTerm) const
{
    // Parse the pci:/usb: modalias into vendor/device/class ids.
    const QString alias = device.value(QStringLiteral("modalias")).toString();
    const QString bus = device.value(QStringLiteral("bus")).toString();
    QString vendor, cls;
    if (alias.startsWith(QStringLiteral("pci:"))) {
        vendor = alias.mid(5, 8);                              // "000010DE"
        while (vendor.size() > 4 && vendor.startsWith(QLatin1Char('0')))
            vendor.remove(0, 1);
        const int bc = alias.indexOf(QLatin1String("bc"), 5);
        if (bc > 0)
            cls = alias.mid(bc + 2, 2);
    } else if (alias.startsWith(QStringLiteral("usb:v"))) {
        vendor = alias.mid(5, 4);                              // "0a5c"
    }
    if (searchTerm)
        searchTerm->clear();

    QString key;
    // NVIDIA GPU
    if (bus == QLatin1String("pci") && vendor == QLatin1String("10de")
        && cls == QLatin1String("03")) {
        key = QStringLiteral("nvidia");
    }
    // Broadcom wireless (pci 14e4:02xx or usb 0a5c)
    else if ((bus == QLatin1String("pci") && vendor == QLatin1String("14e4")
              && cls == QLatin1String("02"))
             || (bus == QLatin1String("usb") && vendor == QLatin1String("0a5c"))) {
        key = QStringLiteral("broadcom");
    }
    // Realtek 2.5GbE needs the out-of-tree r8125 module
    else if (bus == QLatin1String("pci") && vendor == QLatin1String("10ec")
             && alias.contains(QStringLiteral("d00008125"))) {
        key = QStringLiteral("r8125");
    }
    // Realtek r8168 (in-tree r8169 sometimes misbehaves on these chips)
    else if (bus == QLatin1String("pci") && vendor == QLatin1String("10ec")
             && alias.contains(QStringLiteral("d00008168"))) {
        key = QStringLiteral("r8168");
    }

    QStringList pkgs;
    if (key == QLatin1String("nvidia")) {
        if (m_backend == QLatin1String("dnf"))
            pkgs << QStringLiteral("akmod-nvidia");
        else if (m_backend == QLatin1String("apt"))
            pkgs << QStringLiteral("nvidia-driver");
        else if (m_backend == QLatin1String("pacman"))
            pkgs << QStringLiteral("nvidia-dkms");
        else if (m_backend == QLatin1String("zypper"))
            pkgs << QStringLiteral("x11-video-nvidiaG06");
        else if (m_backend == QLatin1String("emerge"))
            pkgs << QStringLiteral("x11-drivers/nvidia-drivers");
    } else if (key == QLatin1String("broadcom")) {
        if (m_backend == QLatin1String("dnf"))
            pkgs << QStringLiteral("akmod-wl");
        else if (m_backend == QLatin1String("apt"))
            pkgs << QStringLiteral("broadcom-sta-dkms");
        else if (m_backend == QLatin1String("pacman")
                 || m_backend == QLatin1String("zypper"))
            pkgs << QStringLiteral("broadcom-wl");
        else if (m_backend == QLatin1String("emerge"))
            pkgs << QStringLiteral("net-wireless/broadcom-sta");
    } else if (key == QLatin1String("r8125")) {
        pkgs << (m_backend == QLatin1String("apt") ? QStringLiteral("r8125-dkms")
                                                   : QStringLiteral("r8125"));
    } else if (key == QLatin1String("r8168")) {
        if (m_backend == QLatin1String("apt"))
            pkgs << QStringLiteral("r8168-dkms");
        else
            pkgs << QStringLiteral("r8168");
    }

    if (searchTerm) {
        if (!key.isEmpty())
            *searchTerm = key;
        else if (vendor == QLatin1String("10de"))
            *searchTerm = QStringLiteral("nvidia");
        else if (vendor == QLatin1String("14e4"))
            *searchTerm = QStringLiteral("broadcom");
    }
    return pkgs;
}

// ---------------------------------------------------------------------------
// package layer: search
// ---------------------------------------------------------------------------

namespace {

// Parse the backend-specific `search` output into [{name, summary}] entries.
QVariantList parseSearchOutput(const QString &backend, const QString &output)
{
    QVariantList out;
    const QStringList lines = output.split(QLatin1Char('\n'));

    if (backend == QLatin1String("pacman")) {
        // "repo/name 1.2.3 [installed]" header followed by an indented summary
        int lastIdx = -1;
        for (int i = 0; i < lines.size(); ++i) {
            const QString raw = lines.at(i);
            if (raw.startsWith(QLatin1Char(' ')) || raw.startsWith(QLatin1Char('\t'))) {
                if (lastIdx >= 0) {
                    QVariantMap m = out.at(lastIdx).toMap();
                    if (m.value(QStringLiteral("summary")).toString().isEmpty())
                        m.insert(QStringLiteral("summary"), raw.trimmed());
                    out.replace(lastIdx, m);
                    lastIdx = -1;
                }
                continue;
            }
            const QStringList parts = raw.simplified().split(QLatin1Char(' '));
            if (parts.isEmpty() || parts.at(0).isEmpty())
                continue;
            QString name = parts.at(0);
            const int slash = name.lastIndexOf(QLatin1Char('/'));
            if (slash >= 0)
                name = name.mid(slash + 1);
            QVariantMap m;
            m.insert(QStringLiteral("name"), name);
            m.insert(QStringLiteral("summary"), QString());
            out.append(m);
            lastIdx = out.size() - 1;
        }
        return out;
    }

    for (const QString &raw : lines) {
        const QString line = raw.trimmed();
        if (line.isEmpty())
            continue;
        QVariantMap m;
        if (backend == QLatin1String("dnf")) {
            // "package-name : summary" plus "==== Name Matched: x ====" banners
            if (line.startsWith(QLatin1Char('=')) || line.startsWith(QLatin1Char('-')))
                continue;
            const int sep = line.indexOf(QStringLiteral(" : "));
            if (sep > 0) {
                // strip a trailing ".arch" (kernel.x86_64 -> kernel) so the
                // name matches what rpm -q reports for installed checks
                static const QRegularExpression archRe(
                    QStringLiteral("\\.(x86_64|aarch64|armv7hl|ppc64le|s390x|riscv64|i686|noarch)$"));
                QString name = line.left(sep);
                name.remove(archRe);
                m.insert(QStringLiteral("name"), name);
                m.insert(QStringLiteral("summary"), line.mid(sep + 3));
            }
        } else if (backend == QLatin1String("apt")) {
            // "package-name - summary"
            const int sep = line.indexOf(QStringLiteral(" - "));
            if (sep > 0) {
                m.insert(QStringLiteral("name"), line.left(sep));
                m.insert(QStringLiteral("summary"), line.mid(sep + 3));
            }
        } else if (backend == QLatin1String("zypper")) {
            // "| S | Name | Summary | Type |" table rows
            if (!line.startsWith(QLatin1Char('|')))
                continue;
            const QStringList cols = line.split(QLatin1Char('|'));
            if (cols.size() < 4)
                continue;
            const QString name = cols.at(1).trimmed();
            if (name.isEmpty() || name == QLatin1String("Name")
                || name.startsWith(QLatin1Char('-')))
                continue;
            m.insert(QStringLiteral("name"), name);
            m.insert(QStringLiteral("summary"),
                     cols.size() > 4 ? cols.at(2).trimmed() : cols.last().trimmed());
        } else if (backend == QLatin1String("apk")) {
            m.insert(QStringLiteral("name"), line.section(QLatin1Char(' '), 0, 0));
            m.insert(QStringLiteral("summary"), QString());
        } else if (backend == QLatin1String("emerge")) {
            // "* category/name" header lines
            if (line.startsWith(QLatin1Char('*')) && !line.contains(QLatin1Char(' ')))
                m.insert(QStringLiteral("name"), line.mid(1));
        }
        if (m.contains(QStringLiteral("name")))
            out.append(m);
    }
    return out;
}

} // namespace

void DriverHelper::searchPackages(const QVariantMap &device)
{
    const QString deviceId = device.value(QStringLiteral("id")).toString();

    if (m_backendPath.isEmpty()) {
        emit searchFailed(deviceId, QStringLiteral("no package manager"));
        emit searchReady(deviceId, {});
        return;
    }

    QString term;
    const QStringList curated = curatedPackages(device, &term);
    QVariantList results;
    for (const QString &p : curated) {
        QVariantMap m;
        m.insert(QStringLiteral("name"), p);
        m.insert(QStringLiteral("summary"), QString());
        m.insert(QStringLiteral("recommended"), true);
        m.insert(QStringLiteral("installed"), false);
        results.append(m);
    }

    const std::function<void(const QVariantList &)> emitReady =
        [this, deviceId](const QVariantList &list) {
        QStringList names;
        for (const QVariant &v : list)
            names.append(v.toMap().value(QStringLiteral("name")).toString());
        queryInstalledState(names, [this, deviceId, list](const QSet<QString> &installed) {
            QVariantList out = list;
            for (QVariant &v : out) {
                QVariantMap m = v.toMap();
                if (installed.contains(m.value(QStringLiteral("name")).toString()))
                    m.insert(QStringLiteral("installed"), true);
                v = m;
            }
            emit searchReady(deviceId, out);
        });
    };

    if (term.isEmpty()) {
        // nothing sensible to search for; the curated list is all we have
        emitReady(results);
        return;
    }

    if (m_searchProcess) {
        // supersede any in-flight search
        m_searchProcess->disconnect(this);
        m_searchProcess->kill();
        m_searchProcess->deleteLater();
        m_searchProcess = nullptr;
    }

    QString program = m_backendPath;
    QStringList args;
    if (m_backend == QLatin1String("dnf")) {
        // assumeno avoids hanging on GPG-key import prompts of broken repos
        args = { QStringLiteral("--assumeno"), QStringLiteral("--quiet"),
                 QStringLiteral("search"), term };
    } else if (m_backend == QLatin1String("apt")) {
        program = QStandardPaths::findExecutable(QStringLiteral("apt-cache"));
        args = { QStringLiteral("search"), term };
    } else if (m_backend == QLatin1String("pacman")) {
        args = { QStringLiteral("-Ss"), term };
    } else if (m_backend == QLatin1String("zypper")) {
        args = { QStringLiteral("--quiet"), QStringLiteral("search"), term };
    } else if (m_backend == QLatin1String("apk")) {
        args = { QStringLiteral("search"), term };
    } else if (m_backend == QLatin1String("emerge")) {
        args = { QStringLiteral("--searchdesc"), term };
    }
    if (program.isEmpty()) {
        emit searchFailed(deviceId, QStringLiteral("search tool not found"));
        emitReady(results);
        return;
    }

    auto *p = new QProcess(this);
    m_searchProcess = p;
    p->setProgram(program);
    p->setArguments(args);
    QTimer::singleShot(60000, p, [p]() {
        if (p->state() != QProcess::NotRunning)
            p->kill();
    });

    connect(p, &QProcess::finished, this,
            [this, p, deviceId, results, term, emitReady](int exitCode, QProcess::ExitStatus) {
        m_searchProcess = nullptr;
        QVariantList found;
        if (exitCode == 0) {
            QStringList already;
            for (const QVariant &v : results)
                already.append(v.toMap().value(QStringLiteral("name")).toString());
            const QString out = QString::fromUtf8(p->readAllStandardOutput());
            for (const QVariant &v : parseSearchOutput(m_backend, out)) {
                const QVariantMap m = v.toMap();
                const QString name = m.value(QStringLiteral("name")).toString();
                if (name.isEmpty() || already.contains(name))
                    continue;
                QVariantMap mm = m;
                mm.insert(QStringLiteral("recommended"), false);
                mm.insert(QStringLiteral("installed"), false);
                found.append(mm);
                if (results.size() + found.size() >= 8)
                    break;
            }
        } else {
            emit searchFailed(deviceId, QStringLiteral("search failed"));
        }
        emitReady(results + found);
        p->deleteLater();
    });
    connect(p, &QProcess::errorOccurred, this, [this, p, deviceId](QProcess::ProcessError err) {
        if (err != QProcess::FailedToStart)
            return;
        m_searchProcess = nullptr;
        emit searchFailed(deviceId, QStringLiteral("search failed to start"));
        p->deleteLater();
    });
    p->start();
}

void DriverHelper::queryInstalledState(const QStringList &names,
                                       const std::function<void(const QSet<QString> &)> &done)
{
    if (names.isEmpty()) {
        done({});
        return;
    }
    QString program;
    QStringList args;
    if (m_backend == QLatin1String("dnf") || m_backend == QLatin1String("zypper")) {
        program = QStandardPaths::findExecutable(QStringLiteral("rpm"));
        args = QStringList{ QStringLiteral("-q"), QStringLiteral("--qf"),
                            QStringLiteral("%{NAME}\\n") }
               + names;
    } else if (m_backend == QLatin1String("apt")) {
        program = QStandardPaths::findExecutable(QStringLiteral("dpkg-query"));
        args = QStringList{ QStringLiteral("-W"),
                            QStringLiteral("-f=${Package} ${db:Status-abbrev}\\n") }
               + names;
    } else if (m_backend == QLatin1String("pacman")) {
        program = m_backendPath;
        args = QStringList{ QStringLiteral("-Qi") } + names;
    } else if (m_backend == QLatin1String("apk")) {
        program = m_backendPath;
        args = QStringList{ QStringLiteral("info"), QStringLiteral("-e") } + names;
    } else {
        done({});
        return;
    }
    if (program.isEmpty()) {
        done({});
        return;
    }

    auto *p = new QProcess(this);
    p->setProgram(program);
    p->setArguments(args);
    QTimer::singleShot(15000, p, [p]() {
        if (p->state() != QProcess::NotRunning)
            p->kill();
    });
    connect(p, &QProcess::finished, this, [this, p, done](int, QProcess::ExitStatus) {
        QSet<QString> installed;
        const QStringList lines =
            QString::fromUtf8(p->readAllStandardOutput()).split(QLatin1Char('\n'));
        for (const QString &line : lines) {
            const QString t = line.simplified();
            if (t.isEmpty())
                continue;
            if (m_backend == QLatin1String("apt")) {
                // "name ii" — the second status char marks installed
                const QStringList parts = t.split(QLatin1Char(' '));
                if (parts.size() >= 2 && parts.at(1).startsWith(QLatin1Char('i')))
                    installed.insert(parts.at(0));
            } else if (m_backend == QLatin1String("pacman")) {
                if (t.startsWith(QStringLiteral("Name")))
                    installed.insert(t.section(QLatin1Char(':'), 1).simplified());
            } else {
                installed.insert(t.section(QLatin1Char(' '), 0, 0));
            }
        }
        done(installed);
        p->deleteLater();
    });
    connect(p, &QProcess::errorOccurred, this, [p, done](QProcess::ProcessError err) {
        if (err != QProcess::FailedToStart)
            return;
        done({});
        p->deleteLater();
    });
    p->start();
}

// ---------------------------------------------------------------------------
// package layer: install (streaming)
// ---------------------------------------------------------------------------

void DriverHelper::installPackage(const QString &deviceId, const QString &pkgName)
{
    if (m_installProcess) {
        emit installFinished(deviceId, pkgName, false, QStringLiteral("busy"));
        return;
    }
    if (m_backendPath.isEmpty()) {
        emit installFinished(deviceId, pkgName, false, QStringLiteral("no package manager"));
        return;
    }

    QStringList args;
    if (m_backend == QLatin1String("dnf"))
        args = { m_backendPath, QStringLiteral("install"), QStringLiteral("-y"), pkgName };
    else if (m_backend == QLatin1String("apt"))
        args = { QStandardPaths::findExecutable(QStringLiteral("apt-get")),
                 QStringLiteral("install"), QStringLiteral("-y"), pkgName };
    else if (m_backend == QLatin1String("pacman"))
        args = { m_backendPath, QStringLiteral("-S"), QStringLiteral("--noconfirm"),
                 QStringLiteral("--needed"), pkgName };
    else if (m_backend == QLatin1String("zypper"))
        args = { m_backendPath, QStringLiteral("--non-interactive"),
                 QStringLiteral("install"), QStringLiteral("--auto-agree-with-licenses"),
                 pkgName };
    else if (m_backend == QLatin1String("apk"))
        args = { m_backendPath, QStringLiteral("add"), pkgName };
    else if (m_backend == QLatin1String("emerge"))
        args = { m_backendPath, QStringLiteral("--quiet"), pkgName };
    if (args.isEmpty() || args.at(0).isEmpty()) {
        emit installFinished(deviceId, pkgName, false, QStringLiteral("backend not found"));
        return;
    }

    runStreamingInstall(QStringLiteral("pkexec"), args, deviceId, pkgName);
}

void DriverHelper::runStreamingInstall(const QString &program, const QStringList &args,
                                       const QString &deviceId, const QString &pkgName)
{
    auto *p = new QProcess(this);
    m_installProcess = p;
    m_outBuffer.clear();
    m_errBuffer.clear();
    emit busyChanged();
    emit installPhase(deviceId, pkgName, QStringLiteral("prepare"));

    p->setProgram(program);
    p->setArguments(args);
    // package installs (and AUR builds) can legitimately take many minutes
    QTimer::singleShot(900000, p, [p]() {
        if (p->state() != QProcess::NotRunning)
            p->kill();
    });

    static const QRegularExpression percentRe(QStringLiteral("(\\d{1,3})\\s*%"));
    const auto streamLine = [this, deviceId, pkgName](const QString &line, bool isError) {
        if (line.simplified().isEmpty())
            return;
        emit installOutput(deviceId, pkgName, line);
        const auto match = percentRe.match(line);
        if (match.hasMatch()) {
            const int percent = match.captured(1).toInt();
            if (percent >= 0 && percent <= 100)
                emit installProgress(deviceId, pkgName, percent);
        }
        if (isError)
            return;
        const QString l = line.toLower();
        QString phase;
        if (l.contains(QLatin1String("download")) || l.contains(QLatin1String("fetch"))
            || l.contains(QLatin1String("making")) || l.contains(QLatin1String("building"))
            || l.contains(QStringLiteral("下载")) || l.contains(QStringLiteral("编译")))
            phase = QStringLiteral("download");
        else if (l.contains(QLatin1String("install")) || l.contains(QLatin1String("upgrad"))
                 || l.contains(QLatin1String("verif")) || l.contains(QLatin1String("transact"))
                 || l.contains(QLatin1String("running")) || l.contains(QLatin1String("script"))
                 || l.contains(QStringLiteral("安装")) || l.contains(QStringLiteral("升级"))
                 || l.contains(QStringLiteral("验证")))
            phase = QStringLiteral("install");
        if (!phase.isEmpty())
            emit installPhase(deviceId, pkgName, phase);
    };

    connect(p, &QProcess::readyReadStandardOutput, this, [this, p, streamLine]() {
        m_outBuffer += QString::fromUtf8(p->readAllStandardOutput());
        int nl;
        while ((nl = m_outBuffer.indexOf(QLatin1Char('\n'))) >= 0) {
            const QString line = m_outBuffer.left(nl);
            m_outBuffer.remove(0, nl + 1);
            streamLine(line, false);
        }
    });
    connect(p, &QProcess::readyReadStandardError, this, [this, p, streamLine]() {
        m_errBuffer += QString::fromUtf8(p->readAllStandardError());
        int nl;
        while ((nl = m_errBuffer.indexOf(QLatin1Char('\n'))) >= 0) {
            const QString line = m_errBuffer.left(nl);
            m_errBuffer.remove(0, nl + 1);
            streamLine(line, true);
        }
    });

    connect(p, &QProcess::finished, this,
            [this, p, deviceId, pkgName, streamLine](int exitCode, QProcess::ExitStatus) {
        m_installProcess = nullptr;
        emit busyChanged();
        if (!m_outBuffer.isEmpty())
            streamLine(m_outBuffer, false);
        if (!m_errBuffer.isEmpty())
            streamLine(m_errBuffer, true);
        const QString err = QString::fromUtf8(p->readAllStandardError()).simplified();
        emit installPhase(deviceId, pkgName, QStringLiteral("done"));
        emit installFinished(deviceId, pkgName, exitCode == 0,
                             exitCode == 0 ? QStringLiteral("ok") : err);
        p->deleteLater();
    });
    connect(p, &QProcess::errorOccurred, this,
            [this, p, deviceId, pkgName](QProcess::ProcessError err) {
        if (err != QProcess::FailedToStart)
            return;
        m_installProcess = nullptr;
        emit busyChanged();
        emit installFinished(deviceId, pkgName, false, QStringLiteral("process failed to start"));
        p->deleteLater();
    });
    p->start();
}

// ---------------------------------------------------------------------------
// package layer: proprietary (closed-source) drivers
// ---------------------------------------------------------------------------

QString DriverHelper::proprietaryPackage(const QString &key) const
{
    if (key == QLatin1String("nvidia")) {
        if (m_backend == QLatin1String("dnf"))
            return QStringLiteral("akmod-nvidia");
        if (m_backend == QLatin1String("apt"))
            return QStringLiteral("nvidia-driver");
        if (m_backend == QLatin1String("pacman"))
            return QStringLiteral("nvidia-dkms");
        if (m_backend == QLatin1String("zypper"))
            return QStringLiteral("x11-video-nvidiaG06");
        if (m_backend == QLatin1String("emerge"))
            return QStringLiteral("x11-drivers/nvidia-drivers");
    } else if (key == QLatin1String("broadcom-wl")) {
        if (m_backend == QLatin1String("dnf"))
            return QStringLiteral("akmod-wl");
        if (m_backend == QLatin1String("apt"))
            return QStringLiteral("broadcom-sta-dkms");
        if (m_backend == QLatin1String("pacman"))
            return QStringLiteral("broadcom-wl");
        if (m_backend == QLatin1String("zypper"))
            return QStringLiteral("broadcom-wl");
        if (m_backend == QLatin1String("emerge"))
            return QStringLiteral("net-wireless/broadcom-sta");
    }
    return QString();
}

// Hardware scan: which proprietary-driver candidates does this machine have?
QVariantList DriverHelper::proprietaryScan(QStringList *packageNames) const
{
    QVariantList options;
    const HwIdsDb &ids = HwIdsDb::instance();
    const QDir dir(QStringLiteral("/sys/bus/pci/devices"));
    const QStringList names = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    struct Found {
        QString key;
        QString device;
        QString currentDriver;
    };
    QList<Found> found;
    for (const QString &name : names) {
        const QString base = dir.filePath(name);
        const QString vendor = readFile(base + QStringLiteral("/vendor")).toLower();
        const QString cls = readFile(base + QStringLiteral("/class")).toLower();
        const bool nvidiaGpu = vendor == QLatin1String("0x10de")
            && cls.startsWith(QStringLiteral("0x03"));
        const bool broadcomWifi = vendor == QLatin1String("0x14e4")
            && cls.startsWith(QStringLiteral("0x02"));
        if (!nvidiaGpu && !broadcomWifi)
            continue;
        const QString key = nvidiaGpu ? QStringLiteral("nvidia")
                                      : QStringLiteral("broadcom-wl");
        // one entry per driver family, keep the first device
        bool dup = false;
        for (const Found &f : found)
            if (f.key == key)
                dup = true;
        if (dup)
            continue;
        QString device = readFile(base + QStringLiteral("/label"));
        if (device.isEmpty()) {
            const QString vid = readFile(base + QStringLiteral("/vendor")).mid(2);
            const QString did = readFile(base + QStringLiteral("/device")).mid(2);
            device = ids.pciDevices.value(vid + QLatin1Char(':') + did.toLower());
        }
        Found f;
        f.key = key;
        f.device = device.isEmpty() ? (nvidiaGpu ? QStringLiteral("NVIDIA GPU")
                                                 : QStringLiteral("Broadcom wireless"))
                                    : device;
        f.currentDriver = symLinkName(base + QStringLiteral("/driver"));
        found.append(f);
    }

    for (const Found &f : found) {
        const QString pkg = proprietaryPackage(f.key);
        if (pkg.isEmpty())
            continue;
        QVariantMap m;
        m.insert(QStringLiteral("key"), f.key);
        m.insert(QStringLiteral("device"), f.device);
        m.insert(QStringLiteral("currentDriver"), f.currentDriver);
        m.insert(QStringLiteral("package"), pkg);
        m.insert(QStringLiteral("installed"), false);
        options.append(m);
        if (packageNames)
            packageNames->append(pkg);
    }
    return options;
}

void DriverHelper::scanProprietary()
{
    QStringList names;
    const QVariantList options = proprietaryScan(&names);
    queryInstalledState(names, [this, options](const QSet<QString> &installed) {
        QVariantList out = options;
        for (QVariant &v : out) {
            QVariantMap m = v.toMap();
            if (installed.contains(m.value(QStringLiteral("package")).toString()))
                m.insert(QStringLiteral("installed"), true);
            v = m;
        }
        emit proprietaryReady(out);
    });
}

void DriverHelper::installProprietary(const QString &key)
{
    const QString pkg = proprietaryPackage(key);
    if (pkg.isEmpty()) {
        emit installFinished(key, QString(), false, QStringLiteral("unsupported"));
        return;
    }

    // Arch: closed-source/AUR packages must be built as the logged-in user,
    // so run yay without pkexec and let it escalate through `--sudo pkexec`
    // (makepkg refuses to run as root, so the reverse order cannot work).
    if (m_backend == QLatin1String("pacman")) {
        const bool aurOnly = key == QLatin1String("broadcom-wl");
        if (!m_yayPath.isEmpty()) {
            runStreamingInstall(m_yayPath,
                                { QStringLiteral("-S"), QStringLiteral("--needed"),
                                  QStringLiteral("--noconfirm"), QStringLiteral("--noedit"),
                                  QStringLiteral("--sudo"), QStringLiteral("pkexec"), pkg },
                                key, pkg);
            return;
        }
        if (aurOnly) {
            emit installFinished(key, pkg, false, QStringLiteral("yay not found"));
            return;
        }
        // official-repo proprietary packages (nvidia-dkms) work via pacman
        runStreamingInstall(QStringLiteral("pkexec"),
                            { m_backendPath, QStringLiteral("-S"), QStringLiteral("--noconfirm"),
                              QStringLiteral("--needed"), pkg },
                            key, pkg);
        return;
    }

    if (m_backend == QLatin1String("dnf")) {
        // NVIDIA lives in RPM Fusion; enable it first when missing
        QString script;
        if (key == QLatin1String("nvidia")) {
            script = QStringLiteral(
                         "if ! %1 repolist --enabled 2>/dev/null | grep -qi rpmfusion-nonfree; then "
                         "%1 install -y "
                         "'https://download1.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %%fedora).noarch.rpm' "
                         "'https://download1.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-$(rpm -E %%fedora).noarch.rpm'; "
                         "fi; %1 install -y %2")
                         .arg(m_backendPath, pkg);
        } else {
            script = QStringLiteral("%1 install -y %2").arg(m_backendPath, pkg);
        }
        runStreamingInstall(QStringLiteral("pkexec"),
                            { QStringLiteral("sh"), QStringLiteral("-c"), script }, key, pkg);
        return;
    }

    if (m_backend == QLatin1String("apt")) {
        // ubuntu-drivers knows how to enable the required components itself
        if (key == QLatin1String("nvidia") && !m_ubuntuDriversPath.isEmpty()) {
            runStreamingInstall(QStringLiteral("pkexec"),
                                { m_ubuntuDriversPath, QStringLiteral("install") }, key, pkg);
            return;
        }
        runStreamingInstall(QStringLiteral("pkexec"),
                            { QStandardPaths::findExecutable(QStringLiteral("apt-get")),
                              QStringLiteral("install"), QStringLiteral("-y"), pkg },
                            key, pkg);
        return;
    }

    if (m_backend == QLatin1String("zypper")) {
        runStreamingInstall(QStringLiteral("pkexec"),
                            { m_backendPath, QStringLiteral("--non-interactive"),
                              QStringLiteral("install"),
                              QStringLiteral("--auto-agree-with-licenses"), pkg },
                            key, pkg);
        return;
    }
    if (m_backend == QLatin1String("emerge")) {
        runStreamingInstall(QStringLiteral("pkexec"),
                            { m_backendPath, QStringLiteral("--quiet"), pkg }, key, pkg);
        return;
    }

    emit installFinished(key, pkg, false, QStringLiteral("no package manager"));
}
