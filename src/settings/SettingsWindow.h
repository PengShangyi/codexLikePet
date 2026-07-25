#pragma once

#include <QRect>
#include <QWidget>
#include <QSharedPointer>
#include <QVector>

#include "pet/PetAtlas.h"
#include "settings/Localization.h"

class AppSettings;
class QButtonGroup;
class QCheckBox;
class QComboBox;
class QPushButton;
class QStackedWidget;
class QTimeEdit;
class QHideEvent;
class QMoveEvent;
class QResizeEvent;
class QTimer;
class QToolButton;
class QPlainTextEdit;
class PetPreviewWidget;
class AnimationClip;
class Disclosure;
class InlineBanner;
class SettingsCard;
class SettingsPage;
class SettingsRow;
class ThemeWatcher;
class ValueSlider;
struct PetRecord;

class SettingsWindow final : public QWidget
{
    Q_OBJECT

public:
    SettingsWindow(AppSettings *settings, Localization *localization, QWidget *parent = nullptr);

    void setPets(const QVector<PetRecord> &pets, const QString &selectedId);
    QString selectedPetId() const;
    void setPreviewAtlas(QSharedPointer<PetAtlas> atlas, bool smoothRendering);
    void setPreviewOptions(const QStringList &labels,
                           const QStringList &paths,
                           const QString &selectedPath);
    void setPreviewClipOptions(const QStringList &labels, const QStringList &keys);
    void setPreviewClip(QSharedPointer<AnimationClip> clip, bool smoothRendering);
    void setResourceSummary(const QString &summary);
    QString resourceSummary() const;
    void setValidationReport(const QString &report, bool error);
    void setReducedMotion(bool reduced);
    // The resolved season and day/night phase, shown as a read-only row on the
    // Environment page. Surfaces the one line users actually wanted from the
    // fallback table that now lives behind a disclosure.
    void setEnvironmentSummary(const QString &summary);

signals:
    void resetPositionRequested();
    void importPackageRequested();
    void importDirectoryRequested();
    void removePetRequested();
    void petSelected(const QString &id);
    void previewAtlasSelected(const QString &relativePath);
    void previewClipSelected(const QString &key);
    void welcomeRequested();

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void buildUi();
    void buildPetPage();
    void buildAppearancePage();
    void buildBehaviorPage();
    void buildEnvironmentPage();
    void buildGeneralPage();
    void bindSettings();
    void retranslate();
    void applyTheme();
    void updateNavIcons();
    QRect availableGeometry() const;
    void restoreGeometry();
    void scheduleGeometrySave();
    void saveGeometry();
    void rebuildPreviewStateItems();

    // Registers a page with the nav bar and returns it, so the button, the stack
    // index, and the TextKey are established in one place.
    SettingsPage *addPage(TextKey title, const QString &symbolName);

    AppSettings *m_settings;
    Localization *m_localization;
    ThemeWatcher *m_theme;
    // The stylesheet pass is deferred to the first show: AppController constructs
    // this window eagerly at launch, and polishing five pages of cards during
    // startup is pure cost for a window most sessions never open.
    bool m_themeApplied = false;
    // Drag-resizing a window emits a move or resize per frame, so writes are
    // coalesced the way AppSettings avoids sync()ing on every slider step.
    QTimer *m_geometrySaveTimer;
    bool m_restoringGeometry = false;

    QWidget *m_navBar;
    QButtonGroup *m_navGroup;
    QVector<QToolButton *> m_navButtons;
    QVector<TextKey> m_navKeys;
    QStringList m_navSymbols;
    QStackedWidget *m_stack;

    // Pet page
    QComboBox *m_petCombo;
    QPushButton *m_importButton;
    QPushButton *m_removeButton;
    PetPreviewWidget *m_preview;
    InlineBanner *m_validationBanner;
    Disclosure *m_resourceDetails;
    QComboBox *m_previewAtlasCombo;
    QComboBox *m_previewStateCombo;
    QComboBox *m_previewClipCombo;
    QPlainTextEdit *m_resourceSummary;

    // Appearance page
    ValueSlider *m_scaleSlider;
    ValueSlider *m_speedSlider;
    ValueSlider *m_opacitySlider;
    QCheckBox *m_topCheck;
    QComboBox *m_motionCombo;

    // Behavior page
    QCheckBox *m_lockCheck;
    QPushButton *m_resetButton;
    QCheckBox *m_typingCheck;

    // Environment page
    SettingsRow *m_environmentRow;
    QComboBox *m_hemisphereCombo;
    QTimeEdit *m_dayStart;
    QTimeEdit *m_nightStart;

    // General page
    QCheckBox *m_loginCheck;
    QComboBox *m_languageCombo;
    QPushButton *m_welcomeButton;

    // Every row on every page, so retranslate() re-asks Localization for each label
    // instead of fishing labels back out of a layout.
    QVector<SettingsRow *> m_rows;
    QVector<SettingsCard *> m_cards;
};
