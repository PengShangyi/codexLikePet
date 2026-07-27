#include "settings/SettingsWindow.h"

#include "platform/SystemSymbols.h"
#include "resources/PetLibrary.h"
#include "settings/AppSettings.h"
#include "settings/Localization.h"
#include "pet/WindowPlacement.h"
#include "settings/PetPreviewWidget.h"
#include "ui/Disclosure.h"
#include "ui/InlineBanner.h"
#include "ui/SettingsCard.h"
#include "ui/SettingsPage.h"
#include "ui/SettingsRow.h"
#include "ui/Theme.h"
#include "ui/ValueSlider.h"

#include <QAction>
#include <QButtonGroup>
#include <QCoreApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QMenu>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QGuiApplication>
#include <QHideEvent>
#include <QMoveEvent>
#include <QResizeEvent>
#include <QScreen>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QTimeEdit>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

namespace {

// Checkboxes carry no text of their own: the row label already names them, and
// setting both would print the label twice. A bare right-aligned indicator is also
// what System Settings shows.
QCheckBox *makeRowCheck(QWidget *parent)
{
    auto *check = new QCheckBox(parent);
    check->setText(QString());
    // Rows give controls a minimum width so neighbouring combos line up; a lone
    // checkbox indicator must not be stretched by it.
    check->setMinimumWidth(1);
    return check;
}

}  // namespace

SettingsWindow::SettingsWindow(AppSettings *settings, Localization *localization, QWidget *parent)
    : QWidget(parent, Qt::Window)
    , m_settings(settings)
    , m_localization(localization)
    , m_theme(new ThemeWatcher(this))
    , m_geometrySaveTimer(new QTimer(this))
{
    setAttribute(Qt::WA_QuitOnClose, false);
    setMinimumSize(Theme::Metrics::windowMinWidth, Theme::Metrics::windowMinHeight);
    m_geometrySaveTimer->setSingleShot(true);
    m_geometrySaveTimer->setInterval(400);
    connect(m_geometrySaveTimer, &QTimer::timeout, this, &SettingsWindow::saveGeometry);
    buildUi();
    bindSettings();
    retranslate();
    restoreGeometry();
    connect(localization, &Localization::languageChanged, this, &SettingsWindow::retranslate);
    connect(m_theme, &ThemeWatcher::schemeChanged, this, &SettingsWindow::applyTheme);
}

void SettingsWindow::buildUi()
{
    setObjectName(QStringLiteral("settingsRoot"));
    // Not a QFrame, so the stylesheet's background rule needs this to bite.
    setAttribute(Qt::WA_StyledBackground, true);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_navBar = new QWidget(this);
    m_navBar->setObjectName(QStringLiteral("navBar"));
    m_navBar->setAttribute(Qt::WA_StyledBackground, true);
    auto *navLayout = new QHBoxLayout(m_navBar);
    navLayout->setContentsMargins(Theme::Metrics::navBarPaddingH,
                                  Theme::Metrics::navBarPaddingV,
                                  Theme::Metrics::navBarPaddingH,
                                  Theme::Metrics::navBarPaddingV);
    navLayout->setSpacing(Theme::Metrics::navItemSpacing);
    navLayout->addStretch();
    navLayout->addStretch();
    root->addWidget(m_navBar);

    m_navGroup = new QButtonGroup(this);
    m_navGroup->setExclusive(true);

    m_stack = new QStackedWidget(this);
    root->addWidget(m_stack, 1);

    buildPetPage();
    buildAppearancePage();
    buildBehaviorPage();
    buildEnvironmentPage();
    buildGeneralPage();

    connect(m_navGroup, &QButtonGroup::idClicked, m_stack, &QStackedWidget::setCurrentIndex);
    // Icons are tinted per selection state, so the pair that changed both need
    // re-rendering after every switch.
    connect(m_navGroup, &QButtonGroup::idClicked, this, [this] { updateNavIcons(); });
    if (!m_navButtons.isEmpty()) {
        m_navButtons.first()->setChecked(true);
        m_stack->setCurrentIndex(0);
    }
}

