#include "platform/WindowOverlay.h"

#include <QTest>

class WindowOverlayTest final : public QObject
{
    Q_OBJECT

private slots:
    // Always-on-top must adopt the overlay level AND both Space behaviors — the
    // level alone does not survive another app going full screen.
    void alwaysOnTopOverlaysEverySpaceAndFullScreen()
    {
        const WindowOverlay::Policy policy = WindowOverlay::policyFor(true);
        QVERIFY(policy.level == WindowOverlay::Level::Overlay);
        QVERIFY(policy.joinAllSpaces);
        QVERIFY(policy.overlayFullScreen);
    }

    // Disabled is a plain, managed window that other windows can cover.
    void disabledIsAnOrdinaryManagedWindow()
    {
        const WindowOverlay::Policy policy = WindowOverlay::policyFor(false);
        QVERIFY(policy.level == WindowOverlay::Level::Normal);
        QVERIFY(!policy.joinAllSpaces);
        QVERIFY(!policy.overlayFullScreen);
    }
};

// Runs inside a grouped binary; see tests/support/TestRunner.h.
QObject *createWindowOverlayTest() { return new WindowOverlayTest; }

#include "test_window_overlay.moc"
