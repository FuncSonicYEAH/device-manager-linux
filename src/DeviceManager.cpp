#include "DeviceManager.h"
#include "Translator.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>
#include <QSet>

#include <algorithm>
#include <functional>
#include <iterator>
#include <utility>

// ---------------------------------------------------------------------------
// Small lookup tables (vendor / driver / category display metadata)
// ---------------------------------------------------------------------------

static const QHash<QString, QString> kPciVendors = {
    { QStringLiteral("8086"), QStringLiteral("Intel") },
    { QStringLiteral("1022"), QStringLiteral("AMD") },
    { QStringLiteral("10de"), QStringLiteral("NVIDIA") },
    { QStringLiteral("10ec"), QStringLiteral("Realtek") },
    { QStringLiteral("1002"), QStringLiteral("AMD/ATI") },
    { QStringLiteral("14e4"), QStringLiteral("Broadcom") },
    { QStringLiteral("168c"), QStringLiteral("Qualcomm Atheros") },
    { QStringLiteral("1969"), QStringLiteral("Qualcomm Atheros") },
    { QStringLiteral("104c"), QStringLiteral("Texas Instruments") },
    { QStringLiteral("11ab"), QStringLiteral("Marvell") },
    { QStringLiteral("1028"), QStringLiteral("Dell") },
    { QStringLiteral("1d6b"), QStringLiteral("Linux Foundation") },
    { QStringLiteral("8087"), QStringLiteral("Intel") },
    { QStringLiteral("1b73"), QStringLiteral("Fresco Logic") },
    { QStringLiteral("1912"), QStringLiteral("Renesas") },
    { QStringLiteral("10b7"), QStringLiteral("3Com") },
    { QStringLiteral("13d3"), QStringLiteral("AzureWave") },
};

static const QHash<QString, QString> kUsbVendors = {
    { QStringLiteral("1d6b"), QStringLiteral("Linux Foundation") },
    { QStringLiteral("8087"), QStringLiteral("Intel") },
    { QStringLiteral("046d"), QStringLiteral("Logitech") },
    { QStringLiteral("0bda"), QStringLiteral("Realtek") },
    { QStringLiteral("0951"), QStringLiteral("Kingston") },
    { QStringLiteral("090c"), QStringLiteral("Silicon Motion") },
    { QStringLiteral("0781"), QStringLiteral("SanDisk") },
    { QStringLiteral("05e3"), QStringLiteral("Genesys Logic") },
    { QStringLiteral("174c"), QStringLiteral("ASMedia") },
    { QStringLiteral("2109"), QStringLiteral("VIA Labs") },
    { QStringLiteral("0461"), QStringLiteral("Primax") },
    { QStringLiteral("045e"), QStringLiteral("Microsoft") },
    { QStringLiteral("05ac"), QStringLiteral("Apple") },
    { QStringLiteral("1bcf"), QStringLiteral("Sunplus") },
    { QStringLiteral("346d"), QStringLiteral("USB") },
};

static const QHash<QString, QString> kDriverNames = {
    { QStringLiteral("r8169"), QStringLiteral("realtekGigabitEthernetController") },
    { QStringLiteral("e1000e"), QStringLiteral("intelEthernetController") },
    { QStringLiteral("igb"), QStringLiteral("intelEthernetController") },
    { QStringLiteral("igc"), QStringLiteral("intelEthernetController") },
    { QStringLiteral("i915"), QStringLiteral("intelIntegratedGraphics") },
    { QStringLiteral("amdgpu"), QStringLiteral("amdGraphics") },
    { QStringLiteral("nvidia"), QStringLiteral("nvidiaGraphics") },
    { QStringLiteral("nouveau"), QStringLiteral("nvidiaGraphics") },
    { QStringLiteral("vmwgfx"), QStringLiteral("vmwareGraphics") },
    { QStringLiteral("iwlwifi"), QStringLiteral("intelWirelessAdapter") },
    { QStringLiteral("rtl8xxxu"), QStringLiteral("realtekWirelessAdapter") },
    { QStringLiteral("rt2800usb"), QStringLiteral("ralinkWirelessAdapter") },
    { QStringLiteral("ath9k"), QStringLiteral("atherosWirelessAdapter") },
    { QStringLiteral("ath10k"), QStringLiteral("qualcommWirelessAdapter") },
    { QStringLiteral("ath11k"), QStringLiteral("qualcommWirelessAdapter") },
    { QStringLiteral("mt76"), QStringLiteral("mediatekWirelessAdapter") },
    { QStringLiteral("xhci_hcd"), QStringLiteral("usb3HostController") },
    { QStringLiteral("ehci_hcd"), QStringLiteral("usb2HostController") },
    { QStringLiteral("ohci_hcd"), QStringLiteral("usbHostController") },
    { QStringLiteral("uhci_hcd"), QStringLiteral("usbHostController") },
    { QStringLiteral("ahci"), QStringLiteral("ahciSataController") },
    { QStringLiteral("nvme"), QStringLiteral("nvmeController") },
    { QStringLiteral("usb-storage"), QStringLiteral("usbMassStorageDevice") },
    { QStringLiteral("hub"), QStringLiteral("usbHub") },
    { QStringLiteral("ehci-pci"), QStringLiteral("usb2HostController") },
    { QStringLiteral("i801_smbus"), QStringLiteral("intelSmbusController") },
    { QStringLiteral("iTCO_wdt"), QStringLiteral("intelTcoWatchdog") },
    { QStringLiteral("mei_wdt"), QStringLiteral("intelMeWatchdog") },
    { QStringLiteral("intel_oc_wdt"), QStringLiteral("intelOverclockWatchdog") },
    { QStringLiteral("snd_hda_intel"), QStringLiteral("intelHdaAudioController") },
    { QStringLiteral("snd_hda_codec_hdmi"), QStringLiteral("hdmiAudio") },
    { QStringLiteral("atkbd"), QStringLiteral("atKeyboard") },
    { QStringLiteral("i8042"), QStringLiteral("ps2Controller") },
    { QStringLiteral("psmouse"), QStringLiteral("ps2Mouse") },
    { QStringLiteral("synaptics"), QStringLiteral("touchpad") },
    { QStringLiteral("coretemp"), QStringLiteral("cpuTempSensor") },
    { QStringLiteral("k10temp"), QStringLiteral("cpuTempSensor") },
    { QStringLiteral("jc42"), QStringLiteral("memoryTempSensor") },
    { QStringLiteral("button"), QStringLiteral("acpiButton") },
    { QStringLiteral("acpi-button"), QStringLiteral("acpiButton") },
    { QStringLiteral("rtc_cmos"), QStringLiteral("cmosRtc") },
    { QStringLiteral("tpm_tis"), QStringLiteral("tpm2SecurityModule") },
    { QStringLiteral("mei_me"), QStringLiteral("intelManagementEngine") },
    { QStringLiteral("watchdog"), QStringLiteral("watchdogTimer") },
    { QStringLiteral("pcspkr"), QStringLiteral("pcSpeaker") },
    { QStringLiteral("pcieport"), QStringLiteral("pciePort") },
    { QStringLiteral("lpc_ich"), QStringLiteral("intelLpcInterfaceController") },
    { QStringLiteral("i2c_i801"), QStringLiteral("intelSmbusController") },
    { QStringLiteral("intel_rapl_msr"), QStringLiteral("intelRaplPowerManagement") },
    { QStringLiteral("intel_smart_connect"), QStringLiteral("intelSmartConnectTechnology") },
    { QStringLiteral("acpi-cpufreq"), QStringLiteral("acpiProcessorFrequencyControl") },
    { QStringLiteral("virtio_blk"), QStringLiteral("virtioBlockDevice") },
    { QStringLiteral("virtio_net"), QStringLiteral("virtioNetworkDevice") },
    { QStringLiteral("simpledrm"), QStringLiteral("efiFramebuffer") },
};

