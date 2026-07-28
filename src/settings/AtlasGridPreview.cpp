#include "settings/AtlasGridPreview.h"

#include "pet/PetAtlas.h"
#include "ui/Theme.h"

#include <QPainter>

namespace {

// Big enough to read a pose in, small enough that eleven rows fit a window.
constexpr int kMinimumHeight = 260;
// One checker square, in device-independent points. Transparency has to read *as*
// transparency here, because a cell that came out empty is the failure this preview
// exists to show, and empty-on-flat-colour looks the same as empty-on-nothing.
constexpr int kCheckerSize = 8;

}  // namespace

AtlasGridPreview::AtlasGridPreview(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(kMinimumHeight);
    // paintEvent fills the whole rect, so Qt need not repaint the stylesheet-styled
    // page behind it. Same reasoning as PetPreviewWidget, and the reason no QSS rule
    // may match this class.
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void AtlasGridPreview::setAtlas(const QImage &atlas)
{
    m_atlas = atlas;
    update();
}

void AtlasGridPreview::setFlaggedCells(const QSet<int> &cells)
{
    if (m_flagged == cells) return;
    m_flagged = cells;
    update();
}

void AtlasGridPreview::setColorScheme(Qt::ColorScheme scheme)
{
    if (m_scheme == scheme) return;
    m_scheme = scheme;
    update();
}

QRect AtlasGridPreview::atlasRect() const
{
    // The atlas is 1536x2288 and the widget is nothing like that shape, so fit it
    // whole and centre it. Fitting rather than cropping is the point: a preview that
    // hid the bottom rows would hide the look directions, which are exactly the rows
    // an author is least sure about.
    const QSize fitted = QSize(PetAtlas::Width, PetAtlas::Height)
                             .scaled(size(), Qt::KeepAspectRatio);
    return QRect(QPoint((width() - fitted.width()) / 2, (height() - fitted.height()) / 2), fitted);
}

void AtlasGridPreview::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    const Theme::Palette palette = Theme::palette(m_scheme);
    QPainter painter(this);
    painter.fillRect(rect(), palette.sunken);

    const QRect target = atlasRect();
    if (target.isEmpty()) return;

    // Checkerboard, clipped to where the atlas will land so the surround stays flat.
    painter.save();
    painter.setClipRect(target);
    QColor light = palette.cardBackground;
    QColor dark = palette.pageBackground;
    for (int y = target.top(); y < target.bottom(); y += kCheckerSize) {
        for (int x = target.left(); x < target.right(); x += kCheckerSize) {
            const bool even = ((x - target.left()) / kCheckerSize + (y - target.top()) / kCheckerSize) % 2 == 0;
            painter.fillRect(QRect(x, y, kCheckerSize, kCheckerSize), even ? light : dark);
        }
    }
    painter.restore();

    if (!m_atlas.isNull()) {
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.drawImage(target, m_atlas);
    }

    const double cellW = target.width() / double(PetAtlas::Columns);
    const double cellH = target.height() / double(PetAtlas::Rows);

    // Flagged cells first, so the gridlines stay on top and keep reading as a grid.
    if (!m_flagged.isEmpty()) {
        QColor tint = palette.danger;
        tint.setAlpha(70);
        for (const int cell : m_flagged) {
            const int row = cell / PetAtlas::Columns;
            const int column = cell % PetAtlas::Columns;
            if (row < 0 || row >= PetAtlas::Rows || column < 0 || column >= PetAtlas::Columns) continue;
            painter.fillRect(QRectF(target.left() + column * cellW,
                                    target.top() + row * cellH,
                                    cellW, cellH),
                             tint);
        }
    }

    painter.setPen(QPen(palette.stageBorder, Theme::Metrics::hairlineThickness));
    for (int column = 1; column < PetAtlas::Columns; ++column) {
        const double x = target.left() + column * cellW;
        painter.drawLine(QPointF(x, target.top()), QPointF(x, target.bottom()));
    }
    for (int row = 1; row < PetAtlas::Rows; ++row) {
        const double y = target.top() + row * cellH;
        painter.drawLine(QPointF(target.left(), y), QPointF(target.right(), y));
    }
    painter.setPen(QPen(palette.cardBorder, Theme::Metrics::hairlineThickness));
    painter.drawRect(target.adjusted(0, 0, -1, -1));
}