SettingsPage *SettingsWindow::addPage(TextKey title, const QString &symbolName)
{
    auto *page = new SettingsPage(m_stack);
    const int index = m_stack->count();
    m_stack->addWidget(page);

    auto *button = new QToolButton(m_navBar);
    button->setObjectName(QStringLiteral("navItem"));
    button->setCheckable(true);
    button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    button->setIconSize(QSize(Theme::Metrics::navIconSize, Theme::Metrics::navIconSize));
    button->setFont(Theme::navItemFont(font()));
    button->setCursor(Qt::PointingHandCursor);
    button->setFocusPolicy(Qt::StrongFocus);
    m_navGroup->addButton(button, index);
    m_navButtons.append(button);
    m_navKeys.append(title);
    m_navSymbols.append(symbolName);

    // Insert between the two stretches so the row stays centred.
    auto *navLayout = static_cast<QHBoxLayout *>(m_navBar->layout());
    navLayout->insertWidget(navLayout->count() - 1, button);
    return page;
}

void SettingsWindow::buildPetPage()
{
    SettingsPage *page = addPage(TextKey::Pet, QStringLiteral("pawprint.fill"));

    auto *petCard = new SettingsCard(TextKey::SectionCurrentPet, m_localization, page);
    petCard->setObjectName(QStringLiteral("petCard"));
    m_petCombo = new QComboBox;
    auto *petRow = new SettingsRow(TextKey::Pet, m_petCombo, m_localization);
    petRow->setObjectName(QStringLiteral("petRow"));
    petCard->addRow(petRow);
    m_rows.append(petRow);

    auto *buttonStrip = new QWidget(petCard);
    auto *buttonLayout = new QHBoxLayout(buttonStrip);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    m_importButton = new QPushButton(buttonStrip);
    m_importButton->setObjectName(QStringLiteral("importButton"));
    auto *importMenu = new QMenu(m_importButton);
    QAction *packageAction = importMenu->addAction(QString());
    packageAction->setObjectName(QStringLiteral("importPackageAction"));
    QAction *directoryAction = importMenu->addAction(QString());
    directoryAction->setObjectName(QStringLiteral("importDirectoryAction"));
    m_importButton->setMenu(importMenu);
    m_removeButton = new QPushButton(buttonStrip);
    m_removeButton->setObjectName(QStringLiteral("removeButton"));
    m_removeButton->setEnabled(false);
    // Sits with the import buttons because it answers the question they raise:
    // importing a pet is no help until you have one to import.
    m_petGuideButton = new QPushButton(buttonStrip);
    m_petGuideButton->setObjectName(QStringLiteral("petGuideButton"));
    buttonLayout->addWidget(m_importButton);
    buttonLayout->addWidget(m_removeButton);
    buttonLayout->addWidget(m_petGuideButton);
    buttonLayout->addStretch();
    petCard->addContent(buttonStrip);
    page->addCard(petCard);
    m_cards.append(petCard);

    m_validationBanner = new InlineBanner(page);
    m_validationBanner->setObjectName(QStringLiteral("validationBanner"));
    page->addContent(m_validationBanner);

    m_preview = new PetPreviewWidget(page);
    page->addStretchingContent(m_preview);

    // The atlas/animation/clip pickers and the season-by-phase fallback table are
    // pet-authoring tools. They used to be most of what this page showed every user,
    // so they live behind a collapsed disclosure now.
    m_resourceDetails = new Disclosure(TextKey::ResourceDetails, m_localization, page);
    m_resourceDetails->setObjectName(QStringLiteral("resourceDetails"));

    m_previewAtlasCombo = new QComboBox;
    m_previewStateCombo = new QComboBox;
    m_previewClipCombo = new QComboBox;
    auto *atlasRow = new SettingsRow(TextKey::PreviewVariant, m_previewAtlasCombo, m_localization);
    auto *stateRow = new SettingsRow(TextKey::PreviewAnimation, m_previewStateCombo, m_localization);
    auto *clipRow = new SettingsRow(TextKey::PreviewClip, m_previewClipCombo, m_localization);
    for (SettingsRow *row : {atlasRow, stateRow, clipRow}) {
        m_resourceDetails->contentLayout()->addWidget(row);
        m_rows.append(row);
    }

    m_resourceSummary = new QPlainTextEdit;
    m_resourceSummary->setObjectName(QStringLiteral("resourceSummary"));
    m_resourceSummary->setReadOnly(true);
    m_resourceSummary->setFrameShape(QFrame::NoFrame);
    // Fixed rather than capped: the table scrolls internally, and on a page whose
    // preview is already claiming the leftover height a floor is the only thing
    // keeping the box from opening as a single-line sliver.
    m_resourceSummary->setMinimumHeight(105);
    m_resourceSummary->setMaximumHeight(105);
    m_resourceDetails->contentLayout()->addWidget(m_resourceSummary);

    page->addContent(m_resourceDetails);

    // This section sits at the bottom of the page, under a preview that absorbs
    // the leftover height, so at the smallest window everything it reveals starts
    // below the fold: expanding it looked like nothing more than a scrollbar
    // appearing. Deferred by one turn because the offset to scroll to only exists
    // once the reveal's posted LayoutRequest has been through the column.
    connect(m_resourceDetails, &Disclosure::expandedChanged, this, [this, page](bool expanded) {
        if (!expanded) return;
        QTimer::singleShot(0, this, [this, page] {
            if (m_resourceDetails->isExpanded()) page->scrollToContent(m_resourceDetails);
        });
    });
}

