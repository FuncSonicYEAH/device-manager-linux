#pragma once

#include <QObject>
#include <QList>
#include <QVariantList>

#include "DeviceEntry.h"

// Enumerates hardware devices from the Linux sysfs interface and exposes two
// grouped views to QML, in the spirit of the Windows Device Manager:
//   - typeGroups        (devices grouped by device type, "byType")
//   - connectionGroups  (devices grouped by connection/bus, "byConnection")
class DeviceManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList typeGroups READ typeGroups NOTIFY groupsChanged)
    Q_PROPERTY(QVariantList connectionGroups READ connectionGroups NOTIFY groupsChanged)
    Q_PROPERTY(int deviceCount READ deviceCount NOTIFY groupsChanged)
    Q_PROPERTY(int problemCount READ problemCount NOTIFY groupsChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY groupsChanged)

public:
    explicit DeviceManager(QObject *parent = nullptr);

    Q_INVOKABLE void refresh();

    QVariantList typeGroups() const { return m_typeGroups; }
    QVariantList connectionGroups() const { return m_connectionGroups; }
    int deviceCount() const { return m_deviceCount; }
    int problemCount() const { return m_problemCount; }
    QString lastError() const { return m_lastError; }

signals:
    void groupsChanged();

private:
    void enumerate();
    void scanClasses();
    void scanBus(const QString &bus);
    void scanCpus();
    void finalizeDevice(DeviceEntry &e);
    void buildGroups();

    DeviceEntry *entryForAnchor(const QString &anchor);

    // helpers
    static QString readFile(const QString &path);
    static QString symlinkTarget(const QString &path);
    static QString canonicalPath(const QString &path);
    static QString findUp(const QString &startPath, const QStringList &names);
    static QString vendorName(const QString &hexId);
    static QString driverDisplayName(const QString &driver);
    static QString driverIcon(const QString &driver);
    static QString categoryName(const QString &key);
    static QString categoryIcon(const QString &key);
    static QString categoryFromClass(const QString &cls);
    static QString busFromPath(const QString &path);
    static QString busName(const QString &key);
    static QString statusName(const QString &status);
    static QString humanBytes(qint64 bytes);

    QList<DeviceEntry> m_devices;
    QVariantList m_typeGroups;
    QVariantList m_connectionGroups;
    int m_deviceCount = 0;
    int m_problemCount = 0;
    QString m_lastError;
};
