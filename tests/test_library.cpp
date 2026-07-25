#include "pet/PetAtlas.h"
#include "resources/PetLibrary.h"
#include "support/AtlasFixture.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

class LibraryTest final : public QObject
{
    Q_OBJECT

    static bool createPet(const QString &root, const QString &id, const QString &name)
    {
        const QString directory = QDir(root).filePath(id);
        if (!QDir().mkpath(directory)) return false;
        if (!TestAtlas::writeValid(QDir(directory).filePath(QStringLiteral("spritesheet.png")))) return false;
        QFile file(QDir(directory).filePath(QStringLiteral("pet.json")));
        if (!file.open(QIODevice::WriteOnly)) return false;
        return file.write(QJsonDocument(QJsonObject{{QStringLiteral("id"), id},
                                                    {QStringLiteral("displayName"), name},
                                                    {QStringLiteral("description"), QStringLiteral("test")},
                                                    {QStringLiteral("spriteVersionNumber"), 2},
                                                    {QStringLiteral("spritesheetPath"), QStringLiteral("spritesheet.png")}}).toJson()) > 0;
    }

private slots:
    void userPetOverridesAndThenRevealsBuiltInPet()
    {
        QTemporaryDir builtIn;
        QTemporaryDir user;
        QVERIFY(createPet(builtIn.path(), QStringLiteral("potato"), QStringLiteral("Built-in")));
        QVERIFY(createPet(user.path(), QStringLiteral("potato"), QStringLiteral("Custom")));
        PetLibrary library(builtIn.path(), user.path());
        library.refresh();
        QCOMPARE(library.pets().size(), 1);
        QCOMPARE(library.find(QStringLiteral("potato"))->package.displayName, QStringLiteral("Custom"));
        QVERIFY(!library.find(QStringLiteral("potato"))->builtIn);
        QString error;
        QVERIFY(library.removeCustomPet(QStringLiteral("potato"), &error));
        QCOMPARE(library.find(QStringLiteral("potato"))->package.displayName, QStringLiteral("Built-in"));
        QVERIFY(library.find(QStringLiteral("potato"))->builtIn);
    }
};

// Runs inside a grouped binary; see tests/support/TestRunner.h.
QObject *createLibraryTest() { return new LibraryTest; }

#include "test_library.moc"
