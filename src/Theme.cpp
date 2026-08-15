#include "Theme.h"

#include <QEvent>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QPalette>
#include <QQmlPropertyMap>
#include <QSettings>
#include <QStyleHints>

#include "ColorUtils.h"

Theme::Theme(QObject *parent)
    : QObject(parent)
{
    m_colorUtils = new ColorUtils(this);

    m_m3colors = QQmlPropertyMap::create(this);
    m_colors = QQmlPropertyMap::create(this);
    m_rounding = QQmlPropertyMap::create(this);
    m_font = QQmlPropertyMap::create(this);
    m_animation = QQmlPropertyMap::create(this);
    m_animationCurves = QQmlPropertyMap::create(this);

    // Rounding tokens (Appearance.rounding)
    m_rounding->insert(QStringLiteral("unsharpen"), 2);
    m_rounding->insert(QStringLiteral("unsharpenmore"), 6);
    m_rounding->insert(QStringLiteral("verysmall"), 8);
    m_rounding->insert(QStringLiteral("small"), 12);
    m_rounding->insert(QStringLiteral("normal"), 17);
    m_rounding->insert(QStringLiteral("large"), 23);
    m_rounding->insert(QStringLiteral("verylarge"), 30);
    m_rounding->insert(QStringLiteral("full"), 9999);
    m_rounding->insert(QStringLiteral("screenRounding"), 23);
    m_rounding->insert(QStringLiteral("windowRounding"), 18);

    // Font tokens (Appearance.font). The family names are filled from the
    // desktop Qt font configuration below (KDE, qt6ct, ...) so the text
    // follows the user's system font instead of a hardcoded default.
    m_fontFamily = QQmlPropertyMap::create(m_font);
    m_font->insert(QStringLiteral("family"), QVariant::fromValue(m_fontFamily));
    rebuildFontFamilies();

    auto *pixelSize = QQmlPropertyMap::create(m_font);
    pixelSize->insert(QStringLiteral("smallest"), 10);
    pixelSize->insert(QStringLiteral("smaller"), 12);
    pixelSize->insert(QStringLiteral("smallie"), 13);
    pixelSize->insert(QStringLiteral("small"), 15);
    pixelSize->insert(QStringLiteral("normal"), 16);
    pixelSize->insert(QStringLiteral("large"), 17);
    pixelSize->insert(QStringLiteral("larger"), 19);
    pixelSize->insert(QStringLiteral("huge"), 22);
    pixelSize->insert(QStringLiteral("hugeass"), 23);
    pixelSize->insert(QStringLiteral("title"), 22);
    m_font->insert(QStringLiteral("pixelSize"), QVariant::fromValue(pixelSize));

    auto *variableAxes = QQmlPropertyMap::create(m_font);
    auto *mainAxes = QQmlPropertyMap::create(m_font);
    mainAxes->insert(QStringLiteral("wght"), 450);
    mainAxes->insert(QStringLiteral("wdth"), 100);
    variableAxes->insert(QStringLiteral("main"), QVariant::fromValue(mainAxes));
    m_font->insert(QStringLiteral("variableAxes"), QVariant::fromValue(variableAxes));

    // Animation curves (Appearance.animationCurves)
    const QVariantList fastSpatial = {0.42, 1.67, 0.21, 0.90, 1, 1};
    const QVariantList defaultSpatial = {0.38, 1.21, 0.22, 1.00, 1, 1};
    const QVariantList slowSpatial = {0.39, 1.29, 0.35, 0.98, 1, 1};
    const QVariantList effects = {0.34, 0.80, 0.34, 1.00, 1, 1};
    const QVariantList emphasized = {0.05, 0, 2.0 / 15, 0.06, 1.0 / 6, 0.4, 5.0 / 24, 0.82, 0.25, 1, 1, 1};
    const QVariantList emphasizedFirstHalf = {0.05, 0, 2.0 / 15, 0.06, 1.0 / 6, 0.4, 5.0 / 24, 0.82};
    const QVariantList emphasizedLastHalf = {5.0 / 24, 0.82, 0.25, 1, 1, 1};
    const QVariantList emphasizedAccel = {0.3, 0, 0.8, 0.15, 1, 1};
    const QVariantList emphasizedDecel = {0.05, 0.7, 0.1, 1, 1, 1};
    const QVariantList standard = {0.2, 0, 0, 1, 1, 1};
    const QVariantList standardAccel = {0.3, 0, 1, 1, 1, 1};
    const QVariantList standardDecel = {0, 0, 0, 1, 1, 1};

    m_animationCurves->insert(QStringLiteral("expressiveFastSpatial"), fastSpatial);
    m_animationCurves->insert(QStringLiteral("expressiveDefaultSpatial"), defaultSpatial);
    m_animationCurves->insert(QStringLiteral("expressiveSlowSpatial"), slowSpatial);
    m_animationCurves->insert(QStringLiteral("expressiveEffects"), effects);
    m_animationCurves->insert(QStringLiteral("emphasized"), emphasized);
    m_animationCurves->insert(QStringLiteral("emphasizedFirstHalf"), emphasizedFirstHalf);
    m_animationCurves->insert(QStringLiteral("emphasizedLastHalf"), emphasizedLastHalf);
    m_animationCurves->insert(QStringLiteral("emphasizedAccel"), emphasizedAccel);
    m_animationCurves->insert(QStringLiteral("emphasizedDecel"), emphasizedDecel);
    m_animationCurves->insert(QStringLiteral("standard"), standard);
    m_animationCurves->insert(QStringLiteral("standardAccel"), standardAccel);
    m_animationCurves->insert(QStringLiteral("standardDecel"), standardDecel);
    m_animationCurves->insert(QStringLiteral("expressiveFastSpatialDuration"), 350);
    m_animationCurves->insert(QStringLiteral("expressiveDefaultSpatialDuration"), 500);
    m_animationCurves->insert(QStringLiteral("expressiveSlowSpatialDuration"), 650);
    m_animationCurves->insert(QStringLiteral("expressiveEffectsDuration"), 200);

    // Animation groups (Appearance.animation)
    auto *elementMove = QQmlPropertyMap::create(m_animation);
    elementMove->insert(QStringLiteral("duration"), 500);
    elementMove->insert(QStringLiteral("bezierCurve"), defaultSpatial);
    elementMove->insert(QStringLiteral("velocity"), 650);
    m_animation->insert(QStringLiteral("elementMove"), QVariant::fromValue(elementMove));

    auto *elementMoveFast = QQmlPropertyMap::create(m_animation);
    elementMoveFast->insert(QStringLiteral("duration"), 200);
    elementMoveFast->insert(QStringLiteral("bezierCurve"), effects);
    elementMoveFast->insert(QStringLiteral("velocity"), 850);
    m_animation->insert(QStringLiteral("elementMoveFast"), QVariant::fromValue(elementMoveFast));

    auto *elementMoveEnter = QQmlPropertyMap::create(m_animation);
    elementMoveEnter->insert(QStringLiteral("duration"), 400);
    elementMoveEnter->insert(QStringLiteral("bezierCurve"), emphasizedDecel);
    elementMoveEnter->insert(QStringLiteral("velocity"), 650);
    m_animation->insert(QStringLiteral("elementMoveEnter"), QVariant::fromValue(elementMoveEnter));

    auto *elementMoveExit = QQmlPropertyMap::create(m_animation);
    elementMoveExit->insert(QStringLiteral("duration"), 200);
    elementMoveExit->insert(QStringLiteral("bezierCurve"), emphasizedAccel);
    elementMoveExit->insert(QStringLiteral("velocity"), 650);
    m_animation->insert(QStringLiteral("elementMoveExit"), QVariant::fromValue(elementMoveExit));

    auto *elementResize = QQmlPropertyMap::create(m_animation);
    elementResize->insert(QStringLiteral("duration"), 300);
    elementResize->insert(QStringLiteral("bezierCurve"), emphasized);
    elementResize->insert(QStringLiteral("velocity"), 650);
    m_animation->insert(QStringLiteral("elementResize"), QVariant::fromValue(elementResize));

    auto *clickBounce = QQmlPropertyMap::create(m_animation);
    clickBounce->insert(QStringLiteral("duration"), 400);
    clickBounce->insert(QStringLiteral("bezierCurve"), defaultSpatial);
    clickBounce->insert(QStringLiteral("velocity"), 850);
    m_animation->insert(QStringLiteral("clickBounce"), QVariant::fromValue(clickBounce));

    buildDarkPalette();

    // Follow the Qt platform theme's palette when it reports a color scheme
    // (QStyleHints::colorScheme is Light or Dark); otherwise keep the built-in
    // M3 palettes as the default colors.
    QGuiApplication::instance()->installEventFilter(this);
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
            this, &Theme::refreshFromSystem);

    // Restore a persisted theme mode ("system" | "light" | "dark")
    const QSettings settings;
    const QString savedMode = settings.value(QStringLiteral("themeMode")).toString();
    if (savedMode == QLatin1String("light")) {
        m_userToggled = true;
        setDarkMode(false);
    } else if (savedMode == QLatin1String("dark")) {
        m_userToggled = true;
        setDarkMode(true);
    } else {
        bool systemDark = false;
        if (detectSystemPalette(&systemDark)) {
            m_usesSystemPalette = true;
            m_dark = systemDark;
            buildSystemPalette();
        }
        rebuildDerivedColors();
    }
}

