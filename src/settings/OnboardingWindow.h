#pragma once

#include "settings/Localization.h"

#include <QVector>
#include <QWidget>

class QLabel;
class QPushButton;

// One-time first-run welcome. Explains the menu-bar (Dock-less) model, Settings,
// opt-in typing detection, and pet import. Informational only — no toggles, so it
// never nudges the user toward a privacy-affecting option.
class OnboardingWindow final : public QWidget
{
    Q_OBJECT

public:
    explicit OnboardingWindow(Localization *localization, QWidget *parent = nullptr);

private:
    struct Section {
        QLabel *heading = nullptr;
        QLabel *body = nullptr;
        TextKey headingKey;
        TextKey bodyKey;
    };

    void retranslate();

    Localization *m_localization;
    QLabel *m_title = nullptr;
    QLabel *m_intro = nullptr;
    QVector<Section> m_sections;
    QPushButton *m_getStarted = nullptr;
};
