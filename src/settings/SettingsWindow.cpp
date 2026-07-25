#include "settings/SettingsWindow.h"

#include "settings/AppSettings.h"
#include "settings/Localization.h"
#include "settings/PetPreviewWidget.h"
#include "resources/PetLibrary.h"

#include <QCheckBox>
#include <QAction>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QSignalBlocker>
#include <QSlider>
#include <QTabWidget>
#include <QTimeEdit>
#include <QVBoxLayout>

#include <algorithm>

SettingsWindow::SettingsWindow(AppSettings *settings, Localization *localization, QWidget *parent)
    : QWidget(parent, Qt::Window)
    , m_settings(settings)
    , m_localization(localization)
{
    setAttribute(Qt::WA_QuitOnClose, false);
    setMinimumSize(600, 650);
    buildUi();
    bindSettings();
    retranslate();
    connect(localization, &Localization::languageChanged, this, &SettingsWindow::retranslate);
}

void SettingsWindow::buildUi()
{
    auto *root = new QVBoxLayout(this);
    m_tabs = new QTabWidget(this);
    root->addWidget(m_tabs);

    m_petTab = new QWidget(m_tabs);
    auto *petLayout = new QVBoxLayout(m_petTab);
    m_petCombo = new QComboBox(m_petTab);
    petLayout->addWidget(m_petCombo);
    auto *petButtons = new QHBoxLayout;
    m_importButton = new QPushButton(m_petTab);
    auto *importMenu = new QMenu(m_importButton);
    QAction *packageAction = importMenu->addAction(QString());
    packageAction->setObjectName(QStringLiteral("importPackageAction"));
    QAction *directoryAction = importMenu->addAction(QString());
    directoryAction->setObjectName(QStringLiteral("importDirectoryAction"));
    m_importButton->setMenu(importMenu);
    m_removeButton = new QPushButton(m_petTab);
    m_removeButton->setEnabled(false);
    petButtons->addWidget(m_importButton);
    petButtons->addWidget(m_removeButton);
    petButtons->addStretch();
    petLayout->addLayout(petButtons);
    m_previewForm = new QFormLayout;
    m_previewAtlasLabel = new QLabel(m_petTab);
    m_previewStateLabel = new QLabel(m_petTab);
    m_previewClipLabel = new QLabel(m_petTab);
    m_previewAtlasCombo = new QComboBox(m_petTab);
    m_previewStateCombo = new QComboBox(m_petTab);
    m_previewClipCombo = new QComboBox(m_petTab);
    m_previewForm->addRow(m_previewAtlasLabel, m_previewAtlasCombo);
    m_previewForm->addRow(m_previewStateLabel, m_previewStateCombo);
    m_previewForm->addRow(m_previewClipLabel, m_previewClipCombo);
    petLayout->addLayout(m_previewForm);
    m_preview = new PetPreviewWidget(m_petTab);
    petLayout->addWidget(m_preview, 1);
    m_resourceSummaryLabel = new QLabel(m_petTab);
    petLayout->addWidget(m_resourceSummaryLabel);
    m_resourceSummary = new QPlainTextEdit(m_petTab);
    m_resourceSummary->setReadOnly(true);
    m_resourceSummary->setMaximumHeight(105);
    petLayout->addWidget(m_resourceSummary);
    m_report = new QPlainTextEdit(m_petTab);
    m_report->setReadOnly(true);
    m_report->setMaximumBlockCount(200);
    m_report->hide();
    petLayout->addWidget(m_report);
    m_tabs->addTab(m_petTab, QString());

    m_generalTab = new QWidget(m_tabs);
    m_generalForm = new QFormLayout(m_generalTab);
    m_scaleSlider = new QSlider(Qt::Horizontal, m_generalTab);
    m_scaleSlider->setRange(50, 200);
    m_speedSlider = new QSlider(Qt::Horizontal, m_generalTab);
    m_speedSlider->setRange(50, 200);
    m_topCheck = new QCheckBox(m_generalTab);
    m_loginCheck = new QCheckBox(m_generalTab);
    m_typingCheck = new QCheckBox(m_generalTab);
    m_typingNote = new QLabel(m_generalTab);
    m_typingNote->setWordWrap(true);
    m_motionCombo = new QComboBox(m_generalTab);
    m_hemisphereCombo = new QComboBox(m_generalTab);
    m_dayStart = new QTimeEdit(m_generalTab);
    m_nightStart = new QTimeEdit(m_generalTab);
    m_dayStart->setDisplayFormat(QStringLiteral("HH:mm"));
    m_nightStart->setDisplayFormat(QStringLiteral("HH:mm"));
    m_languageCombo = new QComboBox(m_generalTab);
    m_resetButton = new QPushButton(m_generalTab);
    m_aboutButton = new QPushButton(m_generalTab);
    m_generalForm->addRow(QStringLiteral(" "), m_scaleSlider);
    m_generalForm->addRow(QStringLiteral(" "), m_speedSlider);
    m_generalForm->addRow(m_topCheck);
    m_generalForm->addRow(m_loginCheck);
    m_generalForm->addRow(m_typingCheck);
    m_generalForm->addRow(QStringLiteral(" "), m_typingNote);
    m_generalForm->addRow(QStringLiteral(" "), m_motionCombo);
    m_generalForm->addRow(QStringLiteral(" "), m_hemisphereCombo);
    m_generalForm->addRow(QStringLiteral(" "), m_dayStart);
    m_generalForm->addRow(QStringLiteral(" "), m_nightStart);
    m_generalForm->addRow(QStringLiteral(" "), m_languageCombo);
    m_generalForm->addRow(m_resetButton);
    m_generalForm->addRow(m_aboutButton);
    m_tabs->addTab(m_generalTab, QString());

    m_closeButton = new QPushButton(this);
    root->addWidget(m_closeButton, 0, Qt::AlignRight);
}