void SettingsWindow::buildAppearancePage()
{
    SettingsPage *page = addPage(TextKey::Appearance, QStringLiteral("slider.horizontal.3"));

    auto *display = new SettingsCard(TextKey::SectionDisplay, m_localization, page);
    display->setObjectName(QStringLiteral("displayCard"));

    m_scaleSlider = new ValueSlider(50, 200, [](int value) {
        return QStringLiteral("%1%").arg(value);
    });
    m_speedSlider = new ValueSlider(50, 200, [](int value) {
        return QStringLiteral("%1×").arg(value / 100.0, 0, 'f', 2);
    });
    // Floor of 30%, matching AppSettings: a fully transparent pet cannot be clicked
    // or found again.
    m_opacitySlider = new ValueSlider(30, 100, [](int value) {
        return QStringLiteral("%1%").arg(value);
    });
    m_topCheck = makeRowCheck(nullptr);
    m_motionCombo = new QComboBox;

    struct RowSpec {
        TextKey key;
        QWidget *control;
        const char *name;
    };
    const RowSpec specs[] = {
        {TextKey::Size, m_scaleSlider, "sizeRow"},
        {TextKey::AnimationSpeed, m_speedSlider, "speedRow"},
        {TextKey::Opacity, m_opacitySlider, "opacityRow"},
        {TextKey::AlwaysOnTop, m_topCheck, "alwaysOnTopRow"},
        {TextKey::ReducedMotion, m_motionCombo, "motionRow"},
    };
    for (const RowSpec &spec : specs) {
        auto *row = new SettingsRow(spec.key, spec.control, m_localization);
        row->setObjectName(QLatin1String(spec.name));
        display->addRow(row);
        m_rows.append(row);
    }
    page->addCard(display);
    m_cards.append(display);
}

void SettingsWindow::buildBehaviorPage()
{
    SettingsPage *page = addPage(TextKey::Behavior, QStringLiteral("hand.tap.fill"));

    auto *interaction = new SettingsCard(TextKey::SectionWindowInteraction, m_localization, page);
    interaction->setObjectName(QStringLiteral("interactionCard"));
    m_lockCheck = makeRowCheck(nullptr);
    auto *lockRow = new SettingsRow(TextKey::LockPosition, m_lockCheck, m_localization);
    lockRow->setObjectName(QStringLiteral("lockPositionRow"));
    // Says outright that this is not click-through, which the project contract rules
    // out and which is the obvious thing to assume a position lock means.
    lockRow->setDescriptionKey(TextKey::LockPositionNote);
    interaction->addRow(lockRow);
    m_rows.append(lockRow);

    auto *resetStrip = new QWidget(interaction);
    auto *resetLayout = new QHBoxLayout(resetStrip);
    resetLayout->setContentsMargins(0, 0, 0, 0);
    m_resetButton = new QPushButton(resetStrip);
    m_resetButton->setObjectName(QStringLiteral("resetPositionButton"));
    resetLayout->addWidget(m_resetButton);
    resetLayout->addStretch();
    interaction->addContent(resetStrip);
    page->addCard(interaction);
    m_cards.append(interaction);

    auto *activity = new SettingsCard(TextKey::SectionActivity, m_localization, page);
    activity->setObjectName(QStringLiteral("activityCard"));
    m_typingCheck = makeRowCheck(nullptr);
    auto *typingRow = new SettingsRow(TextKey::TypingDetection, m_typingCheck, m_localization);
    typingRow->setObjectName(QStringLiteral("typingRow"));
    typingRow->setDescriptionKey(TextKey::TypingPrivacyNote);
    activity->addRow(typingRow);
    m_rows.append(typingRow);
    page->addCard(activity);
    m_cards.append(activity);
}

