#include "TemperatureProbe.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QTimer>

namespace {

// A hwmon chip basename (e.g. "hwmon0") or a "hwmon" intermediate directory.
bool isHwmonDirName(const QString &name)
{
    return name.startsWith(QStringLiteral("hwmon"));
}

} // namespace

TemperatureProbe::TemperatureProbe(QObject *parent)
    : QObject(parent)
{
    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &TemperatureProbe::sample);
}

QString TemperatureProbe::readFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return QString();
    return QString::fromUtf8(f.readAll()).trimmed();
}

QString TemperatureProbe::canonicalPath(const QString &path)
{
    return QFileInfo(path).canonicalFilePath();
}

// Adds every temp*_input attribute of one hwmon directory. Sensor names come
// from the optional tempN_label attribute; single-sensor chips fall back to
// the chip `name` (e.g. "coretemp", "nvme").
void TemperatureProbe::addHwmonSources(const QString &hwmonDir, QList<TempSource> *out)
{
    const QDir dir(hwmonDir);
    if (!dir.exists())
        return;
    const QStringList names = dir.entryList(QDir::Files, QDir::Name);
    const QString chip = readFile(hwmonDir + QStringLiteral("/name"));
    for (const QString &n : names) {
        if (!n.startsWith(QStringLiteral("temp")) || !n.endsWith(QStringLiteral("_input")))
            continue;
        QString num = n;
        num.chop(6);                 // strip "_input"
        num.remove(0, 4);            // strip "temp"
        if (num.isEmpty())
            continue;

        const QString label = readFile(hwmonDir + QStringLiteral("/temp") + num
                                       + QStringLiteral("_label"));
        TempSource s;
        s.id = canonicalPath(hwmonDir) + QLatin1Char('/') + n;
        if (!label.isEmpty())
            s.name = label;
        else if (num == QLatin1String("1") && !chip.isEmpty())
            s.name = chip;
        else
            s.name = chip.isEmpty() ? QStringLiteral("temp") + num
                                    : chip + QStringLiteral(" temp") + num;
        s.path = hwmonDir + QLatin1Char('/') + n;
        s.scale = 0.001; // hwmon temp*_input is in millidegrees
        out->append(s);
    }
}

// Thermal zones that belong to the device (the device's own class entries, or
// any zone whose `device` symlink resolves to the device anchor).
void TemperatureProbe::addThermalZoneSources(const QVariantMap &device, QList<TempSource> *out)
{
    const QString entryPath = device.value(QStringLiteral("entryPath")).toString();
    const QString anchor = device.value(QStringLiteral("sysfsPath")).toString();
    const QString category = device.value(QStringLiteral("category")).toString();

    auto addZone = [out](const QString &zonePath, const QString &name) {
        TempSource s;
        s.id = canonicalPath(zonePath) + QStringLiteral("/temp");
        s.name = name;
        s.path = zonePath + QStringLiteral("/temp");
        s.scale = 0.001; // thermal zone `temp` is in millidegrees
        out->append(s);
    };

    if (category == QLatin1String("thermal") && QFile::exists(entryPath + QStringLiteral("/temp"))) {
        const QString type = readFile(entryPath + QStringLiteral("/type"));
        const QString zoneName = QFileInfo(entryPath).fileName();
        addZone(entryPath, type.isEmpty() ? zoneName
                                          : type + QStringLiteral(" (") + zoneName + QLatin1Char(')'));
    }

    if (anchor.isEmpty())
        return;
    const QDir classDir(QStringLiteral("/sys/class/thermal"));
    const QStringList zones = classDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &z : zones) {
        if (!z.startsWith(QStringLiteral("thermal_zone")))
            continue;
        const QString zonePath = classDir.absoluteFilePath(z);
        if (canonicalPath(zonePath + QStringLiteral("/device")) != anchor)
            continue;
        const QString type = readFile(zonePath + QStringLiteral("/type"));
        addZone(zonePath, type.isEmpty() ? z : type + QStringLiteral(" (") + z + QLatin1Char(')'));
    }
}