void SettingsWindow::bindSettings()
{
    m_scaleSlider->setValue(qRound(m_settings->scale() * 100));
    m_speedSlider->setValue(qRound(m_settings->animationSpeed() * 100));
    m_topCheck->setChecked(m_settings->alwaysOnTop());
    m_loginCheck->setChecked(m_settings->launchAtLogin());
    m_typingCheck->setChecked(m_settings->typingDetectionEnabled());
    m_motionCombo->setCurrentIndex(static_cast<int>(m_settings->motionPreference()));
    m_hemisphereCombo->setCurrentIndex(static_cast<int>(m_settings->hemisphere()));
    m_dayStart->setTime(m_settings->dayStartsAt());
    m_nightStart->setTime(m_settings->nightStartsAt());
    m_languageCombo->setCurrentIndex(static_cast<int>(m_settings->language()));

    connect(m_scaleSlider, &QSlider::valueChanged, this, [this](int value) {
        m_settings->setScale(value / 100.0);
        updateSliderLabels();
    });
    connect(m_speedSlider, &QSlider::valueChanged, this, [this](int value) {
        m_settings->setAnimationSpeed(value / 100.0);
        updateSliderLabels();
    });
    connect(m_topCheck, &QCheckBox::toggled, m_settings, &AppSettings::setAlwaysOnTop);
    connect(m_loginCheck, &QCheckBox::toggled, m_settings, &AppSettings::setLaunchAtLogin);
    connect(m_typingCheck, &QCheckBox::toggled, m_settings, &AppSettings::setTypingDetectionEnabled);
    connect(m_settings, &AppSettings::launchAtLoginChanged, m_loginCheck, &QCheckBox::setChecked);
    connect(m_settings, &AppSettings::typingDetectionEnabledChanged, m_typingCheck, &QCheckBox::setChecked);
    connect(m_motionCombo, &QComboBox::currentIndexChanged, this, [this](int value) { m_settings->setMotionPreference(static_cast<MotionPreference>(value)); });
    connect(m_hemisphereCombo, &QComboBox::currentIndexChanged, this, [this](int value) { m_settings->setHemisphere(static_cast<Hemisphere>(value)); });
    connect(m_dayStart, &QTimeEdit::timeChanged, m_settings, &AppSettings::setDayStartsAt);
    connect(m_nightStart, &QTimeEdit::timeChanged, m_settings, &AppSettings::setNightStartsAt);
    connect(m_languageCombo, &QComboBox::currentIndexChanged, this, [this](int value) { m_settings->setLanguage(static_cast<AppLanguage>(value)); });
    connect(m_resetButton, &QPushButton::clicked, this, &SettingsWindow::resetPositionRequested);
    connect(m_aboutButton, &QPushButton::clicked, this, &SettingsWindow::aboutRequested);
    connect(m_importButton->menu()->findChild<QAction *>(QStringLiteral("importPackageAction")),
            &QAction::triggered,
            this,
            &SettingsWindow::importPackageRequested);
    connect(m_importButton->menu()->findChild<QAction *>(QStringLiteral("importDirectoryAction")),
            &QAction::triggered,
            this,
            &SettingsWindow::importDirectoryRequested);
    connect(m_removeButton, &QPushButton::clicked, this, &SettingsWindow::removePetRequested);
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
    connect(m_closeButton, &QPushButton::clicked, this, &QWidget::hide);
}