QString Theme::themeMode() const
{
    if (!m_userToggled)
        return QStringLiteral("system");
    return m_dark ? QStringLiteral("dark") : QStringLiteral("light");
}

void Theme::setThemeMode(const QString &mode)
{
    const QString current = themeMode();
    if (mode == current)
        return;
    if (mode == QLatin1String("system")) {
        m_userToggled = false;
        bool systemDark = false;
        if (detectSystemPalette(&systemDark)) {
            m_usesSystemPalette = true;
            m_dark = systemDark;
            buildSystemPalette();
        } else {
            m_usesSystemPalette = false;
            if (m_dark)
                buildDarkPalette();
            else
                buildLightPalette();
        }
        // Rebuild the derived Appearance.colors.* tokens (layers, hover/active
        // states, ...) from the refreshed m3 tokens in both branches, otherwise
        // toggling back to "system" leaves the widgets in the previous scheme.
        rebuildDerivedColors();
        QSettings().setValue(QStringLiteral("themeMode"), QStringLiteral("system"));
        emit themeModeChanged();
        emit darkModeChanged();
        return;
    }
    if (mode == QLatin1String("dark"))
        setDarkMode(true);
    else if (mode == QLatin1String("light"))
        setDarkMode(false);
}

bool Theme::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::ApplicationPaletteChange)
        refreshFromSystem();
    else if (event->type() == QEvent::ApplicationFontChange)
        rebuildFontFamilies();
    return QObject::eventFilter(watched, event);
}

void Theme::rebuildFontFamilies()
{
    // Text families follow the application's default font, which the Qt
    // platform theme (KDE, qt6ct, ...) resolves from the desktop
    // configuration. Monospace follows the system fixed font, so it tracks
    // the user's terminal font choice too. The icon family stays the bundled
    // Material Symbols Rounded variable font regardless of the desktop setup.
    const QString defaultFamily = QGuiApplication::font().family();
    const QString fixedFamily = QFontDatabase::systemFont(QFontDatabase::FixedFont).family();

    m_fontFamily->insert(QStringLiteral("main"), defaultFamily);
    m_fontFamily->insert(QStringLiteral("numbers"), defaultFamily);
    m_fontFamily->insert(QStringLiteral("title"), defaultFamily);
    m_fontFamily->insert(QStringLiteral("monospace"), fixedFamily);
    m_fontFamily->insert(QStringLiteral("iconMaterial"), QStringLiteral("Material Symbols Rounded"));
}

bool Theme::detectSystemPalette(bool *darkOut)
{
    const Qt::ColorScheme scheme = QGuiApplication::styleHints()->colorScheme();
    if (scheme == Qt::ColorScheme::Light) {
        *darkOut = false;
        return true;
    }
    if (scheme == Qt::ColorScheme::Dark) {
        *darkOut = true;
        return true;
    }
    return false;
}

void Theme::refreshFromSystem()
{
    bool systemDark = false;
    if (!detectSystemPalette(&systemDark))
        return; // no longer detectable — keep the current colors

    m_usesSystemPalette = true;
    if (!m_userToggled && m_dark != systemDark) {
        m_dark = systemDark;
        emit darkModeChanged();
    }
    buildSystemPalette();
    rebuildDerivedColors();
}

bool Theme::isDark() const
{
    return m_dark;
}

void Theme::setDarkMode(bool dark)
{
    if (m_dark == dark)
        return;
    m_dark = dark;
    m_userToggled = true;
    if (m_usesSystemPalette)
        buildSystemPalette();
    else if (m_dark)
        buildDarkPalette();
    else
        buildLightPalette();
    rebuildDerivedColors();
    QSettings().setValue(QStringLiteral("themeMode"), dark ? QStringLiteral("dark") : QStringLiteral("light"));
    emit themeModeChanged();
    emit darkModeChanged();
}

void Theme::setDummy(const QString &v)
{
    m_dummy = v;
    emit dummyChanged();
}

void Theme::applyColor(const QString &key, const QColor &color)
{
    if (m_m3colors->contains(key))
        m_m3colors->insert(key, color);
}

