#include "app/AppController.h"
#include "app/AppNotifier.h"
#include "environment/EnvironmentClock.h"
#include "input/InputActivitySource.h"
#include "login/LoginItemController.h"
#include "pet/AtlasComposer.h"
#include "pet/PetAtlas.h"
#include "pet/PetWindow.h"
#include "platform/SystemActivitySource.h"
#include "settings/AtlasAssemblerWindow.h"
#include "settings/AppSettings.h"
#include "settings/SettingsWindow.h"
#include "support/AtlasFixture.h"

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QPainter>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

// Purpose-built doubles for AppController's platform boundaries. Each is stack
// allocated in the test and injected via AppController::Dependencies, so no real
// CGEventTap / NSWorkspace / SMAppService is touched and no modal dialog blocks.

class FakeInput final : public InputActivitySource
{
    Q_OBJECT
public:
    using InputActivitySource::InputActivitySource;
    InputStartResult startResult = InputStartResult::Started;
    InputStartResult start() override
    {
        if (startResult == InputStartResult::Started) m_active = true;
        return startResult;
    }
    void stop() override { m_active = false; }
    bool isActive() const override { return m_active; }
    void simulateInvalidation()
    {
        m_active = false;
        emit monitoringInvalidated();
    }
    // Stands in for one key-down reaching the (listen-only) event tap.
    void simulateActivity() { emit activityDetected(); }

private:
    bool m_active = false;
};

class FakeSystem final : public SystemActivitySource
{
    Q_OBJECT
public:
    using SystemActivitySource::SystemActivitySource;
    bool systemReduceMotion() const override { return m_reduced; }
    void setReduced(bool value)
    {
        m_reduced = value;
        emit reduceMotionChanged(value);
    }
    void sleep() { emit willSleep(); }
    void wakeUp() { emit didWake(); }
    void displaySleep() { emit screensDidSleep(); }
    void displayWake() { emit screensDidWake(); }

private:
    bool m_reduced = false;
};

class FakeClock final : public EnvironmentClock
{
    Q_OBJECT
public:
    using EnvironmentClock::EnvironmentClock;
    QDateTime now() const override { return moment; }
    QDateTime moment = QDateTime(QDate(2024, 4, 15), QTime(12, 0));
};

class FakeLoginItem final : public LoginItemController
{
public:
    bool setEnabled(bool, QString *) override { return true; }
};

class RecordingNotifier final : public AppNotifier
{
public:
    int petLoadErrors = 0;
    int permissionPrompts = 0;
    int warnings = 0;
    PermissionChoice permissionResult = PermissionChoice::Dismiss;

    void notifyPetLoadError(const QString &, const QString &) override { ++petLoadErrors; }
    PermissionChoice promptInputPermission(const QString &, const QString &, const QString &) override
    {
        ++permissionPrompts;
        return permissionResult;
    }
    // The text as well as the count: a swallowed reason is the hard part to debug
    // when an install path fails, and asserting on it keeps the message honest.
    QString lastWarning;
    void warn(const QString &, const QString &detail) override
    {
        ++warnings;
        lastWarning = detail;
    }
    bool confirmRemoval(const QString &, const QString &) override { return true; }
    void showFatalStartup(const QString &, const QString &) override {}
};

class AppControllerTest final : public QObject
{
    Q_OBJECT

