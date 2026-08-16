#pragma once

#include <QList>
#include <QObject>
#include <QPair>
#include <QSet>
#include <QStringList>
#include <QVariantList>

#include <functional>

class QProcess;

// Driver detection & installation backend behind the "Drivers" dialog.
//
// Two layers of capability:
//  - kernel-module layer: devices on the pci/usb buses that have no driver
//    bound are matched against /lib/modules/$(uname -r)/modules.alias to find
//    candidate modules, which can then be loaded (pkexec modprobe) or bound
//    (sysfs drivers_probe / driver bind), mirroring what udev does at boot.
//  - distribution-package layer: the system package manager is detected
//    (dnf/apt/pacman/zypper/apk/emerge); driver packages can be searched for
//    a device and installed through pkexec with the output streamed line by
//    line so the UI can show live progress.
class DriverHelper : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList missingDrivers READ missingDrivers NOTIFY scanChanged)
    Q_PROPERTY(QVariantList loadedModules READ loadedModules NOTIFY scanChanged)
    Q_PROPERTY(QString kernelRelease READ kernelRelease NOTIFY scanChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY scanChanged)
    Q_PROPERTY(QString packageBackend READ packageBackend CONSTANT)
    Q_PROPERTY(QString packageBackendName READ packageBackendName CONSTANT)
    Q_PROPERTY(bool hasYay READ hasYay CONSTANT)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit DriverHelper(QObject *parent = nullptr);

    Q_INVOKABLE void scan();

    // kernel-module actions (pkexec, one at a time)
    Q_INVOKABLE void loadModule(const QString &module);
    Q_INVOKABLE void unloadModule(const QString &module);
    Q_INVOKABLE void probeDevice(const QVariantMap &device);
    Q_INVOKABLE void bindDevice(const QVariantMap &device, const QString &driver);

    // async `modinfo <module>`; results arrive via moduleInfoReady()
    Q_INVOKABLE void moduleInfo(const QString &module);

    // distribution package layer
    Q_INVOKABLE void searchPackages(const QVariantMap &device);
    Q_INVOKABLE void installPackage(const QString &deviceId, const QString &pkgName);

    // proprietary (closed-source) drivers: hardware scan + multi-step install
    // (RPM Fusion enablement / ubuntu-drivers / yay for AUR packages)
    QVariantList proprietaryScan(QStringList *packageNames = nullptr) const;
    Q_INVOKABLE void scanProprietary();
    Q_INVOKABLE void installProprietary(const QString &key);

    QVariantList missingDrivers() const { return m_missing; }
    QVariantList loadedModules() const { return m_modules; }
    QString kernelRelease() const { return m_kernelRelease; }
    QString lastError() const { return m_lastError; }
    QString packageBackend() const { return m_backend; }
    QString packageBackendName() const { return m_backendName; }
    bool hasYay() const { return !m_yayPath.isEmpty(); }
    bool busy() const { return m_actionProcess != nullptr || m_installProcess != nullptr; }

signals:
    void scanChanged();
    void busyChanged();

    // module/device actions; `target` is the module or device id
    void actionFinished(const QString &target, const QString &action,
                        bool ok, const QString &message);
    void moduleInfoReady(const QString &module, const QVariantMap &info);

    // package search / install
    void searchReady(const QString &deviceId, const QVariantList &results);
    void searchFailed(const QString &deviceId, const QString &message);
    void installOutput(const QString &deviceId, const QString &pkgName, const QString &line);
    void installProgress(const QString &deviceId, const QString &pkgName, int percent);
    void installPhase(const QString &deviceId, const QString &pkgName, const QString &phase);
    void installFinished(const QString &deviceId, const QString &pkgName,
                         bool ok, const QString &message);
    // proprietary driver options for the detected hardware
    void proprietaryReady(const QVariantList &options);

private:
    struct AliasEntry
    {
        QString pattern;
        QString module;
    };

    void scanBusDevices(const QString &bus, const QList<AliasEntry> &aliases,
                        const QSet<QString> &builtin, const QSet<QString> &loaded,
                        QVariantList *out);
    void detectBackend();
    QStringList curatedPackages(const QVariantMap &device, QString *searchTerm) const;
    void queryInstalledState(const QStringList &names,
                             const std::function<void(const QSet<QString> &)> &done);
    void runAction(const QStringList &pkexecArgs, const QString &target, const QString &action);
    QString proprietaryPackage(const QString &key) const;
    void runStreamingInstall(const QString &program, const QStringList &args,
                             const QString &deviceId, const QString &pkgName);

    QVariantList m_missing;
    QVariantList m_modules;
    QString m_kernelRelease;
    QString m_lastError;
    QString m_backend;        // dnf | apt | pacman | zypper | apk | emerge | unknown
    QString m_backendName;
    QString m_backendPath;    // absolute executable path
    QString m_yayPath;        // AUR helper for closed-source packages on Arch
    QString m_ubuntuDriversPath;

    QProcess *m_actionProcess = nullptr;   // modprobe/rmmod/probe/bind
    QProcess *m_installProcess = nullptr;  // package install (streams)
    QProcess *m_searchProcess = nullptr;   // package search (replaced freely)
    QString m_outBuffer;                   // partial install stdout lines
    QString m_errBuffer;                   // partial install stderr lines
};
