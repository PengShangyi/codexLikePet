#include "pet/AtlasCache.h"
#include "pet/AnimationClip.h"
#include "pet/AnimationPlayer.h"
#include "pet/ClipCache.h"
#include "pet/PetAtlas.h"
#include "pet/TypingAnimationDriver.h"
#include "support/AtlasFixture.h"

#include <QTemporaryDir>
#include <QSignalSpy>
#include <QTest>

class AtlasTest final : public QObject
{
    Q_OBJECT

private:
    QString createAtlas(const QString &name, const QColor &color)
    {
        const QString path = m_temp.filePath(name + QStringLiteral(".png"));
        return TestAtlas::writeFilled(path, color) ? path : QString();
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_temp.isValid());
    }

    void exposesTheV2Contract()
    {
        const AnimationSpec idle = PetAtlas::animationSpec(V2AnimationState::Idle);
        QCOMPARE(idle.row, 0);
        QCOMPARE(idle.frameCount, 6);
        QCOMPARE(idle.durationsMs, QVector<int>({280, 110, 110, 140, 140, 320}));

        const AnimationSpec running = PetAtlas::animationSpec(V2AnimationState::RunningRight);
        QCOMPARE(running.frameCount, 8);
        QCOMPARE(running.durationsMs.last(), 220);
    }

    void loadsAndExtractsFrames()
    {
        PetAtlas atlas;
        QVERIFY(atlas.load(createAtlas(QStringLiteral("valid"), QColor(12, 34, 56, 200))));
        QCOMPARE(atlas.frame(V2AnimationState::Idle, 0).size(), QSize(192, 208));
        QCOMPARE(atlas.lookFrame(15).size(), QSize(192, 208));
        QVERIFY(atlas.frame(V2AnimationState::Idle, 6).isNull());
    }

    void rejectsWrongDimensions()
    {
        const QString path = m_temp.filePath(QStringLiteral("wrong.png"));
        QImage image(100, 100, QImage::Format_RGBA8888);
        image.fill(Qt::transparent);
        QVERIFY(image.save(path));
        PetAtlas atlas;
        QVERIFY(!atlas.load(path));
        QVERIFY(atlas.errorString().contains(QStringLiteral("1536x2288")));
    }

    void evictsTheLeastRecentlyUsedAtlas()
    {
        const QString first = createAtlas(QStringLiteral("first"), Qt::red);
        const QString second = createAtlas(QStringLiteral("second"), Qt::green);
        const QString third = createAtlas(QStringLiteral("third"), Qt::blue);
        AtlasCache cache(2);
        QVERIFY(cache.load(first));
        QVERIFY(cache.load(second));
        QVERIFY(cache.load(first));
        QVERIFY(cache.load(third));
        QVERIFY(cache.contains(first));
        QVERIFY(!cache.contains(second));
        QVERIFY(cache.contains(third));
    }

    void clipCacheEvictsLeastRecentlyUsedAndKeysOnDurations()
    {
        const auto strip = [this](const QString &name, int frames) {
            const QString path = m_temp.filePath(name + QStringLiteral(".png"));
            return TestAtlas::writeClip(path, frames, QColor(10, 20, 30, 200)) ? path : QString();
        };
        const QString first = strip(QStringLiteral("cc-first"), 2);
        const QString second = strip(QStringLiteral("cc-second"), 2);
        const QString third = strip(QStringLiteral("cc-third"), 2);
        QVERIFY(!first.isEmpty() && !second.isEmpty() && !third.isEmpty());
        const QVector<int> durations{80, 80};

        ClipCache cache(2);
        QVERIFY(cache.load(first, durations));
        QVERIFY(cache.load(second, durations));
        QCOMPARE(cache.size(), 2);
        QVERIFY(cache.load(first, durations));   // refresh first's recency
        QVERIFY(cache.load(third, durations));   // evicts second, not first
        QVERIFY(cache.contains(first, durations));
        QVERIFY(!cache.contains(second, durations));
        QVERIFY(cache.contains(third, durations));

        // Same strip, different declared timings -> a distinct entry, because the
        // decoded clips are not interchangeable.
        ClipCache keyed(4);
        QVERIFY(keyed.load(first, {80, 80}));
        QVERIFY(keyed.load(first, {120, 120}));
        QCOMPARE(keyed.size(), 2);
        QVERIFY(keyed.contains(first, {80, 80}));
        QVERIFY(keyed.contains(first, {120, 120}));
        QVERIFY(!keyed.contains(first, {200, 200}));

        // A clip that fails to load reports why and is not cached.
        QString error;
        QVERIFY(!keyed.load(first, {10, 10}, &error)); // below the 50ms floor
        QVERIFY(!error.isEmpty());
        QCOMPARE(keyed.size(), 2);

        keyed.clear();
        QCOMPARE(keyed.size(), 0);
    }

    void loadsAndPlaysAnExtensionClip()
    {
        const QString path = m_temp.filePath(QStringLiteral("clip.png"));
        QVERIFY(TestAtlas::writeClip(path, 2, QColor(20, 40, 60, 180)));

        auto clip = QSharedPointer<AnimationClip>::create();
        QVERIFY(clip->load(path, {50, 50}));
        QCOMPARE(clip->frameCount(), 2);
        QCOMPARE(clip->frame(1).size(), QSize(192, 208));

        AnimationPlayer player;
        QSignalSpy frameSpy(&player, &AnimationPlayer::frameReady);
        QSignalSpy loopSpy(&player, &AnimationPlayer::clipLoopCompleted);
        player.setSpeedFactor(2.0);
        player.setClip(clip, QStringLiteral("click"));
        player.start();
        QTRY_VERIFY_WITH_TIMEOUT(loopSpy.count() >= 1, 250);
        QVERIFY(frameSpy.count() >= 3);
        QCOMPARE(loopSpy.first().at(0).toString(), QStringLiteral("click"));
        player.stop();
    }

    // Regression: the speed slider emits one setSpeedFactor per step. When that
    // restarted the single-shot frame timer, a continuous drag reset the pending
    // frame before it could fire and the pet froze for the whole drag.
    void repeatedSpeedChangesDoNotStallPlayback()
    {
        const QString path = m_temp.filePath(QStringLiteral("speed-clip.png"));
        QVERIFY(TestAtlas::writeClip(path, 2, QColor(30, 90, 120, 200)));
        auto clip = QSharedPointer<AnimationClip>::create();
        QVERIFY(clip->load(path, {50, 50}));

        AnimationPlayer player;
        QSignalSpy loopSpy(&player, &AnimationPlayer::clipLoopCompleted);
        player.setClip(clip, QStringLiteral("typing"));
        player.start();

        // Stand in for a drag: change the speed far more often than the 50ms
        // frame interval. Playback must still advance.
        for (int step = 0; step < 40; ++step) {
            player.setSpeedFactor(0.5 + (step % 16) * 0.1);
            QTest::qWait(5);
        }
        QVERIFY2(loopSpy.count() >= 1, "playback stalled while the speed kept changing");
        player.stop();
    }

    // The keystroke-driven typing animation, extracted from AppController. Each
    // frame of the strip gets a distinct colour so the emitted frame identifies
    // which cell the driver is holding.
    void typingDriverAlternatesPawsAndRelaxesToRest()
    {
        const QString path = m_temp.filePath(QStringLiteral("typing3.png"));
        const QColor colors[3] = {QColor(10, 0, 0, 255), QColor(0, 10, 0, 255), QColor(0, 0, 10, 255)};
        QVERIFY(TestAtlas::writeClipFrames(path, {colors[0], colors[1], colors[2]}));
        auto clip = QSharedPointer<AnimationClip>::create();
        QVERIFY(clip->load(path, {130, 130, 130}));

        AnimationPlayer player;
        int shown = -1;
        connect(&player, &AnimationPlayer::frameReady, this, [&shown, &colors](const QImage &frame) {
            if (frame.isNull()) return;
            const QColor pixel = frame.pixelColor(1, 1);
            for (int index = 0; index < 3; ++index) {
                if (pixel == colors[index]) shown = index;
            }
        });
        TypingAnimationDriver driver(&player);

        driver.begin(clip, false);
        QVERIFY(driver.isPressActive());
        QCOMPARE(shown, 0);                     // rest: hands on the keyboard

        driver.onKey();
        QCOMPARE(shown, 1);                     // left paw
        driver.onKey();
        QCOMPARE(shown, 2);                     // right paw
        driver.onKey();
        QCOMPARE(shown, 1);                     // alternates back

        QTRY_COMPARE_WITH_TIMEOUT(shown, 0, 1000);  // relaxes to rest after the pause

        // The paw alternation carries across a pause rather than restarting, so
        // resuming after the relax continues with the other paw.
        driver.onKey();
        QCOMPARE(shown, 2);

        // Leaving typing disengages: further keys must not move the frame, and a
        // relax already in flight must not fire either.
        driver.stop();
        QVERIFY(!driver.isPressActive());
        shown = -1;
        driver.onKey();
        QCOMPARE(shown, -1);
        QTest::qWait(200);
        QCOMPARE(shown, -1);                    // cancelled relax stayed cancelled

        // Reduced motion holds the same rest frame with no per-key motion.
        driver.begin(clip, true);
        QVERIFY(!driver.isPressActive());
        QCOMPARE(shown, 0);
        shown = -1;
        driver.onKey();
        QCOMPARE(shown, -1);
    }

    void typingDriverFallsBackToTheWorkingRowWithoutAClip()
    {
        const QString path = createAtlas(QStringLiteral("typing-fallback"), QColor(9, 9, 9, 255));
        auto atlas = QSharedPointer<PetAtlas>::create();
        QVERIFY(atlas->load(path));

        AnimationPlayer player;
        player.setAtlas(atlas);
        TypingAnimationDriver driver(&player);

        // No dedicated typing clip: the contract says fall back to the v2 "running"
        // row, which means active work rather than literal foot-running.
        driver.begin({}, false);
        QVERIFY(!driver.isPressActive());
        QVERIFY(player.isRunning());
        player.stop();
    }

    void reducedMotionKeepsAnExtensionClipOnItsRepresentativeFrame()
    {
        const QString path = m_temp.filePath(QStringLiteral("reduced-clip.png"));
        QVERIFY(TestAtlas::writeClip(path, 1, QColor(80, 60, 40, 180)));
        auto clip = QSharedPointer<AnimationClip>::create();
        QVERIFY(clip->load(path, {50}));

        AnimationPlayer player;
        QSignalSpy loopSpy(&player, &AnimationPlayer::clipLoopCompleted);
        player.setClip(clip, QStringLiteral("typing"));
        player.setReducedMotion(true);
        player.start();
        QTest::qWait(80);
        QCOMPARE(loopSpy.count(), 0);
        QVERIFY(!player.isRunning());
    }

private:
    QTemporaryDir m_temp;
};

// Runs inside a grouped binary; see tests/support/TestRunner.h.
QObject *createAtlasTest() { return new AtlasTest; }

#include "test_atlas.moc"