void SettingsWindow::buildEnvironmentPage()
{
    SettingsPage *page = addPage(TextKey::Environment, QStringLiteral("sun.max.fill"));

    auto *card = new SettingsCard(TextKey::SectionSeasonPhase, m_localization, page);
    card->setObjectName(QStringLiteral("seasonCard"));

    m_environmentRow = new SettingsRow(TextKey::CurrentEnvironment, nullptr, m_localization);
    m_environmentRow->setObjectName(QStringLiteral("currentEnvironmentRow"));
    card->addRow(m_environmentRow);
    m_rows.append(m_environmentRow);

    m_hemisphereCombo = new QComboBox;
    m_dayStart = new QTimeEdit;
    m_nightStart = new QTimeEdit;
    m_dayStart->setDisplayFormat(QStringLiteral("HH:mm"));
    m_nightStart->setDisplayFormat(QStringLiteral("HH:mm"));

    struct RowSpec {
        TextKey key;
        QWidget *control;
        const char *name;
    };
    const RowSpec specs[] = {
        {TextKey::Hemisphere, m_hemisphereCombo, "hemisphereRow"},
        {TextKey::DayStarts, m_dayStart, "dayStartRow"},
        {TextKey::NightStarts, m_nightStart, "nightStartRow"},
    };
    for (const RowSpec &spec : specs) {
        auto *row = new SettingsRow(spec.key, spec.control, m_localization);
        row->setObjectName(QLatin1String(spec.name));
        card->addRow(row);
        m_rows.append(row);
    }
    page->addCard(card);
    m_cards.append(card);
}

void SettingsWindow::buildGeneralPage()
{
    SettingsPage *page = addPage(TextKey::General, QStringLiteral("gearshape.fill"));

    auto *startup = new SettingsCard(TextKey::SectionStartup, m_localization, page);
    startup->setObjectName(QStringLiteral("startupCard"));
    m_loginCheck = makeRowCheck(nullptr);
    auto *loginRow = new SettingsRow(TextKey::LaunchAtLogin, m_loginCheck, m_localization);
    loginRow->setObjectName(QStringLiteral("launchAtLoginRow"));
    startup->addRow(loginRow);
    m_rows.append(loginRow);
    page->addCard(startup);
    m_cards.append(startup);

    auto *interfaceCard = new SettingsCard(TextKey::SectionInterface, m_localization, page);
    interfaceCard->setObjectName(QStringLiteral("interfaceCard"));
    m_languageCombo = new QComboBox;
    auto *languageRow = new SettingsRow(TextKey::Language, m_languageCombo, m_localization);
    languageRow->setObjectName(QStringLiteral("languageRow"));
    interfaceCard->addRow(languageRow);
    m_rows.append(languageRow);
    page->addCard(interfaceCard);
    m_cards.append(interfaceCard);

    auto *about = new SettingsCard(TextKey::SectionAbout, m_localization, page);
    about->setObjectName(QStringLiteral("aboutCard"));
    auto *versionRow = new SettingsRow(TextKey::AboutVersionLabel, nullptr, m_localization);
    versionRow->setObjectName(QStringLiteral("versionRow"));
    // Same source AboutWindow uses; main.cpp seeds it from POTATO_VERSION before
    // AppController builds this window.
    versionRow->setValueText(QCoreApplication::applicationVersion());
    about->addRow(versionRow);
    m_rows.append(versionRow);

    // The welcome guide lives here rather than in the tray menu: it is a first-run
    // artefact, and a permanent menu slot for it was one item too many.
    auto *welcomeStrip = new QWidget(about);
    auto *welcomeLayout = new QHBoxLayout(welcomeStrip);
    welcomeLayout->setContentsMargins(0, 0, 0, 0);
    m_welcomeButton = new QPushButton(welcomeStrip);
    m_welcomeButton->setObjectName(QStringLiteral("welcomeButton"));
    welcomeLayout->addWidget(m_welcomeButton);
    welcomeLayout->addStretch();
    about->addContent(welcomeStrip);
    page->addCard(about);
    m_cards.append(about);
}

