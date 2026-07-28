#include "pet/AtlasComposer.h"
#include "pet/PetAtlas.h"

#include <QColorSpace>
#include <QPainter>
#include <QTest>

#include <algorithm>

// A note on what these assert and what they deliberately do not.
//
// QImage::scaled()'s interior pixels are not guaranteed stable across Qt versions,
// and this project develops on 6.10 while flooring the API at 6.8. So every
// assertion below is about geometry, alpha coverage and bounding boxes -- never about
// an exact interior colour. Where a colour is checked it is at a pixel the pipeline
// either zeroes outright or leaves alone.
namespace {

using namespace AtlasComposer;

constexpr QRgb kKeyRgb = qRgb(0x00, 0xB1, 0x40);

// One strip of `count` cells, each holding a solid block of `subject` centred
// horizontally and sitting `bottomGap` above the strip's bottom edge. Cell aspect
// matches the contract so the shape check stays quiet.
QImage makeStrip(int count,
                 int blockW = 60,
                 int blockH = 90,
                 int bottomGap = 10,
                 QRgb subject = qRgb(200, 40, 40),
                 int cellH = 208)
{
    const int cellW = qRound(cellH * static_cast<double>(PetAtlas::CellWidth) / PetAtlas::CellHeight);
    QImage strip(cellW * count, cellH, QImage::Format_ARGB32);
    strip.fill(QColor::fromRgb(kKeyRgb));
    QPainter painter(&strip);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    for (int i = 0; i < count; ++i) {
        const int left = i * cellW + (cellW - blockW) / 2;
        painter.fillRect(left, cellH - bottomGap - blockH, blockW, blockH, QColor::fromRgb(subject));
    }
    painter.end();
    return strip;
}

QVector<RowInput> everyRow()
{
    QVector<RowInput> inputs;
    for (const RowSpec &spec : rows()) inputs.append({spec.row, makeStrip(frameCount(spec.row))});
    return inputs;
}

QRect cellRect(int row, int column)
{
    return QRect(column * PetAtlas::CellWidth,
                 row * PetAtlas::CellHeight,
                 PetAtlas::CellWidth,
                 PetAtlas::CellHeight);
}

bool hasIssue(const Result &result, Problem problem, int row = -1, int column = -1)
{
    return std::any_of(result.issues.cbegin(), result.issues.cend(), [&](const Issue &issue) {
        return issue.problem == problem && (row < 0 || issue.row == row)
            && (column < 0 || issue.column == column);
    });
}

int opaqueCount(const QImage &image)
{
    int count = 0;
    for (int y = 0; y < image.height(); ++y) {
        const auto *line = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(line[x]) != 0) ++count;
        }
    }
    return count;
}

}  // namespace

class AtlasComposerTest final : public QObject
{
    Q_OBJECT

private slots:
    // The row table is the composer's one new fact. Its frame counts must come from
    // the same function the validator enforces, or the composer can produce an atlas
    // that cannot install.
    void rowTableAgreesWithTheAtlasContract()
    {
        QCOMPARE(rows().size(), size_t(PetAtlas::Rows));

        QStringList names;
        int total = 0;
        for (const RowSpec &spec : rows()) {
            QCOMPARE(spec.row, static_cast<int>(&spec - rows().data()));
            QVERIFY(!spec.name.isEmpty());
            names << QString(spec.name);
            QCOMPARE(frameCount(spec.row), PetAtlas::usedColumns(spec.row));
            total += frameCount(spec.row);
        }
        // 6+8+8+4+5+8+6+6+6 animation frames plus two full look rows.
        QCOMPARE(total, 73);
        names.sort();
        QCOMPARE(std::adjacent_find(names.cbegin(), names.cend()), names.cend());
        QCOMPARE(QString(rows()[9].name), QStringLiteral("look-a"));
        QCOMPARE(QString(rows()[10].name), QStringLiteral("look-b"));
    }

