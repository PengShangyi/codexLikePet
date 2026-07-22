#include "pet/AtlasCache.h"
#include "pet/AnimationClip.h"
#include "pet/AnimationPlayer.h"
#include "pet/PetAtlas.h"

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
        QImage image(PetAtlas::Width, PetAtlas::Height, QImage::Format_RGBA8888);
        image.fill(color);
        if (!image.save(path)) {
            return {};
        }
        return path;
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

    void loadsAndPlaysAnExtensionClip()
    {
        const QString path = m_temp.filePath(QStringLiteral("clip.png"));
        QImage strip(PetAtlas::CellWidth * 2,
                     PetAtlas::CellHeight,
                     QImage::Format_RGBA8888);
        strip.fill(QColor(20, 40, 60, 180));
        QVERIFY(strip.save(path));

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

    void reducedMotionKeepsAnExtensionClipOnItsRepresentativeFrame()
    {
        const QString path = m_temp.filePath(QStringLiteral("reduced-clip.png"));
        QImage strip(PetAtlas::CellWidth,
                     PetAtlas::CellHeight,
                     QImage::Format_RGBA8888);
        strip.fill(QColor(80, 60, 40, 180));
        QVERIFY(strip.save(path));
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

QTEST_GUILESS_MAIN(AtlasTest)

#include "test_atlas.moc"
