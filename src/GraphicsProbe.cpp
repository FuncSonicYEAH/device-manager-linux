#include "GraphicsProbe.h"
#include "Translator.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QTimer>

// ---------------------------------------------------------------------------
// Small lookup tables
// ---------------------------------------------------------------------------

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
    { QStringLiteral("1013"), QStringLiteral("Cirrus Logic") },
    { QStringLiteral("1ae0"), QStringLiteral("Google") },
};

// Kernel driver -> translation key; reuses the DeviceManager driver table
// keys where possible so names stay consistent with the main window.
const QHash<QString, QString> kDriverNames = {
    { QStringLiteral("i915"), QStringLiteral("intelIntegratedGraphics") },
    { QStringLiteral("amdgpu"), QStringLiteral("amdGraphics") },
    { QStringLiteral("radeon"), QStringLiteral("amdGraphics") },
    { QStringLiteral("nvidia"), QStringLiteral("nvidiaGraphics") },
    { QStringLiteral("nouveau"), QStringLiteral("nvidiaGraphics") },
    { QStringLiteral("vmwgfx"), QStringLiteral("vmwareGraphics") },
    { QStringLiteral("virtio_gpu"), QStringLiteral("virtioGraphics") },
    { QStringLiteral("qxl"), QStringLiteral("qxlGraphics") },
    { QStringLiteral("vboxvideo"), QStringLiteral("vboxGraphics") },
    { QStringLiteral("cirrus"), QStringLiteral("cirrusGraphics") },
    { QStringLiteral("mga"), QStringLiteral("mgaGraphics") },
    { QStringLiteral("simpledrm"), QStringLiteral("simpleFramebuffer") },
    { QStringLiteral("efifb"), QStringLiteral("simpleFramebuffer") },
    { QStringLiteral("vc4"), QStringLiteral("broadcomGraphics") },
    { QStringLiteral("v3d"), QStringLiteral("broadcomGraphics") },
};

// Vulkan ICD file-name fragment -> driver display name
const QHash<QString, QString> kVkIcdNames = {
    { QStringLiteral("google_swiftshader"), QStringLiteral("SwiftShader (software)") },
    { QStringLiteral("intel"), QStringLiteral("Intel") },
    { QStringLiteral("radeon"), QStringLiteral("AMD (RADV)") },
    { QStringLiteral("amd"), QStringLiteral("AMD") },
    { QStringLiteral("nvidia"), QStringLiteral("NVIDIA") },
    { QStringLiteral("lvp"), QStringLiteral("LLVMpipe (software)") },
    { QStringLiteral("panfrost"), QStringLiteral("ARM Panfrost") },
    { QStringLiteral("mali"), QStringLiteral("ARM Mali") },
    { QStringLiteral("powervr"), QStringLiteral("PowerVR") },
    { QStringLiteral("qualcomm"), QStringLiteral("Qualcomm") },
    { QStringLiteral("v3d"), QStringLiteral("Broadcom") },
    { QStringLiteral("bcm"), QStringLiteral("Broadcom") },
    { QStringLiteral("virtio"), QStringLiteral("VirtIO") },
};

