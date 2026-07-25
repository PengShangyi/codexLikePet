#include "settings/OnboardingWindow.h"

#include <QFont>
#include <QLabel>
#include <QPair>
#include <QPushButton>
#include <QVBoxLayout>

OnboardingWindow::OnboardingWindow(Localization *localization, QWidget *parent)
    : QWidget(parent, Qt::Window)
    , m_localization(localization)
{
    setMinimumWidth(440);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(10);

    m_title = new QLabel(this);
    QFont titleFont = m_title->font();
    titleFont.setPointSize(titleFont.pointSize() + 6);
    titleFont.setBold(true);
    m_title->setFont(titleFont);
    root->addWidget(m_title);

    m_intro = new QLabel(this);
    m_intro->setWordWrap(true);
    root->addWidget(m_intro);

    const QVector<QPair<TextKey, TextKey>> sections = {
        {TextKey::OnboardingMenuBarHeading, TextKey::OnboardingMenuBarBody},
        {TextKey::OnboardingSettingsHeading, TextKey::OnboardingSettingsBody},
        {TextKey::OnboardingTypingHeading, TextKey::OnboardingTypingBody},
        {TextKey::OnboardingImportHeading, TextKey::OnboardingImportBody},
    };
    for (const auto &keys : sections) {
        auto *heading = new QLabel(this);
        QFont headingFont = heading->font();
        headingFont.setBold(true);
        heading->setFont(headingFont);
        auto *body = new QLabel(this);
        body->setWordWrap(true);
        root->addSpacing(4);
        root->addWidget(heading);
        root->addWidget(body);
        m_sections.append({heading, body, keys.first, keys.second});
    }

    root->addStretch(1);
    m_getStarted = new QPushButton(this);
    m_getStarted->setDefault(true);
    connect(m_getStarted, &QPushButton::clicked, this, &QWidget::hide);
    root->addWidget(m_getStarted, 0, Qt::AlignRight);

    if (m_localization) {
        connect(m_localization, &Localization::languageChanged, this, &OnboardingWindow::retranslate);
    }
    retranslate();
}

void OnboardingWindow::retranslate()
{
    if (!m_localization) return;
    setWindowTitle(m_localization->text(TextKey::OnboardingTitle));
    m_title->setText(m_localization->text(TextKey::OnboardingTitle));
    m_intro->setText(m_localization->text(TextKey::OnboardingIntro));
    for (const Section &section : m_sections) {
        section.heading->setText(m_localization->text(section.headingKey));
        section.body->setText(m_localization->text(section.bodyKey));
    }
    m_getStarted->setText(m_localization->text(TextKey::OnboardingGetStarted));
}
