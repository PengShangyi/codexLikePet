#include "pet/PetAtlas.h"
#include "pet/PetWindow.h"
#include "settings/AppSettings.h"

#include <QImage>
#include <QTemporaryDir>
#include <QTest>

// Covers PetWindow's painting, which nothing did before: test_pet_overlay only
// asserts native NSWindow state and skips entirely when headless.
//
// The paint path caches the frame pre-scaled to the window, so it now has state
// that can go stale. These tests exist mostly to catch that.
class PetWindowRenderTest final : public QObject
{
    Q_OBJECT

    static QImage renderPet(PetWindow &pet)
    {
        QImage canvas(pet.size(), QImage::Format_ARGB32_Premultiplied);
        canvas.fill(Qt::transparent);
        pet.render(&canvas);
        return canvas;
    }

    // A frame is an atlas cell, so it is sized in source pixels. This used to read
    // PetWindow::CellWidth, which worked only because the window's logical size
    // happened to equal the cell; taking the size from PetAtlas keeps these tests
    // feeding real frame geometry now that the two differ.
    static QImage solidFrame(const QColor &color)
    {
        QImage frame(PetAtlas::CellWidth, PetAtlas::CellHeight,
                     QImage::Format_ARGB32_Premultiplied);
        frame.fill(color);
        return frame;
    }

    static QImage solidFrameOfSize(const QSize &size, const QColor &color)
    {
        QImage frame(size, QImage::Format_ARGB32_Premultiplied);
        frame.fill(color);
        return frame;
    }

private slots:
    void paintsTheFrameAcrossTheWholeWindow()
    {
        QTemporaryDir dir;
        AppSettings settings(dir.filePath(QStringLiteral("settings.ini")));
        PetWindow pet(&settings);
        pet.setFrame(solidFrame(QColor(200, 100, 50)));

        const QImage painted = renderPet(pet);
        QCOMPARE(painted.size(), pet.size());
        QCOMPARE(painted.pixelColor(1, 1), QColor(200, 100, 50));
        QCOMPARE(painted.pixelColor(pet.width() - 2, pet.height() - 2), QColor(200, 100, 50));
    }

    // The regression the scaled-frame cache invites: a new frame that happens to
    // be the same size as the last one. Keying the cache on size alone would keep
    // painting the old frame forever, and the pet would freeze on one image while
    // every other signal said it was animating.
    void aNewFrameOfTheSameSizeStillRepaints()
    {
        QTemporaryDir dir;
        AppSettings settings(dir.filePath(QStringLiteral("settings.ini")));
        PetWindow pet(&settings);

        pet.setFrame(solidFrame(QColor(10, 20, 30)));
        QCOMPARE(renderPet(pet).pixelColor(1, 1), QColor(10, 20, 30));

        pet.setFrame(solidFrame(QColor(40, 50, 60)));
        QCOMPARE(renderPet(pet).pixelColor(1, 1), QColor(40, 50, 60));
    }

    // Same trap on the other input: the filter changes but the geometry does not.
    void changingTheRenderingFilterRepaints()
    {
        QTemporaryDir dir;
        AppSettings settings(dir.filePath(QStringLiteral("settings.ini")));
        PetWindow pet(&settings);
        pet.setFrame(solidFrame(QColor(70, 80, 90)));
        QCOMPARE(renderPet(pet).pixelColor(1, 1), QColor(70, 80, 90));

        pet.setSmoothRendering(false);
        QCOMPARE(renderPet(pet).pixelColor(1, 1), QColor(70, 80, 90));
    }

    void resizingRescalesTheFrame()
    {
        QTemporaryDir dir;
        AppSettings settings(dir.filePath(QStringLiteral("settings.ini")));
        PetWindow pet(&settings);
        pet.setFrame(solidFrame(QColor(120, 130, 140)));
        const QSize before = pet.size();

        settings.setScale(2.0);
        QVERIFY(pet.size() != before);

        const QImage painted = renderPet(pet);
        QCOMPARE(painted.size(), pet.size());
        // Every corner covered, so the cached pixmap really did grow rather than
        // being blitted at its old size into a bigger window.
        QCOMPARE(painted.pixelColor(1, 1), QColor(120, 130, 140));
        QCOMPARE(painted.pixelColor(pet.width() - 2, pet.height() - 2), QColor(120, 130, 140));
    }

    // The invariant the whole no-resample path rests on. Stated as arithmetic
    // because the offscreen platform these tests run under reports
    // devicePixelRatio 1, so the real Retina geometry cannot be observed here --
    // but if either constant moves, the pet starts being resampled every frame
    // again and only a profiler would say so.
    void theBaseSizeIsOneAtlasCellAtRetinaScale()
    {
        QCOMPARE(PetWindow::BaseWidth * 2, PetAtlas::CellWidth);
        QCOMPARE(PetWindow::BaseHeight * 2, PetAtlas::CellHeight);
    }

    // Exercises both branches of ensureScaledFrame() at whatever ratio the platform
    // gives us: a frame already the size of the backing store must be presented
    // as-is, and one that is not must come out at the backing store's size anyway.
    void aFrameMatchingTheBackingStoreIsPresentedUnscaled()
    {
        QTemporaryDir dir;
        AppSettings settings(dir.filePath(QStringLiteral("settings.ini")));
        PetWindow pet(&settings);
        const QSize deviceSize = pet.size() * pet.devicePixelRatioF();

        pet.setFrame(solidFrameOfSize(deviceSize, QColor(11, 22, 33)));
        QCOMPARE(renderPet(pet).pixelColor(1, 1), QColor(11, 22, 33));
        QCOMPARE(pet.scaledFrameSize(), deviceSize);

        // A cell-sized frame is the other branch unless the ratio happens to make
        // them equal; either way the presented pixmap tracks the backing store.
        pet.setFrame(solidFrame(QColor(44, 55, 66)));
        QCOMPARE(renderPet(pet).pixelColor(1, 1), QColor(44, 55, 66));
        QCOMPARE(pet.scaledFrameSize(), deviceSize);
    }

    // With no validated frame there is still a pet to look at: the drawn fallback.
    void withoutAFrameTheFallbackIsStillDrawn()
    {
        QTemporaryDir dir;
        AppSettings settings(dir.filePath(QStringLiteral("settings.ini")));
        PetWindow pet(&settings);

        const QImage painted = renderPet(pet);
        bool anyOpaquePixel = false;
        for (int y = 0; y < painted.height() && !anyOpaquePixel; ++y) {
            for (int x = 0; x < painted.width(); ++x) {
                if (painted.pixelColor(x, y).alpha() != 0) {
                    anyOpaquePixel = true;
                    break;
                }
            }
        }
        QVERIFY(anyOpaquePixel);
    }
};

// Runs inside a grouped binary; see tests/support/TestRunner.h.
QObject *createPetWindowRenderTest() { return new PetWindowRenderTest; }

#include "test_pet_window.moc"