    // Writes a minimal but contract-valid Codex v2 pet the PetLibrary will accept.
    static bool createPet(const QString &root, const QString &id, const QString &name,
                          bool withTypingClip = false)
    {
        const QString directory = QDir(root).filePath(id);
        if (!QDir().mkpath(directory)) return false;
        if (!TestAtlas::writeValid(QDir(directory).filePath(QStringLiteral("spritesheet.png")))) return false;

        QJsonObject manifest{
            {QStringLiteral("id"), id},
            {QStringLiteral("displayName"), name},
            {QStringLiteral("description"), QStringLiteral("test")},
            {QStringLiteral("spriteVersionNumber"), 2},
            {QStringLiteral("spritesheetPath"), QStringLiteral("spritesheet.png")}};
        QFile file(QDir(directory).filePath(QStringLiteral("pet.json")));
        if (!file.open(QIODevice::WriteOnly)) return false;
        if (file.write(QJsonDocument(manifest).toJson()) <= 0) return false;

        if (withTypingClip) {
            if (!QDir().mkpath(QDir(directory).filePath(QStringLiteral("clips")))) return false;
            // Reuse a real 3-frame typing clip so it passes package validation and
            // AnimationClip::load exactly as the shipped assets do.
            const QString src = QStringLiteral(
                POTATO_SOURCE_DIR "/assets/Pets/potato/clips/spring-day-typing.webp");
            if (!QFile::copy(src, QDir(directory).filePath(QStringLiteral("clips/typing.webp")))) return false;
            // Clips are a Potato extension: they live in potato.json beside pet.json.
            const QJsonObject clipDef{{QStringLiteral("path"), QStringLiteral("clips/typing.webp")},
                                      {QStringLiteral("durationsMs"), QJsonArray{130, 130, 130}}};
            QFile potato(QDir(directory).filePath(QStringLiteral("potato.json")));
            if (!potato.open(QIODevice::WriteOnly)) return false;
            return potato.write(QJsonDocument(QJsonObject{
                                    {QStringLiteral("schemaVersion"), 1},
                                    {QStringLiteral("variants"), QJsonObject{}},
                                    {QStringLiteral("clips"), QJsonObject{{QStringLiteral("typing"), clipDef}}},
                                    {QStringLiteral("variantClips"), QJsonObject{}}})
                                    .toJson())
                > 0;
        }
        return true;
    }

    static SettingsWindow *findSettingsWindow()
    {
        SettingsWindow *window = nullptr;
        for (QWidget *candidate : QApplication::topLevelWidgets()) {
            if (auto *found = qobject_cast<SettingsWindow *>(candidate)) window = found;
        }
        return window;
    }

    struct Fixture {
        QTemporaryDir pets;
        QTemporaryDir config;
        FakeInput input;
        FakeSystem system;
        FakeClock clock;
        FakeLoginItem login;
        RecordingNotifier notifier;

        AppController::Dependencies deps(AppSettings *settings)
        {
            AppController::Dependencies d;
            d.settings = settings;
            d.input = &input;
            d.systemActivity = &system;
            d.clock = &clock;
            d.loginItem = &login;
            d.notifier = &notifier;
            d.builtInPetRoot = pets.path();
            d.userPetRoot = QDir(config.path()).filePath(QStringLiteral("userpets"));
            d.systemTrayAvailable = [] { return true; };
            return d;
        }
        QString settingsPath() { return config.filePath(QStringLiteral("settings.ini")); }
    };

    // The window is private to the controller and stays that way: showing it is a
    // public command, and a window that has been shown is findable. A test-only
    // accessor would be a second way in for no gain.
    static AtlasAssemblerWindow *showAndFindAssembler(AppController &controller)
    {
        controller.showAtlasAssembler();
        for (QWidget *candidate : QApplication::topLevelWidgets()) {
            if (auto *window = qobject_cast<AtlasAssemblerWindow *>(candidate)) return window;
        }
        return nullptr;
    }

private slots:
    void fallsBackToFirstAvailablePetWhenSelectionMissing()
    {
        Fixture fx;
        QVERIFY(createPet(fx.pets.path(), QStringLiteral("alpha"), QStringLiteral("Alpha")));
        AppSettings settings(fx.settingsPath());
        settings.setSelectedPetId(QStringLiteral("ghost"));

        AppController controller(fx.deps(&settings), AppRunMode::RuntimeCheck);
        QVERIFY(controller.start());
        QCOMPARE(settings.selectedPetId(), QStringLiteral("alpha"));
    }