static const QHash<QString, QString> kDriverIcons = {
    { QStringLiteral("r8169"), QStringLiteral("settings_ethernet") },
    { QStringLiteral("e1000e"), QStringLiteral("settings_ethernet") },
    { QStringLiteral("igb"), QStringLiteral("settings_ethernet") },
    { QStringLiteral("igc"), QStringLiteral("settings_ethernet") },
    { QStringLiteral("i915"), QStringLiteral("monitor") },
    { QStringLiteral("amdgpu"), QStringLiteral("monitor") },
    { QStringLiteral("nvidia"), QStringLiteral("monitor") },
    { QStringLiteral("nouveau"), QStringLiteral("monitor") },
    { QStringLiteral("vmwgfx"), QStringLiteral("monitor") },
    { QStringLiteral("iwlwifi"), QStringLiteral("wifi") },
    { QStringLiteral("rtl8xxxu"), QStringLiteral("wifi") },
    { QStringLiteral("rt2800usb"), QStringLiteral("wifi") },
    { QStringLiteral("ath9k"), QStringLiteral("wifi") },
    { QStringLiteral("ath10k"), QStringLiteral("wifi") },
    { QStringLiteral("ath11k"), QStringLiteral("wifi") },
    { QStringLiteral("mt76"), QStringLiteral("wifi") },
    { QStringLiteral("xhci_hcd"), QStringLiteral("usb") },
    { QStringLiteral("ehci_hcd"), QStringLiteral("usb") },
    { QStringLiteral("ohci_hcd"), QStringLiteral("usb") },
    { QStringLiteral("uhci_hcd"), QStringLiteral("usb") },
    { QStringLiteral("ahci"), QStringLiteral("storage") },
    { QStringLiteral("nvme"), QStringLiteral("storage") },
    { QStringLiteral("usb-storage"), QStringLiteral("storage") },
    { QStringLiteral("ehci-pci"), QStringLiteral("usb") },
    { QStringLiteral("snd_hda_intel"), QStringLiteral("volume_up") },
    { QStringLiteral("snd_hda_codec_hdmi"), QStringLiteral("volume_up") },
    { QStringLiteral("atkbd"), QStringLiteral("keyboard") },
    { QStringLiteral("psmouse"), QStringLiteral("mouse") },
    { QStringLiteral("synaptics"), QStringLiteral("touch_app") },
    { QStringLiteral("coretemp"), QStringLiteral("thermostat") },
    { QStringLiteral("k10temp"), QStringLiteral("thermostat") },
    { QStringLiteral("jc42"), QStringLiteral("thermostat") },
    { QStringLiteral("button"), QStringLiteral("power") },
    { QStringLiteral("acpi-button"), QStringLiteral("power") },
    { QStringLiteral("rtc_cmos"), QStringLiteral("schedule") },
    { QStringLiteral("tpm_tis"), QStringLiteral("security") },
    { QStringLiteral("virtio_blk"), QStringLiteral("storage") },
    { QStringLiteral("virtio_net"), QStringLiteral("settings_ethernet") },
};