// Derives the Material 3 tokens from the current Qt palette. The neutral
// roles (window/base/text) come from the platform theme when its scheme
// matches the requested mode; the chroma roles (accent/highlight/link) always
// come from the platform palette, so a manual dark/light toggle keeps the
// system accent while falling back to the built-in M3 neutrals for readability.
void Theme::buildSystemPalette()
{
    const QPalette palette = QGuiApplication::palette();
    const bool dark = m_dark;
    const bool systemDark =
        QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
    const bool matched = (dark == systemDark);

    const QColor surface = matched
        ? palette.color(QPalette::Window)
        : QColor(dark ? QStringLiteral("#141313") : QStringLiteral("#fcf9f9"));
    const QColor onSurface = matched
        ? palette.color(QPalette::WindowText)
        : QColor(dark ? QStringLiteral("#e6e1e1") : QStringLiteral("#1c1b1c"));
    const QColor base = matched
        ? palette.color(QPalette::Base)
        : QColor(dark ? QStringLiteral("#0f0e0e") : QStringLiteral("#ffffff"));

    const QColor accent = palette.color(QPalette::Accent);
    const QColor highlight = palette.color(QPalette::Highlight);
    const QColor highlightedText = palette.color(QPalette::HighlightedText);
    const QColor placeholder = palette.color(QPalette::PlaceholderText);
    const QColor link = palette.color(QPalette::Link);

    QColor primary = accent.isValid() ? accent : highlight;
    QColor onPrimary = highlightedText;
    QColor tertiary = link.isValid() ? link : m_colorUtils->mix(primary, surface, 0.5);
    if (!matched) {
        // The platform palette has the opposite polarity (its accent is light
        // in dark schemes and dark in light schemes). Keep the accent hue but
        // remap the lightness so the tokens read correctly on this scheme:
        // dark mode wants a light primary, light mode a dark primary.
        primary = remapLightness(primary, dark ? 0.72 : 0.38);
        onPrimary = remapLightness(onPrimary, dark ? 0.18 : 0.95);
        tertiary = remapLightness(tertiary, dark ? 0.72 : 0.38);
    }
    // The placeholder tone has the system scheme's polarity, so only adopt it
    // when the requested mode matches the system; otherwise the built-in M3
    // on-surface-variant keeps the sidebar/auxiliary text readable on the
    // toggled scheme (outline/surfaceVariant derive from it below).
    const QColor onSurfaceVariant = (matched && placeholder.isValid())
        ? placeholder
        : QColor(dark ? QStringLiteral("#cbc5ca") : QStringLiteral("#484445"));

    // mix(a, b, p) = p*a + (1-p)*b
    // tone(p): p of the way from the surface toward the on-surface tone.
    const auto tone = [&](qreal p) { return m_colorUtils->mix(surface, onSurface, 1.0 - p); };
    // container(c, k): a container tone of c, k of the way toward the surface.
    const auto container = [&](const QColor &c, qreal k) { return m_colorUtils->mix(c, surface, k); };
    // onContainer(c): readable text for a container tone (light text on dark
    // containers in dark mode, dark text on light containers in light mode).
    const auto onContainer = [&](const QColor &c) {
        return dark ? m_colorUtils->mix(c, onSurface, 0.2)
                    : m_colorUtils->mix(c, onSurface, 0.08);
    };

    const QColor primaryContainer = container(primary, 0.15);
    const QColor onPrimaryContainer = onContainer(primaryContainer);
    const QColor inversePrimary = dark ? container(primary, 0.4) : container(primary, 0.35);
    const QColor secondary = dark ? container(primary, 0.95) : container(primary, 0.9);
    const QColor secondaryContainer = container(secondary, 0.35);
    const QColor onSecondaryContainer = onContainer(secondaryContainer);
    const QColor tertiaryContainer = container(tertiary, 0.15);
    const QColor onTertiaryContainer = onContainer(tertiaryContainer);
    const QColor surfaceVariant = dark ? container(onSurfaceVariant, 0.3) : container(onSurfaceVariant, 0.15);
    const QColor outline = dark ? container(onSurfaceVariant, 0.6) : container(onSurfaceVariant, 0.7);
    const QColor outlineVariant = container(outline, 0.5);
    const QColor inverseSurface = dark ? onSurface : container(onSurface, 0.9);
    const QColor inverseOnSurface = dark ? tone(0.15) : tone(0.08);

    // "Fixed" variants (light-scheme-style tints; not consumed by the gallery
    // widgets but kept consistent with the rest of the tokens).
    const QColor primaryFixed = container(primary, 0.12);
    const QColor primaryFixedDim = container(primary, 0.35);
    const QColor onPrimaryFixedVariant = dark ? container(primary, 0.3) : m_colorUtils->mix(primary, onSurface, 0.55);
    const QColor secondaryFixed = container(secondary, 0.12);
    const QColor secondaryFixedDim = container(secondary, 0.35);
    const QColor onSecondaryFixedVariant = dark ? container(secondary, 0.3) : m_colorUtils->mix(secondary, onSurface, 0.55);
    const QColor tertiaryFixed = container(tertiary, 0.12);
    const QColor tertiaryFixedDim = container(tertiary, 0.35);
    const QColor onTertiaryFixedVariant = dark ? container(tertiary, 0.3) : m_colorUtils->mix(tertiary, onSurface, 0.55);

    // Error/success have no Qt palette role — keep the M3 defaults for the scheme.
    const QColor error = dark ? QColor(QStringLiteral("#ffb4ab")) : QColor(QStringLiteral("#ba1a1a"));
    const QColor onError = dark ? QColor(QStringLiteral("#690005")) : QColor(QStringLiteral("#ffffff"));
    const QColor errorContainer = dark ? QColor(QStringLiteral("#93000a")) : QColor(QStringLiteral("#ffdad6"));
    const QColor onErrorContainer = dark ? QColor(QStringLiteral("#ffdad6")) : QColor(QStringLiteral("#410002"));
    const QColor success = dark ? QColor(QStringLiteral("#B5CCBA")) : QColor(QStringLiteral("#3b7a4e"));
    const QColor onSuccess = dark ? QColor(QStringLiteral("#213528")) : QColor(QStringLiteral("#ffffff"));
    const QColor successContainer = dark ? QColor(QStringLiteral("#374B3E")) : QColor(QStringLiteral("#b7f1c3"));
    const QColor onSuccessContainer = dark ? QColor(QStringLiteral("#D1E9D6")) : QColor(QStringLiteral("#03240e"));

    m_m3colors->insert(QStringLiteral("darkmode"), dark);
    m_m3colors->insert(QStringLiteral("m3background"), surface);
    m_m3colors->insert(QStringLiteral("m3onBackground"), onSurface);
    m_m3colors->insert(QStringLiteral("m3surface"), surface);
    m_m3colors->insert(QStringLiteral("m3surfaceDim"), dark ? surface : tone(0.12));
    m_m3colors->insert(QStringLiteral("m3surfaceBright"), dark ? tone(0.12) : surface);
    m_m3colors->insert(QStringLiteral("m3surfaceContainerLowest"), base);
    m_m3colors->insert(QStringLiteral("m3surfaceContainerLow"), tone(0.03));
    m_m3colors->insert(QStringLiteral("m3surfaceContainer"), tone(0.05));
    m_m3colors->insert(QStringLiteral("m3surfaceContainerHigh"), tone(0.08));
    m_m3colors->insert(QStringLiteral("m3surfaceContainerHighest"), tone(0.12));
    m_m3colors->insert(QStringLiteral("m3onSurface"), onSurface);
    m_m3colors->insert(QStringLiteral("m3surfaceVariant"), surfaceVariant);
    m_m3colors->insert(QStringLiteral("m3onSurfaceVariant"), onSurfaceVariant);
    m_m3colors->insert(QStringLiteral("m3inverseSurface"), inverseSurface);
    m_m3colors->insert(QStringLiteral("m3inverseOnSurface"), inverseOnSurface);
    m_m3colors->insert(QStringLiteral("m3outline"), outline);
    m_m3colors->insert(QStringLiteral("m3outlineVariant"), outlineVariant);
    m_m3colors->insert(QStringLiteral("m3shadow"), QColor(QStringLiteral("#000000")));
    m_m3colors->insert(QStringLiteral("m3scrim"), QColor(QStringLiteral("#000000")));
    m_m3colors->insert(QStringLiteral("m3surfaceTint"), primary);
    m_m3colors->insert(QStringLiteral("m3primary"), primary);
    m_m3colors->insert(QStringLiteral("m3onPrimary"), onPrimary);
    m_m3colors->insert(QStringLiteral("m3primaryContainer"), primaryContainer);
    m_m3colors->insert(QStringLiteral("m3onPrimaryContainer"), onPrimaryContainer);
    m_m3colors->insert(QStringLiteral("m3inversePrimary"), inversePrimary);
    m_m3colors->insert(QStringLiteral("m3secondary"), secondary);
    m_m3colors->insert(QStringLiteral("m3onSecondary"), onPrimary);
    m_m3colors->insert(QStringLiteral("m3secondaryContainer"), secondaryContainer);
    m_m3colors->insert(QStringLiteral("m3onSecondaryContainer"), onSecondaryContainer);
    m_m3colors->insert(QStringLiteral("m3tertiary"), tertiary);
    m_m3colors->insert(QStringLiteral("m3onTertiary"), onPrimary);
    m_m3colors->insert(QStringLiteral("m3tertiaryContainer"), tertiaryContainer);
    m_m3colors->insert(QStringLiteral("m3onTertiaryContainer"), onTertiaryContainer);
    m_m3colors->insert(QStringLiteral("m3error"), error);
    m_m3colors->insert(QStringLiteral("m3onError"), onError);
    m_m3colors->insert(QStringLiteral("m3errorContainer"), errorContainer);
    m_m3colors->insert(QStringLiteral("m3onErrorContainer"), onErrorContainer);
    m_m3colors->insert(QStringLiteral("m3success"), success);
    m_m3colors->insert(QStringLiteral("m3onSuccess"), onSuccess);
    m_m3colors->insert(QStringLiteral("m3successContainer"), successContainer);
    m_m3colors->insert(QStringLiteral("m3onSuccessContainer"), onSuccessContainer);
    m_m3colors->insert(QStringLiteral("m3primaryFixed"), primaryFixed);
    m_m3colors->insert(QStringLiteral("m3primaryFixedDim"), primaryFixedDim);
    m_m3colors->insert(QStringLiteral("m3onPrimaryFixed"), onSurface);
    m_m3colors->insert(QStringLiteral("m3onPrimaryFixedVariant"), onPrimaryFixedVariant);
    m_m3colors->insert(QStringLiteral("m3secondaryFixed"), secondaryFixed);
    m_m3colors->insert(QStringLiteral("m3secondaryFixedDim"), secondaryFixedDim);
    m_m3colors->insert(QStringLiteral("m3onSecondaryFixed"), onSurface);
    m_m3colors->insert(QStringLiteral("m3onSecondaryFixedVariant"), onSecondaryFixedVariant);
    m_m3colors->insert(QStringLiteral("m3tertiaryFixed"), tertiaryFixed);
    m_m3colors->insert(QStringLiteral("m3tertiaryFixedDim"), tertiaryFixedDim);
    m_m3colors->insert(QStringLiteral("m3onTertiaryFixed"), onSurface);
    m_m3colors->insert(QStringLiteral("m3onTertiaryFixedVariant"), onTertiaryFixedVariant);
}

