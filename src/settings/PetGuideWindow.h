#pragma once

#include "settings/Localization.h"

#include <QVector>
#include <QWidget>

class QLabel;
class QPlainTextEdit;
class QPushButton;
class QShowEvent;
class QVBoxLayout;
class ThemeWatcher;

// The authoring guide, opened from "Make a pet…" beside Import Pet on the Pet
// page. It states the v2 atlas contract, hands over ready-to-paste image-model
// prompt templates, and points at the bundled Hatch Pet skill for the assembly
// step. Informational only — it changes nothing and installs nothing.
//
// The prompt templates and the pet.json sample are deliberately English in both
// UI languages. They are input for an image model rather than prose for the
// reader, English is what image models follow most reliably, and pet.json is a
// literal file whose keys are not translatable. Every label around them is a
// TextKey like the rest of the app.
//
// Built on first use like AboutWindow and OnboardingWindow: most sessions never
// open it, and it is four screens of static text.
class PetGuideWindow final : public QWidget
{
    Q_OBJECT

public:
    explicit PetGuideWindow(Localization *localization, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private:
    struct Section {
        QLabel *heading = nullptr;
        QLabel *body = nullptr;
        TextKey headingKey;
        TextKey bodyKey;
    };
    // A copyable code box with its own caption and Copy button.
    struct Snippet {
        QLabel *label = nullptr;
        QPushButton *copyButton = nullptr;
        QPlainTextEdit *box = nullptr;
        TextKey labelKey;
    };

    void retranslate();
    // Deferred to the first show like SettingsWindow's, because a window most
    // sessions never open should not build a stylesheet at construction.
    void applyTheme();
    void addSection(QVBoxLayout *column, TextKey headingKey, TextKey bodyKey);
    void addSnippet(QVBoxLayout *column, TextKey labelKey, const QString &content);
    // Opens a path under the app bundle's Contents/Resources, or does nothing if
    // it is absent — the buttons are disabled in that case anyway.
    void openBundledResource(const QString &relativePath);

    Localization *m_localization;
    ThemeWatcher *m_theme;
    bool m_themeApplied = false;
    QLabel *m_title = nullptr;
    QLabel *m_intro = nullptr;
    QLabel *m_placeholderNote = nullptr;
    QVector<Section> m_sections;
    QVector<Snippet> m_snippets;
    QPushButton *m_openDocButton = nullptr;
    QPushButton *m_revealSkillButton = nullptr;
    QPushButton *m_closeButton = nullptr;
};
