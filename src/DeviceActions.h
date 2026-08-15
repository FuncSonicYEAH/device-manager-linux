#pragma once

#include <QObject>
#include <QVariantMap>

class QProcess;

// Executes device power actions (suspend / enable / start) by writing to the
// device's sysfs attributes. The writes are run through `pkexec` so a polkit
// authentication dialog is shown when the caller is not root.
//
// Semantics (runtime power management):
//   suspend  - USB: authorized=0; otherwise power/control=auto (allow runtime suspend)
//   enable   - USB: authorized=1; PCI/platform: unbind + rebind the driver
//   start    - power/control=on (force the device active / wake it up)
//
// Exposed to QML as the `DeviceActions` context property.
class DeviceActions : public QObject
{
    Q_OBJECT

public:
    explicit DeviceActions(QObject *parent = nullptr);

    // `device` is the device map from DeviceManager (needs sysfsPath, bus,
    // driver). Results are reported through actionFinished().
    Q_INVOKABLE void perform(const QVariantMap &device, const QString &action);
    // Whether the action is available for this device (checks the relevant
    // sysfs files). Synchronous, cheap.
    Q_INVOKABLE bool supportsAction(const QVariantMap &device, const QString &action) const;
    Q_INVOKABLE bool isBusy() const { return m_process != nullptr; }

signals:
    void actionFinished(const QString &deviceId, const QString &action,
                        bool ok, const QString &message);

private:
    // Builds the shell command (run as root via pkexec) for the action, or
    // returns an empty string and sets *error to a short failure reason.
    static QString commandFor(const QVariantMap &device, const QString &action, QString *error);
    static bool hasFile(const QVariantMap &device, const QString &name);

    QProcess *m_process = nullptr;
};
