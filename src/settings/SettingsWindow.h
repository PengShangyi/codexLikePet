#pragma once

#include <QWidget>

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

class SettingsWindow final : public QWidget
{
    Q_OBJECT

public:
    SettingsWindow(AppSettings *settings, Localization *localization, QWidget *parent = nullptr);

signals:
    void resetPositionRequested();
    void importPetRequested();
    void removePetRequested();

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
    QLabel *m_previewLabel;
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
