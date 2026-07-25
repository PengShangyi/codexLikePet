#include "pet/ClipContract.h"
#include "pet/PetAtlas.h"
#include "resources/PackagePolicy.h"

#include <QTest>

// PackagePolicy and ClipContract are the single point of truth for rules that
// used to be duplicated across the validator, the archive extractor, the store
// and the clip loader. They had only indirect coverage through those callers, so
// these tests pin the predicates themselves -- especially the reject cases.
class PackagePolicyTest final : public QObject
{
    Q_OBJECT

private slots:
    void acceptsOrdinaryRelativePaths()
    {
        QString clean;
        QVERIFY(PackagePolicy::isPortableRelativePath(QStringLiteral("spritesheet.webp"), &clean));
        QCOMPARE(clean, QStringLiteral("spritesheet.webp"));

        QVERIFY(PackagePolicy::isPortableRelativePath(QStringLiteral("clips/spring-day-typing.webp"),
                                                      &clean));
        QCOMPARE(clean, QStringLiteral("clips/spring-day-typing.webp"));

        // cleanPath collapses interior traversal that stays inside the root.
        QVERIFY(PackagePolicy::isPortableRelativePath(QStringLiteral("clips/../variants/a.webp"),
                                                      &clean));
        QCOMPARE(clean, QStringLiteral("variants/a.webp"));
        QVERIFY(!PackagePolicy::escapesRoot(clean));
    }

    void rejectsNonPortablePaths_data()
    {
        QTest::addColumn<QString>("path");
        QTest::addColumn<QString>("why");
        QTest::newRow("empty") << QString() << QStringLiteral("empty");
        QTest::newRow("absolute") << QStringLiteral("/etc/passwd") << QStringLiteral("absolute");
        QTest::newRow("home") << QStringLiteral("/Users/someone/x.png") << QStringLiteral("absolute");
        QTest::newRow("backslash") << QStringLiteral("clips\\typing.webp") << QStringLiteral("backslash");
        QTest::newRow("drive letter") << QStringLiteral("C:/spritesheet.png") << QStringLiteral("drive");
        QTest::newRow("drive letter bare") << QStringLiteral("Z:") << QStringLiteral("drive");
        QTest::newRow("NUL") << QStringLiteral("a\0b.png") << QStringLiteral("control");
        QTest::newRow("newline") << QStringLiteral("a\nb.png") << QStringLiteral("control");
        QTest::newRow("tab") << QStringLiteral("a\tb.png") << QStringLiteral("control");
        QTest::newRow("DEL") << QStringLiteral("a\x7f" "b.png") << QStringLiteral("control");
    }

    void rejectsNonPortablePaths()
    {
        QFETCH(QString, path);
        QString clean = QStringLiteral("untouched");
        QVERIFY2(!PackagePolicy::isPortableRelativePath(path, &clean), qPrintable(path));
        QCOMPARE(clean, QStringLiteral("untouched")); // must not write on rejection
    }

    void detectsRootEscapes()
    {
        QVERIFY(PackagePolicy::escapesRoot(QStringLiteral("..")));
        QVERIFY(PackagePolicy::escapesRoot(QStringLiteral("../outside.png")));
        QVERIFY(PackagePolicy::escapesRoot(QStringLiteral("../../etc/passwd")));
        QVERIFY(!PackagePolicy::escapesRoot(QStringLiteral(".")));
        QVERIFY(!PackagePolicy::escapesRoot(QStringLiteral("..hidden.png")));
        QVERIFY(!PackagePolicy::escapesRoot(QStringLiteral("clips/..deep.png")));

        // A traversal path is *portable* but still escapes: the two predicates are
        // deliberately separate because the archive extractor also rejects ".",
        // which the directory validator leaves to its later isFile() check.
        QString clean;
        QVERIFY(PackagePolicy::isPortableRelativePath(QStringLiteral("../outside.png"), &clean));
        QVERIFY(PackagePolicy::escapesRoot(clean));
    }