    // A3: the tray menu is Show/Hide, the Pet submenu, Settings, About, Quit. The
    // Welcome Guide used to hold a permanent slot for a first-run artefact and now
    // lives under Settings > General > About.
    void trayMenuIsTrimmedAndHasNoWelcomeItem()
    {
        Fixture fx;
        QVERIFY(createPet(fx.pets.path(), QStringLiteral("alpha"), QStringLiteral("Alpha")));
        AppSettings settings(fx.settingsPath());
        Localization localization(&settings);

        AppController controller(fx.deps(&settings), AppRunMode::RuntimeCheck);
        QVERIFY(controller.start());

        // Found by objectName, not by type: a QMenu is a Qt::Popup and so counts as a
        // top-level widget even with a parent, which means the settings window's
        // import popup shows up in this list too.
        QMenu *tray = nullptr;
        for (QWidget *candidate : QApplication::topLevelWidgets()) {
            if (candidate->objectName() == QStringLiteral("trayMenu")) {
                tray = qobject_cast<QMenu *>(candidate);
            }
        }
        QVERIFY(tray);

        QStringList texts;
        for (QAction *action : tray->actions()) {
            if (!action->isSeparator()) texts << action->text();
        }
        QCOMPARE(texts, QStringList({localization.text(TextKey::HidePet),
                                     localization.text(TextKey::Pet),
                                     localization.text(TextKey::Settings),
                                     localization.text(TextKey::AboutMenuItem),
                                     localization.text(TextKey::Quit)}));
        QVERIFY(!texts.contains(localization.text(TextKey::WelcomeMenuItem)));
    }

    void opacitySettingReachesThePetWindowAndKeepsItVisible()
    {
        Fixture fx;
        QVERIFY(createPet(fx.pets.path(), QStringLiteral("alpha"), QStringLiteral("Alpha")));
        AppSettings settings(fx.settingsPath());

        AppController controller(fx.deps(&settings), AppRunMode::RuntimeCheck);
        QVERIFY(controller.start());

        PetWindow *pet = nullptr;
        for (QWidget *candidate : QApplication::topLevelWidgets()) {
            if (auto *window = qobject_cast<PetWindow *>(candidate)) pet = window;
        }
        QVERIFY(pet);
        QCOMPARE(pet->windowOpacity(), 1.0);

        // setWindowOpacity round-trips through an 8-bit alpha, so 0.5 comes back as
        // 127/255; compare within one step rather than exactly.
        settings.setOpacity(0.5);
        QVERIFY(qAbs(pet->windowOpacity() - 0.5) <= 1.0 / 255.0);

        // Never fully transparent: an invisible pet cannot be clicked or found again,
        // which is indistinguishable from having lost the window.
        settings.setOpacity(0.0);
        QVERIFY(qAbs(pet->windowOpacity() - 0.3) <= 1.0 / 255.0);
    }

    void permissionDeniedDisablesTypingAndPrompts()
    {
        Fixture fx;
        QVERIFY(createPet(fx.pets.path(), QStringLiteral("alpha"), QStringLiteral("Alpha")));
        fx.input.startResult = InputStartResult::PermissionDenied;
        AppSettings settings(fx.settingsPath());

        AppController controller(fx.deps(&settings), AppRunMode::RuntimeCheck);
        QVERIFY(controller.start());
        settings.setTypingDetectionEnabled(true);

        QVERIFY(!settings.typingDetectionEnabled());
        QCOMPARE(fx.notifier.permissionPrompts, 1);
        QVERIFY(!fx.input.isActive());
    }

    // First-time enable when the OS is showing its own access prompt: the setting
    // is turned back off (can't monitor yet) but we must NOT stack our own dialog
    // on top of the system prompt.
    void firstPermissionRequestDoesNotStackADialog()
    {
        Fixture fx;
        QVERIFY(createPet(fx.pets.path(), QStringLiteral("alpha"), QStringLiteral("Alpha")));
        fx.input.startResult = InputStartResult::PermissionRequested;
        AppSettings settings(fx.settingsPath());

        AppController controller(fx.deps(&settings), AppRunMode::RuntimeCheck);
        QVERIFY(controller.start());
        settings.setTypingDetectionEnabled(true);

        QVERIFY(!settings.typingDetectionEnabled());
        QCOMPARE(fx.notifier.permissionPrompts, 0); // OS prompt stands alone
        QVERIFY(!fx.input.isActive());
    }