    void composesTheExactAtlasGeometryUntaggedAndStraightAlpha()
    {
        const Result result = compose(everyRow(), {});
        QVERIFY2(result.isInstallable(), qPrintable(QString::number(result.issues.size())));
        QCOMPARE(result.atlas.width(), 1536);
        QCOMPARE(result.atlas.height(), 2288);
        QCOMPARE(result.atlas.format(), QImage::Format_ARGB32);
        // A tagged atlas renders differently from the untagged built-in sprites, so
        // the composer must not acquire a colour space from its inputs.
        QVERIFY(!result.atlas.colorSpace().isValid());
    }

    // The check that decides whether the package installs at all, run against the
    // in-memory image so no file is needed.
    void satisfiesTheTwoSidedOccupancyRule()
    {
        const Result result = compose(everyRow(), {});
        QString error;
        QVERIFY2(PetAtlas::validateV2Occupancy(result.atlas, &error), qPrintable(error));
        QVERIFY(!hasIssue(result, Problem::OccupancyFailed));

        // Both halves, stated explicitly: used cells hold pixels, unused ones do not.
        for (const RowSpec &spec : rows()) {
            for (int column = 0; column < PetAtlas::Columns; ++column) {
                const bool used = column < frameCount(spec.row);
                const int pixels = opaqueCount(result.atlas.copy(cellRect(spec.row, column)));
                if (used) {
                    QVERIFY2(pixels > 0,
                             qPrintable(QStringLiteral("row %1 column %2 is empty")
                                            .arg(spec.row).arg(column)));
                } else {
                    QVERIFY2(pixels == 0,
                             qPrintable(QStringLiteral("row %1 column %2 is not transparent")
                                            .arg(spec.row).arg(column)));
                }
            }
        }
    }

    // Transparent pixels must be all-zero words, not merely alpha 0. Qt never sets
    // WebPConfig.exact, so libwebp's WebPCleanupTransparentArea rewrites RGB inside
    // fully transparent blocks -- a no-op only if that RGB is already zero. Without
    // this, the writer's lossless round trip is not reproducible.
    void transparentPixelsAreFullyZeroedNotJustAlphaZero()
    {
        const Result result = compose(everyRow(), {});
        int transparent = 0;
        for (int y = 0; y < result.atlas.height(); ++y) {
            const auto *line = reinterpret_cast<const QRgb *>(result.atlas.constScanLine(y));
            for (int x = 0; x < result.atlas.width(); ++x) {
                if (qAlpha(line[x]) != 0) continue;
                ++transparent;
                QVERIFY2(line[x] == 0u,
                         qPrintable(QStringLiteral("transparent pixel at %1,%2 keeps RGB %3")
                                        .arg(x).arg(y).arg(line[x], 8, 16, QLatin1Char('0'))));
            }
        }
        QVERIFY(transparent > 0);
    }

    // Nothing may touch a cell edge, or one frame's antialiased fringe bleeds into
    // its neighbour when the atlas is sampled.
    void everyFrameKeepsAMarginInsideItsCell()
    {
        const Result result = compose(everyRow(), {});
        for (const RowSpec &spec : rows()) {
            for (int column = 0; column < frameCount(spec.row); ++column) {
                const QRect cell = cellRect(spec.row, column);
                const QRect box = alphaBounds(result.atlas, cell);
                QVERIFY(!box.isEmpty());
                QVERIFY(box.left() > cell.left());
                QVERIFY(box.right() < cell.right());
                QVERIFY(box.top() > cell.top());
                QVERIFY(box.bottom() < cell.bottom());
            }
        }
    }

    void chromaKeyRemovesTheBackgroundAndKeepsTheSubject()
    {
        const QImage keyed = keyToAlpha(makeStrip(4), {});
        QCOMPARE(keyed.format(), QImage::Format_ARGB32);
        // Corner is background, centre-bottom is subject.
        QCOMPARE(keyed.pixel(0, 0), 0u);
        const QRect box = alphaBounds(keyed, keyed.rect());
        QVERIFY(!box.isEmpty());
        QVERIFY(box.width() < keyed.width());
        QVERIFY(box.height() < keyed.height());
    }

