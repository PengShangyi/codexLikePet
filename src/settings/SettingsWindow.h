#pragma once

#include <QWidget>
#include <QSharedPointer>

#include "pet/PetAtlas.h"

class AppSettings;
class Localization;
class QCheckBox;
class QComboBox;
class QFormLayout;
class QLabel;
class QPushButton;
class QSlider;
class QTabWidget;
class QTimeEdit;
class QPlainTextEdit;
class PetPreviewWidget;
class AnimationClip;
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
    void setValidationReport(const QString &report, bool error);
    void setReducedMotion(bool reduced);

signals:
    void resetPositionRequested();
    void importPackageRequested();
    void importDirectoryRequested();
    void removePetRequested();
    void petSelected(const QString &id);
    void previewAtlasSelected(const QString &relativePath);
    void previewClipSelected(const QString &key);

private:
    void buildUi();
    void bindSettings();
    void retranslate();
    void updateSliderLabels();

    AppSettings *m_settings;
    Localization *m_localization;
    QTabWidget *m_tabs;
    QWidget *m_petTab;
    QWidget *m_generalTab;
    QFormLayout *m_generalForm;
    QFormLayout *m_previewForm;
    QComboBox *m_petCombo;
    QComboBox *m_previewAtlasCombo;
    QComboBox *m_previewStateCombo;
    QComboBox *m_previewClipCombo;
    QLabel *m_previewAtlasLabel;
    QLabel *m_previewStateLabel;
    QLabel *m_previewClipLabel;
    QPushButton *m_importButton;
    QPushButton *m_removeButton;
    PetPreviewWidget *m_preview;
    QPlainTextEdit *m_report;
    QPlainTextEdit *m_resourceSummary;
    QLabel *m_resourceSummaryLabel;
    QSlider *m_scaleSlider;
    QSlider *m_speedSlider;
    QCheckBox *m_topCheck;
    QCheckBox *m_loginCheck;
    QCheckBox *m_typingCheck;
    QLabel *m_typingNote;
    QComboBox *m_motionCombo;
    QComboBox *m_hemisphereCombo;
    QTimeEdit *m_dayStart;
    QTimeEdit *m_nightStart;
    QComboBox *m_languageCombo;
    QPushButton *m_resetButton;
    QPushButton *m_closeButton;
};
