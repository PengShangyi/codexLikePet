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
struct PetRecord;

class SettingsWindow final : public QWidget
{
    Q_OBJECT

public:
    SettingsWindow(AppSettings *settings, Localization *localization, QWidget *parent = nullptr);

    void setPets(const QVector<PetRecord> &pets, const QString &selectedId);
    QString selectedPetId() const;
    void setPreviewAtlas(QSharedPointer<PetAtlas> atlas, bool smoothRendering);
    void setValidationReport(const QString &report, bool error);
    void setReducedMotion(bool reduced);

signals:
    void resetPositionRequested();
    void importPackageRequested();
    void importDirectoryRequested();
    void removePetRequested();
    void petSelected(const QString &id);

private:
    void buildUi();
    void bindSettings();
    void retranslate();

    AppSettings *m_settings;
    Localization *m_localization;
    QTabWidget *m_tabs;
    QWidget *m_petTab;
    QWidget *m_generalTab;
    QFormLayout *m_generalForm;
    QComboBox *m_petCombo;
    QPushButton *m_importButton;
    QPushButton *m_removeButton;
    PetPreviewWidget *m_preview;
    QPlainTextEdit *m_report;
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
