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
    QImage frame(int frameIndex) const;

private:
    QImage m_image;
    QVector<int> m_durationsMs;
    QString m_filePath;
    QString m_error;
};
