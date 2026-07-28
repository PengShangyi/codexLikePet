#pragma once

#include <QImage>
#include <QSet>
#include <QWidget>

// The composed atlas, whole, with its cell grid drawn over it.
//
// Deliberately not PetPreviewWidget. That one animates a single cell out of a
// QSharedPointer<PetAtlas>, and a PetAtlas only exists by loading a *file* -- so
// showing an in-memory result through it would mean writing 14 MB to disk on every
// tolerance nudge. What needs reviewing here is also the opposite thing: the whole
// 8x11 grid at once, where a mis-sliced row or an empty cell is obvious at a glance.
// The gridlines are the feature, not decoration.
//
// Takes an already-resolved colour scheme rather than observing the system, the same
// arrangement as PetPreviewWidget and SpeechBubble.
//
// Like PetPreviewWidget, this must never be matched by a stylesheet rule: a match
// makes QStyleSheetStyle set WA_StyledBackground, which paints a background before
// paintEvent and undoes the opaque-paint optimisation below.
class AtlasGridPreview final : public QWidget
{
    Q_OBJECT

public:
    explicit AtlasGridPreview(QWidget *parent = nullptr);

    // A null atlas clears the preview back to its empty state.
    void setAtlas(const QImage &atlas);
    // Cells to draw attention to, as row * Columns + column. Drawn as a tinted
    // overlay, so a problem the banner names in words can also be pointed at.
    void setFlaggedCells(const QSet<int> &cells);
    void setColorScheme(Qt::ColorScheme scheme);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QRect atlasRect() const;

    QImage m_atlas;
    QSet<int> m_flagged;
    Qt::ColorScheme m_scheme = Qt::ColorScheme::Light;
};
