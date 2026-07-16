#include "pet/PetAtlas.h"

#include <QFileInfo>
#include <QImageReader>

#include <algorithm>

namespace {
QVector<int> repeatedDurations(int count, int duration, int finalDuration)
{
    QVector<int> values(count, duration);
    if (!values.isEmpty()) {
        values.last() = finalDuration;
    }
    return values;
}
}

bool PetAtlas::load(const QString &filePath)
{
    m_image = {};
    m_error.clear();
    m_filePath = QFileInfo(filePath).absoluteFilePath();

    QImageReader reader(m_filePath);
    reader.setAutoTransform(false);
    const QSize sourceSize = reader.size();
    if (sourceSize.isValid() && sourceSize != QSize(Width, Height)) {
        m_error = QStringLiteral("Atlas must be %1x%2; got %3x%4")
                      .arg(Width)
                      .arg(Height)
                      .arg(sourceSize.width())
                      .arg(sourceSize.height());
        return false;
    }

    QImage image = reader.read();
    if (image.isNull()) {
        m_error = reader.errorString().isEmpty() ? QStringLiteral("Unable to decode atlas")
                                                  : reader.errorString();
        return false;
    }
    if (image.size() != QSize(Width, Height)) {
        m_error = QStringLiteral("Atlas must be %1x%2").arg(Width).arg(Height);
        return false;
    }
    if (!image.hasAlphaChannel()) {
        m_error = QStringLiteral("Atlas must have an alpha channel");
        return false;
    }

    m_image = image.convertToFormat(QImage::Format_RGBA8888);
    return true;
}

bool PetAtlas::isValid() const
{
    return !m_image.isNull();
}

QString PetAtlas::errorString() const
{
    return m_error;
}

QString PetAtlas::filePath() const
{
    return m_filePath;
}

qsizetype PetAtlas::decodedByteCount() const
{
    return m_image.sizeInBytes();
}

QImage PetAtlas::frame(V2AnimationState state, int frameIndex) const
{
    if (!isValid()) {
        return {};
    }
    const AnimationSpec spec = animationSpec(state);
    if (frameIndex < 0 || frameIndex >= spec.frameCount) {
        return {};
    }
    return m_image.copy(frameIndex * CellWidth, spec.row * CellHeight, CellWidth, CellHeight);
}

QImage PetAtlas::lookFrame(int clockwiseIndex) const
{
    if (clockwiseIndex < 0 || clockwiseIndex >= 16 || !isValid()) {
        return {};
    }
    const int row = clockwiseIndex < 8 ? 9 : 10;
    const int column = clockwiseIndex % 8;
    return m_image.copy(column * CellWidth, row * CellHeight, CellWidth, CellHeight);
}

bool PetAtlas::validateV2Occupancy(QString *error) const
{
    if (!isValid()) {
        if (error) {
            *error = QStringLiteral("Atlas is not loaded");
        }
        return false;
    }

    for (int row = 0; row < Rows; ++row) {
        const int usedColumns = row <= 8
            ? animationSpec(static_cast<V2AnimationState>(row)).frameCount
            : Columns;
        for (int column = 0; column < Columns; ++column) {
            bool hasVisiblePixel = false;
            const int startX = column * CellWidth;
            const int startY = row * CellHeight;
            for (int y = startY; y < startY + CellHeight && !hasVisiblePixel; ++y) {
                const QRgb *line = reinterpret_cast<const QRgb *>(m_image.constScanLine(y));
                for (int x = startX; x < startX + CellWidth; ++x) {
                    if (qAlpha(line[x]) != 0) {
                        hasVisiblePixel = true;
                        break;
                    }
                }
            }

            const bool shouldBeUsed = column < usedColumns;
            if (shouldBeUsed != hasVisiblePixel) {
                if (error) {
                    *error = shouldBeUsed
                        ? QStringLiteral("Used cell row %1 column %2 is empty").arg(row).arg(column)
                        : QStringLiteral("Unused cell row %1 column %2 is not transparent")
                              .arg(row)
                              .arg(column);
                }
                return false;
            }
        }
    }
    return true;
}

AnimationSpec PetAtlas::animationSpec(V2AnimationState state)
{
    switch (state) {
    case V2AnimationState::Idle:
        return {0, 6, {280, 110, 110, 140, 140, 320}};
    case V2AnimationState::RunningRight:
        return {1, 8, repeatedDurations(8, 120, 220)};
    case V2AnimationState::RunningLeft:
        return {2, 8, repeatedDurations(8, 120, 220)};
    case V2AnimationState::Waving:
        return {3, 4, repeatedDurations(4, 140, 280)};
    case V2AnimationState::Jumping:
        return {4, 5, repeatedDurations(5, 140, 280)};
    case V2AnimationState::Failed:
        return {5, 8, repeatedDurations(8, 140, 240)};
    case V2AnimationState::Waiting:
        return {6, 6, repeatedDurations(6, 150, 260)};
    case V2AnimationState::Running:
        return {7, 6, repeatedDurations(6, 120, 220)};
    case V2AnimationState::Review:
        return {8, 6, repeatedDurations(6, 150, 280)};
    case V2AnimationState::LookA:
        return {9, 8, repeatedDurations(8, 150, 150)};
    case V2AnimationState::LookB:
        return {10, 8, repeatedDurations(8, 150, 150)};
    }
    return {};
}