    void monitoringInvalidationDisablesTyping()
    {
        Fixture fx;
        QVERIFY(createPet(fx.pets.path(), QStringLiteral("alpha"), QStringLiteral("Alpha")));
        AppSettings settings(fx.settingsPath());

        AppController controller(fx.deps(&settings), AppRunMode::RuntimeCheck);
        QVERIFY(controller.start());
        settings.setTypingDetectionEnabled(true);
        QVERIFY(settings.typingDetectionEnabled());
        QVERIFY(fx.input.isActive());

        fx.input.simulateInvalidation();
        QVERIFY(!settings.typingDetectionEnabled());
    }

    void sleepStopsAndWakeRestartsTypingMonitoring()
    {
        Fixture fx;
        QVERIFY(createPet(fx.pets.path(), QStringLiteral("alpha"), QStringLiteral("Alpha")));
        AppSettings settings(fx.settingsPath());

        AppController controller(fx.deps(&settings), AppRunMode::RuntimeCheck);
        QVERIFY(controller.start());
        settings.setTypingDetectionEnabled(true);
        QVERIFY(fx.input.isActive());

        fx.system.sleep();
        QVERIFY(!fx.input.isActive());

        fx.system.wakeUp();
        QVERIFY(fx.input.isActive());
    }

    void idleFidgetArmsOnlyWhenIdleVisibleAwakeAndMotionAllowed()
    {
        Fixture fx;
        QVERIFY(createPet(fx.pets.path(), QStringLiteral("alpha"), QStringLiteral("Alpha")));
        AppSettings settings(fx.settingsPath());
        auto deps = fx.deps(&settings);
        deps.idlePolicy.nextIntervalMs = [] { return 100000; }; // long: stays armed for the test
        deps.idlePolicy.nextAnimation = [] { return V2AnimationState::Jumping; };

        AppController controller(deps, AppRunMode::RuntimeCheck);
        QVERIFY(controller.start());
        QVERIFY(controller.isIdleFidgetArmed()); // idle + visible + awake + motion + atlas

        fx.system.setReduced(true);
        QVERIFY(!controller.isIdleFidgetArmed()); // reduced motion disarms
        fx.system.setReduced(false);
        QVERIFY(controller.isIdleFidgetArmed());

        fx.system.sleep();
        QVERIFY(!controller.isIdleFidgetArmed()); // asleep disarms
        fx.system.wakeUp();
        QVERIFY(controller.isIdleFidgetArmed()); // re-armed on wake

        fx.system.displaySleep();
        QVERIFY(!controller.isIdleFidgetArmed()); // nothing on screen can be seen
        fx.system.displayWake();
        QVERIFY(controller.isIdleFidgetArmed());
    }