    void wideningToleranceKeysStrictlyMore()
    {
        const QImage strip = makeStrip(4);
        int previous = std::numeric_limits<int>::max();
        for (int tolerance : {0, 20, 60, 96, 140}) {
            Options options;
            options.keyTolerance = tolerance;
            const int opaque = opaqueCount(keyToAlpha(strip, options));
            QVERIFY2(opaque <= previous, qPrintable(QStringLiteral("tolerance %1").arg(tolerance)));
            previous = opaque;
        }
        // Tolerance 0 keys nothing at all, so the strip is untouched.
        Options none;
        none.keyTolerance = 0;
        QCOMPARE(opaqueCount(keyToAlpha(strip, none)), strip.width() * strip.height());
    }

    // The test that pins chrominance distance over Euclidean RGB.
    //
    // A real chroma card is not evenly lit, so the background arrives as a range of
    // greens from the key colour down into shadow. Euclidean RGB distance is
    // dominated by the luminance axis, so a shadowed green measures *further* from
    // the key than a bright one and survives the key as a green fringe: at the
    // shipped tolerance of 96, #003614 sits about 130 away in RGB and is kept, while
    // its chroma distance is about 84 and it goes. Comparing (Cb, Cr) positions
    // instead makes one tolerance number behave the same at every brightness.
    //
    // The flip side is inherent to chroma keying rather than to this metric: a
    // subject genuinely the same hue as the key cannot be told from it, which is why
    // the prompt templates specify the key and not the character.
    void keyingIsChrominanceBasedSoAnUnevenlyLitKeyStillGoes()
    {
        const int cellH = 208;
        const int cellW = qRound(cellH * static_cast<double>(PetAtlas::CellWidth) / PetAtlas::CellHeight);
        QImage strip(cellW * 4, cellH, QImage::Format_ARGB32);

        // Background graded from the key colour at the top into deep shadow at the
        // bottom, as an unevenly lit card actually photographs.
        for (int y = 0; y < cellH; ++y) {
            const double t = y / double(cellH - 1);
            const QRgb shade = qRgb(0, qRound(0xB1 * (1.0 - 0.7 * t)), qRound(0x40 * (1.0 - 0.7 * t)));
            auto *line = reinterpret_cast<QRgb *>(strip.scanLine(y));
            std::fill(line, line + strip.width(), shade);
        }
        QPainter painter(&strip);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        for (int i = 0; i < 4; ++i) {
            painter.fillRect(i * cellW + (cellW - 60) / 2, cellH - 100, 60, 90, QColor(200, 40, 40));
        }
        painter.end();

        const QImage keyed = keyToAlpha(strip, {});

        // The whole graded background goes, top to bottom.
        QCOMPARE(keyed.pixel(0, 0), 0u);
        QCOMPARE(keyed.pixel(0, cellH - 1), 0u);
        QCOMPARE(keyed.pixel(2, cellH / 2), 0u);

        // ...and the subject is untouched, at full height in every frame.
        const QRect box = alphaBounds(keyed, keyed.rect());
        QCOMPARE(box.height(), 90);
        QCOMPARE(box.width(), 3 * cellW + 60);
    }

    void antialiasedEdgesGetPartialAlphaRatherThanAHardCutout()
    {
        // A single row blending subject into key across its width.
        QImage ramp(256, 8, QImage::Format_ARGB32);
        for (int y = 0; y < ramp.height(); ++y) {
            auto *line = reinterpret_cast<QRgb *>(ramp.scanLine(y));
            for (int x = 0; x < ramp.width(); ++x) {
                const double t = x / 255.0;
                line[x] = qRgb(qRound(0x00 * (1 - t) + 200 * t),
                               qRound(0xB1 * (1 - t) + 40 * t),
                               qRound(0x40 * (1 - t) + 40 * t));
            }
        }
        const QImage keyed = keyToAlpha(ramp, {});

        int partial = 0;
        for (int x = 0; x < keyed.width(); ++x) {
            const int alpha = qAlpha(keyed.pixel(x, 0));
            if (alpha > 0 && alpha < 255) ++partial;
        }
        QVERIFY2(partial > 0, "the key produced a hard cutout with no soft edge");
        QCOMPARE(qAlpha(keyed.pixel(0, 0)), 0);
        QCOMPARE(qAlpha(keyed.pixel(255, 0)), 255);
    }

