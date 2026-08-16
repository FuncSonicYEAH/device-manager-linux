#pragma once

#include <QObject>
#include <QHash>
#include <QVariantList>

class QTimer;
class QProcess;
class QElapsedTimer;

// Real-time hardware monitoring backend. Two independent monitors share one
// second tick:
//   - GPU: utilization (%) + VRAM used/total. Sources are, in order of
//     preference, the sysfs attributes exposed by amdgpu/radeon
//     (`gpu_busy_percent`, `mem_info_vram_*`), the NVIDIA `nvidia-smi`
//     utility, and for Intel i915 the `intel_gpu_top -J` JSON stream. When a
//     GPU exposes none of them the dialog reports "not supported".
//   - Network: per-interface traffic rates (bytes/second) computed from the
//     cumulative byte counters in `/proc/net/dev`.
//
// Both monitors also build a per-process usage list on the same tick:
//   - GPU processes: engine busy time / memory from the DRM fdinfo of each
//     client (`/proc/<pid>/fdinfo/*`, amdgpu/i915/nouveau/...); NVIDIA goes
//     through `nvidia-smi pmon` instead because its fds carry no fdinfo.
//   - Network processes: per-socket byte counters (netlink INET_DIAG +
//     tcp_info) mapped back to PIDs via the `socket:[inode]` links in
//     `/proc/<pid>/fd`. Sockets of other users are aggregated into one
//     "other processes" row; loopback-only peers are excluded.
//
// Each monitor keeps a rolling history (3 minutes at 1 sample/second) so QML
// can draw live curves. Exposed to QML as the `Monitor` context property.
class MonitorProbe : public QObject
{
    Q_OBJECT

    // ---- GPU ---------------------------------------------------------------
    Q_PROPERTY(QVariantList gpus READ gpus NOTIFY gpuDataChanged)
    Q_PROPERTY(QVariantList gpuHistory READ gpuHistory NOTIFY gpuDataChanged)
    Q_PROPERTY(bool gpuRunning READ gpuRunning NOTIFY gpuRunningChanged)

    // ---- network -----------------------------------------------------------
    Q_PROPERTY(QVariantList netInterfaces READ netInterfaces NOTIFY netDataChanged)
    Q_PROPERTY(QVariantList netHistory READ netHistory NOTIFY netDataChanged)
    Q_PROPERTY(bool netRunning READ netRunning NOTIFY netRunningChanged)

    // ---- per-process usage ------------------------------------------------
    Q_PROPERTY(QVariantList gpuProcesses READ gpuProcesses NOTIFY gpuProcessesChanged)
    Q_PROPERTY(QVariantList netProcesses READ netProcesses NOTIFY netProcessesChanged)

public:
    explicit MonitorProbe(QObject *parent = nullptr);
    ~MonitorProbe() override;

    QVariantList gpus() const { return m_gpus; }
    QVariantList gpuHistory() const { return m_gpuHistory; }
    bool gpuRunning() const { return m_gpuRunning; }

    QVariantList netInterfaces() const { return m_netInterfaces; }
    QVariantList netHistory() const { return m_netHistory; }
    bool netRunning() const { return m_netRunning; }

    QVariantList gpuProcesses() const { return m_gpuProcesses; }
    QVariantList netProcesses() const { return m_netProcesses; }

    // Per-process details need /proc access to foreign PIDs; without root
    // only the current user's processes can be attributed (the network list
    // additionally aggregates the rest into an "other processes" row).
    Q_INVOKABLE bool processesLimited() const;

    // Cheap capability checks, used to show/hide the entry buttons.
    Q_INVOKABLE bool supportsGpuMonitoring(const QVariantMap &device) const;
    Q_INVOKABLE bool supportsNetMonitoring(const QVariantMap &device) const;

    // ---- GPU sampling ------------------------------------------------------
    Q_INVOKABLE void startGpu(const QVariantMap &device);
    Q_INVOKABLE void stopGpu();
    Q_INVOKABLE void clearGpu();

