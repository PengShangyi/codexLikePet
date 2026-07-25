#include "pet/PetWindow.h"
#include "settings/AppSettings.h"

#include <QGuiApplication>
#include <QTemporaryDir>
#include <QTest>

#import <AppKit/AppKit.h>

// Verifies the *native* always-on-top overlay end to end: that showing the pet
// actually pushes NSStatusWindowLevel + the full-screen/all-Spaces collection
// behavior onto its NSWindow, that disabling restores an ordinary window, and
// that reapplyAlwaysOnTop() re-pushes the level without a re-show. These assert
// real AppKit state, so they only run under the cocoa QPA plugin (a dev Mac with
// a window server); headless CI (offscreen/minimal) skips them.
class PetOverlayTest final : public QObject
{
    Q_OBJECT

    static NSWindow *nativeWindow(PetWindow &pet)
    {
        return reinterpret_cast<NSView *>(pet.winId()).window;
    }

    static bool requireCocoa()
    {
        return QGuiApplication::platformName() == QLatin1String("cocoa");
    }

private slots:
    void showingAdoptsOverlayLevelAndFullScreenSpaces()
    {
        if (!requireCocoa()) QSKIP("native NSWindow behavior requires the cocoa QPA platform");
        QTemporaryDir dir;
        AppSettings settings(dir.filePath(QStringLiteral("settings.ini")));
        settings.setAlwaysOnTop(true);

        PetWindow pet(&settings);
        pet.show();
        QVERIFY(QTest::qWaitForWindowExposed(&pet));

        NSWindow *window = nativeWindow(pet);
        QVERIFY(window != nil);
        QCOMPARE(window.level, static_cast<NSInteger>(NSStatusWindowLevel));
        QVERIFY(window.collectionBehavior & NSWindowCollectionBehaviorCanJoinAllSpaces);
        QVERIFY(window.collectionBehavior & NSWindowCollectionBehaviorFullScreenAuxiliary);
        QVERIFY(!window.hidesOnDeactivate);
    }

    void disablingRestoresAnOrdinaryWindowAndReenablingRestoresOverlay()
    {
        if (!requireCocoa()) QSKIP("native NSWindow behavior requires the cocoa QPA platform");
        QTemporaryDir dir;
        AppSettings settings(dir.filePath(QStringLiteral("settings.ini")));
        settings.setAlwaysOnTop(true);

        PetWindow pet(&settings);
        pet.show();
        QVERIFY(QTest::qWaitForWindowExposed(&pet));

        pet.setAlwaysOnTop(false);
        QVERIFY(QTest::qWaitForWindowExposed(&pet));
        NSWindow *window = nativeWindow(pet);
        QCOMPARE(window.level, static_cast<NSInteger>(NSNormalWindowLevel));
        QVERIFY(!(window.collectionBehavior & NSWindowCollectionBehaviorCanJoinAllSpaces));
        QVERIFY(!(window.collectionBehavior & NSWindowCollectionBehaviorFullScreenAuxiliary));

        pet.setAlwaysOnTop(true);
        QVERIFY(QTest::qWaitForWindowExposed(&pet));
        window = nativeWindow(pet);
        QCOMPARE(window.level, static_cast<NSInteger>(NSStatusWindowLevel));
        QVERIFY(window.collectionBehavior & NSWindowCollectionBehaviorFullScreenAuxiliary);
    }

    // Guards the reassert wiring: if something resets the level while the window
    // stays shown (as a native-window recreation would), reapplyAlwaysOnTop() must
    // put it back. Deleting the reassert calls makes this fail.
    void reapplyRestoresOverlayLevelWithoutReshow()
    {
        if (!requireCocoa()) QSKIP("native NSWindow behavior requires the cocoa QPA platform");
        QTemporaryDir dir;
        AppSettings settings(dir.filePath(QStringLiteral("settings.ini")));
        settings.setAlwaysOnTop(true);

        PetWindow pet(&settings);
        pet.show();
        QVERIFY(QTest::qWaitForWindowExposed(&pet));

        NSWindow *window = nativeWindow(pet);
        window.level = NSNormalWindowLevel;
        window.collectionBehavior = NSWindowCollectionBehaviorDefault;

        pet.reapplyAlwaysOnTop();

        QCOMPARE(window.level, static_cast<NSInteger>(NSStatusWindowLevel));
        QVERIFY(window.collectionBehavior & NSWindowCollectionBehaviorCanJoinAllSpaces);
        QVERIFY(window.collectionBehavior & NSWindowCollectionBehaviorFullScreenAuxiliary);
    }
};

QTEST_MAIN(PetOverlayTest)

#include "test_pet_overlay.moc"