QList<TemperatureProbe::TempSource> TemperatureProbe::collectSources(const QVariantMap &device)
{
    QList<TempSource> out;
    const QString entryPath = device.value(QStringLiteral("entryPath")).toString();
    const QString anchor = device.value(QStringLiteral("sysfsPath")).toString();
    const QString category = device.value(QStringLiteral("category")).toString();

    // hwmon class entries are their own hwmon directory
    if (category == QLatin1String("sensors") && !entryPath.isEmpty())
        addHwmonSources(entryPath, &out);

    // walk up the device tree looking for hwmon children (disks, GPUs, ...)
    if (!anchor.isEmpty()) {
        QSet<QString> seenDirs;
        QString dir = anchor;
        for (int depth = 0; depth < 8 && !dir.isEmpty(); ++depth) {
            const QDir d(dir);
            const QStringList names = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
            for (const QString &n : names) {
                if (!isHwmonDirName(n))
                    continue;
                const QString canon = canonicalPath(d.absoluteFilePath(n));
                if (canon.isEmpty() || seenDirs.contains(canon))
                    continue;
                seenDirs.insert(canon);
                // "hwmon/hwmon0" resolves to the chip dir; a bare "hwmon"
                // entry resolves to its parent and yields nothing
                addHwmonSources(canon, &out);
            }
            const int slash = dir.lastIndexOf(QLatin1Char('/'));
            if (slash <= 0)
                break;
            dir = dir.left(slash);
        }
    }

    addThermalZoneSources(device, &out);

    // de-duplicate by source id (a class entry and the tree walk can find the
    // same attribute twice)
    QSet<QString> seen;
    QList<TempSource> unique;
    for (const TempSource &s : out) {
        if (seen.contains(s.id))
            continue;
        seen.insert(s.id);
        unique.append(s);
    }
    return unique;
}

bool TemperatureProbe::supportsTemperature(const QVariantMap &device) const
{
    return !collectSources(device).isEmpty();
}

void TemperatureProbe::start(const QVariantMap &device)
{
    stop();
    m_sources = collectSources(device);
    m_samples.clear();
    m_history.clear();

    QVariantList sensors;
    for (const TempSource &s : m_sources) {
        QVariantMap m;
        m.insert(QStringLiteral("id"), s.id);
        m.insert(QStringLiteral("name"), s.name);
        m.insert(QStringLiteral("unit"), QStringLiteral("°C"));
        m.insert(QStringLiteral("current"), QVariant());
        sensors.append(m);
    }
    m_sensors = sensors;
    emit dataChanged();

    if (m_sources.isEmpty())
        return;

    m_running = true;
    emit runningChanged();
    m_timer->start();
    sample(); // grab the first point right away
}

void TemperatureProbe::stop()
{
    m_timer->stop();
    if (m_running) {
        m_running = false;
        emit runningChanged();
    }
}

void TemperatureProbe::clear()
{
    m_samples.clear();
    m_history.clear();
    emit dataChanged();
}

void TemperatureProbe::sample()
{
    if (m_sources.isEmpty())
        return;

    QVector<double> values;
    values.reserve(m_sources.size());
    QVariantList sensors = m_sensors;
    for (int i = 0; i < m_sources.size(); ++i) {
        bool ok = false;
        const double raw = readFile(m_sources[i].path).toDouble(&ok);
        const double v = ok ? raw * m_sources[i].scale : qQNaN();
        values.append(v);
        QVariantMap sm = sensors.at(i).toMap();
        sm.insert(QStringLiteral("current"), ok ? v : QVariant());
        sensors[i] = sm;
    }
    m_sensors = sensors;
    m_samples.append(values);
    while (m_samples.size() > kMaxSamples)
        m_samples.removeFirst();

    rebuildHistory();
    emit dataChanged();
}

void TemperatureProbe::rebuildHistory()
{
    m_history.clear();
    const int n = m_samples.size();
    for (int i = 0; i < n; ++i) {
        QVariantMap m;
        // seconds relative to now: the oldest sample is -(n-1), the newest is 0
        m.insert(QStringLiteral("time"), i - (n - 1));
        QVariantList vals;
        vals.reserve(m_samples[i].size());
        for (double v : m_samples[i])
            vals.append(v);
        m.insert(QStringLiteral("values"), vals);
        m_history.append(m);
    }
}