void SettingsWindow::bindSettings()
{
    m_scaleSlider->setValue(qRound(m_settings->scale() * 100));
    m_speedSlider->setValue(qRound(m_settings->animationSpeed() * 100));
    m_opacitySlider->setValue(qRound(m_settings->opacity() * 100));
    m_topCheck->setChecked(m_settings->alwaysOnTop());
    m_lockCheck->setChecked(m_settings->positionLocked());
    m_loginCheck->setChecked(m_settings->launchAtLogin());
    m_typingCheck->setChecked(m_settings->typingDetectionEnabled());
    m_dayStart->setTime(m_settings->dayStartsAt());
    m_nightStart->setTime(m_settings->nightStartsAt());
    // The motion, hemisphere, and language combos are seeded in retranslate(), which
    // is where their items are created; setting an index here would be a no-op on an
    // empty combo.

    connect(m_scaleSlider, &ValueSlider::valueChanged, this, [this](int value) {
        m_settings->setScale(value / 100.0);
    });
    connect(m_speedSlider, &ValueSlider::valueChanged, this, [this](int value) {
        m_settings->setAnimationSpeed(value / 100.0);
    });
    connect(m_opacitySlider, &ValueSlider::valueChanged, this, [this](int value) {
        m_settings->setOpacity(value / 100.0);
    });
    connect(m_topCheck, &QCheckBox::toggled, m_settings, &AppSettings::setAlwaysOnTop);
    connect(m_lockCheck, &QCheckBox::toggled, m_settings, &AppSettings::setPositionLocked);
    connect(m_loginCheck, &QCheckBox::toggled, m_settings, &AppSettings::setLaunchAtLogin);
    connect(m_typingCheck, &QCheckBox::toggled, m_settings, &AppSettings::setTypingDetectionEnabled);
    // Two-way: a failed login-item registration or a denied Input Monitoring prompt
    // reverts the setting, and the box has to follow it back.
    //
    // Re-seeded from AppSettings under a blocker, never from the signal argument.
    // The revert is emitted from *inside* this signal's own delivery, and a nested
    // emission does not rewrite the outer frame's argument -- so the outer delivery
    // arrives after the revert still carrying the rejected value. Trusting it
    // re-checked the box, whose toggled() wrote the rejection straight back, and
    // the whole reject-and-warn path ran again: one modal per round, unbounded,
    // which is what made Cancel on the Input Monitoring dialog reopen it forever.
    // Blocking the box on the way in is the other half; without it the correction
    // itself re-arms the path it is correcting.
    const auto syncCheck = [](QCheckBox *box, bool value) {
        const QSignalBlocker blocker(box);
        box->setChecked(value);
    };
    connect(m_settings, &AppSettings::launchAtLoginChanged, this, [this, syncCheck] {
        syncCheck(m_loginCheck, m_settings->launchAtLogin());
    });
    connect(m_settings, &AppSettings::typingDetectionEnabledChanged, this, [this, syncCheck] {
        syncCheck(m_typingCheck, m_settings->typingDetectionEnabled());
    });
    connect(m_motionCombo, &QComboBox::currentIndexChanged, this, [this](int value) { m_settings->setMotionPreference(static_cast<MotionPreference>(value)); });
    connect(m_hemisphereCombo, &QComboBox::currentIndexChanged, this, [this](int value) { m_settings->setHemisphere(static_cast<Hemisphere>(value)); });
    connect(m_dayStart, &QTimeEdit::timeChanged, m_settings, &AppSettings::setDayStartsAt);
    connect(m_nightStart, &QTimeEdit::timeChanged, m_settings, &AppSettings::setNightStartsAt);
    connect(m_languageCombo, &QComboBox::currentIndexChanged, this, [this](int value) { m_settings->setLanguage(static_cast<AppLanguage>(value)); });
    connect(m_resetButton, &QPushButton::clicked, this, &SettingsWindow::resetPositionRequested);
    connect(m_welcomeButton, &QPushButton::clicked, this, &SettingsWindow::welcomeRequested);
    connect(m_importButton->menu()->findChild<QAction *>(QStringLiteral("importPackageAction")),
            &QAction::triggered,
            this,
            &SettingsWindow::importPackageRequested);
    connect(m_importButton->menu()->findChild<QAction *>(QStringLiteral("importDirectoryAction")),
            &QAction::triggered,
            this,
            &SettingsWindow::importDirectoryRequested);
    connect(m_removeButton, &QPushButton::clicked, this, &SettingsWindow::removePetRequested);
    connect(m_petGuideButton, &QPushButton::clicked, this, &SettingsWindow::petGuideRequested);
    connect(m_petCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        const QString id = index >= 0 ? m_petCombo->itemData(index).toString() : QString();
        const bool builtIn = index >= 0 && m_petCombo->itemData(index, Qt::UserRole + 1).toBool();
        m_removeButton->setEnabled(!id.isEmpty() && !builtIn);
        if (!id.isEmpty()) emit petSelected(id);
    });
    connect(m_previewAtlasCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index >= 0) {
            const QSignalBlocker blocker(m_previewClipCombo);
            m_previewClipCombo->setCurrentIndex(0);
            emit previewAtlasSelected(m_previewAtlasCombo->itemData(index).toString());
        }
    });
    connect(m_previewStateCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index >= 0) {
            const QSignalBlocker blocker(m_previewClipCombo);
            m_previewClipCombo->setCurrentIndex(0);
            m_preview->setState(static_cast<V2AnimationState>(m_previewStateCombo->itemData(index).toInt()));
        }
    });
    connect(m_previewClipCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index <= 0) {
            m_preview->setState(static_cast<V2AnimationState>(m_previewStateCombo->currentData().toInt()));
            return;
        }
        emit previewClipSelected(m_previewClipCombo->itemData(index).toString());
    });
}

void SettingsWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (!m_themeApplied) applyTheme();
    // The saved rect may predate a display change, so re-fit on every show rather
    // than trusting what was restored at construction.
    const QRect available = availableGeometry();
    const QRect fitted = WindowPlacement::fitToAvailableGeometry(geometry(), available);
    if (fitted != geometry()) {
        m_restoringGeometry = true;
        setGeometry(fitted);
        m_restoringGeometry = false;
    }
}

void SettingsWindow::hideEvent(QHideEvent *event)
{
    // Flush immediately: the window may not be shown again this session, and the
    // pending coalesced write would be lost.
    if (m_geometrySaveTimer->isActive()) {
        m_geometrySaveTimer->stop();
        saveGeometry();
    }
    QWidget::hideEvent(event);
}

void SettingsWindow::moveEvent(QMoveEvent *event)
{
    QWidget::moveEvent(event);
    scheduleGeometrySave();
}

void SettingsWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    scheduleGeometrySave();
}

QRect SettingsWindow::availableGeometry() const
{
    const QScreen *screen = QGuiApplication::primaryScreen();
    return screen ? screen->availableGeometry() : QRect(0, 0, 1440, 900);
}

void SettingsWindow::restoreGeometry()
{
    const QRect available = availableGeometry();
    const QSize initial = size().expandedTo(minimumSize());
    QRect target(WindowPlacement::centeredPosition(initial, available), initial);
    if (m_settings->hasSettingsGeometry()) {
        const QRect saved = m_settings->settingsGeometry();
        // Discard a rect saved on a display that is gone; keep and clamp one that
        // merely hangs off an edge.
        if (WindowPlacement::isUsableSavedGeometry(saved, available)) {
            target = WindowPlacement::fitToAvailableGeometry(saved, available);
        }
    }
    m_restoringGeometry = true;
    setGeometry(target);
    m_restoringGeometry = false;
}

void SettingsWindow::scheduleGeometrySave()
{
    // Restoring must not immediately re-save what it just read, or a discarded
    // off-screen rect would be overwritten by its clamped form before the user ever
    // moved the window.
    if (m_restoringGeometry) return;
    m_geometrySaveTimer->start();
}

void SettingsWindow::saveGeometry()
{
    m_settings->setSettingsGeometry(geometry());
}

void SettingsWindow::applyTheme()
{
    m_themeApplied = true;
    // Scoped to this window's subtree, never qApp: the pet window and the speech
    // bubble are custom-painted and must stay untouched.
    setStyleSheet(Theme::styleSheet(m_theme->scheme()));
    m_preview->setColorScheme(m_theme->scheme());
    updateNavIcons();
}

void SettingsWindow::updateNavIcons()
{
    const Theme::Palette palette = Theme::palette(m_theme->scheme());
    for (int index = 0; index < m_navButtons.size(); ++index) {
        QToolButton *button = m_navButtons.at(index);
        const bool selected = button->isChecked();
        // SF Symbols come back as fixed bitmaps, so they are re-tinted per scheme
        // and per selection state rather than relying on NSImage template behavior.
        const QIcon icon = SystemSymbols::icon(m_navSymbols.at(index),
                                               Theme::Metrics::navIconSize,
                                               selected ? palette.accentText : palette.textSecondary);
        // A null icon is expected on any platform that cannot supply the symbol; the
        // button then renders text-only, which still reads correctly.
        button->setIcon(icon);
    }
}

