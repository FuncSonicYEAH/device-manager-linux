#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>

class QProcess;

// Detects whether the system's GPUs support OpenGL and Vulkan.
//
// Detection is two-tiered:
//   - file-based (synchronous, always available): GPUs are enumerated from the
//     DRM class and the PCI display controllers; OpenGL support comes from the
//     glvnd EGL vendor manifests and the GLX/EGL/Mesa libraries found in the
//     standard library paths; Vulkan support comes from the Vulkan loader and
//     the ICD manifests under /usr/share/vulkan/icd.d (and /etc, /usr/local).
//   - enrichment (asynchronous, only when the tools exist): glxinfo -B and
//     vulkaninfo --summary are run to fetch the actual renderer / device
//     strings, in the same spirit as the `Smart` backend.
//
// Exposed to QML as the `Graphics` context property.
class GraphicsProbe : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList gpus READ gpus NOTIFY changed)
    Q_PROPERTY(bool openGLSupported READ openGLSupported NOTIFY changed)
    Q_PROPERTY(QStringList openGLProviders READ openGLProviders NOTIFY changed)
    Q_PROPERTY(QString openGLRenderer READ openGLRenderer NOTIFY changed)
    Q_PROPERTY(QString openGLVersion READ openGLVersion NOTIFY changed)
    Q_PROPERTY(bool vulkanSupported READ vulkanSupported NOTIFY changed)
    Q_PROPERTY(QStringList vulkanDrivers READ vulkanDrivers NOTIFY changed)
    Q_PROPERTY(QString vulkanApiVersion READ vulkanApiVersion NOTIFY changed)
    Q_PROPERTY(QStringList vulkanDevices READ vulkanDevices NOTIFY changed)
    Q_PROPERTY(QString vulkanDriverInfo READ vulkanDriverInfo NOTIFY changed)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)

public:
    explicit GraphicsProbe(QObject *parent = nullptr);

    QVariantList gpus() const { return m_gpus; }
    bool openGLSupported() const { return m_openGLSupported; }
    QStringList openGLProviders() const { return m_openGLProviders; }
    QString openGLRenderer() const { return m_openGLRenderer; }
    QString openGLVersion() const { return m_openGLVersion; }
    bool vulkanSupported() const { return m_vulkanSupported; }
    QStringList vulkanDrivers() const { return m_vulkanDrivers; }
    QString vulkanApiVersion() const { return m_vulkanApiVersion; }
    QStringList vulkanDevices() const { return m_vulkanDevices; }
    QString vulkanDriverInfo() const { return m_vulkanDriverInfo; }
    bool loading() const { return m_loading; }

    // Re-runs the (synchronous) file-based detection.
    Q_INVOKABLE void refresh();
    // Runs glxinfo -B / vulkaninfo --summary when those tools are installed to
    // fill in the actual renderer / device strings (asynchronous).
    Q_INVOKABLE void requestDetails();

signals:
    void changed();
    void loadingChanged();

private:
    struct Gpu
    {
        QString id;       // canonical sysfs path (matches DeviceManager ids)
        QString name;
        QString vendor;   // display name
        QString vendorId; // PCI vendor id, hex without "0x"
        QString driver;
        bool glSupported = false;
        QString glNote;
        bool vkSupported = false;
        QString vkNote;
    };

    void detectGpus();
    void detectOpenGL();
    void detectVulkan();
    void finishDetails();

    static QString readFile(const QString &path);
    static QString canonicalPath(const QString &path);
    static QStringList findLibraries(const QString &namePattern);
    static QString providerNameFromFile(const QString &fileName);
    static QString icdNameFromFile(const QString &fileName);

    QList<Gpu> m_gpuList;
    QVariantList m_gpus;

    bool m_openGLSupported = false;
    QStringList m_openGLProviders;
    QString m_openGLRenderer;
    QString m_openGLVersion;

    bool m_vulkanSupported = false;
    QStringList m_vulkanDrivers;
    QString m_vulkanApiVersion;
    QStringList m_vulkanDevices;
    QString m_vulkanDriverInfo;

    bool m_loading = false;
    int m_runningTools = 0;
    QProcess *m_glxProc = nullptr;
    QProcess *m_vkProc = nullptr;
};
