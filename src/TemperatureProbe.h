#pragma once

#include <QObject>
#include <QVariantList>
#include <QVector>

class QTimer;

// Samples the temperature of a device from its sysfs attributes (hwmon
// temp*_input files and matching thermal zones) and keeps a rolling history
// so QML can draw a live temperature curve.
//
// A device "supports temperature" when at least one readable source exists:
//   - sensors (hwmon class entries): their own temp*_input attributes
//   - thermal zones: their `temp` attribute
//   - anything else (disks / GPUs / ...): hwmon children found by walking up
//     the device tree, or a thermal zone whose `device` points at the device
//
// Exposed to QML as the `Temperature` context property.
class TemperatureProbe : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList sensors READ sensors NOTIFY dataChanged)
    Q_PROPERTY(QVariantList history READ history NOTIFY dataChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)

public:
    explicit TemperatureProbe(QObject *parent = nullptr);

    QVariantList sensors() const { return m_sensors; }
    QVariantList history() const { return m_history; }
    bool running() const { return m_running; }

    // Cheap check: does this device expose any readable temperature source?
    Q_INVOKABLE bool supportsTemperature(const QVariantMap &device) const;

    // Starts sampling `device` once per second (resets any previous history).
    Q_INVOKABLE void start(const QVariantMap &device);
    // Pauses sampling; the collected history is kept.
    Q_INVOKABLE void stop();
    // Drops the collected history (sampling continues if running).
    Q_INVOKABLE void clear();

signals:
    void dataChanged();
    void runningChanged();

private:
    struct TempSource
    {
        QString id;      // stable key, e.g. "<hwmon path>/temp1"
        QString name;    // display label (sensor label / chip name / zone type)
        QString path;    // sysfs file to read
        double scale = 0.001; // raw value * scale = degrees Celsius
    };

    static QList<TempSource> collectSources(const QVariantMap &device);
    static void addHwmonSources(const QString &hwmonDir, QList<TempSource> *out);
    static void addThermalZoneSources(const QVariantMap &device, QList<TempSource> *out);
    static QString readFile(const QString &path);
    static QString canonicalPath(const QString &path);

    void sample();
    void rebuildHistory();

    QList<TempSource> m_sources;
    QVariantList m_sensors;  // { id, name, unit, current }
    QList<QVector<double>> m_samples; // one entry per tick, aligned with m_sources
    QVariantList m_history;  // { time, values: [...] }
    QTimer *m_timer = nullptr;
    bool m_running = false;

    static constexpr int kMaxSamples = 180; // 3 minutes at 1 sample/second
};
