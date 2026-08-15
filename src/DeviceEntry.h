#pragma once

#include <QString>
#include <QVariantList>
#include <QVariantMap>

// One enumerated device. `anchor` is the canonical sysfs path of the physical
// device (or of the class entry when no backing device exists), used as the
// stable id for selection across refreshes.
struct DeviceEntry
{
    QString id;          // anchor path (stable across refreshes)
    QString anchor;      // canonical sysfs path
    QString entryPath;   // class entry path (e.g. /sys/class/net/enp2s0)
    QString entryName;   // entry basename (e.g. enp2s0, sda, cpu0)
    QString name;        // display name
    QString vendor;      // vendor name or hex id
    QString driver;      // driver name ("" if none)
    QString bus;         // connection key: pci/usb/platform/acpi/mmc/virtio/virtual/other
    QString category;    // type-view category key
    QString modalias;    // hardware id
    QString status;      // ok | disabled | suspended | unplugged
    QString sysfsPath;   // same as anchor, exposed for the details pane

    QVariantList props;  // [{name, value}, ...]

    QVariantMap toMap() const
    {
        QVariantMap m;
        m.insert(QStringLiteral("id"), id);
        m.insert(QStringLiteral("name"), name);
        m.insert(QStringLiteral("vendor"), vendor);
        m.insert(QStringLiteral("driver"), driver);
        m.insert(QStringLiteral("bus"), bus);
        m.insert(QStringLiteral("category"), category);
        m.insert(QStringLiteral("modalias"), modalias);
        m.insert(QStringLiteral("status"), status);
        m.insert(QStringLiteral("sysfsPath"), sysfsPath);
        m.insert(QStringLiteral("props"), props);
        return m;
    }
};
