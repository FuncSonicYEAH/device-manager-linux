#pragma once

#include <QHash>
#include <QObject>
#include <QStringList>

// Lightweight application-wide translator. UI strings are looked up by stable
// English identifiers (e.g. "statusOk"); the actual text for every language,
// including the default Simplified Chinese, lives in
// qml/i18n/translations.json inside the resources.
//
// Usage:
//   QML:      Tr.t("statusOk")          (binding tracks Tr.language)
//   C++:      Translator::translate("statusOk")
//
// The current language follows QSettings, falling back to the system locale.
class Translator : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString language READ language NOTIFY languageChanged)
    Q_PROPERTY(QStringList languageCodes READ languageCodes CONSTANT)
    Q_PROPERTY(QStringList languageNames READ languageNames CONSTANT)

public:
    explicit Translator(QObject *parent = nullptr);
    static Translator *instance();
    static void setInstance(Translator *tr);

    QString language() const { return m_language; }
    QStringList languageCodes() const { return m_codes; }
    QStringList languageNames() const { return m_names; }

    // Persists the choice so it survives restarts (settings dialog).
    Q_INVOKABLE void setLanguage(const QString &code);
    // Same but without persisting, used by the --lang / --switch CLI debug
    // flags so a forced test language is not recorded as the user's choice.
    void setLanguage(const QString &code, bool save);
    // QML bindings must pass Tr.language as the second argument so they
    // re-evaluate when the language changes at runtime.
    Q_INVOKABLE QString t(const QString &key, const QString &language) const;

    static QString translate(const QString &key);

signals:
    void languageChanged();

private:
    void loadAllDictionaries();
    QString detectSystemLanguage() const;

    QHash<QString, QHash<QString, QString>> m_dicts;
    QString m_language;
    const QStringList m_codes = { QStringLiteral("zh_CN"), QStringLiteral("zh_TW"),
                                  QStringLiteral("en"), QStringLiteral("ja"),
                                  QStringLiteral("ru") };
    const QStringList m_names = { QStringLiteral("简体中文"), QStringLiteral("繁體中文"),
                                  QStringLiteral("English"), QStringLiteral("日本語"),
                                  QStringLiteral("Русский") };
};