QColor Theme::remapLightness(const QColor &color, qreal targetLightness) const
{
    const qreal hue = color.hslHueF();
    const qreal sat = color.hslSaturationF();
    if (hue < 0.0) // achromatic
        return QColor::fromHslF(0.0, 0.0, targetLightness, color.alphaF());
    return QColor::fromHslF(hue, sat, targetLightness, color.alphaF());
}

void Theme::buildDarkPalette()
{
    m_m3colors->insert(QStringLiteral("darkmode"), true);
    m_m3colors->insert(QStringLiteral("m3background"), QColor(QStringLiteral("#141313")));
    m_m3colors->insert(QStringLiteral("m3onBackground"), QColor(QStringLiteral("#e6e1e1")));
    m_m3colors->insert(QStringLiteral("m3surface"), QColor(QStringLiteral("#141313")));
    m_m3colors->insert(QStringLiteral("m3surfaceDim"), QColor(QStringLiteral("#141313")));
    m_m3colors->insert(QStringLiteral("m3surfaceBright"), QColor(QStringLiteral("#3a3939")));
    m_m3colors->insert(QStringLiteral("m3surfaceContainerLowest"), QColor(QStringLiteral("#0f0e0e")));
    m_m3colors->insert(QStringLiteral("m3surfaceContainerLow"), QColor(QStringLiteral("#1c1b1c")));
    m_m3colors->insert(QStringLiteral("m3surfaceContainer"), QColor(QStringLiteral("#201f20")));
    m_m3colors->insert(QStringLiteral("m3surfaceContainerHigh"), QColor(QStringLiteral("#2b2a2a")));
    m_m3colors->insert(QStringLiteral("m3surfaceContainerHighest"), QColor(QStringLiteral("#363435")));
    m_m3colors->insert(QStringLiteral("m3onSurface"), QColor(QStringLiteral("#e6e1e1")));
    m_m3colors->insert(QStringLiteral("m3surfaceVariant"), QColor(QStringLiteral("#49464a")));
    m_m3colors->insert(QStringLiteral("m3onSurfaceVariant"), QColor(QStringLiteral("#cbc5ca")));
    m_m3colors->insert(QStringLiteral("m3inverseSurface"), QColor(QStringLiteral("#e6e1e1")));
    m_m3colors->insert(QStringLiteral("m3inverseOnSurface"), QColor(QStringLiteral("#313030")));
    m_m3colors->insert(QStringLiteral("m3outline"), QColor(QStringLiteral("#948f94")));
    m_m3colors->insert(QStringLiteral("m3outlineVariant"), QColor(QStringLiteral("#49464a")));
    m_m3colors->insert(QStringLiteral("m3shadow"), QColor(QStringLiteral("#000000")));
    m_m3colors->insert(QStringLiteral("m3scrim"), QColor(QStringLiteral("#000000")));
    m_m3colors->insert(QStringLiteral("m3surfaceTint"), QColor(QStringLiteral("#cbc4cb")));
    m_m3colors->insert(QStringLiteral("m3primary"), QColor(QStringLiteral("#cbc4cb")));
    m_m3colors->insert(QStringLiteral("m3onPrimary"), QColor(QStringLiteral("#322f34")));
    m_m3colors->insert(QStringLiteral("m3primaryContainer"), QColor(QStringLiteral("#2d2a2f")));
    m_m3colors->insert(QStringLiteral("m3onPrimaryContainer"), QColor(QStringLiteral("#bcb6bc")));
    m_m3colors->insert(QStringLiteral("m3inversePrimary"), QColor(QStringLiteral("#615d63")));
    m_m3colors->insert(QStringLiteral("m3secondary"), QColor(QStringLiteral("#cac5c8")));
    m_m3colors->insert(QStringLiteral("m3onSecondary"), QColor(QStringLiteral("#323032")));
    m_m3colors->insert(QStringLiteral("m3secondaryContainer"), QColor(QStringLiteral("#4d4b4d")));
    m_m3colors->insert(QStringLiteral("m3onSecondaryContainer"), QColor(QStringLiteral("#ece6e9")));
    m_m3colors->insert(QStringLiteral("m3tertiary"), QColor(QStringLiteral("#d1c3c6")));
    m_m3colors->insert(QStringLiteral("m3onTertiary"), QColor(QStringLiteral("#372e30")));
    m_m3colors->insert(QStringLiteral("m3tertiaryContainer"), QColor(QStringLiteral("#31292b")));
    m_m3colors->insert(QStringLiteral("m3onTertiaryContainer"), QColor(QStringLiteral("#c1b4b7")));
    m_m3colors->insert(QStringLiteral("m3error"), QColor(QStringLiteral("#ffb4ab")));
    m_m3colors->insert(QStringLiteral("m3onError"), QColor(QStringLiteral("#690005")));
    m_m3colors->insert(QStringLiteral("m3errorContainer"), QColor(QStringLiteral("#93000a")));
    m_m3colors->insert(QStringLiteral("m3onErrorContainer"), QColor(QStringLiteral("#ffdad6")));
    m_m3colors->insert(QStringLiteral("m3primaryFixed"), QColor(QStringLiteral("#e7e0e7")));
    m_m3colors->insert(QStringLiteral("m3primaryFixedDim"), QColor(QStringLiteral("#cbc4cb")));
    m_m3colors->insert(QStringLiteral("m3onPrimaryFixed"), QColor(QStringLiteral("#1d1b1f")));
    m_m3colors->insert(QStringLiteral("m3onPrimaryFixedVariant"), QColor(QStringLiteral("#49454b")));
    m_m3colors->insert(QStringLiteral("m3secondaryFixed"), QColor(QStringLiteral("#e6e1e4")));
    m_m3colors->insert(QStringLiteral("m3secondaryFixedDim"), QColor(QStringLiteral("#cac5c8")));
    m_m3colors->insert(QStringLiteral("m3onSecondaryFixed"), QColor(QStringLiteral("#1d1b1d")));
    m_m3colors->insert(QStringLiteral("m3onSecondaryFixedVariant"), QColor(QStringLiteral("#484648")));
    m_m3colors->insert(QStringLiteral("m3tertiaryFixed"), QColor(QStringLiteral("#eddfe1")));
    m_m3colors->insert(QStringLiteral("m3tertiaryFixedDim"), QColor(QStringLiteral("#d1c3c6")));
    m_m3colors->insert(QStringLiteral("m3onTertiaryFixed"), QColor(QStringLiteral("#211a1c")));
    m_m3colors->insert(QStringLiteral("m3onTertiaryFixedVariant"), QColor(QStringLiteral("#4e4447")));
    m_m3colors->insert(QStringLiteral("m3success"), QColor(QStringLiteral("#B5CCBA")));
    m_m3colors->insert(QStringLiteral("m3onSuccess"), QColor(QStringLiteral("#213528")));
    m_m3colors->insert(QStringLiteral("m3successContainer"), QColor(QStringLiteral("#374B3E")));
    m_m3colors->insert(QStringLiteral("m3onSuccessContainer"), QColor(QStringLiteral("#D1E9D6")));
}