// PCI class code (first two bytes) -> device type name
static const QHash<QString, QString> kPciClasses = {
    { QStringLiteral("00"), QStringLiteral("unclassifiedDevice") },
    { QStringLiteral("01"), QStringLiteral("storageControllers") },
    { QStringLiteral("02"), QStringLiteral("networkController") },
    { QStringLiteral("03"), QStringLiteral("displayController") },
    { QStringLiteral("04"), QStringLiteral("multimediaController") },
    { QStringLiteral("05"), QStringLiteral("memoryController") },
    { QStringLiteral("06"), QStringLiteral("bridgeDevice") },
    { QStringLiteral("07"), QStringLiteral("communicationController") },
    { QStringLiteral("08"), QStringLiteral("systemPeripheral") },
    { QStringLiteral("09"), QStringLiteral("inputDeviceController") },
    { QStringLiteral("0a"), QStringLiteral("expansionSlot") },
    { QStringLiteral("0b"), QStringLiteral("processors") },
    { QStringLiteral("0c"), QStringLiteral("serialBusController") },
    { QStringLiteral("0d"), QStringLiteral("wirelessController") },
    { QStringLiteral("0e"), QStringLiteral("smartCardController") },
    { QStringLiteral("0f"), QStringLiteral("encryptionController") },
    { QStringLiteral("10"), QStringLiteral("signalProcessingController") },
};

// Category key -> {display name, icon, display order}
struct CategoryInfo { const char *key; const char *name; const char *icon; int order; };

static const CategoryInfo kCategories[] = {
    { "display",      "displayAdapters",            "monitor",           0 },
    { "network",      "networkAdapters",            "settings_ethernet", 1 },
    { "disk",         "diskDrives",            "storage",           2 },
    { "storage-ctrl", "storageControllers",            "sd_card",           3 },
    { "sound",        "soundVideoGameControllers",  "volume_up",         4 },
    { "input",        "humanInterfaceDevices",         "keyboard",          5 },
    { "camera",       "imagingDevices",              "photo_camera",      6 },
    { "sensors",      "sensors",                "sensors",           7 },
    { "thermal",      "coolingDevices",              "thermostat",        8 },
    { "battery",      "batteries",                  "battery_full",      9 },
    { "wireless",     "wirelessDevices",              "wifi",              10 },
    { "ports",        "portsComLpt",     "usb",               11 },
    { "usb-ctrl",     "universalSerialBusControllers",      "usb",               12 },
    { "processor",    "processors",                "developer_board",   13 },
    { "security",     "securityDevices",              "security",          14 },
    { "clock",        "systemClocks",              "schedule",          15 },
    { "firmware",     "firmware",                  "dns",               16 },
    { "system",       "systemDevices",              "settings_suggest",  17 },
    { "other",        "otherDevices",              "devices_other",     18 },
};

static const CategoryInfo kConnections[] = {
    { "pci",     "pciDevices",     "developer_board", 0 },
    { "usb",     "usbDevices",     "usb",             1 },
    { "platform", "platformDevices",     "settings_suggest", 2 },
    { "mmc",     "mmcDevices",     "sd_card",         3 },
    { "virtio",  "virtioDevices",  "dns",             4 },
    { "virtual", "virtualDevices",     "devices_other",   5 },
    { "other",   "otherConnections",     "devices_other",   6 },
};

