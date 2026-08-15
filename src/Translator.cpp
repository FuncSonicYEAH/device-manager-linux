#include "Translator.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QSettings>

Translator::Translator(QObject *parent)
    : QObject(parent)
{
    loadAllDictionaries();
    m_language = detectSystemLanguage();
    const QSettings settings;
    const QString saved = settings.value(QStringLiteral("language")).toString();
    if (m_codes.contains(saved))
        m_language = saved;
    // First run: the config file does not exist yet, so record the effective
    // (default) language to create it and make the choice restorable later.
    if (!settings.contains(QStringLiteral("language")))
        QSettings().setValue(QStringLiteral("language"), m_language);
}

static Translator *g_translator = nullptr;

Translator *Translator::instance()
{
    return g_translator;
}

void Translator::setInstance(Translator *tr)
{
    g_translator = tr;
}

QString Translator::detectSystemLanguage() const
{
    const QString name = QLocale::system().name(); // e.g. zh_CN, ja_JP, ru_RU
    if (name.startsWith(QStringLiteral("zh_TW"))
        || name.startsWith(QStringLiteral("zh_HK"))
        || name.startsWith(QStringLiteral("zh_MO")))
        return QStringLiteral("zh_TW");
    if (name.startsWith(QStringLiteral("zh")))
        return QStringLiteral("zh_CN");
    if (name.startsWith(QStringLiteral("ja")))
        return QStringLiteral("ja");
    if (name.startsWith(QStringLiteral("ru")))
        return QStringLiteral("ru");
    return QStringLiteral("en");
}

void Translator::loadAllDictionaries()
{
    QFile file(QStringLiteral(":/qml/i18n/translations.json"));
    if (!file.open(QIODevice::ReadOnly))
        return;
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
        return;
    const QJsonObject all = doc.object();
    for (const QString &code : m_codes) {
        const QJsonObject langObj = all.value(code).toObject();
        if (langObj.isEmpty())
            continue;
        QHash<QString, QString> dict;
        for (auto it = langObj.begin(); it != langObj.end(); ++it)
            dict.insert(it.key(), it.value().toString());
        m_dicts.insert(code, dict);
    }
}

void Translator::setLanguage(const QString &code)
{
    setLanguage(code, true);
}

void Translator::setLanguage(const QString &code, bool save)
{
    if (!m_codes.contains(code))
        return;
    if (save)
        QSettings().setValue(QStringLiteral("language"), code);
    if (code == m_language)
        return;
    m_language = code;
    emit languageChanged();
}

QString Translator::t(const QString &key, const QString &language) const
{
    // Keys are stable English identifiers; unknown keys fall back to identity.
    // Latin kernel values (e.g. battery status) are dictionary keys as well.
    const auto langIt = m_dicts.constFind(language);
    if (langIt != m_dicts.cend()) {
        const auto keyIt = langIt->constFind(key);
        if (keyIt != langIt->cend())
            return keyIt.value();
    }
    return key;
}

QString Translator::translate(const QString &key)
{
    Translator *tr = instance();
    return tr->t(key, tr->language());
}
