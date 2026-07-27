#pragma once

#include <QWidget>

#include "settings/Localization.h"

class QToolButton;
class QVBoxLayout;

// A collapsible section, used to get the pet-resource debugging controls out of
// the way without removing them: the preview atlas/animation/clip pickers and the
// season-by-phase fallback table are authoring tools, and they were previously the
// majority of what the Pet page showed every user.
//
// Deliberately not animated. An expand transition would mean threading
// MotionController into the settings window and reopening the reduced-motion
// contract for a purely decorative gain.
class Disclosure final : public QWidget
{
    Q_OBJECT

public:
    Disclosure(TextKey title, Localization *localization, QWidget *parent = nullptr);

    // Where callers add the content that gets hidden and shown.
    QVBoxLayout *contentLayout() const;

    bool isExpanded() const;
    void setExpanded(bool expanded);

    void retranslate();

signals:
    // The revealed rows can land outside a scrolled page, so whoever placed this
    // section has to be told when it opens. Emitted for a programmatic
    // setExpanded() as well as for a click on the header.
    void expandedChanged(bool expanded);

private:
    Localization *m_localization;
    TextKey m_titleKey;
    QToolButton *m_header;
    QWidget *m_content;
    QVBoxLayout *m_contentLayout;
};