void SettingsWindow::retranslate()
{
    setWindowTitle(QStringLiteral("Potato — %1").arg(m_localization->text(TextKey::Settings).remove(QChar(0x2026))));
    m_tabs->setTabText(m_tabs->indexOf(m_petTab), m_localization->text(TextKey::Pet));
    m_tabs->setTabText(m_tabs->indexOf(m_generalTab), m_localization->text(TextKey::General));
    if (m_petCombo->count() == 0) {
        m_petCombo->addItem(m_localization->text(TextKey::NoPetSelected), QString());
    } else if (m_petCombo->count() == 1 && m_petCombo->itemData(0).toString().isEmpty()) {
        m_petCombo->setItemText(0, m_localization->text(TextKey::NoPetSelected));
    }
    m_importButton->setText(m_localization->text(TextKey::ImportPet));
    m_removeButton->setText(m_localization->text(TextKey::RemovePet));
    if (QAction *action = m_importButton->menu()->findChild<QAction *>(QStringLiteral("importPackageAction"))) action->setText(m_localization->text(TextKey::ImportPackage));
    if (QAction *action = m_importButton->menu()->findChild<QAction *>(QStringLiteral("importDirectoryAction"))) action->setText(m_localization->text(TextKey::ImportDirectory));
    m_report->setPlaceholderText(m_localization->text(TextKey::ValidationReport));
    m_previewAtlasLabel->setText(m_localization->text(TextKey::PreviewVariant));
    m_previewStateLabel->setText(m_localization->text(TextKey::PreviewAnimation));
    m_previewClipLabel->setText(m_localization->text(TextKey::PreviewClip));
    if (m_previewClipCombo->count() > 0) {
        m_previewClipCombo->setItemText(0,
                                        m_localization->usesChinese()
                                            ? QStringLiteral("使用标准动作")
                                            : QStringLiteral("Use standard animation"));
    }
    m_previewAtlasCombo->setToolTip(m_localization->text(TextKey::PreviewVariant));
    m_previewStateCombo->setToolTip(m_localization->text(TextKey::PreviewAnimation));
    m_resourceSummary->setPlaceholderText(m_localization->text(TextKey::ResourceFallbacks));
    m_resourceSummaryLabel->setText(m_localization->text(TextKey::ResourceFallbacks));

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
    updateSliderLabels();
    m_topCheck->setText(m_localization->text(TextKey::AlwaysOnTop));
    m_loginCheck->setText(m_localization->text(TextKey::LaunchAtLogin));
    m_typingCheck->setText(m_localization->text(TextKey::TypingDetection));
    m_typingNote->setText(m_localization->text(TextKey::TypingPrivacyNote));

    const QSignalBlocker motionBlocker(m_motionCombo);
    const QSignalBlocker hemisphereBlocker(m_hemisphereCombo);
    const QSignalBlocker languageBlocker(m_languageCombo);
    const int motion = m_motionCombo->currentIndex();
    m_motionCombo->clear();
    m_motionCombo->addItems({m_localization->text(TextKey::FollowSystem), m_localization->text(TextKey::ReduceMotion), m_localization->text(TextKey::FullMotion)});
    m_motionCombo->setCurrentIndex(motion < 0 ? 0 : motion);
    const int hemisphere = m_hemisphereCombo->currentIndex();
    m_hemisphereCombo->clear();
    m_hemisphereCombo->addItems({m_localization->text(TextKey::North), m_localization->text(TextKey::South)});
    m_hemisphereCombo->setCurrentIndex(hemisphere < 0 ? 0 : hemisphere);
    const int language = m_languageCombo->currentIndex();
    m_languageCombo->clear();
    m_languageCombo->addItems({m_localization->text(TextKey::SystemLanguage), m_localization->text(TextKey::English), m_localization->text(TextKey::SimplifiedChinese)});
    m_languageCombo->setCurrentIndex(language < 0 ? 0 : language);

    static_cast<QLabel *>(m_generalForm->labelForField(m_motionCombo))->setText(m_localization->text(TextKey::ReducedMotion));
    static_cast<QLabel *>(m_generalForm->labelForField(m_hemisphereCombo))->setText(m_localization->text(TextKey::Hemisphere));
    static_cast<QLabel *>(m_generalForm->labelForField(m_dayStart))->setText(m_localization->text(TextKey::DayStarts));
    static_cast<QLabel *>(m_generalForm->labelForField(m_nightStart))->setText(m_localization->text(TextKey::NightStarts));
    static_cast<QLabel *>(m_generalForm->labelForField(m_languageCombo))->setText(m_localization->text(TextKey::Language));
    m_resetButton->setText(m_localization->text(TextKey::ResetPosition));
    m_aboutButton->setText(m_localization->text(TextKey::AboutMenuItem));
    m_closeButton->setText(m_localization->text(TextKey::Close));

    // Accessible names for VoiceOver: sliders and combos sit next to blank form
    // labels, so their visible context isn't otherwise exposed to assistive tech.
    m_petCombo->setAccessibleName(m_localization->text(TextKey::Pet));
    m_importButton->setAccessibleName(m_localization->text(TextKey::ImportPet));
    m_removeButton->setAccessibleName(m_localization->text(TextKey::RemovePet));
    m_scaleSlider->setAccessibleName(m_localization->text(TextKey::Size));
    m_speedSlider->setAccessibleName(m_localization->text(TextKey::AnimationSpeed));
    m_typingCheck->setAccessibleDescription(m_localization->text(TextKey::TypingPrivacyNote));
    m_motionCombo->setAccessibleName(m_localization->text(TextKey::ReducedMotion));
    m_hemisphereCombo->setAccessibleName(m_localization->text(TextKey::Hemisphere));
    m_dayStart->setAccessibleName(m_localization->text(TextKey::DayStarts));
    m_nightStart->setAccessibleName(m_localization->text(TextKey::NightStarts));
    m_languageCombo->setAccessibleName(m_localization->text(TextKey::Language));
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
    m_previewClipCombo->addItem(m_localization->usesChinese()
                                    ? QStringLiteral("使用标准动作")
                                    : QStringLiteral("Use standard animation"),
                                QString());
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

void SettingsWindow::setValidationReport(const QString &report, bool error)
{
    m_report->setVisible(!report.isEmpty());
    m_report->setPlainText(report);
    m_report->setStyleSheet(error ? QStringLiteral("QPlainTextEdit { color: #a02020; }") : QString());
}

void SettingsWindow::setReducedMotion(bool reduced)
{
    m_preview->setReducedMotion(reduced);
}

void SettingsWindow::updateSliderLabels()
{
    static_cast<QLabel *>(m_generalForm->labelForField(m_scaleSlider))
        ->setText(QStringLiteral("%1 — %2%")
                      .arg(m_localization->text(TextKey::Size))
                      .arg(m_scaleSlider->value()));
    static_cast<QLabel *>(m_generalForm->labelForField(m_speedSlider))
        ->setText(QStringLiteral("%1 — %2×")
                      .arg(m_localization->text(TextKey::AnimationSpeed))
                      .arg(m_speedSlider->value() / 100.0, 0, 'f', 2));
}