static const CategoryInfo *categoryInfo(const CategoryInfo *table, int count, const QString &key)
{
    for (int i = 0; i < count; ++i) {
        if (key == QLatin1String(table[i].key))
            return &table[i];
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

QString DeviceManager::readFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return QString();
    return QString::fromUtf8(f.readAll()).trimmed();
}

QString DeviceManager::canonicalPath(const QString &path)
{
    return QFileInfo(path).canonicalFilePath();
}

QString DeviceManager::findUp(const QString &startPath, const QStringList &names)
{
    QString dir = startPath;
    for (int depth = 0; depth < 12 && !dir.isEmpty(); ++depth) {
        for (const QString &n : names) {
            const QString v = readFile(dir + QLatin1Char('/') + n);
            if (!v.isEmpty())
                return v;
        }
        const int slash = dir.lastIndexOf(QLatin1Char('/'));
        if (slash <= 0)
            break;
        dir = dir.left(slash);
    }
    return QString();
}

// Find the nearest `driver` symlink walking up the device tree.
static QString findDriver(const QString &startPath)
{
    QString dir = startPath;
    for (int depth = 0; depth < 12 && !dir.isEmpty(); ++depth) {
        QFileInfo fi(dir + QStringLiteral("/driver"));
        if (fi.isSymLink()) {
            const QString name = QFileInfo(fi.symLinkTarget()).fileName();
            // "usb"/"hub" are generic USB core drivers; keep walking up
            if (!name.isEmpty() && name != QLatin1String("driver")
                && name != QLatin1String("usb") && name != QLatin1String("hub"))
                return name;
        }
        const int slash = dir.lastIndexOf(QLatin1Char('/'));
        if (slash <= 0)
            break;
        dir = dir.left(slash);
    }
    return QString();
}

QString DeviceManager::vendorName(const QString &hexId)
{
    QString norm = hexId;
    norm.remove(QLatin1String("0x"));
    const QString upper = norm.toUpper();
    const QString lower = norm.toLower();
    const QString with0x = QLatin1String("0x") + lower;
    if (kPciVendors.contains(upper))
        return kPciVendors.value(upper);
    if (kPciVendors.contains(lower))
        return kPciVendors.value(lower);
    if (kPciVendors.contains(with0x))
        return kPciVendors.value(with0x);
    if (kUsbVendors.contains(upper))
        return kUsbVendors.value(upper);
    if (kUsbVendors.contains(lower))
        return kUsbVendors.value(lower);
    return hexId;
}

QString DeviceManager::driverDisplayName(const QString &driver)
{
    if (driver.isEmpty())
        return QString();
    return Translator::translate(kDriverNames.value(driver));
}

QString DeviceManager::driverIcon(const QString &driver)
{
    if (driver.isEmpty())
        return QString();
    return kDriverIcons.value(driver);
}

QString DeviceManager::categoryName(const QString &key)
{
    if (const CategoryInfo *ci = categoryInfo(kCategories, int(std::size(kCategories)), key))
        return Translator::translate(QString::fromUtf8(ci->name));
    return key;
}

QString DeviceManager::categoryIcon(const QString &key)
{
    if (const CategoryInfo *ci = categoryInfo(kCategories, int(std::size(kCategories)), key))
        return QString::fromUtf8(ci->icon);
    return QStringLiteral("devices_other");
}

QString DeviceManager::categoryFromClass(const QString &cls)
{
    if (cls == QLatin1String("block")) return QStringLiteral("disk");
    if (cls == QLatin1String("net") || cls == QLatin1String("ieee80211")) return QStringLiteral("network");
    if (cls == QLatin1String("drm") || cls == QLatin1String("accel")) return QStringLiteral("display");
    if (cls == QLatin1String("sound")) return QStringLiteral("sound");
    if (cls == QLatin1String("input")) return QStringLiteral("input");
    if (cls == QLatin1String("video4linux")) return QStringLiteral("camera");
    if (cls == QLatin1String("power_supply")) return QStringLiteral("battery");
    if (cls == QLatin1String("hwmon")) return QStringLiteral("sensors");
    if (cls == QLatin1String("thermal")) return QStringLiteral("thermal");
    if (cls == QLatin1String("rfkill")) return QStringLiteral("wireless");
    if (cls == QLatin1String("rtc")) return QStringLiteral("clock");
    if (cls == QLatin1String("tpm")) return QStringLiteral("security");
    if (cls == QLatin1String("watchdog")) return QStringLiteral("system");
    if (cls == QLatin1String("dmi")) return QStringLiteral("firmware");
    return QStringLiteral("other");
}

QString DeviceManager::busFromPath(const QString &path)
{
    if (path.contains(QStringLiteral("/pci"))) return QStringLiteral("pci");
    if (path.contains(QStringLiteral("/usb"))) return QStringLiteral("usb");
    if (path.contains(QStringLiteral("/platform"))) return QStringLiteral("platform");
    if (path.contains(QStringLiteral("/mmc"))) return QStringLiteral("mmc");
    if (path.contains(QStringLiteral("/virtio"))) return QStringLiteral("virtio");
    if (path.contains(QStringLiteral("/virtual/"))) return QStringLiteral("virtual");
    return QStringLiteral("other");
}

QString DeviceManager::busName(const QString &key)
{
    if (const CategoryInfo *ci = categoryInfo(kConnections, int(std::size(kConnections)), key))
        return Translator::translate(QString::fromUtf8(ci->name));
    return key;
}

QString DeviceManager::statusName(const QString &status)
{
    if (status == QLatin1String("ok")) return Translator::translate(QStringLiteral("statusOk"));
    if (status == QLatin1String("disabled")) return Translator::translate(QStringLiteral("statusDisabled"));
    if (status == QLatin1String("suspended")) return Translator::translate(QStringLiteral("statusSuspended"));
    if (status == QLatin1String("unplugged")) return Translator::translate(QStringLiteral("statusUnplugged"));
    return Translator::translate(QStringLiteral("unknown"));
}

QString DeviceManager::humanBytes(qint64 bytes)
{
    if (bytes >= 1LL << 40)
        return QString::number(bytes / double(1LL << 40), 'f', 2) + QStringLiteral(" TB");
    if (bytes >= 1LL << 30)
        return QString::number(bytes / double(1LL << 30), 'f', 1) + QStringLiteral(" GB");
    if (bytes >= 1LL << 20)
        return QString::number(bytes / double(1LL << 20), 'f', 0) + QStringLiteral(" MB");
    return QString::number(bytes) + QStringLiteral(" B");
}

// ---------------------------------------------------------------------------
// DeviceManager
// ---------------------------------------------------------------------------

DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent)
{
    refresh();
}

DeviceEntry *DeviceManager::entryForAnchor(const QString &anchor)
{
    for (auto &e : m_devices) {
        if (e.anchor == anchor)
            return &e;
    }
    DeviceEntry e;
    e.id = anchor;
    e.anchor = anchor;
    e.sysfsPath = anchor;
    m_devices.append(e);
    return &m_devices.last();
}

