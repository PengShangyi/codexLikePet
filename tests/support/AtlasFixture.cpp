#include "support/AtlasFixture.h"

#include "pet/PetAtlas.h"

#include <QBuffer>
#include <QFile>
#include <QHash>
#include <QPainter>

#include <functional>

namespace {

// Keyed by a description of the fixture rather than by its pixels: hashing 14MB
// to avoid a 69ms encode would defeat the purpose. Returned by value because
// QByteArray is implicitly shared -- a reference into the hash would dangle the
// moment another fixture is inserted and rehashes it.
QByteArray encodedPng(const QString &key, const std::function<QImage()> &make)
{
    static QHash<QString, QByteArray> cache;
    const auto existing = cache.constFind(key);
    if (existing != cache.cend()) return *existing;

    QByteArray blob;
    QBuffer buffer(&blob);
    buffer.open(QIODevice::WriteOnly);
    make().save(&buffer, "PNG");
    cache.insert(key, blob);
    return blob;
}

bool writeBytes(const QString &path, const QByteArray &bytes)
{
    if (bytes.isEmpty()) return false;
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}

QImage buildValid()
{
    QImage image(PetAtlas::Width, PetAtlas::Height, QImage::Format_RGBA8888);
    image.fill(Qt::transparent);
    for (int row = 0; row < PetAtlas::Rows; ++row) {
        // The same function the occupancy check enforces, so a fixture built here
        // cannot disagree with the rule it is meant to satisfy.
        for (int column = 0; column < PetAtlas::usedColumns(row); ++column) {
            image.setPixelColor(column * PetAtlas::CellWidth + 1,
                                row * PetAtlas::CellHeight + 1,
                                Qt::white);
        }
    }
    return image;
}

const QImage &validImageRef()
{
    static const QImage image = buildValid();
    return image;
}

}  // namespace

namespace TestAtlas {

const QImage &validImage()
{
    return validImageRef();
}

bool writeValid(const QString &path)
{
    return writeBytes(path, encodedPng(QStringLiteral("valid"), &validImageRef));
}

bool writeFilled(const QString &path, const QColor &color)
{
    const QString key = QStringLiteral("filled:") + color.name(QColor::HexArgb);
    return writeBytes(path, encodedPng(key, [color] {
        QImage image(PetAtlas::Width, PetAtlas::Height, QImage::Format_RGBA8888);
        image.fill(color);
        return image;
    }));
}

bool writeClip(const QString &path, int frames, const QColor &color)
{
    const QString key = QStringLiteral("clip:%1:%2").arg(frames).arg(color.name(QColor::HexArgb));
    return writeBytes(path, encodedPng(key, [frames, color] {
        QImage image(PetAtlas::CellWidth * frames, PetAtlas::CellHeight,
                     QImage::Format_RGBA8888);
        image.fill(color);
        return image;
    }));
}

bool writeClipFrames(const QString &path, const QList<QColor> &colors)
{
    QStringList parts;
    parts.reserve(colors.size());
    for (const QColor &color : colors) parts.append(color.name(QColor::HexArgb));
    const QString key = QStringLiteral("clipframes:") + parts.join(QLatin1Char(','));

    return writeBytes(path, encodedPng(key, [colors] {
        QImage image(PetAtlas::CellWidth * colors.size(), PetAtlas::CellHeight,
                     QImage::Format_RGBA8888);
        image.fill(Qt::transparent);
        // fillRect, not a setPixelColor loop: the loop this replaces walked
        // 120k pixels one virtual call at a time.
        QPainter painter(&image);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        for (int frame = 0; frame < colors.size(); ++frame) {
            painter.fillRect(frame * PetAtlas::CellWidth, 0,
                             PetAtlas::CellWidth, PetAtlas::CellHeight,
                             colors.at(frame));
        }
        return image;
    }));
}

}  // namespace TestAtlas
