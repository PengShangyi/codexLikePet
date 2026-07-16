#pragma once

#include <QImage>
#include <QMetaType>
#include <QString>
#include <QVector>

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

struct AnimationSpec {
    int row = 0;
    int frameCount = 0;
    QVector<int> durationsMs;
};

class PetAtlas final
{
public:
    static constexpr int CellWidth = 192;
    static constexpr int CellHeight = 208;
    static constexpr int Columns = 8;
    static constexpr int Rows = 11;
    static constexpr int Width = Columns * CellWidth;
    static constexpr int Height = Rows * CellHeight;

    bool load(const QString &filePath);
    bool isValid() const;
    QString errorString() const;
    QString filePath() const;
    qsizetype decodedByteCount() const;

    QImage frame(V2AnimationState state, int frameIndex) const;
    QImage lookFrame(int clockwiseIndex) const;

    static AnimationSpec animationSpec(V2AnimationState state);

private:
    QImage m_image;
    QString m_filePath;
    QString m_error;
};

Q_DECLARE_METATYPE(V2AnimationState)