void DeviceManager::scanClasses()
{
    // Scan order defines category priority when one physical device belongs to
    // several classes (e.g. a GPU shows up in drm, accel and hwmon).
    struct ClassScan {
        const char *cls;
        std::function<bool(const QString &entryPath, const QString &entryName)> keep;
        std::function<QString(const QString &entryPath)> explicitName;
    };

    const QRegularExpression ttyRe(QStringLiteral("^(ttyS|ttyUSB|ttyACM)"));

    const QList<ClassScan> scans = {
        // network
        { "net", [](const QString &, const QString &) { return true; },
          [](const QString &ep) { return QFileInfo(ep).fileName(); } },
        // display
        { "drm", [](const QString &, const QString &n) { return n.startsWith(QStringLiteral("card")) && !n.contains(QLatin1Char('-')); }, nullptr },
        { "accel", [](const QString &, const QString &) { return true; }, nullptr },
        // sound
        { "sound", [](const QString &, const QString &n) { return n.startsWith(QStringLiteral("card")); }, nullptr },
        // disk
        { "block", [](const QString &ep, const QString &n) {
              if (n.startsWith(QStringLiteral("loop")) || n.startsWith(QStringLiteral("ram")))
                  return false;
              return !QFileInfo::exists(ep + QStringLiteral("/partition"));
          }, nullptr },
        // input
        { "input", [](const QString &, const QString &n) {
              // only the inputN nodes (skip event*/mouse*/kbd*/js* interfaces)
              return n.startsWith(QStringLiteral("input"));
          },
          [](const QString &ep) { return readFile(ep + QStringLiteral("/name")); } },
        // camera
        { "video4linux", [](const QString &, const QString &n) { return n.startsWith(QStringLiteral("video")); }, nullptr },
        // battery
        { "power_supply", [](const QString &, const QString &) { return true; },
          [](const QString &ep) {
              const QString type = readFile(ep + QStringLiteral("/type"));
              const QString name = QFileInfo(ep).fileName();
              if (type == QLatin1String("Battery"))
                  return Translator::translate(QStringLiteral("laptopBattery"));
              if (type == QLatin1String("Mains"))
                  return Translator::translate(QStringLiteral("acPower"));
              if (type == QLatin1String("USB"))
                  return Translator::translate(QStringLiteral("usbPower"));
              return name;
          } },
        // sensors
        { "hwmon", [](const QString &, const QString &) { return true; }, nullptr },
        // thermal
        { "thermal", [](const QString &, const QString &n) { return n.startsWith(QStringLiteral("thermal_zone")); }, nullptr },
        // wireless
        { "rfkill", [](const QString &, const QString &) { return true; }, nullptr },
        // clock
        { "rtc", [](const QString &, const QString &n) { return n.startsWith(QStringLiteral("rtc")); }, nullptr },
        // security
        { "tpm", [](const QString &, const QString &) { return true; }, nullptr },
        // system
        { "watchdog", [](const QString &, const QString &) { return true; }, nullptr },
        // firmware
        { "dmi", [](const QString &, const QString &n) { return n == QLatin1String("id"); }, nullptr },
    };

    for (const ClassScan &cs : scans) {
        const QString base = QStringLiteral("/sys/class/") + QLatin1String(cs.cls);
        const QDir dir(base);
        const QStringList names = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        QSet<QString> seenAnchors;
        for (const QString &name : names) {
            const QString entryPath = base + QLatin1Char('/') + name;
            if (cs.keep && !cs.keep(entryPath, name))
                continue;

            QString anchor = canonicalPath(entryPath + QStringLiteral("/device"));
            if (anchor.isEmpty())
                anchor = canonicalPath(entryPath);

            // input event interfaces point back to the inputN device node
            if (QLatin1String(cs.cls) == QLatin1String("input")
                && !anchor.isEmpty() && QFileInfo::exists(anchor + QStringLiteral("/name")))
                continue;
            // HDMI jack detection input nodes duplicate the sound device
            if (QLatin1String(cs.cls) == QLatin1String("input") && anchor.contains(QStringLiteral("/sound/")))
                continue;

            if (seenAnchors.contains(anchor))
                continue;
            seenAnchors.insert(anchor);

            DeviceEntry *e = entryForAnchor(anchor);
            if (e->category.isEmpty())
                e->category = categoryFromClass(QLatin1String(cs.cls));
            if (cs.explicitName) {
                const QString n = cs.explicitName(entryPath);
                if (!n.isEmpty())
                    e->name = n;
            }
            e->entryPath = entryPath;
            e->entryName = name;
        }
    }
}

void DeviceManager::scanBus(const QString &bus)
{
    const QString base = QStringLiteral("/sys/bus/") + bus + QStringLiteral("/devices");
    const QDir dir(base);
    const QStringList names = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &name : names) {
        const QString entryPath = base + QLatin1Char('/') + name;
        const QString anchor = canonicalPath(entryPath);
        if (anchor.isEmpty())
            continue;

        // USB interfaces (3-1:1.0) are function nodes, not physical devices
        if (bus == QLatin1String("usb") && name.contains(QLatin1Char(':')))
            continue;
        // the platform bus also lists ACPI children of PCI devices; only keep
        // genuine top-level platform devices
        if (bus == QLatin1String("platform") && !anchor.startsWith(QStringLiteral("/sys/devices/platform/")))
            continue;

        DeviceEntry *e = entryForAnchor(anchor);
        if (e->category.isEmpty()) {
            if (bus == QLatin1String("pci"))
                e->category = QStringLiteral("system");
            else if (bus == QLatin1String("usb"))
                e->category = QStringLiteral("usb-ctrl");
            else if (bus == QLatin1String("mmc"))
                e->category = QStringLiteral("storage-ctrl");
            else
                e->category = QStringLiteral("system");
        }
        if (e->bus.isEmpty())
            e->bus = bus;
        if (e->entryPath.isEmpty()) {
            e->entryPath = entryPath;
            e->entryName = name;
        }

        // USB devices bind drivers at their interface nodes (4-1.5:1.0)
        if (bus == QLatin1String("usb") && e->driver.isEmpty()) {
            const QDir devDir(entryPath);
            const QStringList subs = devDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
            for (const QString &s : subs) {
                if (!s.contains(QLatin1Char(':')))
                    continue;
                QFileInfo fi(entryPath + QLatin1Char('/') + s + QStringLiteral("/driver"));
                if (fi.isSymLink()) {
                    e->driver = QFileInfo(fi.symLinkTarget()).fileName();
                    break;
                }
            }
        }
    }
}

void DeviceManager::scanCpus()
{
    const QString modelName = [] {
        QFile f(QStringLiteral("/proc/cpuinfo"));
        if (!f.open(QIODevice::ReadOnly))
            return QString();
        const QString data = QString::fromUtf8(f.readAll());
        for (const QString &line : data.split(QLatin1Char('\n'))) {
            if (line.startsWith(QStringLiteral("model name"))) {
                const int c = line.indexOf(QLatin1Char(':'));
                return line.mid(c + 1).trimmed();
            }
        }
        return QString();
    }();

    const QDir dir(QStringLiteral("/sys/devices/system/cpu"));
    const QStringList names = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &name : names) {
        if (!name.startsWith(QStringLiteral("cpu")))
            continue;
        bool ok = false;
        name.mid(3).toInt(&ok);
        if (!ok)
            continue;
        const QString anchor = canonicalPath(dir.absoluteFilePath(name));
        if (anchor.isEmpty())
            continue;
        DeviceEntry *e = entryForAnchor(anchor);
        if (e->category.isEmpty())
            e->category = QStringLiteral("processor");
        if (e->bus.isEmpty())
            e->bus = QStringLiteral("other");
        if (e->name.isEmpty())
            e->name = modelName.isEmpty() ? name : modelName;
        if (e->entryPath.isEmpty()) {
            e->entryPath = dir.absoluteFilePath(name);
            e->entryName = name;
        }
    }
}

