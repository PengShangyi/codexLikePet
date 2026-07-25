#include "pet/PetAtlas.h"

#include <QFileInfo>
#include <QImageReader>

#include <algorithm>

namespace {

// Every frame in the row holds for `duration`, except the last, which holds
// longer so the loop reads as a beat rather than a spin.
constexpr AnimationSpec repeated(int row, int count, int duration, int finalDuration)
{
    AnimationSpec spec{row, count, {}};
    for (int index = 0; index < count; ++index) {
        spec.durationsMs[static_cast<size_t>(index)] = duration;
    }
    if (count > 0) spec.durationsMs[static_cast<size_t>(count - 1)] = finalDuration;
    return spec;
}

// Built once at compile time, indexed by V2AnimationState. Replaces a switch
// that constructed a fresh QVector on every call.
constexpr std::array<AnimationSpec, PetAtlas::Rows> kAnimationSpecs{{
    AnimationSpec{0, 6, {{280, 110, 110, 140, 140, 320}}},  // Idle
    repeated(1, 8, 120, 220),                               // RunningRight
    repeated(2, 8, 120, 220),                               // RunningLeft
    repeated(3, 4, 140, 280),                               // Waving
    repeated(4, 5, 140, 280),                               // Jumping
    repeated(5, 8, 140, 240),                               // Failed
    repeated(6, 6, 150, 260),                               // Waiting
    repeated(7, 6, 120, 220),                               // Running
    repeated(8, 6, 150, 280),                               // Review
    repeated(9, 8, 150, 150),                               // LookA
    repeated(10, 8, 150, 150),                              // LookB
}};

}  // namespace

QImage cellViewOf(const QImage &owner, int x, int y, int width, int height)
{
    if (owner.isNull()) return {};
    // Four bytes per pixel holds for both formats these images are stored in
    // (ARGB32_Premultiplied) and the stride comes from the owner, so the view
    // addresses the right pixels even though it is narrower than its source.
    const uchar *origin = owner.constScanLine(y) + static_cast<qsizetype>(x) * 4;
    auto *retained = new QImage(owner);
    return QImage(origin,
                  width,
                  height,
                  owner.bytesPerLine(),
                  owner.format(),
                  [](void *image) { delete static_cast<QImage *>(image); },
                  retained);
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

    // ARGB32_Premultiplied is the raster engine's native format. Stored as
    // RGBA8888, every drawImage converted the whole cell first -- on each paint,
    // not once per load. Converting here does that work a single time.
    //
    // Two things this does not break: validateV2Occupancy reads scanlines as
    // QRgb and calls qAlpha, which still finds alpha in the top byte on
    // little-endian; and premultiplication only perturbs RGB where alpha < 255,
    // which is exactly what drawImage was doing per paint anyway.
    m_image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
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

QImage PetAtlas::frame(V2AnimationState state, int frameIndex) const
{
    if (!isValid()) {
        return {};
    }
    const AnimationSpec &spec = animationSpec(state);
    if (frameIndex < 0 || frameIndex >= spec.frameCount) {
        return {};
    }
    return m_image.copy(frameIndex * CellWidth, spec.row * CellHeight, CellWidth, CellHeight);
}

QImage PetAtlas::frameView(V2AnimationState state, int frameIndex) const
{
    if (!isValid()) {
        return {};
    }
    const AnimationSpec &spec = animationSpec(state);
    if (frameIndex < 0 || frameIndex >= spec.frameCount) {
        return {};
    }
    return cellViewOf(m_image, frameIndex * CellWidth, spec.row * CellHeight, CellWidth, CellHeight);
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

            // Hatch Pet's extended v2 layout may place a dedicated neutral/front
            // still in row 0, column 6. The slot is optional and is not part of
            // the six-frame idle loop; row 0, column 7 remains unused.
            if (row == 0 && column == 6) continue;
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

const AnimationSpec &PetAtlas::animationSpec(V2AnimationState state)
{
    const auto index = static_cast<size_t>(state);
    if (index >= kAnimationSpecs.size()) {
        static constexpr AnimationSpec unknown{};
        return unknown;
    }
    return kAnimationSpecs[index];
}
