#pragma once

#include <QImage>
#include <QString>
#include <QVector>

class AnimationClip final
{
public:
    bool load(const QString &filePath, const QVector<int> &durationsMs);
    bool isValid() const;
    QString errorString() const;
    QString filePath() const;
    int frameCount() const;
    int durationMs(int frameIndex) const;

    // An independent copy of one frame.
    QImage frame(int frameIndex) const;

    // The same frame without copying its pixels. See PetAtlas::frameView for the
    // lifetime rule: the view keeps the pixels alive on its own, and writing to
    // it detaches and undoes the point of it.
    QImage frameView(int frameIndex) const;

private:
    QImage m_image;
    QVector<int> m_durationsMs;
    QString m_filePath;
    QString m_error;
};
