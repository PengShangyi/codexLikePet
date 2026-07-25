#include "pet/AnimationClip.h"

#include "pet/ClipContract.h"
#include "pet/PetAtlas.h"

#include <QFileInfo>
#include <QImageReader>

bool AnimationClip::load(const QString &filePath, const QVector<int> &durationsMs)
{
    m_image = {};
    m_durationsMs.clear();
    m_error.clear();
    m_filePath = QFileInfo(filePath).absoluteFilePath();

    QImageReader reader(m_filePath);
    reader.setAutoTransform(false);
    if (!ClipContract::isValidGeometry(reader.size())) {
        m_error = QStringLiteral("Clip must be transparent and contain 1 to 8 192x208 frames");
        return false;
    }

    QImage image = reader.read();
    if (image.isNull()) {
        m_error = reader.errorString().isEmpty() ? QStringLiteral("Unable to decode clip")
                                                  : reader.errorString();
        return false;
    }
    // Re-check against the decoded image, not just the header it advertised.
    int frames = 0;
    if (!image.hasAlphaChannel() || !ClipContract::isValidGeometry(image.size(), &frames)) {
        m_error = QStringLiteral("Clip must be transparent and contain 1 to 8 192x208 frames");
        return false;
    }
    if (durationsMs.size() != frames) {
        m_error = QStringLiteral("Clip frame count and durations do not match");
        return false;
    }
    if (!ClipContract::areValidDurations(durationsMs)) {
        m_error = QStringLiteral("Clip durations must be between 50 and 2000ms");
        return false;
    }

    m_image = image.convertToFormat(QImage::Format_RGBA8888);
    m_durationsMs = durationsMs;
    return true;
}

bool AnimationClip::isValid() const { return !m_image.isNull(); }
QString AnimationClip::errorString() const { return m_error; }
QString AnimationClip::filePath() const { return m_filePath; }
int AnimationClip::frameCount() const { return m_durationsMs.size(); }

int AnimationClip::durationMs(int frameIndex) const
{
    return m_durationsMs.value(frameIndex, 150);
}

QImage AnimationClip::frame(int frameIndex) const
{
    if (!isValid() || frameIndex < 0 || frameIndex >= frameCount()) return {};
    return m_image.copy(frameIndex * PetAtlas::CellWidth,
                        0,
                        PetAtlas::CellWidth,
                        PetAtlas::CellHeight);
}
