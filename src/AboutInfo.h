#pragma once

#include <QObject>
#include <QString>

// Static build / runtime information shown in the About dialog. The version is
// baked in at build time from the meson project version (-DAPP_VERSION).
//
// Exposed to QML as the `AboutInfo` context property.
class AboutInfo : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    Q_PROPERTY(QString qtVersion READ qtVersion CONSTANT)
    Q_PROPERTY(QString osName READ osName CONSTANT)
    Q_PROPERTY(QString kernel READ kernel CONSTANT)
    Q_PROPERTY(QString cpuArch READ cpuArch CONSTANT)

public:
    explicit AboutInfo(QObject *parent = nullptr);

    QString appVersion() const;
    QString qtVersion() const;
    QString osName() const;
    QString kernel() const;
    QString cpuArch() const;
};