void DeviceManager::finalizeDevice(DeviceEntry &e)
{
    if (e.name.isEmpty())
        e.name = e.entryName;

    const QString anchor = e.anchor;
    const QString entryPath = e.entryPath;

    // driver ---------------------------------------------------------------
    if (e.driver.isEmpty() && e.category != QLatin1String("processor"))
        e.driver = findDriver(anchor);

    // bus (class entries without a bus scan) --------------------------------
    if (e.bus.isEmpty())
        e.bus = busFromPath(anchor);
    if (e.bus == QLatin1String("other") && anchor.contains(QStringLiteral("/serial8250")))
        e.bus = QStringLiteral("platform");

    // vendor ----------------------------------------------------------------
    if (e.vendor.isEmpty()) {
        // PCI function nodes have the "vendor" attribute; SCSI devices also
        // expose a "vendor" file, so only trust it for real PCI BDF nodes.
        static const QRegularExpression bdfRe(
            QStringLiteral("^0000:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}\\.[0-9a-fA-F]$"));
        const QString baseName = QFileInfo(anchor).fileName();
        const QString pciVendor = readFile(anchor + QStringLiteral("/vendor"));
        const QString usbVendor = readFile(anchor + QStringLiteral("/idVendor"));
        const QString usbMfr = readFile(anchor + QStringLiteral("/manufacturer"));
        if (bdfRe.match(baseName).hasMatch())
            e.vendor = vendorName(pciVendor);
        else if (!usbVendor.isEmpty())
            e.vendor = vendorName(usbVendor);
        else if (!usbMfr.isEmpty() && usbMfr != QLatin1String("unknown"))
            e.vendor = usbMfr;
        else
            e.vendor = findUp(anchor, { QStringLiteral("manufacturer") });
    }

    // modalias --------------------------------------------------------------
    if (e.modalias.isEmpty())
        e.modalias = readFile(anchor + QStringLiteral("/modalias"));

    // DMI device vendor -----------------------------------------------------
    if (e.category == QLatin1String("firmware")) {
        const QString sysVendor = readFile(entryPath + QStringLiteral("/sys_vendor"));
        if (!sysVendor.isEmpty())
            e.vendor = sysVendor;
    }

    // status ----------------------------------------------------------------
    e.status = QStringLiteral("ok");
    if (e.category == QLatin1String("wireless")) {
        if (readFile(anchor + QStringLiteral("/hard")) == QLatin1String("1")
            || readFile(anchor + QStringLiteral("/soft")) == QLatin1String("1"))
            e.status = QStringLiteral("disabled");
    } else if (e.category == QLatin1String("network")) {
        const QString carrier = readFile(entryPath + QStringLiteral("/carrier"));
        if (carrier == QLatin1String("0"))
            e.status = QStringLiteral("unplugged");
    }
    if (e.status == QLatin1String("ok")) {
        const QString rs = readFile(anchor + QStringLiteral("/power/runtime_status"));
        if (rs == QLatin1String("suspended"))
            e.status = QStringLiteral("suspended");
    }

    // name ------------------------------------------------------------------
    const QString driverName = driverDisplayName(e.driver);
    if (e.category == QLatin1String("network")) {
        if (e.entryName == QLatin1String("lo"))
            e.name = Translator::translate(QStringLiteral("loopbackAdapter"));
        else if (!driverName.isEmpty())
            e.name = driverName + QStringLiteral(" (") + e.entryName + QLatin1Char(')');
        else
            e.name = e.entryName;
    } else if (e.category == QLatin1String("display") || e.category == QLatin1String("sound")
               || e.category == QLatin1String("clock")) {
        if (!driverName.isEmpty())
            e.name = driverName;
    } else if (e.category == QLatin1String("disk")) {
        const QString model = findUp(anchor, { QStringLiteral("model"), QStringLiteral("product") });
        if (!model.isEmpty())
            e.name = model + QStringLiteral(" (") + e.entryName + QLatin1Char(')');
    } else if (e.category == QLatin1String("sensors")) {
        if (!driverName.isEmpty()) {
            e.name = driverName;
        } else {
            const QString n = readFile(entryPath + QStringLiteral("/name"));
            if (n == QLatin1String("coretemp") || n == QLatin1String("k10temp"))
                e.name = Translator::translate(QStringLiteral("cpuTempSensor"));
            else if (!n.isEmpty())
                e.name = n;
        }
    } else if (e.category == QLatin1String("usb-ctrl")) {
        if (e.driver == QLatin1String("hub")) {
            if (e.entryName.startsWith(QStringLiteral("usb")))
                e.name = findUp(anchor, { QStringLiteral("product"), QStringLiteral("name") });
            else
                e.name = Translator::translate(QStringLiteral("usbHub")) + QStringLiteral(" (")
                    + e.entryName + QLatin1Char(')');
        } else {
            const QString product = findUp(anchor, { QStringLiteral("product"), QStringLiteral("name") });
            if (!product.isEmpty())
                e.name = product;
        }
    } else if (e.category == QLatin1String("thermal")) {
        const QString t = readFile(entryPath + QStringLiteral("/type"));
        if (!t.isEmpty())
            e.name = t;
    } else if (e.category == QLatin1String("rfkill")) {
        const QString t = readFile(entryPath + QStringLiteral("/type"));
        if (t == QLatin1String("wlan"))
            e.name = Translator::translate(QStringLiteral("wlanAdapter"));
        else if (t == QLatin1String("bluetooth"))
            e.name = Translator::translate(QStringLiteral("bluetoothAdapter"));
        else if (t == QLatin1String("wwan"))
            e.name = Translator::translate(QStringLiteral("mobileBroadbandDevice"));
    } else if (e.category == QLatin1String("firmware")) {
        if (e.entryName == QLatin1String("id"))
            e.name = Translator::translate(QStringLiteral("systemFirmware"));
    } else if (e.category == QLatin1String("system")) {
        // PCI devices: try the driver table, then the PCI class table
        if (!driverName.isEmpty()) {
            e.name = driverName;
        } else {
            const QString cls = readFile(anchor + QStringLiteral("/class"));
            if (cls.size() >= 2 && e.bus == QLatin1String("pci")) {
                const QString cn = kPciClasses.value(cls.mid(2, 2));
                if (!cn.isEmpty() && !e.vendor.isEmpty())
                    e.name = e.vendor + QLatin1Char(' ') + Translator::translate(cn);
            }
        }
    } else if (e.category == QLatin1String("battery")) {
        // explicitName already set in the class scan
    } else if (e.category == QLatin1String("input") || e.category == QLatin1String("camera")) {
        if (e.name.isEmpty() || e.name == e.entryName) {
            const QString product = findUp(anchor, { QStringLiteral("product"), QStringLiteral("name") });
            if (!product.isEmpty())
                e.name = product;
        }
    }

    if (e.name.isEmpty())
        e.name = e.entryName.isEmpty() ? QFileInfo(anchor).fileName() : e.entryName;

    // props -----------------------------------------------------------------
    QVariantList props;
    auto tr = [](const QString &s) { return Translator::translate(s); };
    auto addProp = [&props](const QString &k, const QString &v) {
        if (!v.isEmpty())
            props.append(QVariantMap{ { QStringLiteral("name"), k }, { QStringLiteral("value"), v } });
    };

    addProp(tr(QStringLiteral("deviceType")), categoryName(e.category));
    addProp(tr(QStringLiteral("status")), statusName(e.status));
    addProp(tr(QStringLiteral("manufacturer")), e.vendor);
    addProp(tr(QStringLiteral("bus")), busName(e.bus));
    addProp(tr(QStringLiteral("driver")), e.driver.isEmpty() ? QStringLiteral("—") : e.driver);
    addProp(tr(QStringLiteral("hardwareId")), e.modalias.isEmpty() ? QStringLiteral("—") : e.modalias);
    addProp(tr(QStringLiteral("deviceInstancePath")), anchor);

    if (e.category == QLatin1String("network")) {
        addProp(tr(QStringLiteral("interface")), e.entryName);
        addProp(tr(QStringLiteral("macAddress")), readFile(entryPath + QStringLiteral("/address")));
        const QString speed = readFile(entryPath + QStringLiteral("/speed"));
        if (!speed.isEmpty())
            addProp(tr(QStringLiteral("linkSpeed")), speed + QStringLiteral(" Mb/s"));
    } else if (e.category == QLatin1String("disk")) {
        bool ok = false;
        const qint64 sectors = readFile(entryPath + QStringLiteral("/size")).toLongLong(&ok);
        if (ok && sectors > 0)
            addProp(tr(QStringLiteral("capacity")), humanBytes(sectors * 512));
        const QString removable = readFile(entryPath + QStringLiteral("/removable"));
        if (!removable.isEmpty())
            addProp(tr(QStringLiteral("removable")),
                    removable == QLatin1String("1") ? tr(QStringLiteral("yes")) : tr(QStringLiteral("no")));
        addProp(tr(QStringLiteral("deviceNode")), readFile(entryPath + QStringLiteral("/dev")));
    } else if (e.category == QLatin1String("usb-ctrl") || (e.bus == QLatin1String("usb") && e.category == QLatin1String("other"))) {
        addProp(tr(QStringLiteral("vendorId")), readFile(anchor + QStringLiteral("/idVendor")));
        addProp(tr(QStringLiteral("productId")), readFile(anchor + QStringLiteral("/idProduct")));
        addProp(tr(QStringLiteral("serialNumber")), readFile(anchor + QStringLiteral("/serial")));
    } else if (e.bus == QLatin1String("pci")) {
        addProp(tr(QStringLiteral("pciVendorId")), readFile(anchor + QStringLiteral("/vendor")));
        addProp(tr(QStringLiteral("pciDeviceId")), readFile(anchor + QStringLiteral("/device")));
        const QString cls = readFile(anchor + QStringLiteral("/class"));
        if (cls.size() >= 2)
            addProp(tr(QStringLiteral("pciClass")), tr(kPciClasses.value(cls.mid(2, 2), cls)));
    } else if (e.category == QLatin1String("processor")) {
        addProp(tr(QStringLiteral("logicalCore")), e.entryName);
        const QString freq = readFile(anchor + QStringLiteral("/cpufreq/cpuinfo_max_freq"));
        if (!freq.isEmpty())
            addProp(tr(QStringLiteral("maxFrequency")), QString::number(freq.toDouble() / 1000.0, 'f', 0) + QStringLiteral(" MHz"));
    } else if (e.category == QLatin1String("thermal")) {
        const QString t = readFile(entryPath + QStringLiteral("/temp"));
        if (!t.isEmpty())
            addProp(tr(QStringLiteral("currentTemperature")), QString::number(t.toDouble() / 1000.0, 'f', 1) + QStringLiteral(" °C"));
    } else if (e.category == QLatin1String("battery")) {
        addProp(tr(QStringLiteral("chargeLevel")), readFile(entryPath + QStringLiteral("/capacity")) + QStringLiteral("%"));
        addProp(tr(QStringLiteral("powerStatus")), tr(readFile(entryPath + QStringLiteral("/status"))));
    } else if (e.category == QLatin1String("firmware")) {
        const QString dmi = entryPath;
        addProp(tr(QStringLiteral("productModel")), readFile(dmi + QStringLiteral("/product_name")));
        addProp(tr(QStringLiteral("motherboardModel")), readFile(dmi + QStringLiteral("/board_name")));
        addProp(tr(QStringLiteral("biosVersion")), readFile(dmi + QStringLiteral("/bios_version")));
        addProp(tr(QStringLiteral("productSerial")), readFile(dmi + QStringLiteral("/product_serial")));
    }

    e.props = props;
}

