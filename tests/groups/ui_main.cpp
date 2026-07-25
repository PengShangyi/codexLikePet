#include "support/TestRunner.h"

#include <QtWidgets/QApplication>

// The widget layer: the palette, the settings design system built on it, and
// the settings window that composes both. Runs offscreen.
//
// Each class below is still its own CTest entry, registered with --class in
// tests/CMakeLists.txt, so filtering and parallelism are unchanged.
QObject *createThemeTest();
QObject *createUiKitTest();
QObject *createSettingsWindowTest();

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    return TestRunner::run(argc, argv, {
        {"ThemeTest", createThemeTest},
        {"UiKitTest", createUiKitTest},
        {"SettingsWindowTest", createSettingsWindowTest},
    });
}