void Theme::buildLightPalette()
{
    m_m3colors->insert(QStringLiteral("darkmode"), false);
    m_m3colors->insert(QStringLiteral("m3background"), QColor(QStringLiteral("#fcf9f9")));
    m_m3colors->insert(QStringLiteral("m3onBackground"), QColor(QStringLiteral("#1c1b1c")));
    m_m3colors->insert(QStringLiteral("m3surface"), QColor(QStringLiteral("#fcf9f9")));
    m_m3colors->insert(QStringLiteral("m3surfaceDim"), QColor(QStringLiteral("#ded8d9")));
    m_m3colors->insert(QStringLiteral("m3surfaceBright"), QColor(QStringLiteral("#fcf9f9")));
    m_m3colors->insert(QStringLiteral("m3surfaceContainerLowest"), QColor(QStringLiteral("#ffffff")));
    m_m3colors->insert(QStringLiteral("m3surfaceContainerLow"), QColor(QStringLiteral("#f6f1f1")));
    m_m3colors->insert(QStringLiteral("m3surfaceContainer"), QColor(QStringLiteral("#f0ebec")));
    m_m3colors->insert(QStringLiteral("m3surfaceContainerHigh"), QColor(QStringLiteral("#eae5e6")));
    m_m3colors->insert(QStringLiteral("m3surfaceContainerHighest"), QColor(QStringLiteral("#e4dfe0")));
    m_m3colors->insert(QStringLiteral("m3onSurface"), QColor(QStringLiteral("#1c1b1c")));
    m_m3colors->insert(QStringLiteral("m3surfaceVariant"), QColor(QStringLiteral("#e7e1e2")));
    m_m3colors->insert(QStringLiteral("m3onSurfaceVariant"), QColor(QStringLiteral("#484445")));
    m_m3colors->insert(QStringLiteral("m3inverseSurface"), QColor(QStringLiteral("#343133")));
    m_m3colors->insert(QStringLiteral("m3inverseOnSurface"), QColor(QStringLiteral("#f6f0f1")));
    m_m3colors->insert(QStringLiteral("m3outline"), QColor(QStringLiteral("#797576")));
    m_m3colors->insert(QStringLiteral("m3outlineVariant"), QColor(QStringLiteral("#cbc5c6")));
    m_m3colors->insert(QStringLiteral("m3shadow"), QColor(QStringLiteral("#000000")));
    m_m3colors->insert(QStringLiteral("m3scrim"), QColor(QStringLiteral("#000000")));
    m_m3colors->insert(QStringLiteral("m3surfaceTint"), QColor(QStringLiteral("#6a656b")));
    m_m3colors->insert(QStringLiteral("m3primary"), QColor(QStringLiteral("#6a656b")));
    m_m3colors->insert(QStringLiteral("m3onPrimary"), QColor(QStringLiteral("#ffffff")));
    m_m3colors->insert(QStringLiteral("m3primaryContainer"), QColor(QStringLiteral("#ece6ec")));
    m_m3colors->insert(QStringLiteral("m3onPrimaryContainer"), QColor(QStringLiteral("#262328")));
    m_m3colors->insert(QStringLiteral("m3inversePrimary"), QColor(QStringLiteral("#d2cbd1")));
    m_m3colors->insert(QStringLiteral("m3secondary"), QColor(QStringLiteral("#747075")));
    m_m3colors->insert(QStringLiteral("m3onSecondary"), QColor(QStringLiteral("#ffffff")));
    m_m3colors->insert(QStringLiteral("m3secondaryContainer"), QColor(QStringLiteral("#d4d0d5")));
    m_m3colors->insert(QStringLiteral("m3onSecondaryContainer"), QColor(QStringLiteral("#2c292d")));
    m_m3colors->insert(QStringLiteral("m3tertiary"), QColor(QStringLiteral("#6a656b")));
    m_m3colors->insert(QStringLiteral("m3onTertiary"), QColor(QStringLiteral("#ffffff")));
    m_m3colors->insert(QStringLiteral("m3tertiaryContainer"), QColor(QStringLiteral("#ebe5ec")));
    m_m3colors->insert(QStringLiteral("m3onTertiaryContainer"), QColor(QStringLiteral("#262328")));
    m_m3colors->insert(QStringLiteral("m3error"), QColor(QStringLiteral("#ba1a1a")));
    m_m3colors->insert(QStringLiteral("m3onError"), QColor(QStringLiteral("#ffffff")));
    m_m3colors->insert(QStringLiteral("m3errorContainer"), QColor(QStringLiteral("#ffdad6")));
    m_m3colors->insert(QStringLiteral("m3onErrorContainer"), QColor(QStringLiteral("#410002")));
    m_m3colors->insert(QStringLiteral("m3primaryFixed"), QColor(QStringLiteral("#ede6ec")));
    m_m3colors->insert(QStringLiteral("m3primaryFixedDim"), QColor(QStringLiteral("#d2cbd1")));
    m_m3colors->insert(QStringLiteral("m3onPrimaryFixed"), QColor(QStringLiteral("#262328")));
    m_m3colors->insert(QStringLiteral("m3onPrimaryFixedVariant"), QColor(QStringLiteral("#575258")));
    m_m3colors->insert(QStringLiteral("m3secondaryFixed"), QColor(QStringLiteral("#e1dce1")));
    m_m3colors->insert(QStringLiteral("m3secondaryFixedDim"), QColor(QStringLiteral("#c5c0c5")));
    m_m3colors->insert(QStringLiteral("m3onSecondaryFixed"), QColor(QStringLiteral("#28252a")));
    m_m3colors->insert(QStringLiteral("m3onSecondaryFixedVariant"), QColor(QStringLiteral("#5d595e")));
    m_m3colors->insert(QStringLiteral("m3tertiaryFixed"), QColor(QStringLiteral("#ebe5ec")));
    m_m3colors->insert(QStringLiteral("m3tertiaryFixedDim"), QColor(QStringLiteral("#d2cbd1")));
    m_m3colors->insert(QStringLiteral("m3onTertiaryFixed"), QColor(QStringLiteral("#262328")));
    m_m3colors->insert(QStringLiteral("m3onTertiaryFixedVariant"), QColor(QStringLiteral("#575258")));
    m_m3colors->insert(QStringLiteral("m3success"), QColor(QStringLiteral("#3b7a4e")));
    m_m3colors->insert(QStringLiteral("m3onSuccess"), QColor(QStringLiteral("#ffffff")));
    m_m3colors->insert(QStringLiteral("m3successContainer"), QColor(QStringLiteral("#b7f1c3")));
    m_m3colors->insert(QStringLiteral("m3onSuccessContainer"), QColor(QStringLiteral("#03240e")));
}

