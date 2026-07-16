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
    m_preview = new PetPreviewWidget(m_petTab);
    petLayout->addWidget(m_preview, 1);
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
