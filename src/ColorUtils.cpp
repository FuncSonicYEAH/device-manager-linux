#include "ColorUtils.h"

#include <QtMath>

ColorUtils::ColorUtils(QObject *parent)
    : QObject(parent)
{
}

QColor ColorUtils::mix(const QColor &c1, const QColor &c2, qreal percentage) const
{
    return QColor::fromRgbF(percentage * c1.redF() + (1 - percentage) * c2.redF(),
                            percentage * c1.greenF() + (1 - percentage) * c2.greenF(),
                            percentage * c1.blueF() + (1 - percentage) * c2.blueF(),
                            percentage * c1.alphaF() + (1 - percentage) * c2.alphaF());
}

QColor ColorUtils::transparentize(const QColor &color, qreal percentage) const
{
    return QColor::fromRgbF(color.redF(), color.greenF(), color.blueF(),
                            color.alphaF() * (1 - percentage));
}

QColor ColorUtils::applyAlpha(const QColor &color, qreal alpha) const
{
    const qreal a = qBound<qreal>(0.0, alpha, 1.0);
    return QColor::fromRgbF(color.redF(), color.greenF(), color.blueF(), a);
}

QColor ColorUtils::solveOverlayColor(const QColor &baseColor,
                                     const QColor &targetColor,
                                     qreal overlayOpacity) const
{
    const qreal invA = 1.0 - overlayOpacity;
    const qreal r = clamp01((targetColor.redF() - baseColor.redF() * invA) / overlayOpacity);
    const qreal g = clamp01((targetColor.greenF() - baseColor.greenF() * invA) / overlayOpacity);
    const qreal b = clamp01((targetColor.blueF() - baseColor.blueF() * invA) / overlayOpacity);
    return QColor::fromRgbF(r, g, b, overlayOpacity);
}

bool ColorUtils::isDark(const QColor &color) const
{
    return color.lightnessF() < 0.5;
}

qreal ColorUtils::clamp01(qreal x) const
{
    return qBound<qreal>(0.0, x, 1.0);
}