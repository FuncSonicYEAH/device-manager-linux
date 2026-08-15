#include "SmartReader.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QProcess>
#include <QTimer>

namespace {

QVariantMap errorResult(const QString &message)
{
    QVariantMap m;
    m.insert(QStringLiteral("ok"), false);
    m.insert(QStringLiteral("available"), false);
    m.insert(QStringLiteral("message"), message);
    return m;
}

} // namespace

SmartReader::SmartReader(QObject *parent)
    : QObject(parent)
{
}

void SmartReader::request(const QString &deviceNode, const QString &deviceId)
{
    if (m_process) {
        emit smartReady(deviceId, errorResult(QStringLiteral("busy")));
        return;
    }
    if (!QFile::exists(deviceNode)) {
        emit smartReady(deviceId, errorResult(QStringLiteral("no such device node: ") + deviceNode));
        return;
    }

    auto *p = new QProcess(this);
    m_process = p;
    // SMART data is usually only readable by root; elevate through pkexec so
    // a polkit authorization prompt appears (same as the device actions).
    p->setProgram(QStringLiteral("pkexec"));
    p->setArguments({ QStringLiteral("smartctl"), QStringLiteral("-a"),
                      QStringLiteral("-j"), deviceNode });

    // The polkit prompt can stay up for a while; give it a generous timeout.
    QTimer::singleShot(60000, p, [p]() {
        if (p->state() != QProcess::NotRunning)
            p->kill();
    });

    connect(p, &QProcess::finished, this, [this, p, deviceNode, deviceId](int exitCode, QProcess::ExitStatus) {
        m_process = nullptr;
        const QByteArray errorOutput = p->readAllStandardError();
        parseOutput(deviceId, deviceNode, p->readAllStandardOutput(), errorOutput, exitCode);
        p->deleteLater();
    });
    connect(p, &QProcess::errorOccurred, this, [this, p, deviceId](QProcess::ProcessError err) {
        if (err == QProcess::FailedToStart) {
            m_process = nullptr;
            emit smartReady(deviceId, errorResult(QStringLiteral("pkexec not found")));
            p->deleteLater();
        }
    });
    p->start();
}