    // The reason a bool per quiet state was not enough. Two of them can hold at
    // once, and clearing one must not resume the pet while the other still stands --
    // the sequence that used to get this wrong is display-sleeps-then-wakes while the
    // pet is hidden, which would have started animating a window the user had put
    // away. Ordering matters both ways round, so both are exercised.
    void quietReasonsCompose()
    {
        Fixture fx;
        QVERIFY(createPet(fx.pets.path(), QStringLiteral("alpha"), QStringLiteral("Alpha")));
        AppSettings settings(fx.settingsPath());
        auto deps = fx.deps(&settings);
        deps.idlePolicy.nextIntervalMs = [] { return 100000; };
        deps.idlePolicy.nextAnimation = [] { return V2AnimationState::Jumping; };

        AppController controller(deps, AppRunMode::RuntimeCheck);
        QVERIFY(controller.start());
        QVERIFY(controller.isIdleFidgetArmed());

        controller.setPetVisible(false);
        QVERIFY(!controller.isIdleFidgetArmed());
        fx.system.displaySleep();
        QVERIFY(!controller.isIdleFidgetArmed());
        fx.system.displayWake();
        QVERIFY2(!controller.isIdleFidgetArmed(), "still hidden, so the display waking is not enough");
        // The fidget gate recomputes from the reasons, so it would read correctly even
        // if the pet had been resumed behind it. The rollover poll is plain timer
        // state, so it is what shows whether anything actually restarted.
        QVERIFY2(!controller.isEnvironmentPollRunning(),
                 "the rollover poll restarted while the pet was still hidden");
        controller.setPetVisible(true);
        QVERIFY(controller.isIdleFidgetArmed());
        QVERIFY(controller.isEnvironmentPollRunning());

        // Other order: the display sleeps first, and showing the pet must not resume
        // it either.
        fx.system.displaySleep();
        controller.setPetVisible(false);
        controller.setPetVisible(true);
        QVERIFY2(!controller.isIdleFidgetArmed(), "the display is still asleep");
        QVERIFY2(!controller.isEnvironmentPollRunning(),
                 "showing the pet restarted the poll while the display was asleep");
        fx.system.displayWake();
        QVERIFY(controller.isIdleFidgetArmed());
        QVERIFY(controller.isEnvironmentPollRunning());
    }

    // Sleeping the display stops the pet moving; it does not put the pet away. If it
    // reached the window, the pet would still be gone after the display woke, and the
    // tray would offer to show a pet that had never been hidden.
    void aSleepingDisplayNeitherHidesThePetNorRelabelsTheTray()
    {
        Fixture fx;
        QVERIFY(createPet(fx.pets.path(), QStringLiteral("alpha"), QStringLiteral("Alpha")));
        AppSettings settings(fx.settingsPath());

        AppController controller(fx.deps(&settings), AppRunMode::RuntimeCheck);
        QVERIFY(controller.start());
        QSignalSpy visibility(&controller, &AppController::petVisibilityRequested);

        fx.system.displaySleep();
        fx.system.wakeUp();
        fx.system.sleep();
        fx.system.displayWake();
        QCOMPARE(visibility.count(), 0);

        // ...whereas the tray toggle does reach it, exactly once per change.
        controller.setPetVisible(false);
        QCOMPARE(visibility.count(), 1);
        QCOMPARE(visibility.last().first().toBool(), false);
        controller.setPetVisible(true);
        QCOMPARE(visibility.count(), 2);
        QCOMPARE(visibility.last().first().toBool(), true);
    }

    // End-to-end typing wiring: a keystroke must move the pet out of Idle (into
    // Typing), then it must return to Idle after the inactivity window. Uses the
    // idle-fidget scheduler as an observable proxy for "the pet is Idle" — it is
    // armed only while Idle. Exercises InputActivitySource -> TypingActivityDetector
    // -> BehaviorController -> AppController::applyBehaviorState/updateIdleScheduler.
    void typingActivityMovesThePetOutOfIdleThenBack()
    {
        Fixture fx;
        QVERIFY(createPet(fx.pets.path(), QStringLiteral("alpha"), QStringLiteral("Alpha")));
        AppSettings settings(fx.settingsPath());
        auto deps = fx.deps(&settings);
        deps.idlePolicy.nextIntervalMs = [] { return 100000; }; // long: stays armed while Idle
        deps.idlePolicy.nextAnimation = [] { return V2AnimationState::Jumping; };

        AppController controller(deps, AppRunMode::RuntimeCheck);
        QVERIFY(controller.start());
        settings.setTypingDetectionEnabled(true);
        QVERIFY(fx.input.isActive());
        QVERIFY(controller.isIdleFidgetArmed()); // Idle at rest

        fx.input.simulateActivity();             // a key-down
        QVERIFY(!controller.isIdleFidgetArmed()); // Typing is not Idle -> disarmed
        QVERIFY(!controller.isTypingPressActive()); // no typing clip here -> running-row fallback

        // TypingActivityDetector's 1500ms inactivity timer then clears typing and
        // the pet returns to Idle, re-arming the scheduler.
        QTRY_VERIFY_WITH_TIMEOUT(controller.isIdleFidgetArmed(), 4000);
    }

