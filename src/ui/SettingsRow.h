#pragma once

#include <QWidget>

#include "settings/Localization.h"

class QHBoxLayout;
class QLabel;
class QVBoxLayout;

// One label-plus-control line inside a SettingsCard.
//
// This replaces a QFormLayout idiom that did three jobs badly: rows were added
// with a QStringLiteral(" ") placeholder label, the real text was written later
// by fishing the label back out with labelForField() and an unchecked
// static_cast<QLabel *>, and accessibility was left to a separate block that
// restated every TextKey a second time.
//
// The fix is structural rather than conventional: the constructor *requires* a
// TextKey, so a row cannot exist without a translatable label, and the row sets
// its own control's accessible name. retranslate() then just walks the rows.
// (OnboardingWindow already keeps heading/body TextKeys next to their widgets
// this way; this generalises that.)
class SettingsRow final : public QWidget
{
    Q_OBJECT

public:
    SettingsRow(TextKey label,
                QWidget *control,
                Localization *localization,
                QWidget *parent = nullptr);

    // Smaller secondary line under the label -- e.g. the typing privacy note.
    // Also becomes the control's accessible description.
    void setDescriptionKey(TextKey description);

    // Read-only value shown where a control would otherwise sit, for rows that
    // report state rather than accept input (the resolved environment, the app
    // version).
    void setValueText(const QString &text);

    TextKey labelKey() const;
    QWidget *control() const;
    QString labelText() const;

    void retranslate();

private:
    Localization *m_localization;
    TextKey m_labelKey;
    // TextKey has no "none" member, so presence is tracked separately rather than
    // by reserving a sentinel enumerator that Localization would have to answer for.
    bool m_hasDescription = false;
    TextKey m_descriptionKey;
    QHBoxLayout *m_root;
    QVBoxLayout *m_textColumn;
    QLabel *m_label;
    QLabel *m_description = nullptr;
    QLabel *m_value = nullptr;
    QWidget *m_control;
};