void SmartReader::parseOutput(const QString &deviceId, const QString &deviceNode,
                              const QByteArray &output, const QByteArray &errorOutput,
                              int exitCode)
{
    QVariantMap result;
    result.insert(QStringLiteral("deviceNode"), deviceNode);

    QJsonParseError perr;
    const QJsonDocument doc = QJsonDocument::fromJson(output, &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
        // pkexec failed before smartctl could run (authorization denied,
        // no polkit agent, ...): map its stderr to a user-facing reason.
        if (exitCode != 0 && !errorOutput.isEmpty()) {
            const QString err = QString::fromUtf8(errorOutput).toLower();
            if (err.contains(QStringLiteral("not authorized"))
                || err.contains(QStringLiteral("cancelled"))
                || err.contains(QStringLiteral("canceled"))
                || err.contains(QStringLiteral("no authentication agent"))
                || err.contains(QStringLiteral("polkit"))) {
                emit smartReady(deviceId, errorResult(QStringLiteral("not authorized")));
                return;
            }
        }
        emit smartReady(deviceId, errorResult(QStringLiteral("smartctl returned no data")));
        return;
    }
    const QJsonObject root = doc.object();

    // device identity -------------------------------------------------------
    const QJsonObject dev = root.value(QStringLiteral("device")).toObject();
    const QString protocol = dev.value(QStringLiteral("protocol")).toString();
    result.insert(QStringLiteral("interfaceType"), protocol);
    result.insert(QStringLiteral("modelName"), root.value(QStringLiteral("model_name")).toString());
    result.insert(QStringLiteral("modelFamily"), root.value(QStringLiteral("model_family")).toString());
    result.insert(QStringLiteral("serialNumber"), root.value(QStringLiteral("serial_number")).toString());
    result.insert(QStringLiteral("firmware"), root.value(QStringLiteral("firmware_version")).toString());

    // smartctl diagnostic messages ------------------------------------------
    bool permissionDenied = false;
    QStringList messages;
    const QJsonArray msgs = root.value(QStringLiteral("smartctl")).toObject()
                                .value(QStringLiteral("messages")).toArray();
    for (const QJsonValue &v : msgs) {
        const QString s = v.toObject().value(QStringLiteral("string")).toString();
        if (s.contains(QStringLiteral("Permission denied")))
            permissionDenied = true;
        messages.append(s);
    }

    const bool available = root.value(QStringLiteral("smart_support")).toObject()
                               .value(QStringLiteral("available")).toBool(false);
    result.insert(QStringLiteral("available"), available);

    // overall health ---------------------------------------------------------
    const QJsonObject status = root.value(QStringLiteral("smart_status")).toObject();
    const bool hasHealth = status.contains(QStringLiteral("passed"));
    result.insert(QStringLiteral("healthKnown"), hasHealth);
    result.insert(QStringLiteral("healthPassed"), status.value(QStringLiteral("passed")).toBool(false));

    // temperature / power counters -------------------------------------------
    int temperature = root.value(QStringLiteral("temperature")).toObject()
                          .value(QStringLiteral("current")).toInt(-1);
    const int powerOnHours = root.value(QStringLiteral("power_on_time")).toObject()
                                 .value(QStringLiteral("hours")).toInt(-1);
    const int powerCycles = root.value(QStringLiteral("power_cycle_count")).toInt(-1);
    result.insert(QStringLiteral("temperature"), temperature);
    result.insert(QStringLiteral("powerOnHours"), powerOnHours);
    result.insert(QStringLiteral("powerCycles"), powerCycles);

    // attribute table ----------------------------------------------------------
    QVariantList attributes;
    if (protocol == QLatin1String("ATA") || protocol == QLatin1String("SATA")) {
        const QJsonArray table = root.value(QStringLiteral("ata_smart_attributes")).toObject()
                                     .value(QStringLiteral("table")).toArray();
        for (const QJsonValue &v : table) {
            const QJsonObject a = v.toObject();
            QVariantMap m;
            m.insert(QStringLiteral("id"), a.value(QStringLiteral("id")).toInt());
            m.insert(QStringLiteral("name"), a.value(QStringLiteral("name")).toString());
            m.insert(QStringLiteral("value"), a.value(QStringLiteral("value")).toInt());
            m.insert(QStringLiteral("worst"), a.value(QStringLiteral("worst")).toInt());
            m.insert(QStringLiteral("thresh"), a.value(QStringLiteral("thresh")).toInt());
            m.insert(QStringLiteral("raw"), a.value(QStringLiteral("raw")).toObject()
                     .value(QStringLiteral("string")).toString());
            attributes.append(m);
        }
        // temperature may only be reported via attribute 194 on some drives
        if (temperature < 0) {
            for (const QVariant &v : attributes) {
                if (v.toMap().value(QStringLiteral("id")).toInt() == 194) {
                    bool ok = false;
                    const int t = v.toMap().value(QStringLiteral("raw")).toString().toInt(&ok);
                    if (ok)
                        result.insert(QStringLiteral("temperature"), t);
                    break;
                }
            }
        }
    } else if (protocol == QLatin1String("NVMe")) {
        const QJsonObject nvme = root.value(QStringLiteral("nvme_smart_health_information_log")).toObject();
        const QStringList keys = nvme.keys();
        for (const QString &k : keys) {
            QVariantMap m;
            m.insert(QStringLiteral("id"), 0);
            m.insert(QStringLiteral("name"), k);
            const QJsonValue val = nvme.value(k);
            if (val.isDouble()) {
                m.insert(QStringLiteral("value"), val.toInt());
                m.insert(QStringLiteral("raw"), QString::number(val.toInt()));
            } else {
                m.insert(QStringLiteral("value"), 0);
                m.insert(QStringLiteral("raw"), val.toString());
            }
            m.insert(QStringLiteral("worst"), -1);
            m.insert(QStringLiteral("thresh"), -1);
            attributes.append(m);
        }
    }
    result.insert(QStringLiteral("attributes"), attributes);

    // error handling ------------------------------------------------------------
    if (exitCode != 0 || !available) {
        if (permissionDenied) {
            result.insert(QStringLiteral("ok"), false);
            result.insert(QStringLiteral("message"), QStringLiteral("permission denied"));
        } else if (!available) {
            result.insert(QStringLiteral("ok"), true);
            result.insert(QStringLiteral("available"), false);
            result.insert(QStringLiteral("message"), QStringLiteral("not supported"));
        } else {
            result.insert(QStringLiteral("ok"), false);
            result.insert(QStringLiteral("message"), messages.isEmpty()
                ? QStringLiteral("smartctl failed") : messages.first());
        }
        emit smartReady(deviceId, result);
        return;
    }

    result.insert(QStringLiteral("ok"), true);
    emit smartReady(deviceId, result);
}
