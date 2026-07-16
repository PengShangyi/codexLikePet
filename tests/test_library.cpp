#include "pet/PetAtlas.h"
#include "resources/PetLibrary.h"

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
        QImage image(PetAtlas::Width, PetAtlas::Height, QImage::Format_RGBA8888);
        image.fill(Qt::transparent);
        for (int row = 0; row < PetAtlas::Rows; ++row) {
            const int columns = row <= 8 ? PetAtlas::animationSpec(static_cast<V2AnimationState>(row)).frameCount : 8;
            for (int column = 0; column < columns; ++column) image.setPixelColor(column * 192 + 1, row * 208 + 1, Qt::white);
        }
        if (!image.save(QDir(directory).filePath(QStringLiteral("spritesheet.png")))) return false;
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

QTEST_GUILESS_MAIN(LibraryTest)

#include "test_library.moc"