    // With a dedicated typing clip, each keystroke engages the clip-driven press
    // machinery (held clip, advanced per key). Exercises the full chain including
    // AppController::onTypingKey/beginTypingAnimation and the clip-vs-fallback choice.
    void keystrokesEngageTheTypingPressAnimation()
    {
        Fixture fx;
        QVERIFY(createPet(fx.pets.path(), QStringLiteral("alpha"), QStringLiteral("Alpha"), true));
        AppSettings settings(fx.settingsPath());

        AppController controller(fx.deps(&settings), AppRunMode::RuntimeCheck);
        QVERIFY(controller.start());
        QVERIFY2(controller.isIdleFidgetArmed(), "pet/atlas did not load (clip validation?)");
        settings.setTypingDetectionEnabled(true);
        QVERIFY(fx.input.isActive());
        QVERIFY(!controller.isTypingPressActive()); // idle at rest

        fx.input.simulateActivity();                 // first key -> Typing + clip-driven press
        QVERIFY(controller.isTypingPressActive());
        fx.input.simulateActivity();                 // further keys pulse; still engaged, no crash
        QVERIFY(controller.isTypingPressActive());

        // After the inactivity window, typing clears and press mode disengages.
        QTRY_VERIFY_WITH_TIMEOUT(!controller.isTypingPressActive(), 4000);
    }

    // The season/day-night fallback table shown in Settings is built from the
    // package and the UI language only. It used to be rebuilt from
    // loadCurrentVariant(), i.e. on every day/night and season rollover as well;
    // moving it out of that path must not leave it empty or stale.
    void resourceSummaryCoversEveryVariantAndFollowsLanguage()
    {
        Fixture fx;
        QVERIFY(createPet(fx.pets.path(), QStringLiteral("alpha"), QStringLiteral("Alpha")));
        AppSettings settings(fx.settingsPath());
        settings.setLanguage(AppLanguage::English);

        AppController controller(fx.deps(&settings), AppRunMode::RuntimeCheck);
        QVERIFY(controller.start());

        // The window is built on first use, so ask for it: the summary was
        // recorded while it did not exist and is replayed on attach.
        controller.requestSettings();
        SettingsWindow *window = findSettingsWindow();
        QVERIFY2(window, "settings window not found among top-level widgets");

        const QStringList variants{
            QStringLiteral("spring-day"), QStringLiteral("spring-night"),
            QStringLiteral("summer-day"), QStringLiteral("summer-night"),
            QStringLiteral("autumn-day"), QStringLiteral("autumn-night"),
            QStringLiteral("winter-day"), QStringLiteral("winter-night")};
        const QString summary = window->resourceSummary();
        QVERIFY(!summary.isEmpty());
        for (const QString &variant : variants) {
            QVERIFY2(summary.contains(variant), qPrintable(variant));
        }
        // This pet ships no clips, so every clip slot reports the v2 fallback.
        QVERIFY(summary.contains(QStringLiteral("v2 fallback")));

        // An environment rollover no longer rebuilds the summary; it must still be
        // the same complete table afterwards rather than empty or truncated.
        settings.setHemisphere(Hemisphere::South);
        QCOMPARE(window->resourceSummary(), summary);

        // Language does drive it: the fallback label is localized.
        settings.setLanguage(AppLanguage::SimplifiedChinese);
        const QString localized = window->resourceSummary();
        QVERIFY(!localized.contains(QStringLiteral("v2 fallback")));
        QVERIFY(localized.contains(QStringLiteral("v2 内置回退")));
        for (const QString &variant : variants) {
            QVERIFY2(localized.contains(variant), qPrintable(variant));
        }
    }

    void startsCleanlyWithNoPetsAvailable()
    {
        Fixture fx; // no pets seeded
        AppSettings settings(fx.settingsPath());
        AppController controller(fx.deps(&settings), AppRunMode::RuntimeCheck);
        QVERIFY(controller.start()); // empty library must not crash or fail startup
    }