    // A strip the model already returned with transparency must keep it.
    void keyingOnlyEverReducesAlpha()
    {
        QImage strip = makeStrip(4);
        for (int x = 0; x < 20; ++x) strip.setPixel(x, 0, 0u);
        strip.setPixelColor(21, 0, QColor(200, 40, 40, 80));

        const QImage keyed = keyToAlpha(strip, {});
        for (int x = 0; x < 20; ++x) QCOMPARE(keyed.pixel(x, 0), 0u);
        QCOMPARE(qAlpha(keyed.pixel(21, 0)), 80);
    }

    void despillPullsTheKeyHueOutOfEdgePixelsWithoutReshapingTheSubject()
    {
        // A subject with a genuinely blended border, which is what the key leaves
        // behind and what despill exists to clean. Drawn antialiased so the edge
        // really does carry a mix of subject and key rather than a hard step.
        const int count = frameCount(0);
        const int cellH = 208;
        const int cellW = qRound(cellH * static_cast<double>(PetAtlas::CellWidth) / PetAtlas::CellHeight);
        QImage strip(cellW * count, cellH, QImage::Format_ARGB32);
        strip.fill(QColor::fromRgb(kKeyRgb));
        QPainter painter(&strip);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setBrush(QColor(200, 40, 40));
        painter.setPen(Qt::NoPen);
        for (int i = 0; i < count; ++i) {
            painter.drawEllipse(QRectF(i * cellW + (cellW - 60) / 2.0, cellH - 100, 60, 90));
        }
        painter.end();

        QVector<RowInput> inputs = everyRow();
        for (RowInput &input : inputs) {
            if (input.row == 0) input.strip = strip;
        }

        Options off;
        off.despill = DespillEdges::Off;
        Options on;
        on.despill = DespillEdges::On;
        const Result withoutDespill = compose(inputs, off);
        const Result withDespill = compose(inputs, on);
        QVERIFY(withoutDespill.isInstallable());
        QVERIFY(withDespill.isInstallable());

        // Mean green over the frame, not individual pixels: both runs go through the
        // same scaler, so the difference between them is the despill and nothing else,
        // while an exact interior value would be a hostage to the Qt version.
        const auto meanGreen = [](const QImage &atlas) {
            const QImage cell = atlas.copy(cellRect(0, 0));
            qint64 total = 0;
            int counted = 0;
            for (int y = 0; y < cell.height(); ++y) {
                const auto *line = reinterpret_cast<const QRgb *>(cell.constScanLine(y));
                for (int x = 0; x < cell.width(); ++x) {
                    if (qAlpha(line[x]) == 0) continue;
                    total += qGreen(line[x]);
                    ++counted;
                }
            }
            return counted > 0 ? double(total) / counted : 0.0;
        };
        QVERIFY2(meanGreen(withDespill.atlas) < meanGreen(withoutDespill.atlas),
                 qPrintable(QStringLiteral("despill left the key hue in place: %1 vs %2")
                                .arg(meanGreen(withDespill.atlas))
                                .arg(meanGreen(withoutDespill.atlas))));

        // Despill moves colour, not shape. The alpha floor may drop a few near-
        // invisible edge pixels, so allow a little, but nothing like an outline.
        const int on_ = opaqueCount(withDespill.atlas);
        const int off_ = opaqueCount(withoutDespill.atlas);
        QVERIFY2(on_ <= off_ && on_ > 0.97 * off_,
                 qPrintable(QStringLiteral("alpha coverage moved from %1 to %2").arg(off_).arg(on_)));
    }