    // ---- network sampling --------------------------------------------------
    Q_INVOKABLE void startNet(const QString &iface);
    Q_INVOKABLE void stopNet();
    Q_INVOKABLE void clearNet();

signals:
    void gpuDataChanged();
    void gpuRunningChanged();
    void netDataChanged();
    void netRunningChanged();
    void gpuProcessesChanged();
    void netProcessesChanged();

private:
    struct GpuSource
    {
        QString id;          // canonical sysfs path of the GPU device
        QString name;        // display name (vendor / driver)
        QString driver;      // kernel driver, e.g. "amdgpu", "i915", "nvidia"
        // sysfs attributes (amdgpu/radeon)
        QString busyPath;       // gpu_busy_percent, 0..100
        QString vramUsedPath;   // mem_info_vram_used, bytes
        QString vramTotalPath;  // mem_info_vram_total, bytes
        // external tools
        bool intelTop = false;  // i915: busy via `intel_gpu_top -J`
        bool nvidiaSmi = false; // nvidia: busy + vram via `nvidia-smi`
        bool busySupported = false;
        bool vramSupported = false;
    };

    struct NetCounters
    {
        qint64 rx = 0;
        qint64 tx = 0;
    };

    // per-socket byte counters from netlink INET_DIAG (tcp_info)
    struct SocketBytes
    {
        qint64 sent = 0;
        qint64 received = 0;
    };

    static QString readFile(const QString &path);
    static QString canonicalPath(const QString &path);
    static QString driverOf(const QString &anchor);
    static QString gpuNameFor(const QString &vendorHex, const QString &driver);
    static QHash<QString, NetCounters> readProcNetDev();

    // per-process helpers
    static QString fdLinkTarget(const QString &path);
    static bool isLoopbackAddress(int family, const quint32 *addr);
    static bool isZeroAddress(int family, const quint32 *addr);
    static QHash<qint64, SocketBytes> inetDiagQuery(int protocol, int family);
    static QHash<qint64, int> socketOwners(QHash<int, QString> *names);

    void detectGpus();
    void setupGpuSources(const QVariantMap &device);
    void ensureIntelTopProcess();
    void teardownIntelTopProcess();

    void sampleGpu();
    void rebuildGpuHistory();

    void updateNetInterfaces();
    void sampleNet();
    void rebuildNetHistory();

    // per-process sampling (called from sampleGpu/sampleNet while running)
    QStringList drmDeviceNodesOf(const QString &anchor) const;
    int nvidiaIndexOf(const QString &pdev) const;
    void sampleGpuProcesses(const GpuSource &s, double vramTotal);
    void sampleNetProcesses();

    QVariantList m_gpus;         // { id, name, driver, busySupported, vramSupported, monitorable }
    QList<GpuSource> m_gpuSources;
    QVariantList m_gpuHistory;   // { time, usage, vramUsed, vramTotal }
    QList<QVector<double>> m_gpuSamples;
    QTimer *m_gpuTimer = nullptr;
    bool m_gpuRunning = false;

    QProcess *m_intelTop = nullptr; // persistent `intel_gpu_top -J`
    QByteArray m_intelBuffer;       // consumed JSON fragment
    double m_intelBusy = qQNaN();

    QVariantList m_netInterfaces;  // { name, rxTotal, txTotal }
    QHash<QString, NetCounters> m_netCounters;
    QHash<QString, NetCounters> m_netLast;
    QVariantList m_netHistory;     // { time, rx, tx, rxTotal, txTotal }
    QList<QVariantMap> m_netSamples;
    QString m_netIface;
    QTimer *m_netTimer = nullptr;
    bool m_netRunning = false;
    qint64 m_netLastMsec = 0;

    // ---- per-process usage ---------------------------------------------
    QVariantList m_gpuProcesses;       // { pid, name, usage, mem }
    QVariantList m_netProcesses;       // { pid, name, rx, tx, sockets }
    QHash<QString, qint64> m_gpuEngineLast; // "pid:fd" -> cumulative engine ns
    QHash<int, QPair<qint64, qint64>> m_netProcLast; // pid -> (inB, outB)
    QString m_gpuPdev;                 // PCI address of the monitored GPU
    QStringList m_drmNodes;            // /dev/dri nodes of the monitored GPU
    int m_nvidiaIndex = -1;            // nvidia-smi index of the monitored GPU
    qint64 m_gpuProcLastMsec = 0;
    qint64 m_netProcLastMsec = 0;

    static constexpr int kMaxSamples = 180;   // 3 minutes at 1 sample/second
    static constexpr int kMaxProcRows = 20;   // per-process list length cap
};