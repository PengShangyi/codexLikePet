#include "pet/AtlasComposer.h"

#include <QtMath>

#include <algorithm>
#include <cmath>

namespace {

// Inset kept clear inside every cell, so one frame's antialiased edge cannot bleed
// into its neighbour when the atlas is sampled.
constexpr int kCellMargin = 5;

// Alpha at or above which a pixel counts as content when measuring a bounding box.
// Not >0: a single stray antialiased pixel would otherwise inflate the box and shrink
// the whole atlas to accommodate it.
constexpr int kBoundsAlpha = 8;

// The soft edge. Chroma distance at or below keyTolerance is background, at or above
// keyTolerance + this is subject, and between the two alpha ramps linearly. Edge
// pixels lie on the segment between the key's chroma and the subject's, so distance
// grows monotonically outward and the ramp falls out for free.
constexpr int kKeySoftness = 24;

// Below this alpha, unmixing amplifies noise by more than tenfold and the pixel is
// over 90% background anyway. Dropped outright instead.
constexpr int kDespillMinAlpha = 24;

// How far above the other two channels the key's dominant channel may sit, once a
// pixel is in the edge band.
constexpr int kSpillHeadroom = 8;

// A frame taller than this multiple of the median is called out. It still counts
// towards the fit -- excluding it would mean clipping it, which is worse than
// shrinking everything else.
constexpr double kOutlierFactor = 1.6;

struct Chroma {
    int cb;
    int cr;
};

Chroma chromaOf(int r, int g, int b)
{
    // BT.601 luma, integer. The choice that matters here is comparing *chrominance*
    // rather than RGB: with plain Euclidean RGB the luminance axis dominates, so a
    // dark green subject on a green key measures further away than a light green one
    // and survives while the light one is eaten. Positions in (Cb, Cr) make one
    // tolerance number behave the same way across the whole brightness range, and
    // need no saturation floor -- a neutral grey already sits at Cb = Cr = 0.
    const int y = (77 * r + 150 * g + 29 * b) >> 8;
    return {b - y, r - y};
}

double chromaDistance(const Chroma &a, const Chroma &b)
{
    const double dcb = a.cb - b.cb;
    const double dcr = a.cr - b.cr;
    return std::sqrt(dcb * dcb + dcr * dcr);
}

int dominantChannel(const QColor &key)
{
    const int r = key.red();
    const int g = key.green();
    const int b = key.blue();
    if (g >= r && g >= b) return 1;
    return r >= b ? 0 : 2;
}

QImage toStraightArgb32(const QImage &image)
{
    // Straight alpha, never premultiplied, all the way to the file. We create partial
    // alpha ourselves in the keying pass, and premultiplying then unpremultiplying on
    // save loses up to 255/alpha levels per channel on exactly the edge pixels the
    // despill pass just cleaned. ARGB32 is also what Qt's WebP writer swizzles from
    // losslessly. PetAtlas::load premultiplies at load time, so the runtime format
    // contract is untouched.
    return image.format() == QImage::Format_ARGB32 ? image
                                                   : image.convertedTo(QImage::Format_ARGB32);
}

}  // namespace

