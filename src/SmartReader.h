#pragma once

#include <QObject>
#include <QVariantMap>

class QProcess;

// Runs `smartctl -a -j <node>` (smartmontools) asynchronously for a block
// device and exposes the parsed SMART data to QML (exposed as the `Smart`
// context property). The device node (e.g. /dev/sda) must be passed in;
// permission problems (root required) are reported in the result map so the
// UI can translate them.
class SmartReader : public QObject
{
    Q_OBJECT

public:
    explicit SmartReader(QObject *parent = nullptr);

    // Starts reading SMART data for `deviceNode`; `deviceId` is echoed back
    // with the result so a dialog can ignore stale replies.
    Q_INVOKABLE void request(const QString &deviceNode, const QString &deviceId);
    Q_INVOKABLE bool isBusy() const { return m_process != nullptr; }

signals:
    // `data` is a QVariantMap with the parsed SMART payload; see parseOutput()
    // for the exact keys.
    void smartReady(const QString &deviceId, const QVariantMap &data);

private:
    void parseOutput(const QString &deviceId, const QString &deviceNode,
                     const QByteArray &output, const QByteArray &errorOutput,
                     int exitCode);

    QProcess *m_process = nullptr;
};