void DeviceManager::buildGroups()
{
    // type view -------------------------------------------------------------
    QHash<QString, QVariantList> byCat;
    for (const DeviceEntry &e : m_devices) {
        QVariantMap m = e.toMap();
        m.insert(QStringLiteral("subtitle"), [&] {
            if (!e.vendor.isEmpty() && !e.driver.isEmpty())
                return e.vendor + QStringLiteral(" · ") + e.driver;
            if (!e.vendor.isEmpty())
                return e.vendor;
            if (!e.driver.isEmpty())
                return e.driver;
            return busName(e.bus);
        }());
        m.insert(QStringLiteral("icon"), [&] {
            const QString di = driverIcon(e.driver);
            return di.isEmpty() ? categoryIcon(e.category) : di;
        }());
        m.insert(QStringLiteral("categoryName"), categoryName(e.category));
        byCat[e.category].append(m);
    }

    m_typeGroups.clear();
    QSet<QString> used;
    // emit in fixed order, then any leftovers
    QList<QPair<int, QString>> ordered;
    for (const CategoryInfo &ci : kCategories)
        ordered.append({ ci.order, QString::fromUtf8(ci.key) });
    for (const QString &key : byCat.keys())
        if (!categoryInfo(kCategories, int(std::size(kCategories)), key))
            ordered.append({ 99, key });
    std::sort(ordered.begin(), ordered.end(), [](const QPair<int, QString> &a, const QPair<int, QString> &b) {
        return a.first < b.first;
    });
    for (const auto &[order, key] : ordered) {
        if (!byCat.contains(key) || used.contains(key))
            continue;
        used.insert(key);
        QVariantMap g;
        g.insert(QStringLiteral("key"), key);
        g.insert(QStringLiteral("name"), categoryName(key));
        g.insert(QStringLiteral("icon"), categoryIcon(key));
        g.insert(QStringLiteral("devices"), byCat.value(key));
        m_typeGroups.append(g);
    }

    // connection view --------------------------------------------------------
    QHash<QString, QVariantList> byBus;
    for (const DeviceEntry &e : m_devices) {
        QVariantMap m = e.toMap();
        m.insert(QStringLiteral("subtitle"), e.driver.isEmpty() ? e.name : e.driver);
        m.insert(QStringLiteral("icon"), [&] {
            const QString di = driverIcon(e.driver);
            return di.isEmpty() ? categoryIcon(e.category) : di;
        }());
        m.insert(QStringLiteral("categoryName"), categoryName(e.category));
        byBus[e.bus.isEmpty() ? QStringLiteral("other") : e.bus].append(m);
    }

    m_connectionGroups.clear();
    QSet<QString> usedBus;
    for (const CategoryInfo &ci : kConnections) {
        const QString key = QString::fromUtf8(ci.key);
        if (!byBus.contains(key))
            continue;
        usedBus.insert(key);
        QVariantMap g;
        g.insert(QStringLiteral("key"), key);
        g.insert(QStringLiteral("name"), busName(key));
        g.insert(QStringLiteral("icon"), QString::fromUtf8(ci.icon));
        g.insert(QStringLiteral("devices"), byBus.value(key));
        m_connectionGroups.append(g);
    }
    for (const QString &key : byBus.keys()) {
        if (usedBus.contains(key))
            continue;
        QVariantMap g;
        g.insert(QStringLiteral("key"), key);
        g.insert(QStringLiteral("name"), busName(key));
        g.insert(QStringLiteral("icon"), QStringLiteral("devices_other"));
        g.insert(QStringLiteral("devices"), byBus.value(key));
        m_connectionGroups.append(g);
    }

    // summary ---------------------------------------------------------------
    m_deviceCount = m_devices.size();
    m_problemCount = 0;
    for (const DeviceEntry &e : m_devices)
        if (e.status != QLatin1String("ok"))
            ++m_problemCount;
}