namespace AtlasComposer {

const std::array<RowSpec, PetAtlas::Rows> &rows()
{
    // look-a and look-b rather than the reference's look-000-to-157.5 /
    // look-180-to-337.5: those are unusable as names a person types at a file.
    static const std::array<RowSpec, PetAtlas::Rows> table{{
        {0, QLatin1StringView("idle")},
        {1, QLatin1StringView("running-right")},
        {2, QLatin1StringView("running-left")},
        {3, QLatin1StringView("waving")},
        {4, QLatin1StringView("jumping")},
        {5, QLatin1StringView("failed")},
        {6, QLatin1StringView("waiting")},
        {7, QLatin1StringView("running")},
        {8, QLatin1StringView("review")},
        {9, QLatin1StringView("look-a")},
        {10, QLatin1StringView("look-b")},
    }};
    return table;
}

QColor defaultChromaKey()
{
    return QColor(0x00, 0xB1, 0x40);
}

QColor detectChromaKey(const QImage &strip)
{
    if (strip.isNull()) return defaultChromaKey();
    const QColor corner = toStraightArgb32(strip).pixelColor(0, 0);
    // A transparent corner means the strip was keyed already, so there is no key to
    // read and the default is the honest answer.
    return corner.alpha() == 0 ? defaultChromaKey() : corner;
}

bool isBlocking(Problem problem)
{
    switch (problem) {
    case Problem::MissingRow:
    case Problem::StripTooSmall:
    case Problem::EmptyFrame:
    case Problem::OccupancyFailed:
        return true;
    case Problem::UnexpectedStripAspect:
    case Problem::OutlierFrame:
    case Problem::FrameDoesNotFit:
        return false;
    }
    return true;
}

bool Result::hasBlockingIssue() const
{
    return std::any_of(issues.cbegin(), issues.cend(), [](const Issue &issue) {
        return isBlocking(issue.problem);
    });
}

bool Result::isInstallable() const
{
    return !atlas.isNull() && !hasBlockingIssue();
}

QImage keyToAlpha(const QImage &strip, const Options &options)
{
    QImage out = toStraightArgb32(strip);
    if (out.isNull() || options.keyTolerance <= 0) return out;
    out.detach();

    const QColor key = options.chromaKey;
    const Chroma keyChroma = chromaOf(key.red(), key.green(), key.blue());
    const double inner = options.keyTolerance;
    const double outer = inner + kKeySoftness;

    for (int y = 0; y < out.height(); ++y) {
        auto *line = reinterpret_cast<QRgb *>(out.scanLine(y));
        for (int x = 0; x < out.width(); ++x) {
            const QRgb pixel = line[x];
            const int alpha = qAlpha(pixel);
            if (alpha == 0) {
                // Zero the colour too. libwebp rewrites RGB inside fully transparent
                // blocks unless WebPConfig.exact is set, which Qt never sets, so a
                // lossless round trip only holds if transparent pixels are already
                // all-zero words.
                line[x] = 0u;
                continue;
            }
            const double distance =
                chromaDistance(chromaOf(qRed(pixel), qGreen(pixel), qBlue(pixel)), keyChroma);
            int keyed = 255;
            if (distance <= inner) {
                keyed = 0;
            } else if (distance < outer) {
                keyed = qRound(255.0 * (distance - inner) / (outer - inner));
            }
            // Alpha only ever decreases, so a strip the model already returned with
            // transparency keeps it, and a strip that matches nothing is a legitimate
            // no-op rather than something to report.
            const int result = qMin(alpha, keyed);
            line[x] = result == 0 ? 0u : qRgba(qRed(pixel), qGreen(pixel), qBlue(pixel), result);
        }
    }
    return out;
}

namespace {

// Recovers the subject's colour from a pixel the key bled into, in place.
//
// Two passes plus a floor, all at strip resolution and before any scaling, because
// the spill exists at source resolution and scaling smears it.
void despillStrip(QImage &strip, const QColor &key)
{
    if (strip.isNull()) return;
    strip.detach();
    const int keyR = key.red();
    const int keyG = key.green();
    const int keyB = key.blue();
    const int dominant = dominantChannel(key);

    // 1. Unmix by alpha. A blended pixel is C = a*O + (1-a)*K, so O = (C - (1-a)*K)/a.
    //    Exact for a linear blend, one pass, no neighbourhood. Done in sRGB bytes and
    //    not linear light on purpose: the model's rasteriser antialiased in sRGB, so
    //    linearising would undo a blend that never happened.
    for (int y = 0; y < strip.height(); ++y) {
        auto *line = reinterpret_cast<QRgb *>(strip.scanLine(y));
        for (int x = 0; x < strip.width(); ++x) {
            const QRgb pixel = line[x];
            const int alpha = qAlpha(pixel);
            if (alpha == 0 || alpha == 255) continue;
            if (alpha < kDespillMinAlpha) {
                line[x] = 0u;
                continue;
            }
            const double a = alpha / 255.0;
            const auto unmix = [a](int channel, int keyChannel) {
                const double value = (channel - (1.0 - a) * keyChannel) / a;
                return qBound(0, qRound(value), 255);
            };
            line[x] = qRgba(unmix(qRed(pixel), keyR),
                            unmix(qGreen(pixel), keyG),
                            unmix(qBlue(pixel), keyB),
                            alpha);
        }
    }

    // 2. Clamp the key's dominant channel on pixels in the edge band. Unmixing cannot
    //    help a *fully opaque* pixel that caught bounce light off the green card,
    //    because (1-a) is zero there. Restricted to pixels with a partially
    //    transparent neighbour so a genuinely green character is not desaturated all
    //    over, and the channel is derived from the key so a magenta or blue key works
    //    unchanged. Reads the alpha of the pass above, which never changes alpha
    //    except by the floor, so a copy of the alpha plane is not needed.
    const QImage before = strip;
    for (int y = 0; y < strip.height(); ++y) {
        auto *line = reinterpret_cast<QRgb *>(strip.scanLine(y));
        for (int x = 0; x < strip.width(); ++x) {
            const QRgb pixel = line[x];
            if (qAlpha(pixel) != 255) continue;
            bool nearEdge = false;
            for (int dy = -1; dy <= 1 && !nearEdge; ++dy) {
                const int ny = y + dy;
                if (ny < 0 || ny >= before.height()) continue;
                const auto *neighbours = reinterpret_cast<const QRgb *>(before.constScanLine(ny));
                for (int dx = -1; dx <= 1; ++dx) {
                    const int nx = x + dx;
                    if (nx < 0 || nx >= before.width()) continue;
                    if (qAlpha(neighbours[nx]) != 255) {
                        nearEdge = true;
                        break;
                    }
                }
            }
            if (!nearEdge) continue;

            int channels[3] = {qRed(pixel), qGreen(pixel), qBlue(pixel)};
            const int other = qMax(channels[(dominant + 1) % 3], channels[(dominant + 2) % 3]);
            const int ceiling = qMin(255, other + kSpillHeadroom);
            if (channels[dominant] <= ceiling) continue;
            channels[dominant] = ceiling;
            line[x] = qRgba(channels[0], channels[1], channels[2], 255);
        }
    }
}

QRect sliceRect(const QImage &strip, int index, int count)
{
    // Matches the skill's slicing: widths may differ by a pixel, which is harmless
    // because every slot is bounding-boxed independently afterwards.
    const int left = qRound(static_cast<double>(index) * strip.width() / count);
    const int right = qRound(static_cast<double>(index + 1) * strip.width() / count);
    return QRect(left, 0, right - left, strip.height());
}

}  // namespace

QRect alphaBounds(const QImage &image, const QRect &within)
{
    const QRect area = within.intersected(image.rect());
    if (area.isEmpty()) return {};

    int minX = area.right() + 1;
    int minY = area.bottom() + 1;
    int maxX = area.left() - 1;
    int maxY = area.top() - 1;
    for (int y = area.top(); y <= area.bottom(); ++y) {
        const auto *line = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        for (int x = area.left(); x <= area.right(); ++x) {
            if (qAlpha(line[x]) < kBoundsAlpha) continue;
            minX = qMin(minX, x);
            maxX = qMax(maxX, x);
            minY = qMin(minY, y);
            maxY = qMax(maxY, y);
        }
    }
    if (maxX < minX || maxY < minY) return {};
    return QRect(QPoint(minX, minY), QPoint(maxX, maxY));
}

namespace {

struct PreparedRow {
    QImage strip;                  // keyed and despilled, straight ARGB32
    QVector<QRect> frameBoxes;     // one per used column, in strip coordinates
    QRect rowBox;                  // union of the above
    // CellHeight / strip height: what this row's pixels must be multiplied by to
    // land in a common coordinate space before anything is compared across rows.
    //
    // Rows arrive from separate model calls, so they arrive at different
    // resolutions, and the character usually occupies a similar *fraction* of each
    // one. Fitting on raw pixel sizes would therefore preserve the resolution
    // difference as a size difference and the pet would change size between
    // animations -- which is the exact thing one common scale exists to prevent.
    double normalize = 1.0;
    bool usable = false;
};

}  // namespace

Result compose(const QVector<RowInput> &inputs, const Options &options)
{
    Result result;
    result.baselineY = PetAtlas::CellHeight - kCellMargin;

    std::array<PreparedRow, PetAtlas::Rows> prepared{};

    // Pass A: key, despill, and measure. Issues are appended row-then-column so the
    // report a user reads never depends on iteration incidentals.
    for (const RowSpec &spec : rows()) {
        const auto found = std::find_if(inputs.cbegin(), inputs.cend(), [&spec](const RowInput &in) {
            return in.row == spec.row;
        });
        if (found == inputs.cend() || found->strip.isNull()) {
            result.issues.append({Problem::MissingRow, spec.row, -1});
            continue;
        }

        const int count = frameCount(spec.row);
        const QImage &source = found->strip;
        if (source.width() < count || source.height() < 1) {
            result.issues.append({Problem::StripTooSmall, spec.row, -1});
            continue;
        }

        // Cheap shape check that catches the commonest model failure -- a strip with
        // the wrong frame count, or two rows stacked -- before anything downstream
        // silently slices it into nonsense.
        const double expected = count * static_cast<double>(PetAtlas::CellWidth) / PetAtlas::CellHeight;
        const double actual = static_cast<double>(source.width()) / source.height();
        if (qAbs(actual / expected - 1.0) > 0.25) {
            result.issues.append({Problem::UnexpectedStripAspect, spec.row, -1});
        }

        PreparedRow &row = prepared[static_cast<size_t>(spec.row)];
        row.strip = keyToAlpha(source, options);
        if (options.despill == DespillEdges::On) despillStrip(row.strip, options.chromaKey);
        row.normalize = static_cast<double>(PetAtlas::CellHeight) / row.strip.height();

        row.frameBoxes.resize(count);
        bool everyFrameFilled = true;
        for (int column = 0; column < count; ++column) {
            const QRect box = alphaBounds(row.strip, sliceRect(row.strip, column, count));
            if (box.isEmpty()) {
                // Blocking: this cell would fail occupancy with "Used cell row N
                // column M is empty", so the package could never install.
                result.issues.append({Problem::EmptyFrame, spec.row, column});
                everyFrameFilled = false;
                continue;
            }
            row.frameBoxes[column] = box;
            row.rowBox = row.rowBox.isNull() ? box : row.rowBox.united(box);
        }
        row.usable = everyFrameFilled;
    }

    // Deliberately no early return on a blocking issue. Composing whatever rows *are*
    // usable is the difference between a preview showing nine good rows and two gaps,
    // and a preview that is simply blank -- and blank is precisely when the user most
    // needs to see which of their strips landed. isInstallable() still says no.
    const bool blocked = result.hasBlockingIssue();

    // One scale and one ground line for the whole atlas. Per-frame scaling would make
    // the character change size between frames; per-frame vertical centring -- which
    // is what the skill's fit_to_cell does -- would make a jump bob in place.
    // Everything below is measured in normalized units -- source pixels times the
    // row's own normalize factor -- so a row authored at twice the resolution of
    // another contributes the same numbers.
    const int fitWidth = PetAtlas::CellWidth - 2 * kCellMargin;
    const int fitHeight = PetAtlas::CellHeight - 2 * kCellMargin;
    double widest = 1.0;
    double tallest = 1.0;
    QVector<double> frameHeights;
    for (const PreparedRow &row : prepared) {
        if (!row.usable) continue;
        tallest = qMax(tallest, row.rowBox.height() * row.normalize);
        for (const QRect &box : row.frameBoxes) {
            widest = qMax(widest, box.width() * row.normalize);
            frameHeights.append(box.height() * row.normalize);
        }
    }

    result.scale = qMin(fitWidth / widest, fitHeight / tallest);
    if (options.upscale == UpscaleFrames::Never) result.scale = qMin(result.scale, 1.0);

    // Report an outlier rather than absorbing it: the visible symptom is a tiny pet
    // in eighty-seven cells, and the user needs to know which frame caused it.
    if (!frameHeights.isEmpty()) {
        QVector<double> sorted = frameHeights;
        std::sort(sorted.begin(), sorted.end());
        const double limit = sorted.at(sorted.size() / 2) * kOutlierFactor;
        for (const RowSpec &spec : rows()) {
            const PreparedRow &row = prepared[static_cast<size_t>(spec.row)];
            if (!row.usable) continue;
            for (int column = 0; column < row.frameBoxes.size(); ++column) {
                if (row.frameBoxes.at(column).height() * row.normalize > limit) {
                    result.issues.append({Problem::OutlierFrame, spec.row, column});
                }
            }
        }
    }

    // Zero-filled, so every unused cell satisfies the transparent half of the
    // occupancy rule with no per-cell erase -- and satisfies libwebp's transparent
    // area cleanup at the same time.
    QImage atlas(PetAtlas::Width, PetAtlas::Height, QImage::Format_ARGB32);
    atlas.fill(Qt::transparent);

    for (const RowSpec &spec : rows()) {
        const PreparedRow &row = prepared[static_cast<size_t>(spec.row)];
        if (!row.usable) continue;
        const int rowGroundY = row.rowBox.bottom();
        // One scale in normalized space, applied to each row through its own
        // normalize factor. Equal for every row when the strips share a resolution,
        // which is the common case.
        const double rowScale = result.scale * row.normalize;

        for (int column = 0; column < row.frameBoxes.size(); ++column) {
            const QRect box = row.frameBoxes.at(column);
            const int targetW = qMax(1, qRound(rowScale * box.width()));
            const int targetH = qMax(1, qRound(rowScale * box.height()));

            // Qt's smooth scaler interpolates correctly only on premultiplied input,
            // so premultiply for the scale and come straight back. That excursion is
            // confined to already-despilled colours and never reaches the file.
            QImage frame = row.strip.copy(box)
                               .convertedTo(QImage::Format_ARGB32_Premultiplied)
                               .scaled(targetW, targetH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                               .convertedTo(QImage::Format_ARGB32);

            // Asymmetric on purpose. Horizontal uses each frame's own centre, which
            // removes drift so the pet never slides inside its window. Vertical uses
            // the row's shared ground line, which preserves the arc so a jumping
            // frame actually leaves the floor.
            const int cellLeft = column * PetAtlas::CellWidth;
            const int cellTop = spec.row * PetAtlas::CellHeight;
            int offsetX = (PetAtlas::CellWidth - targetW) / 2;
            int offsetY = result.baselineY - qRound(rowScale * (rowGroundY - box.top()));

            // rowGroundY - box.top() is at most rowBox.height(), which the scale was
            // chosen to fit, so this holds by construction. Kept as an assertion
            // against rounding rather than as an expected outcome.
            const int clampedX = qBound(0, offsetX, PetAtlas::CellWidth - targetW);
            const int clampedY = qBound(0, offsetY, PetAtlas::CellHeight - targetH);
            if (clampedX != offsetX || clampedY != offsetY) {
                result.issues.append({Problem::FrameDoesNotFit, spec.row, column});
                offsetX = clampedX;
                offsetY = clampedY;
            }

            // A plain scanline copy, not QPainter: the destination rect is disjoint
            // and already transparent, so this is a copy rather than a blend and
            // needs no premultiplied alpha. It also copies pixels without metadata,
            // which is what keeps the source strip's colour space out of the atlas --
            // QImage::copy and scaled() would both carry one along, and a tagged
            // atlas renders differently from the untagged built-in sprites.
            for (int y = 0; y < frame.height(); ++y) {
                const auto *src = reinterpret_cast<const QRgb *>(frame.constScanLine(y));
                auto *dst = reinterpret_cast<QRgb *>(atlas.scanLine(cellTop + offsetY + y));
                std::copy(src, src + frame.width(), dst + cellLeft + offsetX);
            }
        }
    }

    // The single check that decides whether the import will succeed, so pay for it
    // here rather than discovering it at install. About 2ms over 3.5M pixels.
    //
    // Skipped when something blocking was already reported: a partial atlas fails
    // occupancy by definition, and saying so on top of "idle: no strip chosen yet"
    // adds a line that explains nothing and reads like a second, deeper problem.
    if (!blocked && !PetAtlas::validateV2Occupancy(atlas)) {
        result.issues.append({Problem::OccupancyFailed, -1, -1});
    }
    result.atlas = atlas;
    return result;
}

}  // namespace AtlasComposer
