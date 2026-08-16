// Reusable smoothed line chart used by the monitoring dialogs. The `series`
// array holds one entry per curve with parallel `values` lists aligned with
// the `timeCount` most recent samples (oldest first). Time on the x axis runs
// from -(timeCount-1) to 0 seconds. Y is auto-scaled to the data unless
// fixedMin/fixedMax are set.
import QtQuick
import QtQuick.Controls
import Components

Item {
    id: chart

    property var series: []        // [{ color: string, values: [number] }]
    property int timeCount: 0
    property real fixedMin: Number.NaN
    property real fixedMax: Number.NaN
    property string unit: ""       // small label in the top-left corner
    property var formatValue: function(v) { return String(Math.round(v)) }

    function niceStep(range) {
        if (range <= 0)
            return 1
        var pow = Math.pow(10, Math.floor(Math.log(range) / Math.LN10))
        var frac = range / pow
        return (frac >= 5 ? 10 : frac >= 2 ? 5 : 2) * pow
    }

    Canvas {
        id: canvas
        anchors.fill: parent

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        Connections {
            target: chart
            function onSeriesChanged() { canvas.requestPaint() }
            function onTimeCountChanged() { canvas.requestPaint() }
            function onFixedMinChanged() { canvas.requestPaint() }
            function onFixedMaxChanged() { canvas.requestPaint() }
            function onUnitChanged() { canvas.requestPaint() }
        }

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.setTransform(1, 0, 0, 1, 0, 0)

            var ml = 44, mr = 10, mt = 12, mb = 24
            var pw = width - ml - mr
            var ph = height - mt - mb
            if (pw <= 0 || ph <= 0)
                return

            var n = chart.timeCount
            var sCount = chart.series.length

            // ---- value range -------------------------------------------
            var vmin = isFinite(chart.fixedMin) ? chart.fixedMin : Infinity
            var vmax = isFinite(chart.fixedMax) ? chart.fixedMax : -Infinity
            if (!isFinite(chart.fixedMin) || !isFinite(chart.fixedMax)) {
                for (var s = 0; s < sCount; s++) {
                    var vals = chart.series[s].values
                    for (var i = 0; i < vals.length; i++) {
                        var v = vals[i]
                        if (isNaN(v))
                            continue
                        if (!isFinite(chart.fixedMin) && v < vmin) vmin = v
                        if (!isFinite(chart.fixedMax) && v > vmax) vmax = v
                    }
                }
                if (!isFinite(vmin) || !isFinite(vmax)) {
                    vmin = 0
                    vmax = 100
                }
                var pad = Math.max(1, (vmax - vmin) * 0.12)
                if (!isFinite(chart.fixedMin))
                    vmin = Math.floor(vmin - pad)
                if (!isFinite(chart.fixedMax))
                    vmax = Math.ceil(vmax + pad)
                if (vmax - vmin < 2)
                    vmax = vmin + 2
            }

            var yStep = chart.niceStep((vmax - vmin) / 4)
            var y0 = Math.floor(vmin / yStep) * yStep

            function xFor(i) { return ml + (n <= 1 ? pw : (i / (n - 1)) * pw) }
            function yFor(v) { return mt + ph - ((v - vmin) / (vmax - vmin)) * ph }

            // ---- horizontal grid + y labels ------------------------------
            ctx.font = "10px sans-serif"
            ctx.textAlign = "right"
            ctx.textBaseline = "middle"
            for (var gy = y0; gy <= vmax; gy += yStep) {
                var yy = yFor(gy)
                ctx.strokeStyle = Appearance.colors.colOnLayer1
                ctx.globalAlpha = 0.18
                ctx.lineWidth = 1
                ctx.beginPath()
                ctx.moveTo(ml, yy)
                ctx.lineTo(ml + pw, yy)
                ctx.stroke()
                ctx.globalAlpha = 1
                ctx.fillStyle = Appearance.colors.colOnLayer1
                ctx.fillText(chart.formatValue(gy), ml - 6, yy)
            }
            ctx.textAlign = "left"
            if (chart.unit !== "")
                ctx.fillText(chart.unit, ml + 4, mt + 4)

            // ---- vertical grid + x labels (time) -------------------------
            if (n > 1) {
                var tmin = -(n - 1)
                var tStep = chart.niceStep((0 - tmin) / 5)
                var t0 = Math.floor(tmin / tStep) * tStep
                ctx.textAlign = "center"
                for (var tx = t0; tx <= 0; tx += tStep) {
                    var xi = ml + ((tx - tmin) / (0 - tmin)) * pw
                    ctx.strokeStyle = Appearance.colors.colOnLayer1
                    ctx.globalAlpha = 0.1
                    ctx.beginPath()
                    ctx.moveTo(xi, mt)
                    ctx.lineTo(xi, mt + ph)
                    ctx.stroke()
                    ctx.globalAlpha = 1
                    ctx.fillStyle = Appearance.colors.colOnLayer1
                    ctx.fillText(tx + "s", xi, mt + ph + 14)
                }
            }

            // ---- curves --------------------------------------------------
            for (s = 0; s < sCount; s++) {
                var svals = chart.series[s].values
                var color = chart.series[s].color
                var pts = []
                for (i = 0; i < n; i++) {
                    var value = i < svals.length ? svals[i] : NaN
                    if (isNaN(value))
                        continue
                    pts.push({ x: xFor(i), y: yFor(value) })
                }
                if (pts.length < 2)
                    continue

                ctx.strokeStyle = color
                ctx.lineWidth = 2
                ctx.lineJoin = "round"
                ctx.lineCap = "round"
                ctx.beginPath()
                ctx.moveTo(pts[0].x, pts[0].y)
                // smooth polyline: quadratic curves through midpoints
                for (var k = 1; k < pts.length - 1; k++) {
                    var xc = (pts[k].x + pts[k + 1].x) / 2
                    var yc = (pts[k].y + pts[k + 1].y) / 2
                    ctx.quadraticCurveTo(pts[k].x, pts[k].y, xc, yc)
                }
                ctx.lineTo(pts[pts.length - 1].x, pts[pts.length - 1].y)
                ctx.stroke()

                // dot on the latest point
                var last = pts[pts.length - 1]
                ctx.fillStyle = color
                ctx.beginPath()
                ctx.arc(last.x, last.y, 3.5, 0, Math.PI * 2)
                ctx.fill()
            }
        }
    }
}