void DeviceManager::enumerate()
{
    m_devices.clear();
    m_lastError.clear();

    scanClasses();
    scanBus(QStringLiteral("pci"));
    scanBus(QStringLiteral("usb"));
    scanBus(QStringLiteral("platform"));
    scanCpus();

    // platform scan may create noisy synthetic entries; drop them
    static const QStringList platformSkips = {
        QStringLiteral("serial8250"), QStringLiteral("kgdboc"), QStringLiteral("power"),
        QStringLiteral("efivars.0"), QStringLiteral("uevent"), QStringLiteral("INT33A0:00"),
    };
    m_devices.erase(std::remove_if(m_devices.begin(), m_devices.end(), [&](const DeviceEntry &e) {
        if (e.bus == QLatin1String("platform")) {
            const QString base = QFileInfo(e.anchor).fileName();
            if (platformSkips.contains(base))
                return true;
            // keep only platform devices that have a driver or a class
            if (e.driver.isEmpty() && !QFileInfo::exists(e.anchor + QStringLiteral("/modalias")))
                return true;
        }
        // classless PCI root/host bridges are fine; drop purely virtual noise
        if (e.anchor.startsWith(QStringLiteral("/sys/devices/virtual"))
            && e.category == QLatin1String("system") && e.driver.isEmpty())
            return true;
        return false;
    }), m_devices.end());

    for (DeviceEntry &e : m_devices)
        finalizeDevice(e);

    buildGroups();
}

void DeviceManager::refresh()
{
    enumerate();
    emit groupsChanged();
}