void Theme::rebuildDerivedColors()
{
    const QColor background = m_m3colors->value(QStringLiteral("m3background")).value<QColor>();
    const QColor onBackground = m_m3colors->value(QStringLiteral("m3onBackground")).value<QColor>();
    const QColor primary = m_m3colors->value(QStringLiteral("m3primary")).value<QColor>();
    const QColor onPrimary = m_m3colors->value(QStringLiteral("m3onPrimary")).value<QColor>();
    const QColor primaryContainer = m_m3colors->value(QStringLiteral("m3primaryContainer")).value<QColor>();
    const QColor onPrimaryContainer = m_m3colors->value(QStringLiteral("m3onPrimaryContainer")).value<QColor>();
    const QColor secondary = m_m3colors->value(QStringLiteral("m3secondary")).value<QColor>();
    const QColor onSecondary = m_m3colors->value(QStringLiteral("m3onSecondary")).value<QColor>();
    const QColor secondaryContainer = m_m3colors->value(QStringLiteral("m3secondaryContainer")).value<QColor>();
    const QColor onSecondaryContainer = m_m3colors->value(QStringLiteral("m3onSecondaryContainer")).value<QColor>();
    const QColor tertiary = m_m3colors->value(QStringLiteral("m3tertiary")).value<QColor>();
    const QColor onTertiary = m_m3colors->value(QStringLiteral("m3onTertiary")).value<QColor>();
    const QColor tertiaryContainer = m_m3colors->value(QStringLiteral("m3tertiaryContainer")).value<QColor>();
    const QColor onTertiaryContainer = m_m3colors->value(QStringLiteral("m3onTertiaryContainer")).value<QColor>();
    const QColor surfaceContainerLowest = m_m3colors->value(QStringLiteral("m3surfaceContainerLowest")).value<QColor>();
    const QColor surfaceContainerLow = m_m3colors->value(QStringLiteral("m3surfaceContainerLow")).value<QColor>();
    const QColor surfaceContainer = m_m3colors->value(QStringLiteral("m3surfaceContainer")).value<QColor>();
    const QColor surfaceContainerHigh = m_m3colors->value(QStringLiteral("m3surfaceContainerHigh")).value<QColor>();
    const QColor surfaceContainerHighest = m_m3colors->value(QStringLiteral("m3surfaceContainerHighest")).value<QColor>();
    const QColor onSurface = m_m3colors->value(QStringLiteral("m3onSurface")).value<QColor>();
    const QColor onSurfaceVariant = m_m3colors->value(QStringLiteral("m3onSurfaceVariant")).value<QColor>();
    const QColor outline = m_m3colors->value(QStringLiteral("m3outline")).value<QColor>();
    const QColor outlineVariant = m_m3colors->value(QStringLiteral("m3outlineVariant")).value<QColor>();
    const QColor error = m_m3colors->value(QStringLiteral("m3error")).value<QColor>();
    const QColor onError = m_m3colors->value(QStringLiteral("m3onError")).value<QColor>();
    const QColor errorContainer = m_m3colors->value(QStringLiteral("m3errorContainer")).value<QColor>();
    const QColor onErrorContainer = m_m3colors->value(QStringLiteral("m3onErrorContainer")).value<QColor>();
    const QColor inverseSurface = m_m3colors->value(QStringLiteral("m3inverseSurface")).value<QColor>();
    const QColor inverseOnSurface = m_m3colors->value(QStringLiteral("m3inverseOnSurface")).value<QColor>();
    const QColor shadow = m_m3colors->value(QStringLiteral("m3shadow")).value<QColor>();
    const QColor scrim = m_m3colors->value(QStringLiteral("m3scrim")).value<QColor>();

    // Transparency (matches Config default: disabled, so solid)
    const qreal backgroundTransparency = 0.0;
    const qreal contentTransparency = 0.57;

    const QColor colLayer0Base = m_colorUtils->mix(background, primary, 0.99);
    const QColor colLayer0 = m_colorUtils->transparentize(colLayer0Base, backgroundTransparency);
    const QColor colOnLayer0 = onBackground;
    const QColor colLayer0Hover = m_colorUtils->transparentize(m_colorUtils->mix(colLayer0, colOnLayer0, 0.9), contentTransparency);
    const QColor colLayer0Active = m_colorUtils->transparentize(m_colorUtils->mix(colLayer0, colOnLayer0, 0.8), contentTransparency);
    const QColor colLayer0Border = m_colorUtils->mix(outlineVariant, colLayer0, 0.4);

    const QColor colLayer1Base = surfaceContainerLow;
    const QColor colLayer1 = m_colorUtils->solveOverlayColor(colLayer0Base, colLayer1Base, 1 - contentTransparency);
    const QColor colOnLayer1 = onSurfaceVariant;
    const QColor colOnLayer1Inactive = m_colorUtils->mix(colOnLayer1, colLayer1, 0.45);
    const QColor colLayer1Hover = m_colorUtils->transparentize(m_colorUtils->mix(colLayer1, colOnLayer1, 0.92), contentTransparency);
    const QColor colLayer1Active = m_colorUtils->transparentize(m_colorUtils->mix(colLayer1, colOnLayer1, 0.85), contentTransparency);

    const QColor colLayer2Base = surfaceContainer;
    const QColor colLayer2 = m_colorUtils->solveOverlayColor(colLayer1Base, colLayer2Base, 1 - contentTransparency);
    const QColor colOnLayer2 = onSurface;
    const QColor colLayer2Hover = m_colorUtils->solveOverlayColor(colLayer1Base, m_colorUtils->mix(colLayer2Base, colOnLayer2, 0.90), 1 - contentTransparency);
    const QColor colLayer2Active = m_colorUtils->solveOverlayColor(colLayer1Base, m_colorUtils->mix(colLayer2Base, colOnLayer2, 0.80), 1 - contentTransparency);
    const QColor colLayer2Disabled = m_colorUtils->solveOverlayColor(colLayer1Base, m_colorUtils->mix(colLayer2Base, background, 0.8), 1 - contentTransparency);
    const QColor colOnLayer2Disabled = m_colorUtils->mix(colOnLayer2, background, 0.4);

    const QColor colLayer3Base = surfaceContainerHigh;
    const QColor colLayer3 = m_colorUtils->solveOverlayColor(colLayer2Base, colLayer3Base, 1 - contentTransparency);
    const QColor colOnLayer3 = onSurface;
    const QColor colLayer3Hover = m_colorUtils->solveOverlayColor(colLayer2Base, m_colorUtils->mix(colLayer3Base, colOnLayer3, 0.90), 1 - contentTransparency);
    const QColor colLayer3Active = m_colorUtils->solveOverlayColor(colLayer2Base, m_colorUtils->mix(colLayer3Base, colOnLayer3, 0.80), 1 - contentTransparency);

    const QColor colLayer4Base = surfaceContainerHighest;
    const QColor colLayer4 = m_colorUtils->solveOverlayColor(colLayer3Base, colLayer4Base, 1 - contentTransparency);
    const QColor colOnLayer4 = onSurface;
    const QColor colLayer4Hover = m_colorUtils->solveOverlayColor(colLayer3Base, m_colorUtils->mix(colLayer4Base, colOnLayer4, 0.90), 1 - contentTransparency);
    const QColor colLayer4Active = m_colorUtils->solveOverlayColor(colLayer3Base, m_colorUtils->mix(colLayer4Base, colOnLayer4, 0.80), 1 - contentTransparency);

    const QColor colPrimary = primary;
    const QColor colOnPrimary = onPrimary;
    const QColor colOnPrimaryContainer = onPrimaryContainer;
    const QColor colPrimaryHover = m_colorUtils->mix(colPrimary, colLayer1Hover, 0.87);
    const QColor colPrimaryActive = m_colorUtils->mix(colPrimary, colLayer1Active, 0.7);
    const QColor colPrimaryContainer = primaryContainer;
    const QColor colPrimaryContainerHover = m_colorUtils->mix(colPrimaryContainer, colOnPrimaryContainer, 0.9);
    const QColor colPrimaryContainerActive = m_colorUtils->mix(colPrimaryContainer, colOnPrimaryContainer, 0.8);

    const QColor colSecondary = secondary;
    const QColor colSecondaryHover = m_colorUtils->mix(secondary, colLayer1Hover, 0.85);
    const QColor colSecondaryActive = m_colorUtils->mix(secondary, colLayer1Active, 0.4);
    const QColor colOnSecondary = onSecondary;
    const QColor colSecondaryContainer = secondaryContainer;
    const QColor colSecondaryContainerHover = m_colorUtils->mix(secondaryContainer, onSecondaryContainer, 0.90);
    const QColor colSecondaryContainerActive = m_colorUtils->mix(secondaryContainer, onSecondaryContainer, 0.54);
    const QColor colOnSecondaryContainer = onSecondaryContainer;

    const QColor colTertiary = tertiary;
    const QColor colTertiaryHover = m_colorUtils->mix(tertiary, colLayer1Hover, 0.85);
    const QColor colTertiaryActive = m_colorUtils->mix(tertiary, colLayer1Active, 0.4);
    const QColor colTertiaryContainer = tertiaryContainer;
    const QColor colTertiaryContainerHover = m_colorUtils->mix(tertiaryContainer, onTertiaryContainer, 0.90);
    const QColor colTertiaryContainerActive = m_colorUtils->mix(tertiaryContainer, colLayer1Active, 0.54);
    const QColor colOnTertiary = onTertiary;
    const QColor colOnTertiaryContainer = onTertiaryContainer;

    const QColor colSurfaceContainerLow = m_colorUtils->solveOverlayColor(background, surfaceContainerLowest, 1 - contentTransparency);
    const QColor colSurfaceContainer = m_colorUtils->solveOverlayColor(surfaceContainerLowest, surfaceContainerLow, 1 - contentTransparency);
    const QColor colSurfaceContainerHigh = m_colorUtils->solveOverlayColor(surfaceContainerLow, surfaceContainer, 1 - contentTransparency);
    const QColor colSurfaceContainerHighest = m_colorUtils->solveOverlayColor(surfaceContainer, surfaceContainerHigh, 1 - contentTransparency);
    const QColor colSurfaceContainerHighestHover = m_colorUtils->mix(surfaceContainerHighest, onSurface, 0.95);
    const QColor colSurfaceContainerHighestActive = m_colorUtils->mix(surfaceContainerHighest, onSurface, 0.85);

    const QColor colTooltip = inverseSurface;
    const QColor colOnTooltip = inverseOnSurface;
    const QColor colScrim = m_colorUtils->transparentize(scrim, 0.5);
    const QColor colShadow = m_colorUtils->transparentize(shadow, 0.7);
    const QColor colOutline = outline;
    const QColor colOutlineVariant = outlineVariant;
    const QColor colError = error;
    const QColor colErrorHover = m_colorUtils->mix(error, colLayer1Hover, 0.85);
    const QColor colErrorActive = m_colorUtils->mix(error, colLayer1Active, 0.7);
    const QColor colOnError = onError;
    const QColor colErrorContainer = errorContainer;
    const QColor colErrorContainerHover = m_colorUtils->mix(errorContainer, onErrorContainer, 0.90);
    const QColor colErrorContainerActive = m_colorUtils->mix(errorContainer, onErrorContainer, 0.70);
    const QColor colOnErrorContainer = onErrorContainer;

    m_colors->insert(QStringLiteral("colSubtext"), colOutline);
    m_colors->insert(QStringLiteral("colLayer0Base"), colLayer0Base);
    m_colors->insert(QStringLiteral("colLayer0"), colLayer0);
    m_colors->insert(QStringLiteral("colOnLayer0"), colOnLayer0);
    m_colors->insert(QStringLiteral("colLayer0Hover"), colLayer0Hover);
    m_colors->insert(QStringLiteral("colLayer0Active"), colLayer0Active);
    m_colors->insert(QStringLiteral("colLayer0Border"), colLayer0Border);
    m_colors->insert(QStringLiteral("colLayer1Base"), colLayer1Base);
    m_colors->insert(QStringLiteral("colLayer1"), colLayer1);
    m_colors->insert(QStringLiteral("colOnLayer1"), colOnLayer1);
    m_colors->insert(QStringLiteral("colOnLayer1Inactive"), colOnLayer1Inactive);
    m_colors->insert(QStringLiteral("colLayer1Hover"), colLayer1Hover);
    m_colors->insert(QStringLiteral("colLayer1Active"), colLayer1Active);
    m_colors->insert(QStringLiteral("colLayer2Base"), colLayer2Base);
    m_colors->insert(QStringLiteral("colLayer2"), colLayer2);
    m_colors->insert(QStringLiteral("colOnLayer2"), colOnLayer2);
    m_colors->insert(QStringLiteral("colLayer2Hover"), colLayer2Hover);
    m_colors->insert(QStringLiteral("colLayer2Active"), colLayer2Active);
    m_colors->insert(QStringLiteral("colLayer2Disabled"), colLayer2Disabled);
    m_colors->insert(QStringLiteral("colOnLayer2Disabled"), colOnLayer2Disabled);
    m_colors->insert(QStringLiteral("colLayer3Base"), colLayer3Base);
    m_colors->insert(QStringLiteral("colLayer3"), colLayer3);
    m_colors->insert(QStringLiteral("colOnLayer3"), colOnLayer3);
    m_colors->insert(QStringLiteral("colLayer3Hover"), colLayer3Hover);
    m_colors->insert(QStringLiteral("colLayer3Active"), colLayer3Active);
    m_colors->insert(QStringLiteral("colLayer4Base"), colLayer4Base);
    m_colors->insert(QStringLiteral("colLayer4"), colLayer4);
    m_colors->insert(QStringLiteral("colOnLayer4"), colOnLayer4);
    m_colors->insert(QStringLiteral("colLayer4Hover"), colLayer4Hover);
    m_colors->insert(QStringLiteral("colLayer4Active"), colLayer4Active);
    m_colors->insert(QStringLiteral("colPrimary"), colPrimary);
    m_colors->insert(QStringLiteral("colOnPrimary"), colOnPrimary);
    m_colors->insert(QStringLiteral("colPrimaryHover"), colPrimaryHover);
    m_colors->insert(QStringLiteral("colPrimaryActive"), colPrimaryActive);
    m_colors->insert(QStringLiteral("colPrimaryContainer"), colPrimaryContainer);
    m_colors->insert(QStringLiteral("colPrimaryContainerHover"), colPrimaryContainerHover);
    m_colors->insert(QStringLiteral("colPrimaryContainerActive"), colPrimaryContainerActive);
    m_colors->insert(QStringLiteral("colOnPrimaryContainer"), colOnPrimaryContainer);
    m_colors->insert(QStringLiteral("colSecondary"), colSecondary);
    m_colors->insert(QStringLiteral("colSecondaryHover"), colSecondaryHover);
    m_colors->insert(QStringLiteral("colSecondaryActive"), colSecondaryActive);
    m_colors->insert(QStringLiteral("colOnSecondary"), colOnSecondary);
    m_colors->insert(QStringLiteral("colSecondaryContainer"), colSecondaryContainer);
    m_colors->insert(QStringLiteral("colSecondaryContainerHover"), colSecondaryContainerHover);
    m_colors->insert(QStringLiteral("colSecondaryContainerActive"), colSecondaryContainerActive);
    m_colors->insert(QStringLiteral("colOnSecondaryContainer"), colOnSecondaryContainer);
    m_colors->insert(QStringLiteral("colTertiary"), colTertiary);
    m_colors->insert(QStringLiteral("colTertiaryHover"), colTertiaryHover);
    m_colors->insert(QStringLiteral("colTertiaryActive"), colTertiaryActive);
    m_colors->insert(QStringLiteral("colTertiaryContainer"), colTertiaryContainer);
    m_colors->insert(QStringLiteral("colTertiaryContainerHover"), colTertiaryContainerHover);
    m_colors->insert(QStringLiteral("colTertiaryContainerActive"), colTertiaryContainerActive);
    m_colors->insert(QStringLiteral("colOnTertiary"), colOnTertiary);
    m_colors->insert(QStringLiteral("colOnTertiaryContainer"), colOnTertiaryContainer);
    m_colors->insert(QStringLiteral("colSurfaceContainerLow"), colSurfaceContainerLow);
    m_colors->insert(QStringLiteral("colSurfaceContainer"), colSurfaceContainer);
    m_colors->insert(QStringLiteral("colSurfaceContainerHigh"), colSurfaceContainerHigh);
    m_colors->insert(QStringLiteral("colSurfaceContainerHighest"), colSurfaceContainerHighest);
    m_colors->insert(QStringLiteral("colSurfaceContainerHighestHover"), colSurfaceContainerHighestHover);
    m_colors->insert(QStringLiteral("colSurfaceContainerHighestActive"), colSurfaceContainerHighestActive);
    m_colors->insert(QStringLiteral("colOnSurface"), onSurface);
    m_colors->insert(QStringLiteral("colOnSurfaceVariant"), onSurfaceVariant);
    m_colors->insert(QStringLiteral("colTooltip"), colTooltip);
    m_colors->insert(QStringLiteral("colOnTooltip"), colOnTooltip);
    m_colors->insert(QStringLiteral("colScrim"), colScrim);
    m_colors->insert(QStringLiteral("colShadow"), colShadow);
    m_colors->insert(QStringLiteral("colOutline"), colOutline);
    m_colors->insert(QStringLiteral("colOutlineVariant"), colOutlineVariant);
    m_colors->insert(QStringLiteral("colError"), colError);
    m_colors->insert(QStringLiteral("colErrorHover"), colErrorHover);
    m_colors->insert(QStringLiteral("colErrorActive"), colErrorActive);
    m_colors->insert(QStringLiteral("colOnError"), colOnError);
    m_colors->insert(QStringLiteral("colErrorContainer"), colErrorContainer);
    m_colors->insert(QStringLiteral("colErrorContainerHover"), colErrorContainerHover);
    m_colors->insert(QStringLiteral("colErrorContainerActive"), colErrorContainerActive);
    m_colors->insert(QStringLiteral("colOnErrorContainer"), colOnErrorContainer);

    emit colorsChanged();
}