    // The settings window is five pages of widgets that most sessions never
    // open, so a started controller must not have built one.
    void startupDoesNotBuildTheSettingsWindow()
    {
        Fixture fx;
        QVERIFY(createPet(fx.pets.path(), QStringLiteral("alpha"), QStringLiteral("Alpha")));
        AppSettings settings(fx.settingsPath());

        AppController controller(fx.deps(&settings), AppRunMode::RuntimeCheck);
        QVERIFY(controller.start());
        QVERIFY2(!findSettingsWindow(), "settings window was built before anyone asked for it");

        controller.requestSettings();
        QVERIFY(findSettingsWindow());
    }

    // Opening it must produce the same window the eager version did, built from
    // state pushed at it before it existed.
    void openingSettingsReplaysTheStateItMissed()
    {
        Fixture fx;
        QVERIFY(createPet(fx.pets.path(), QStringLiteral("alpha"), QStringLiteral("Alpha")));
        QVERIFY(createPet(fx.pets.path(), QStringLiteral("beta"), QStringLiteral("Beta")));
        AppSettings settings(fx.settingsPath());
        settings.setLanguage(AppLanguage::English);

        AppController controller(fx.deps(&settings), AppRunMode::RuntimeCheck);
        QVERIFY(controller.start());

        // Changed after startup and still with no window in existence, so this
        // proves the proxy carries late state and not just what start() pushed.
        fx.system.setReduced(true);
        settings.setLanguage(AppLanguage::SimplifiedChinese);
        QVERIFY(!findSettingsWindow());

        controller.requestSettings();
        SettingsWindow *window = findSettingsWindow();
        QVERIFY(window);

        // The pet list and its selection arrived during start().
        QCOMPARE(window->selectedPetId(), settings.selectedPetId());
        QVERIFY(!window->selectedPetId().isEmpty());

        // The summary is the full table, and in the language chosen after start().
        const QString summary = window->resourceSummary();
        QVERIFY(!summary.isEmpty());
        QVERIFY(summary.contains(QStringLiteral("spring-day")));
        QVERIFY(summary.contains(QStringLiteral("winter-night")));
        QVERIFY(summary.contains(QStringLiteral("v2 内置回退")));
    }

    // Asking twice must hand back the same window rather than stacking a second
    // widget tree behind the first.
    void reopeningSettingsReusesTheSameWindow()
    {
        Fixture fx;
        QVERIFY(createPet(fx.pets.path(), QStringLiteral("alpha"), QStringLiteral("Alpha")));
        AppSettings settings(fx.settingsPath());

        AppController controller(fx.deps(&settings), AppRunMode::RuntimeCheck);
        QVERIFY(controller.start());

        controller.requestSettings();
        SettingsWindow *first = findSettingsWindow();
        QVERIFY(first);

        first->hide();
        controller.requestSettings();
        QCOMPARE(findSettingsWindow(), first);

        int windows = 0;
        for (QWidget *candidate : QApplication::topLevelWidgets()) {
            if (qobject_cast<SettingsWindow *>(candidate)) ++windows;
        }
        QCOMPARE(windows, 1);
    }

    // The other two windows are lazy for the same reason, and neither is on any
    // path the app takes by itself.
    void aboutAndWelcomeWindowsAreAlsoBuiltOnDemand()
    {
        Fixture fx;
        QVERIFY(createPet(fx.pets.path(), QStringLiteral("alpha"), QStringLiteral("Alpha")));
        AppSettings settings(fx.settingsPath());

        AppController controller(fx.deps(&settings), AppRunMode::RuntimeCheck);
        QVERIFY(controller.start());

        const auto countTopLevel = [] {
            int total = 0;
            for (QWidget *candidate : QApplication::topLevelWidgets()) {
                if (candidate->isWindow() && !qobject_cast<QMenu *>(candidate)) ++total;
            }
            return total;
        };
        const int atStartup = countTopLevel();

        controller.showAbout();
        QVERIFY(countTopLevel() > atStartup);

        const int withAbout = countTopLevel();
        controller.showAtlasAssembler();
        QVERIFY2(countTopLevel() > withAbout, "the assembler window was not built on demand");
    }

