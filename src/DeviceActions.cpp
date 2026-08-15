#include "DeviceActions.h"

#include <QFileInfo>
#include <QProcess>
#include <QTimer>

DeviceActions::DeviceActions(QObject *parent)
    : QObject(parent)
{
}

bool DeviceActions::hasFile(const QVariantMap &device, const QString &name)
{
    const QString base = device.value(QStringLiteral("sysfsPath")).toString();
    if (base.isEmpty())
        return false;
    return QFileInfo::exists(base + QLatin1Char('/') + name);
}

QString DeviceActions::commandFor(const QVariantMap &device, const QString &action, QString *error)
{
    const QString anchor = device.value(QStringLiteral("sysfsPath")).toString();
    const QString bus = device.value(QStringLiteral("bus")).toString();
    const QString driver = device.value(QStringLiteral("driver")).toString();

    if (action == QLatin1String("suspend")) {
        if (hasFile(device, QStringLiteral("authorized")))
            return QStringLiteral("echo 0 > '%1/authorized'").arg(anchor);
        if (hasFile(device, QStringLiteral("power/control")))
            return QStringLiteral("echo auto > '%1/power/control'").arg(anchor);
        *error = QStringLiteral("unsupported");
        return QString();
    }
    if (action == QLatin1String("enable")) {
        if (hasFile(device, QStringLiteral("authorized")))
            return QStringLiteral("echo 1 > '%1/authorized'").arg(anchor);
        if (!driver.isEmpty() && !bus.isEmpty() && bus != QLatin1String("other")) {
            const QString devName = QFileInfo(anchor).fileName();
            const QString driverPath = QStringLiteral("/sys/bus/%1/drivers/%2").arg(bus, driver);
            return QStringLiteral("echo '%1' > '%2/unbind' && echo '%1' > '%2/bind'")
                .arg(devName, driverPath);
        }
        *error = QStringLiteral("unsupported");
        return QString();
    }
    if (action == QLatin1String("start")) {
        if (hasFile(device, QStringLiteral("power/control")))
            return QStringLiteral("echo on > '%1/power/control'").arg(anchor);
        *error = QStringLiteral("unsupported");
        return QString();
    }

    *error = QStringLiteral("unknown action");
    return QString();
}

void DeviceActions::perform(const QVariantMap &device, const QString &action)
{
    const QString deviceId = device.value(QStringLiteral("id")).toString();
    if (m_process) {
        emit actionFinished(deviceId, action, false, QStringLiteral("busy"));
        return;
    }

    QString error;
    const QString command = commandFor(device, action, &error);
    if (command.isEmpty()) {
        emit actionFinished(deviceId, action, false, error);
        return;
    }

    auto *p = new QProcess(this);
    m_process = p;
    p->setProgram(QStringLiteral("pkexec"));
    p->setArguments({ QStringLiteral("sh"), QStringLiteral("-c"), command });

    // The polkit prompt can stay up for a while; give it a generous timeout.
    QTimer::singleShot(60000, p, [p]() {
        if (p->state() != QProcess::NotRunning)
            p->kill();
    });

    connect(p, &QProcess::finished, this, [this, p, deviceId, action](int exitCode, QProcess::ExitStatus) {
        m_process = nullptr;
        const QString message = QString::fromUtf8(p->readAllStandardError()).trimmed();
        emit actionFinished(deviceId, action, exitCode == 0, message);
        p->deleteLater();
    });
    connect(p, &QProcess::errorOccurred, this, [this, p, deviceId, action](QProcess::ProcessError err) {
        if (err == QProcess::FailedToStart) {
            m_process = nullptr;
            emit actionFinished(deviceId, action, false, QStringLiteral("pkexec not found"));
            p->deleteLater();
        }
    });
    p->start();
}

bool DeviceActions::supportsAction(const QVariantMap &device, const QString &action) const
{
    QString error;
    return !commandFor(device, action, &error).isEmpty();
}
