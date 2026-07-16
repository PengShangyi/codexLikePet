#include "pet/AtlasCache.h"
#include "pet/PetAtlas.h"

#include <QTemporaryDir>
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

private:
    QTemporaryDir m_temp;
};

QTEST_GUILESS_MAIN(AtlasTest)

#include "test_atlas.moc"
