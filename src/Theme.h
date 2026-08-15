#pragma once

#include <QColor>
#include <QObject>
#include <QQmlPropertyMap>

class ColorUtils;
class QEvent;

// Port of the `Appearance` singleton from modules/common/Appearance.qml.
// Holds the Material 3 color tokens, derived layer colors, radii, fonts and
// animation curves used by the whole widget set.
//
// When the Qt platform theme reports a color scheme (QStyleHints::colorScheme
// is Light or Dark), the M3 tokens are derived from the current
// QGuiApplication palette so the gallery follows the system look; otherwise
// the built-in M3 palettes are used as a fallback.
class Theme : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool darkmode READ isDark WRITE setDarkMode NOTIFY darkModeChanged)
    Q_PROPERTY(QString themeMode READ themeMode WRITE setThemeMode NOTIFY themeModeChanged)
    Q_PROPERTY(QString dummy READ dummy WRITE setDummy NOTIFY dummyChanged)
    Q_PROPERTY(QQmlPropertyMap *m3colors READ m3colors CONSTANT)
    Q_PROPERTY(QQmlPropertyMap *colors READ colors NOTIFY colorsChanged)
    Q_PROPERTY(QQmlPropertyMap *rounding READ rounding CONSTANT)
    Q_PROPERTY(QQmlPropertyMap *font READ font CONSTANT)
    Q_PROPERTY(QQmlPropertyMap *animation READ animation CONSTANT)
    Q_PROPERTY(QQmlPropertyMap *animationCurves READ animationCurves CONSTANT)

public:
    explicit Theme(QObject *parent = nullptr);

    bool isDark() const;
    Q_INVOKABLE void setDarkMode(bool dark);

    // "system" | "light" | "dark" — persisted via QSettings; "system" follows
    // the Qt platform color scheme.
    QString themeMode() const;
    Q_INVOKABLE void setThemeMode(const QString &mode);
    QString dummy() const { return m_dummy; }
    Q_INVOKABLE void setDummy(const QString &v);

    QQmlPropertyMap *m3colors() const { return m_m3colors; }
    QQmlPropertyMap *colors() const { return m_colors; }
    QQmlPropertyMap *rounding() const { return m_rounding; }
    QQmlPropertyMap *font() const { return m_font; }
    QQmlPropertyMap *animation() const { return m_animation; }
    QQmlPropertyMap *animationCurves() const { return m_animationCurves; }

    Q_INVOKABLE void applyColor(const QString &key, const QColor &color);

    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void darkModeChanged();
    void colorsChanged();
    void themeModeChanged();
    void dummyChanged();

private:
    // Returns true and sets *darkOut when a usable system palette is
    // available (QStyleHints::colorScheme != Unknown).
    bool detectSystemPalette(bool *darkOut);
    // Rebuilds the m3colors tokens from the current QGuiApplication palette,
    // following the requested dark/light mode (m_dark).
    void buildSystemPalette();
    // Re-detects and reapplies the system palette (palette/colorScheme changed).
    void refreshFromSystem();

    void buildDarkPalette();
    void buildLightPalette();
    void setPalette(const QHash<QString, QColor> &palette);
    void rebuildDerivedColors();

    // Keeps hue/saturation but forces the HSL lightness to targetLightness,
    // used to fix the polarity of accent colors when the gallery is toggled
    // to the scheme opposite to the system palette.
    QColor remapLightness(const QColor &color, qreal targetLightness) const;

    // Refreshes the Appearance.font.family tokens from the desktop Qt font
    // configuration (KDE, qt6ct, ...): the text families follow the
    // application default font, the monospace family follows the system
    // fixed font, and the icon family stays the bundled Material Symbols
    // Rounded variable font.
    void rebuildFontFamilies();

    ColorUtils *m_colorUtils = nullptr;

    QQmlPropertyMap *m_m3colors = nullptr;
    QQmlPropertyMap *m_colors = nullptr;
    QQmlPropertyMap *m_rounding = nullptr;
    QQmlPropertyMap *m_font = nullptr;
    QQmlPropertyMap *m_fontFamily = nullptr;
    QQmlPropertyMap *m_animation = nullptr;
    QQmlPropertyMap *m_animationCurves = nullptr;

    QString m_dummy = QStringLiteral("x");
    bool m_dark = true;
    // Whether the palette is currently derived from the Qt system palette.
    bool m_usesSystemPalette = false;
    // Set once the user toggles dark/light manually; while set, the scheme
    // choice is kept even when the system palette changes.
    bool m_userToggled = false;
};
