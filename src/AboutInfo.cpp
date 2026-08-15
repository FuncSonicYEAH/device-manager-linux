#include "AboutInfo.h"

#include <QSysInfo>
#include <QtGlobal>

#ifndef APP_VERSION
#define APP_VERSION "0.1.0"
#endif

AboutInfo::AboutInfo(QObject *parent)
    : QObject(parent)
{
}

QString AboutInfo::appVersion() const
{
    return QString::fromLatin1(APP_VERSION);
}

QString AboutInfo::qtVersion() const
{
    return QString::fromLatin1(qVersion());
}

QString AboutInfo::osName() const
{
    return QSysInfo::prettyProductName();
}

QString AboutInfo::kernel() const
{
    return QSysInfo::kernelType() + QLatin1Char(' ') + QSysInfo::kernelVersion();
}

QString AboutInfo::cpuArch() const
{
    return QSysInfo::currentCpuArchitecture();
}