    // The end of the assembly path: a composed atlas goes through the same importer
    // the Import Pet buttons use, lands in the library, and becomes the selected pet.
    // Nothing here is a new boundary -- the importer, notifier and settings were all
    // already injectable, which is why Dependencies did not have to grow.
    void installingAnAssembledPetSelectsItAndReportsBack()
    {
        Fixture fx;
        QVERIFY(createPet(fx.pets.path(), QStringLiteral("alpha"), QStringLiteral("Alpha")));
        AppSettings settings(fx.settingsPath());
        Localization localization(&settings);

        AppController controller(fx.deps(&settings), AppRunMode::RuntimeCheck);
        QVERIFY(controller.start());
        QCOMPARE(settings.selectedPetId(), QStringLiteral("alpha"));

        // Compose a real atlas from synthetic strips, exactly as the window does.
        QVector<AtlasComposer::RowInput> inputs;
        for (const AtlasComposer::RowSpec &spec : AtlasComposer::rows()) {
            const int count = AtlasComposer::frameCount(spec.row);
            QImage strip(count * PetAtlas::CellWidth, PetAtlas::CellHeight,
                         QImage::Format_ARGB32);
            strip.fill(AtlasComposer::defaultChromaKey());
            QPainter painter(&strip);
            painter.setCompositionMode(QPainter::CompositionMode_Source);
            for (int column = 0; column < count; ++column) {
                painter.fillRect(column * PetAtlas::CellWidth + 60, PetAtlas::CellHeight - 100,
                                 60, 90, QColor(200, 40, 40));
            }
            painter.end();
            inputs.append({spec.row, strip});
        }
        const AtlasComposer::Result composed = AtlasComposer::compose(inputs, {});
        QVERIFY(composed.isInstallable());

        AtlasAssemblerWindow *window = showAndFindAssembler(controller);
        QVERIFY(window);
        emit window->installRequested(composed.atlas,
                                      {QStringLiteral("spud"), QStringLiteral("Spud"), QString()});

        QCOMPARE(fx.notifier.lastWarning, QString());
        QCOMPARE(fx.notifier.warnings, 0);
        QCOMPARE(settings.selectedPetId(), QStringLiteral("spud"));
        // The staging directory was temporary, so the installed copy has to live under
        // the injected user pets root rather than wherever it was written.
        QVERIFY(QFile::exists(QDir(QDir(fx.config.path()).filePath(QStringLiteral("userpets")))
                                  .filePath(QStringLiteral("spud/pet.json"))));
    }

    // A predictable failure still has to be reported rather than swallowed: the writer
    // refuses an id the validator would reject, and the user hears about it.
    void anInvalidAssembledPetIsReportedAndInstallsNothing()
    {
        Fixture fx;
        QVERIFY(createPet(fx.pets.path(), QStringLiteral("alpha"), QStringLiteral("Alpha")));
        AppSettings settings(fx.settingsPath());
        Localization localization(&settings);

        AppController controller(fx.deps(&settings), AppRunMode::RuntimeCheck);
        QVERIFY(controller.start());

        AtlasAssemblerWindow *window = showAndFindAssembler(controller);
        QVERIFY(window);
        QImage atlas(PetAtlas::Width, PetAtlas::Height, QImage::Format_ARGB32);
        atlas.fill(Qt::transparent);
        emit window->installRequested(atlas, {QStringLiteral("Not Valid"),
                                             QStringLiteral("Nope"), QString()});

        QCOMPARE(settings.selectedPetId(), QStringLiteral("alpha"));
        QVERIFY(fx.notifier.warnings > 0);
    }
};

QTEST_MAIN(AppControllerTest)

#include "test_app_controller.moc"