    // Rows arrive from separate model calls and therefore at different resolutions,
    // with the character occupying a similar fraction of each. Fitting on raw pixel
    // sizes would carry that resolution difference through as a size difference, and
    // the pet would change size between animations -- so each row is normalized to
    // the cell before one common scale is chosen. This is the test that pins it: the
    // odd rows below are authored at exactly twice the scale of the even ones.
    void differentSourceResolutionsStillGiveTheSameSizeInEveryCell()
    {
        QVector<RowInput> inputs;
        for (const RowSpec &spec : rows()) {
            const int cellH = spec.row % 2 == 0 ? 208 : 416;
            const double k = cellH / 208.0;
            inputs.append({spec.row,
                           makeStrip(frameCount(spec.row), qRound(60 * k), qRound(90 * k),
                                     qRound(10 * k), qRgb(200, 40, 40), cellH)});
        }

        const Result result = compose(inputs, {});
        QVERIFY(result.isInstallable());

        QVector<int> widths;
        QVector<int> heights;
        for (const RowSpec &spec : rows()) {
            for (int column = 0; column < frameCount(spec.row); ++column) {
                const QRect box = alphaBounds(result.atlas, cellRect(spec.row, column));
                widths.append(box.width());
                heights.append(box.height());
            }
        }
        QCOMPARE(widths.size(), 73);
        const auto [wLo, wHi] = std::minmax_element(widths.cbegin(), widths.cend());
        const auto [hLo, hHi] = std::minmax_element(heights.cbegin(), heights.cend());
        // Within a pixel: the two source resolutions round differently under the
        // scaler, but neither may end up a different size.
        QVERIFY2(*wHi - *wLo <= 1,
                 qPrintable(QStringLiteral("frame widths span %1..%2").arg(*wLo).arg(*wHi)));
        QVERIFY2(*hHi - *hLo <= 1,
                 qPrintable(QStringLiteral("frame heights span %1..%2").arg(*hLo).arg(*hHi)));
    }

    // And the same guarantee in the ordinary case, where every row shares a
    // resolution but the character is drawn at different sizes within them.
    void oneCommonScaleIsAppliedRatherThanOnePerRow()
    {
        QVector<RowInput> inputs;
        for (const RowSpec &spec : rows()) {
            // Row 4's character is drawn smaller at the same resolution. That is a
            // genuine difference in the artwork, so it must survive: normalizing it
            // away would be the composer inventing a size the author did not draw.
            const int blockH = spec.row == 4 ? 45 : 90;
            inputs.append({spec.row, makeStrip(frameCount(spec.row), 60, blockH)});
        }

        const Result result = compose(inputs, {});
        QVERIFY(result.isInstallable());
        QCOMPARE(alphaBounds(result.atlas, cellRect(4, 0)).height() * 2,
                 alphaBounds(result.atlas, cellRect(0, 0)).height());
    }

    void theSharedGroundLineLetsAJumpLeaveTheFloor()
    {
        // One row where a single frame is lifted 20px off the ground.
        const int count = frameCount(4);
        QImage strip = makeStrip(count);
        const int cellW = strip.width() / count;
        const int lift = 20;
        QPainter painter(&strip);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        const int left = 2 * cellW + (cellW - 60) / 2;
        painter.fillRect(2 * cellW, 0, cellW, strip.height(), QColor::fromRgb(kKeyRgb));
        painter.fillRect(left, strip.height() - 10 - 90 - lift, 60, 90, QColor(200, 40, 40));
        painter.end();

        QVector<RowInput> inputs = everyRow();
        for (RowInput &input : inputs) {
            if (input.row == 4) input.strip = strip;
        }
        const Result result = compose(inputs, {});
        QVERIFY(result.isInstallable());

        QVector<int> bottoms;
        for (int column = 0; column < count; ++column) {
            const QRect box = alphaBounds(result.atlas, cellRect(4, column));
            bottoms.append(box.bottom() - cellRect(4, column).top());
        }
        // Every grounded frame shares a baseline...
        for (int column : {0, 1, 3, 4}) {
            QCOMPARE(bottoms.at(column), bottoms.at(0));
        }
        // ...and the lifted one is genuinely above it, by the scaled lift.
        const int expected = bottoms.at(0) - qRound(result.scale * lift);
        QVERIFY2(qAbs(bottoms.at(2) - expected) <= 1,
                 qPrintable(QStringLiteral("lifted frame bottom %1, expected about %2")
                                .arg(bottoms.at(2)).arg(expected)));
    }

