#include "settings/SettingsWindow.h"

#include "settings/AppSettings.h"
#include "settings/Localization.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QTabWidget>
#include <QTimeEdit>
#include <QVBoxLayout>

SettingsWindow::SettingsWindow(AppSettings *settings, Localization *localization, QWidget *parent)
    : QWidget(parent, Qt::Window)
    , m_settings(settings)
    , m_localization(localization)
{
    setAttribute(Qt::WA_QuitOnClose, false);
    setMinimumSize(520, 470);
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
    m_petCombo->setEnabled(false);
    petLayout->addWidget(m_petCombo);
    auto *petButtons = new QHBoxLayout;
    m_importButton = new QPushButton(m_petTab);
    m_removeButton = new QPushButton(m_petTab);
    m_removeButton->setEnabled(false);
    petButtons->addWidget(m_importButton);
    petButtons->addWidget(m_removeButton);
    petButtons->addStretch();
    petLayout->addLayout(petButtons);
    m_previewLabel = new QLabel(m_petTab);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setMinimumHeight(240);
    m_previewLabel->setFrameShape(QFrame::StyledPanel);
    petLayout->addWidget(m_previewLabel, 1);
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
    m_generalForm->addRow(QString(), m_scaleSlider);
    m_generalForm->addRow(QString(), m_speedSlider);
    m_generalForm->addRow(m_topCheck);
    m_generalForm->addRow(m_loginCheck);
    m_generalForm->addRow(m_typingCheck);
    m_generalForm->addRow(QString(), m_typingNote);
    m_generalForm->addRow(QString(), m_motionCombo);
    m_generalForm->addRow(QString(), m_hemisphereCombo);
    m_generalForm->addRow(QString(), m_dayStart);
    m_generalForm->addRow(QString(), m_nightStart);
    m_generalForm->addRow(QString(), m_languageCombo);
    m_generalForm->addRow(m_resetButton);
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

    connect(m_scaleSlider, &QSlider::valueChanged, this, [this](int value) { m_settings->setScale(value / 100.0); });
    connect(m_speedSlider, &QSlider::valueChanged, this, [this](int value) { m_settings->setAnimationSpeed(value / 100.0); });
    connect(m_topCheck, &QCheckBox::toggled, m_settings, &AppSettings::setAlwaysOnTop);
    connect(m_loginCheck, &QCheckBox::toggled, m_settings, &AppSettings::setLaunchAtLogin);
    connect(m_typingCheck, &QCheckBox::toggled, m_settings, &AppSettings::setTypingDetectionEnabled);
    connect(m_motionCombo, &QComboBox::currentIndexChanged, this, [this](int value) { m_settings->setMotionPreference(static_cast<MotionPreference>(value)); });
    connect(m_hemisphereCombo, &QComboBox::currentIndexChanged, this, [this](int value) { m_settings->setHemisphere(static_cast<Hemisphere>(value)); });
    connect(m_dayStart, &QTimeEdit::timeChanged, m_settings, &AppSettings::setDayStartsAt);
    connect(m_nightStart, &QTimeEdit::timeChanged, m_settings, &AppSettings::setNightStartsAt);
    connect(m_languageCombo, &QComboBox::currentIndexChanged, this, [this](int value) { m_settings->setLanguage(static_cast<AppLanguage>(value)); });
    connect(m_resetButton, &QPushButton::clicked, this, &SettingsWindow::resetPositionRequested);
    connect(m_importButton, &QPushButton::clicked, this, &SettingsWindow::importPetRequested);
    connect(m_removeButton, &QPushButton::clicked, this, &SettingsWindow::removePetRequested);
    connect(m_closeButton, &QPushButton::clicked, this, &QWidget::hide);
}

void SettingsWindow::retranslate()
{
    setWindowTitle(QStringLiteral("Potato — %1").arg(m_localization->text(TextKey::Settings).remove(QChar(0x2026))));
    m_tabs->setTabText(m_tabs->indexOf(m_petTab), m_localization->text(TextKey::Pet));
    m_tabs->setTabText(m_tabs->indexOf(m_generalTab), m_localization->text(TextKey::General));
    m_petCombo->clear();
    m_petCombo->addItem(m_localization->text(TextKey::NoPetSelected));
    m_importButton->setText(m_localization->text(TextKey::ImportPet));
    m_removeButton->setText(m_localization->text(TextKey::RemovePet));
    m_previewLabel->setText(m_localization->text(TextKey::Preview));
    static_cast<QLabel *>(m_generalForm->labelForField(m_scaleSlider))->setText(m_localization->text(TextKey::Size));
    static_cast<QLabel *>(m_generalForm->labelForField(m_speedSlider))->setText(m_localization->text(TextKey::AnimationSpeed));
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
    m_closeButton->setText(m_localization->text(TextKey::Close));
}
