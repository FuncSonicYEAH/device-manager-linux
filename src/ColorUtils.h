#pragma once

#include <QColor>
#include <QObject>

// Port of the `ColorUtils` singleton from modules/common/functions/ColorUtils.qml.
class ColorUtils : public QObject
{
    Q_OBJECT

public:
    explicit ColorUtils(QObject *parent = nullptr);

    Q_INVOKABLE QColor mix(const QColor &c1, const QColor &c2, qreal percentage = 0.5) const;
    Q_INVOKABLE QColor transparentize(const QColor &color, qreal percentage = 1.0) const;
    Q_INVOKABLE QColor applyAlpha(const QColor &color, qreal alpha) const;
    Q_INVOKABLE QColor solveOverlayColor(const QColor &baseColor,
                                         const QColor &targetColor,
                                         qreal overlayOpacity) const;
    Q_INVOKABLE bool isDark(const QColor &color) const;
    Q_INVOKABLE qreal clamp01(qreal x) const;
};