    void horizontalDriftIsRemovedSoThePetDoesNotSlideInItsWindow()
    {
        // A row whose subject wanders left to right between frames.
        const int count = frameCount(1);
        const int cellH = 208;
        const int cellW = qRound(cellH * static_cast<double>(PetAtlas::CellWidth) / PetAtlas::CellHeight);
        QImage strip(cellW * count, cellH, QImage::Format_ARGB32);
        strip.fill(QColor::fromRgb(kKeyRgb));
        QPainter painter(&strip);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        for (int i = 0; i < count; ++i) {
            painter.fillRect(i * cellW + 10 + i * 8, cellH - 100, 60, 90, QColor(200, 40, 40));
        }
        painter.end();

        QVector<RowInput> inputs = everyRow();
        for (RowInput &input : inputs) {
            if (input.row == 1) input.strip = strip;
        }
        const Result result = compose(inputs, {});
        QVERIFY(result.isInstallable());

        for (int column = 0; column < count; ++column) {
            const QRect cell = cellRect(1, column);
            const QRect box = alphaBounds(result.atlas, cell);
            const int centre = box.center().x() - cell.left();
            QVERIFY2(qAbs(centre - PetAtlas::CellWidth / 2) <= 1,
                     qPrintable(QStringLiteral("frame %1 centred at %2").arg(column).arg(centre)));
        }
    }

    void anOversizedFrameShrinksEverythingAndIsReported()
    {
        QVector<RowInput> inputs = everyRow();
        for (RowInput &input : inputs) {
            if (input.row != 5) continue;
            const int count = frameCount(5);
            QImage strip = makeStrip(count);
            const int cellW = strip.width() / count;
            QPainter painter(&strip);
            painter.setCompositionMode(QPainter::CompositionMode_Source);
            painter.fillRect(3 * cellW, 0, cellW, strip.height(), QColor::fromRgb(kKeyRgb));
            painter.fillRect(3 * cellW + 20, 4, 100, 200, QColor(200, 40, 40));
            painter.end();
            input.strip = strip;
        }

        const Result result = compose(inputs, {});
        // Advisory, not blocking: it still installs, just small.
        QVERIFY(result.isInstallable());
        QVERIFY2(hasIssue(result, Problem::OutlierFrame, 5, 3), "the oversized frame was not named");
        QVERIFY(result.scale < 1.0);
        // Nothing is clipped to make room for it.
        QVERIFY(!hasIssue(result, Problem::FrameDoesNotFit));
    }

    void aMissingRowIsBlockingAndNamed()
    {
        QVector<RowInput> inputs = everyRow();
        inputs.removeIf([](const RowInput &input) { return input.row == 7; });

        const Result result = compose(inputs, {});
        QVERIFY(!result.isInstallable());
        QVERIFY(hasIssue(result, Problem::MissingRow, 7));
        QVERIFY(!hasIssue(result, Problem::MissingRow, 6));
    }

    // An all-background frame would fail occupancy with "Used cell ... is empty", so
    // it has to be caught before anything tries to install it.
    void anEntirelyBackgroundFrameIsReportedAsEmpty()
    {
        QVector<RowInput> inputs = everyRow();
        for (RowInput &input : inputs) {
            if (input.row != 3) continue;
            const int cellW = input.strip.width() / frameCount(3);
            QPainter painter(&input.strip);
            painter.setCompositionMode(QPainter::CompositionMode_Source);
            painter.fillRect(cellW, 0, cellW, input.strip.height(), QColor::fromRgb(kKeyRgb));
            painter.end();
        }

        const Result result = compose(inputs, {});
        QVERIFY(!result.isInstallable());
        QVERIFY(hasIssue(result, Problem::EmptyFrame, 3, 1));
    }