void SettingsWindow::rebuildPreviewStateItems()
{
    const QSignalBlocker stateBlocker(m_previewStateCombo);
    const int previousState = m_previewStateCombo->currentData().toInt();
    m_previewStateCombo->clear();
    const bool zh = m_localization->usesChinese();
    const QVector<QPair<V2AnimationState, QString>> states = {
        {V2AnimationState::Idle, zh ? QStringLiteral("空闲") : QStringLiteral("Idle")},
        {V2AnimationState::RunningRight, zh ? QStringLiteral("向右奔跑") : QStringLiteral("Running right")},
        {V2AnimationState::RunningLeft, zh ? QStringLiteral("向左奔跑") : QStringLiteral("Running left")},
        {V2AnimationState::Waving, zh ? QStringLiteral("挥手") : QStringLiteral("Waving")},
        {V2AnimationState::Jumping, zh ? QStringLiteral("跳跃") : QStringLiteral("Jumping")},
        {V2AnimationState::Failed, zh ? QStringLiteral("失败") : QStringLiteral("Failed")},
        {V2AnimationState::Waiting, zh ? QStringLiteral("等待") : QStringLiteral("Waiting")},
        {V2AnimationState::Running, zh ? QStringLiteral("工作") : QStringLiteral("Working")},
        {V2AnimationState::Review, zh ? QStringLiteral("检查") : QStringLiteral("Review")},
    };
    int selectedStateIndex = 0;
    for (const auto &[state, label] : states) {
        m_previewStateCombo->addItem(label, static_cast<int>(state));
        if (static_cast<int>(state) == previousState) selectedStateIndex = m_previewStateCombo->count() - 1;
    }
    m_previewStateCombo->setCurrentIndex(selectedStateIndex);
}

void SettingsWindow::retranslate()
{
    setWindowTitle(QStringLiteral("Potato — %1")
                       .arg(m_localization->text(TextKey::Settings).remove(QChar(0x2026))));

    for (int index = 0; index < m_navButtons.size(); ++index) {
        const QString title = m_localization->text(m_navKeys.at(index));
        m_navButtons.at(index)->setText(title);
        m_navButtons.at(index)->setAccessibleName(title);
    }

    for (SettingsCard *card : m_cards) card->retranslate();
    // Rows inside disclosures are not owned by a card, so walk every row too. The
    // duplicate retranslate() on card-owned rows is idempotent.
    for (SettingsRow *row : m_rows) row->retranslate();
    m_resourceDetails->retranslate();

    if (m_petCombo->count() == 0) {
        m_petCombo->addItem(m_localization->text(TextKey::NoPetSelected), QString());
    } else if (m_petCombo->count() == 1 && m_petCombo->itemData(0).toString().isEmpty()) {
        m_petCombo->setItemText(0, m_localization->text(TextKey::NoPetSelected));
    }
    m_importButton->setText(m_localization->text(TextKey::ImportPet));
    m_removeButton->setText(m_localization->text(TextKey::RemovePet));
    m_petGuideButton->setText(m_localization->text(TextKey::PetGuideButton));
    if (QAction *action = m_importButton->menu()->findChild<QAction *>(QStringLiteral("importPackageAction"))) action->setText(m_localization->text(TextKey::ImportPackage));
    if (QAction *action = m_importButton->menu()->findChild<QAction *>(QStringLiteral("importDirectoryAction"))) action->setText(m_localization->text(TextKey::ImportDirectory));
    m_resourceSummary->setPlaceholderText(m_localization->text(TextKey::ResourceFallbacks));
    if (m_previewClipCombo->count() > 0) {
        m_previewClipCombo->setItemText(0, m_localization->text(TextKey::UseStandardAnimation));
    }
    m_resetButton->setText(m_localization->text(TextKey::ResetPosition));
    m_welcomeButton->setText(m_localization->text(TextKey::WelcomeMenuItem));

    rebuildPreviewStateItems();

    // These three combos are rebuilt rather than relabelled, so repopulating them
    // has to happen under a signal blocker: otherwise clear() emits
    // currentIndexChanged(-1) and the handler writes that back as the user's choice.
    //
    // The selection is restored from AppSettings rather than from the combo's own
    // index. Reading the index back looks equivalent but is not: retranslate() also
    // runs once from the constructor, where the items do not exist yet, so the index
    // is still -1 and every one of these would settle on its first entry. That is
    // why reopening Settings used to show "Follow system", "Northern", and "System
    // default" no matter what had been chosen.
    const QSignalBlocker motionBlocker(m_motionCombo);
    const QSignalBlocker hemisphereBlocker(m_hemisphereCombo);
    const QSignalBlocker languageBlocker(m_languageCombo);
    m_motionCombo->clear();
    m_motionCombo->addItems({m_localization->text(TextKey::FollowSystem), m_localization->text(TextKey::ReduceMotion), m_localization->text(TextKey::FullMotion)});
    m_motionCombo->setCurrentIndex(static_cast<int>(m_settings->motionPreference()));
    m_hemisphereCombo->clear();
    m_hemisphereCombo->addItems({m_localization->text(TextKey::North), m_localization->text(TextKey::South)});
    m_hemisphereCombo->setCurrentIndex(static_cast<int>(m_settings->hemisphere()));
    m_languageCombo->clear();
    m_languageCombo->addItems({m_localization->text(TextKey::SystemLanguage), m_localization->text(TextKey::English), m_localization->text(TextKey::SimplifiedChinese)});
    m_languageCombo->setCurrentIndex(static_cast<int>(m_settings->language()));

    // The controls that are not row-owned still need naming by hand; every other
    // control gets its accessible name from its SettingsRow.
    m_importButton->setAccessibleName(m_localization->text(TextKey::ImportPet));
    m_removeButton->setAccessibleName(m_localization->text(TextKey::RemovePet));
    m_petGuideButton->setAccessibleName(m_localization->text(TextKey::PetGuideButton));
    m_resetButton->setAccessibleName(m_localization->text(TextKey::ResetPosition));
    m_welcomeButton->setAccessibleName(m_localization->text(TextKey::WelcomeMenuItem));
    m_resourceSummary->setAccessibleName(m_localization->text(TextKey::ResourceFallbacks));
}

