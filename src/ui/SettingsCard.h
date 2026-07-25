#pragma once

#include <QFrame>
#include <QVector>

#include "settings/Localization.h"

class QLabel;
class QVBoxLayout;
class SettingsRow;

// A titled, rounded container holding a stack of rows separated by hairlines --
// the same shape macOS System Settings uses, and the reason native controls can
// sit inside it unstyled and still look deliberate.
//
// Kept final: the stylesheet targets it by class name, and a QSS class selector
// matches only the exact Q_OBJECT class, so a subclass would silently lose its
// styling.
class SettingsCard final : public QFrame
{
    Q_OBJECT

public:
    SettingsCard(TextKey title, Localization *localization, QWidget *parent = nullptr);

    // Appends a row, inserting a hairline separator above it if it is not first.
    void addRow(SettingsRow *row);
    // For content that is not a label/control pair (the preview stage, a button
    // strip, a disclosure). Gets the same hairline treatment.
    void addContent(QWidget *widget, bool separated = true);

    void retranslate();

private:
    Localization *m_localization;
    TextKey m_titleKey;
    QLabel *m_title;
    QVBoxLayout *m_body;
    QVector<SettingsRow *> m_rows;
    bool m_hasBodyContent = false;
};