    void degenerateStripsAreReportedRatherThanCrashing()
    {
        // Fully keyed: every frame is empty, nothing divides by zero.
        QVector<RowInput> allKey;
        for (const RowSpec &spec : rows()) {
            QImage strip = makeStrip(frameCount(spec.row));
            strip.fill(QColor::fromRgb(kKeyRgb));
            allKey.append({spec.row, strip});
        }
        const Result keyed = compose(allKey, {});
        QVERIFY(!keyed.isInstallable());
        QVERIFY(hasIssue(keyed, Problem::EmptyFrame));

        // Null and narrower-than-its-frame-count strips.
        QVector<RowInput> broken;
        for (const RowSpec &spec : rows()) {
            broken.append({spec.row, spec.row == 0 ? QImage() : QImage(3, 3, QImage::Format_ARGB32)});
        }
        for (RowInput &input : broken) {
            if (!input.strip.isNull()) input.strip.fill(QColor::fromRgb(kKeyRgb));
        }
        const Result small = compose(broken, {});
        QVERIFY(!small.isInstallable());
        QVERIFY(hasIssue(small, Problem::MissingRow, 0));
        QVERIFY(hasIssue(small, Problem::StripTooSmall, 1));

        // An unexpected aspect is advisory, and names the row.
        QVector<RowInput> squashed = everyRow();
        for (RowInput &input : squashed) {
            if (input.row == 8) input.strip = input.strip.scaled(input.strip.width() / 3, input.strip.height());
        }
        const Result aspect = compose(squashed, {});
        QVERIFY(hasIssue(aspect, Problem::UnexpectedStripAspect, 8));
    }

    void issuesComeBackInRowThenColumnOrder()
    {
        QVector<RowInput> inputs = everyRow();
        inputs.removeIf([](const RowInput &input) { return input.row == 9 || input.row == 2; });
        const Result result = compose(inputs, {});

        QVector<QPair<int, int>> seen;
        for (const Issue &issue : result.issues) seen.append({issue.row, issue.column});
        QVector<QPair<int, int>> sorted = seen;
        std::stable_sort(sorted.begin(), sorted.end());
        QCOMPARE(seen, sorted);
    }

    void composeIsPureAndRepeatable()
    {
        const QVector<RowInput> inputs = everyRow();
        QVector<QByteArray> before;
        for (const RowInput &input : inputs) {
            before.append(QByteArray(reinterpret_cast<const char *>(input.strip.constBits()),
                                     static_cast<int>(input.strip.sizeInBytes())));
        }

        const Result first = compose(inputs, {});
        const Result second = compose(inputs, {});
        QVERIFY(first.isInstallable());
        QCOMPARE(first.atlas, second.atlas);
        QCOMPARE(first.scale, second.scale);
        QCOMPARE(first.issues.size(), second.issues.size());

        // The inputs are const refs; prove nothing wrote through them.
        for (int i = 0; i < inputs.size(); ++i) {
            const QByteArray after(reinterpret_cast<const char *>(inputs.at(i).strip.constBits()),
                                   static_cast<int>(inputs.at(i).strip.sizeInBytes()));
            QCOMPARE(after, before.at(i));
        }
    }

    void detectChromaKeyReadsTheCornerAndFallsBackWhenThereIsNone()
    {
        QCOMPARE(detectChromaKey(makeStrip(4)).rgb(), kKeyRgb);
        QCOMPARE(defaultChromaKey().name(QColor::HexRgb).toUpper(), QStringLiteral("#00B140"));

        // Already keyed: no background to read, so the default is the honest answer.
        QImage keyed = makeStrip(4);
        keyed.setPixel(0, 0, 0u);
        QCOMPARE(detectChromaKey(keyed), defaultChromaKey());
        QCOMPARE(detectChromaKey(QImage()), defaultChromaKey());
    }
};

// Runs inside a grouped binary; see tests/support/TestRunner.h.
QObject *createAtlasComposerTest() { return new AtlasComposerTest; }
#include "test_atlas_composer.moc"