QString prettyName(const QString &raw)
{
    QString base = raw;
    // glvnd manifests follow the "NN_name.json" convention
    static const QRegularExpression prefixRe(QStringLiteral("^\\d+_"));
    base.remove(prefixRe);
    if (base.compare(QStringLiteral("mesa"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("Mesa");
    if (base.compare(QStringLiteral("nvidia"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("NVIDIA");
    if (base.compare(QStringLiteral("mali"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("ARM Mali");
    if (base.compare(QStringLiteral("arm"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("ARM");
    if (base.compare(QStringLiteral("amd"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("AMD");
    if (base.compare(QStringLiteral("broadcom"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("Broadcom");
    if (base.isEmpty())
        return QString();
    base[0] = base[0].toUpper();
    return base;
}

} // namespace

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

QString GraphicsProbe::readFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return QString();
    return QString::fromUtf8(f.readAll()).trimmed();
}

QString GraphicsProbe::canonicalPath(const QString &path)
{
    return QFileInfo(path).canonicalFilePath();
}

// Looks for a shared library by basename pattern (e.g. "libvulkan.so*") in the
// standard library directories plus $LD_LIBRARY_PATH. Follows symlinks, so the
// usual loader symlink chains (libvulkan.so.1 -> libvulkan.so.1.3.275) count.
QStringList GraphicsProbe::findLibraries(const QString &namePattern)
{
    QStringList dirs;
    const QByteArray env = qgetenv("LD_LIBRARY_PATH");
    for (const QByteArray &p : env.split(':')) {
        if (!p.isEmpty())
            dirs.append(QString::fromLocal8Bit(p));
    }
    dirs.append({
        QStringLiteral("/usr/lib"), QStringLiteral("/usr/lib64"),
        QStringLiteral("/usr/lib/x86_64-linux-gnu"), QStringLiteral("/usr/lib/i386-linux-gnu"),
        QStringLiteral("/usr/lib/i686-linux-gnu"), QStringLiteral("/usr/lib/aarch64-linux-gnu"),
        QStringLiteral("/usr/lib/arm-linux-gnueabihf"), QStringLiteral("/usr/lib/powerpc64le-linux-gnu"),
        QStringLiteral("/usr/lib/riscv64-linux-gnu"), QStringLiteral("/usr/lib32"),
        QStringLiteral("/usr/local/lib"), QStringLiteral("/lib"), QStringLiteral("/lib64"),
    });

    QSet<QString> seen;
    QStringList found;
    for (const QString &d : dirs) {
        if (seen.contains(d))
            continue;
        seen.insert(d);
        const QDir dir(d);
        if (!dir.exists())
            continue;
        const QStringList names = dir.entryList({ namePattern }, QDir::Files, QDir::Name);
        for (const QString &n : names)
            found.append(d + QLatin1Char('/') + n);
    }
    return found;
}

// "50_mesa.json" -> "Mesa", "10_nvidia.json" -> "NVIDIA", ...
QString GraphicsProbe::providerNameFromFile(const QString &fileName)
{
    return prettyName(QFileInfo(fileName).completeBaseName());
}

// "intel_icd.x86_64.json" -> "Intel", "radeon_icd.json" -> "AMD (RADV)", ...
QString GraphicsProbe::icdNameFromFile(const QString &fileName)
{
    QString base = QFileInfo(fileName).completeBaseName();
    base = base.split(QLatin1Char('.')).first(); // strip arch suffixes
    if (base.endsWith(QStringLiteral("_icd")))
        base.chop(4);
    for (auto it = kVkIcdNames.constBegin(); it != kVkIcdNames.constEnd(); ++it) {
        if (base.contains(it.key()))
            return it.value();
    }
    return prettyName(base);
}

// ---------------------------------------------------------------------------
// GraphicsProbe
// ---------------------------------------------------------------------------

GraphicsProbe::GraphicsProbe(QObject *parent)
    : QObject(parent)
{
    refresh();
}

void GraphicsProbe::refresh()
{
    detectGpus();
    detectOpenGL();
    detectVulkan();

    // per-GPU capability notes -------------------------------------------------
    for (Gpu &g : m_gpuList) {
        // OpenGL: the NVIDIA proprietary driver only works with its own GLX
        // library; every other kernel driver is served by Mesa (which also
        // provides llvmpipe/swrast software rendering as a fallback).
        if (g.driver == QLatin1String("nvidia")) {
            g.glSupported = m_openGLProviders.contains(QStringLiteral("NVIDIA"));
            g.glNote = g.glSupported ? QStringLiteral("NVIDIA") : QString();
        } else {
            g.glSupported = m_openGLSupported;
            g.glNote = m_openGLProviders.join(QStringLiteral(", "));
        }

        // Vulkan: match the GPU vendor against the installed ICDs; without a
        // vendor ICD, LLVMpipe still provides software Vulkan.
        const bool intel = g.vendorId == QLatin1String("8086");
        const bool amd = g.vendorId == QLatin1String("1002");
        const bool nvidia = g.vendorId == QLatin1String("10de");
        QString icd;
        for (const QString &d : m_vulkanDrivers) {
            if ((intel && d.contains(QStringLiteral("Intel")))
                || (amd && d.contains(QStringLiteral("AMD")))
                || (nvidia && d.contains(QStringLiteral("NVIDIA")))) {
                icd = d;
                break;
            }
        }
        if (icd.isEmpty() && m_vulkanDrivers.contains(QStringLiteral("LLVMpipe (software)")))
            icd = QStringLiteral("LLVMpipe (software)");
        g.vkSupported = !icd.isEmpty();
        g.vkNote = icd;
    }

    m_gpus.clear();
    for (const Gpu &g : m_gpuList) {
        QVariantMap m;
        m.insert(QStringLiteral("id"), g.id);
        m.insert(QStringLiteral("name"), g.name);
        m.insert(QStringLiteral("vendor"), g.vendor);
        m.insert(QStringLiteral("driver"), g.driver);
        m.insert(QStringLiteral("glSupported"), g.glSupported);
        m.insert(QStringLiteral("glNote"), g.glNote);
        m.insert(QStringLiteral("vkSupported"), g.vkSupported);
        m.insert(QStringLiteral("vkNote"), g.vkNote);
        m_gpus.append(m);
    }

    emit changed();
}

void GraphicsProbe::detectGpus()
{
    m_gpuList.clear();
    QSet<QString> seen;

    auto addGpu = [&](const QString &anchor) {
        if (anchor.isEmpty() || seen.contains(anchor))
            return;
        seen.insert(anchor);
        Gpu g;
        g.id = anchor;
        const QFileInfo drv(anchor + QStringLiteral("/driver"));
        if (drv.isSymLink())
            g.driver = QFileInfo(drv.symLinkTarget()).fileName();

        QString vendorHex = readFile(anchor + QStringLiteral("/vendor"));
        vendorHex.remove(QLatin1String("0x"));
        vendorHex = vendorHex.toLower();
        g.vendorId = vendorHex;
        g.vendor = kVendors.value(vendorHex, vendorHex.isEmpty() ? QString() : vendorHex);

        const QString key = kDriverNames.value(g.driver);
        if (!key.isEmpty()) {
            g.name = Translator::translate(key);
        } else if (!g.vendor.isEmpty()) {
            g.name = g.vendor;
        } else {
            g.name = g.driver.isEmpty() ? QFileInfo(anchor).fileName() : g.driver;
        }
        m_gpuList.append(g);
    };

    // DRM cards first (each card maps to one render device)
    const QDir drm(QStringLiteral("/sys/class/drm"));
    const QStringList drmNames = drm.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &n : drmNames) {
        if (!n.startsWith(QStringLiteral("card")) || n.contains(QLatin1Char('-')))
            continue; // "card0", not "card0-DP-1"
        addGpu(canonicalPath(drm.absoluteFilePath(n) + QStringLiteral("/device")));
    }

    // PCI display controllers without a DRM driver (e.g. headless setups)
    const QDir pci(QStringLiteral("/sys/bus/pci/devices"));
    const QStringList pciNames = pci.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &n : pciNames) {
        const QString cls = readFile(pci.absoluteFilePath(n) + QStringLiteral("/class"));
        if (!cls.startsWith(QStringLiteral("0x0300")))
            continue;
        addGpu(canonicalPath(pci.absoluteFilePath(n)));
    }
}

void GraphicsProbe::detectOpenGL()
{
    m_openGLProviders.clear();
    m_openGLSupported = false;

    QSet<QString> providers;

    // glvnd EGL vendor manifests name the OpenGL providers explicitly
    const QStringList glvndDirs = {
        QStringLiteral("/usr/share/glvnd/egl_vendor.d"),
        QStringLiteral("/etc/glvnd/egl_vendor.d"),
    };
    for (const QString &d : glvndDirs) {
        const QDir dir(d);
        if (!dir.exists())
            continue;
        for (const QString &f : dir.entryList({ QStringLiteral("*.json") }, QDir::Files, QDir::Name)) {
            const QString p = providerNameFromFile(f);
            if (!p.isEmpty())
                providers.insert(p);
        }
    }

    // ...or the driver libraries themselves
    if (!findLibraries(QStringLiteral("libGLX_mesa.so*")).isEmpty())
        providers.insert(QStringLiteral("Mesa"));
    if (!findLibraries(QStringLiteral("libGLX_nvidia.so*")).isEmpty())
        providers.insert(QStringLiteral("NVIDIA"));
    if (!findLibraries(QStringLiteral("libGL_mesa.so*")).isEmpty())
        providers.insert(QStringLiteral("Mesa"));
    if (!findLibraries(QStringLiteral("libEGL_mesa.so*")).isEmpty())
        providers.insert(QStringLiteral("Mesa"));
    if (!findLibraries(QStringLiteral("libGL.so*")).isEmpty())
        providers.insert(QStringLiteral("Mesa")); // legacy standalone GL

    // stable display order
    static const QStringList order = {
        QStringLiteral("Mesa"), QStringLiteral("NVIDIA"), QStringLiteral("AMD"),
        QStringLiteral("ARM Mali"), QStringLiteral("ARM"), QStringLiteral("Broadcom"),
    };
    for (const QString &o : order)
        if (providers.contains(o))
            m_openGLProviders.append(o);
    for (const QString &p : providers)
        if (!m_openGLProviders.contains(p))
            m_openGLProviders.append(p);

    m_openGLSupported = !m_openGLProviders.isEmpty();
}

void GraphicsProbe::detectVulkan()
{
    m_vulkanDrivers.clear();
    m_vulkanApiVersion.clear();
    m_vulkanSupported = false;

    const bool loader = !findLibraries(QStringLiteral("libvulkan.so*")).isEmpty();

    struct Icd { QString name; QString version; };
    QList<Icd> icds;
    const QStringList icdDirs = {
        QStringLiteral("/usr/share/vulkan/icd.d"),
        QStringLiteral("/etc/vulkan/icd.d"),
        QStringLiteral("/usr/local/share/vulkan/icd.d"),
    };
    for (const QString &d : icdDirs) {
        const QDir dir(d);
        if (!dir.exists())
            continue;
        for (const QString &f : dir.entryList({ QStringLiteral("*.json") }, QDir::Files, QDir::Name)) {
            QFile file(d + QLatin1Char('/') + f);
            if (!file.open(QIODevice::ReadOnly))
                continue;
            const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            const QJsonObject icd = doc.object().value(QStringLiteral("ICD")).toObject();
            const QString lib = icd.value(QStringLiteral("library_path")).toString();
            // skip manifests whose driver library is not installed
            if (lib.startsWith(QLatin1Char('/'))) {
                if (!QFile::exists(lib))
                    continue;
            } else if (!lib.isEmpty() && findLibraries(lib + QStringLiteral("*")).isEmpty()) {
                continue;
            }
            Icd i;
            i.name = icdNameFromFile(f);
            i.version = icd.value(QStringLiteral("api_version")).toString();
            if (i.name.isEmpty())
                i.name = QStringLiteral("Vulkan");
            icds.append(i);
        }
    }

    m_vulkanSupported = loader && !icds.isEmpty();
    for (const Icd &i : icds) {
        if (!m_vulkanDrivers.contains(i.name))
            m_vulkanDrivers.append(i.name);
        if (i.version.isEmpty() || (!m_vulkanApiVersion.isEmpty() && m_vulkanApiVersion < i.version))
            continue;
        if (!i.version.isEmpty())
            m_vulkanApiVersion = i.version;
    }
}

void GraphicsProbe::finishDetails()
{
    if (m_runningTools > 0)
        return;
    m_loading = false;
    emit loadingChanged();
    emit changed();
}

void GraphicsProbe::requestDetails()
{
    if (m_loading)
        return;
    // only re-run when the tools are installed; file-based detection above
    // already answers the supported/not-supported question
    m_glxProc = new QProcess(this);
    m_vkProc = new QProcess(this);

    auto run = [this](QProcess *p, const QString &program, const QStringList &args,
                      std::function<void(const QByteArray &)> parse) {
        ++m_runningTools;
        connect(p, &QProcess::finished, this, [this, p, parse](int, QProcess::ExitStatus) {
            parse(p->readAllStandardOutput());
            --m_runningTools;
            p->deleteLater();
            finishDetails();
        });
        connect(p, &QProcess::errorOccurred, this, [this, p](QProcess::ProcessError err) {
            if (err == QProcess::FailedToStart) {
                --m_runningTools;
                p->deleteLater();
                finishDetails();
            }
        });
        // generous timeout; glxinfo can block on a missing X server briefly
        QTimer::singleShot(15000, p, [p]() {
            if (p->state() != QProcess::NotRunning)
                p->kill();
        });
        p->start(program, args);
    };

    bool any = false;
    if (!QStandardPaths::findExecutable(QStringLiteral("glxinfo")).isEmpty()) {
        any = true;
        run(m_glxProc, QStringLiteral("glxinfo"), { QStringLiteral("-B") },
            [this](const QByteArray &out) {
                m_openGLRenderer.clear();
                m_openGLVersion.clear();
                const QStringList lines = QString::fromUtf8(out).split(QLatin1Char('\n'));
                for (const QString &line : lines) {
                    if (line.startsWith(QStringLiteral("OpenGL renderer string:")))
                        m_openGLRenderer = line.section(QLatin1Char(':'), 1).trimmed();
                    else if (line.startsWith(QStringLiteral("OpenGL version string:")))
                        m_openGLVersion = line.section(QLatin1Char(':'), 1).trimmed();
                }
            });
    }
    if (!QStandardPaths::findExecutable(QStringLiteral("vulkaninfo")).isEmpty()) {
        any = true;
        run(m_vkProc, QStringLiteral("vulkaninfo"), { QStringLiteral("--summary") },
            [this](const QByteArray &out) {
                m_vulkanDevices.clear();
                m_vulkanDriverInfo.clear();
                const QStringList lines = QString::fromUtf8(out).split(QLatin1Char('\n'));
                for (const QString &line : lines) {
                    const int eq = line.indexOf(QLatin1Char('='));
                    if (eq < 0)
                        continue;
                    const QString key = line.left(eq).trimmed();
                    const QString value = line.mid(eq + 1).trimmed();
                    if (key == QLatin1String("deviceName")) {
                        if (!value.isEmpty() && !m_vulkanDevices.contains(value))
                            m_vulkanDevices.append(value);
                    } else if (key == QLatin1String("driverInfo")) {
                        if (!value.isEmpty() && !m_vulkanDriverInfo.contains(value))
                            m_vulkanDriverInfo.append(value);
                    }
                }
            });
    }

    if (any) {
        m_loading = true;
        emit loadingChanged();
    } else {
        m_glxProc->deleteLater();
        m_vkProc->deleteLater();
        m_glxProc = nullptr;
        m_vkProc = nullptr;
    }
}