    void enforcesTheFileTypeAllowlist()
    {
        for (const QString &name : {QStringLiteral("pet.json"),
                                    QStringLiteral("spritesheet.webp"),
                                    QStringLiteral("atlas.png"),
                                    QStringLiteral("notes.txt"),
                                    QStringLiteral("README.md"),
                                    QStringLiteral("LICENSE"),
                                    QStringLiteral("license.txt"),
                                    QStringLiteral("licenses/LICENSE-MIT"),
                                    QStringLiteral("clips/a.WEBP")}) {
            QVERIFY2(PackagePolicy::isAllowedPackageFileName(name), qPrintable(name));
        }
        for (const QString &name : {QStringLiteral("payload.bin"),
                                    QStringLiteral("run.sh"),
                                    QStringLiteral("evil.dylib"),
                                    QStringLiteral("a.jpg"),
                                    QStringLiteral("notice"),
                                    QStringLiteral("Makefile")}) {
            QVERIFY2(!PackagePolicy::isAllowedPackageFileName(name), qPrintable(name));
        }
    }

    void constrainsPetIdsToPathSafeNames()
    {
        QVERIFY(PackagePolicy::isValidPetId(QStringLiteral("potato")));
        QVERIFY(PackagePolicy::isValidPetId(QStringLiteral("a")));
        QVERIFY(PackagePolicy::isValidPetId(QStringLiteral("pet-2")));
        QVERIFY(PackagePolicy::isValidPetId(QString(64, QLatin1Char('a'))));

        QVERIFY(!PackagePolicy::isValidPetId(QString()));
        QVERIFY(!PackagePolicy::isValidPetId(QString(65, QLatin1Char('a'))));
        QVERIFY(!PackagePolicy::isValidPetId(QStringLiteral("-leading")));
        QVERIFY(!PackagePolicy::isValidPetId(QStringLiteral("Potato")));      // uppercase
        QVERIFY(!PackagePolicy::isValidPetId(QStringLiteral("../escape")));
        QVERIFY(!PackagePolicy::isValidPetId(QStringLiteral("with space")));
        QVERIFY(!PackagePolicy::isValidPetId(QStringLiteral("dot.dot")));
        QVERIFY(!PackagePolicy::isValidPetId(QStringLiteral("a/b")));
    }

    void pinsTheClipGeometryAndDurationContract()
    {
        int frames = -1;
        QVERIFY(ClipContract::isValidGeometry(QSize(PetAtlas::CellWidth, PetAtlas::CellHeight),
                                              &frames));
        QCOMPARE(frames, 1);
        QVERIFY(ClipContract::isValidGeometry(QSize(PetAtlas::CellWidth * 8, PetAtlas::CellHeight),
                                              &frames));
        QCOMPARE(frames, 8);

        // Nine cells, a short strip, a ragged width, and an invalid size all fail,
        // and the out-parameter is zeroed rather than left stale.
        frames = -1;
        QVERIFY(!ClipContract::isValidGeometry(QSize(PetAtlas::CellWidth * 9, PetAtlas::CellHeight),
                                               &frames));
        QCOMPARE(frames, 0);
        QVERIFY(!ClipContract::isValidGeometry(QSize(0, PetAtlas::CellHeight)));
        QVERIFY(!ClipContract::isValidGeometry(QSize(PetAtlas::CellWidth, PetAtlas::CellHeight + 1)));
        QVERIFY(!ClipContract::isValidGeometry(QSize(PetAtlas::CellWidth + 1, PetAtlas::CellHeight)));
        QVERIFY(!ClipContract::isValidGeometry(QSize()));

        QVERIFY(ClipContract::isValidDuration(ClipContract::MinDurationMs));
        QVERIFY(ClipContract::isValidDuration(ClipContract::MaxDurationMs));
        QVERIFY(!ClipContract::isValidDuration(ClipContract::MinDurationMs - 1));
        QVERIFY(!ClipContract::isValidDuration(ClipContract::MaxDurationMs + 1));
        QVERIFY(!ClipContract::isValidDuration(0));
        QVERIFY(!ClipContract::isValidDuration(-1));

        QVERIFY(ClipContract::areValidDurations({}));
        QVERIFY(ClipContract::areValidDurations({50, 130, 2000}));
        QVERIFY(!ClipContract::areValidDurations({130, 49}));
    }
};

QTEST_GUILESS_MAIN(PackagePolicyTest)

#include "test_package_policy.moc"
