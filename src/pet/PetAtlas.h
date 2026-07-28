#pragma once

#include <QImage>
#include <QMetaType>
#include <QString>

#include <array>

enum class V2AnimationState {
    Idle = 0,
    RunningRight = 1,
    RunningLeft = 2,
    Waving = 3,
    Jumping = 4,
    Failed = 5,
    Waiting = 6,
    Running = 7,
    Review = 8,
    LookA = 9,
    LookB = 10,
};

// The v2 grid's column count, at namespace scope because AnimationSpec needs it
// and is declared before PetAtlas. PetAtlas::Columns is the name to use
// elsewhere.
inline constexpr int AtlasColumns = 8;

struct AnimationSpec {
    int row = 0;
    int frameCount = 0;
    // Fixed size, not a QVector: animationSpec() is called twice per animation
    // frame by AnimationPlayer and twice more by the settings preview, and a
    // QVector member meant every one of those calls heap-allocated a duration
    // list only to read one element out of it. Entries past frameCount are zero
    // and meaningless -- go through durationAt().
    std::array<int, AtlasColumns> durationsMs{};

    constexpr int durationAt(int frameIndex, int fallback) const
    {
        return frameIndex >= 0 && frameIndex < frameCount ? durationsMs[static_cast<size_t>(frameIndex)]
                                                          : fallback;
    }
};

// A QImage over a rectangle of `owner`'s pixels, without copying them.
//
// The view keeps those pixels alive by itself: it retains a copy of the owning
// QImage (implicitly shared, so a refcount rather than the buffer) and releases
// it when the last copy of the view goes away. That makes a frame safe to hold
// after its atlas has been evicted from AtlasCache, and independent of how the
// owner is allocated -- there is no QSharedPointer to borrow from a
// stack-allocated PetAtlas.
//
// Read-only by construction: writing detaches into a full copy, which gives back
// the allocation this exists to avoid. Shared by PetAtlas and AnimationClip so
// the lifetime rule is written down once.
QImage cellViewOf(const QImage &owner, int x, int y, int width, int height);

class PetAtlas final
{
public:
    static constexpr int CellWidth = 192;
    static constexpr int CellHeight = 208;
    static constexpr int Columns = AtlasColumns;
    static constexpr int Rows = 11;
    static constexpr int Width = Columns * CellWidth;
    static constexpr int Height = Rows * CellHeight;

    bool load(const QString &filePath);
    bool isValid() const;
    QString errorString() const;
    QString filePath() const;

    // An independent copy of one cell. Prefer frameView() on any path that runs
    // per animation frame; this allocates and memcpys 156KB every call.
    QImage frame(V2AnimationState state, int frameIndex) const;

    // The same cell WITHOUT copying its pixels: the returned QImage points into
    // this atlas's buffer and keeps that buffer alive by itself, so it stays
    // valid after the atlas is evicted from AtlasCache or this PetAtlas is
    // destroyed. Treat it as read-only -- writing to it detaches into a copy,
    // which silently gives back the allocation this exists to avoid.
    QImage frameView(V2AnimationState state, int frameIndex) const;

    QImage lookFrame(int clockwiseIndex) const;
    bool validateV2Occupancy(QString *error = nullptr) const;

    static const AnimationSpec &animationSpec(V2AnimationState state);

    // How many of a row's eight columns hold a frame. Rows 0-8 take it from their
    // animation spec; rows 9 and 10 are the sixteen look directions and use every
    // column. The occupancy rule below is two-sided, so this is the one number that
    // decides both "must have pixels" and "must be transparent" -- it exists as a
    // function because that derivation was written out separately in the validator
    // and in the test fixture, and AtlasComposer would have been a third copy.
    static int usedColumns(int row);

    // Occupancy against an image this class does not own, so a composed atlas can be
    // checked before anything writes it to disk. Note the rule is two-sided: a cell
    // that *should* hold a frame and is empty fails just as loudly as an unused cell
    // that is not transparent.
    static bool validateV2Occupancy(const QImage &image, QString *error = nullptr);

private:
    QImage m_image;
    QString m_filePath;
    QString m_error;
};

Q_DECLARE_METATYPE(V2AnimationState)