void SettingsWindow::setPets(const QVector<PetRecord> &pets, const QString &selectedId)
{
    const QSignalBlocker blocker(m_petCombo);
    m_petCombo->clear();
    int selectedIndex = -1;
    for (const PetRecord &record : pets) {
        m_petCombo->addItem(record.package.displayName, record.package.id);
        m_petCombo->setItemData(m_petCombo->count() - 1, record.builtIn, Qt::UserRole + 1);
        if (record.package.id == selectedId) selectedIndex = m_petCombo->count() - 1;
    }
    if (pets.isEmpty()) {
        m_petCombo->addItem(m_localization->text(TextKey::NoPetSelected), QString());
        m_petCombo->setEnabled(false);
        m_removeButton->setEnabled(false);
    } else {
        m_petCombo->setEnabled(true);
        m_petCombo->setCurrentIndex(selectedIndex >= 0 ? selectedIndex : 0);
        const bool builtIn = m_petCombo->currentData(Qt::UserRole + 1).toBool();
        m_removeButton->setEnabled(!builtIn);
    }
}

QString SettingsWindow::selectedPetId() const
{
    return m_petCombo->currentData().toString();
}

void SettingsWindow::setPreviewAtlas(QSharedPointer<PetAtlas> atlas, bool smoothRendering)
{
    if (atlas) m_preview->setAtlas(std::move(atlas), smoothRendering);
    else m_preview->clear();
}

void SettingsWindow::setPreviewOptions(const QStringList &labels,
                                       const QStringList &paths,
                                       const QString &selectedPath)
{
    const QSignalBlocker blocker(m_previewAtlasCombo);
    m_previewAtlasCombo->clear();
    const int count = std::min(labels.size(), paths.size());
    int selectedIndex = 0;
    for (int index = 0; index < count; ++index) {
        m_previewAtlasCombo->addItem(labels.at(index), paths.at(index));
        if (paths.at(index) == selectedPath) selectedIndex = index;
    }
    m_previewAtlasCombo->setEnabled(count > 0);
    if (count > 0) m_previewAtlasCombo->setCurrentIndex(selectedIndex);
}

void SettingsWindow::setPreviewClipOptions(const QStringList &labels, const QStringList &keys)
{
    const QSignalBlocker blocker(m_previewClipCombo);
    m_previewClipCombo->clear();
    m_previewClipCombo->addItem(m_localization->text(TextKey::UseStandardAnimation), QString());
    const int count = std::min(labels.size(), keys.size());
    for (int index = 0; index < count; ++index) {
        m_previewClipCombo->addItem(labels.at(index), keys.at(index));
    }
    m_previewClipCombo->setEnabled(count > 0);
    m_previewClipCombo->setCurrentIndex(0);
}

void SettingsWindow::setPreviewClip(QSharedPointer<AnimationClip> clip, bool smoothRendering)
{
    if (clip) m_preview->setClip(std::move(clip), smoothRendering);
}

void SettingsWindow::setResourceSummary(const QString &summary)
{
    m_resourceSummary->setPlainText(summary);
}

QString SettingsWindow::resourceSummary() const
{
    return m_resourceSummary->toPlainText();
}

void SettingsWindow::setValidationReport(const QString &report, bool error)
{
    m_validationBanner->setMessage(report,
                                   error ? InlineBanner::Severity::Error
                                         : InlineBanner::Severity::Info);
}

void SettingsWindow::setReducedMotion(bool reduced)
{
    m_preview->setReducedMotion(reduced);
}

void SettingsWindow::setEnvironmentSummary(const QString &summary)
{
    m_environmentRow->setValueText(summary);
}
