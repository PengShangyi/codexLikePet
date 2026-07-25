#include "settings/AboutWindow.h"

#include "settings/Localization.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QLabel>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

AboutWindow::AboutWindow(Localization *localization, QWidget *parent)
    : QWidget(parent, Qt::Window)
    , m_localization(localization)
{
    setMinimumWidth(360);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(8);

    m_name = new QLabel(QStringLiteral("Potato"), this);
    QFont nameFont = m_name->font();
    nameFont.setPointSize(nameFont.pointSize() + 6);
    nameFont.setBold(true);
    m_name->setFont(nameFont);
    root->addWidget(m_name);

    m_version = new QLabel(this);
    root->addWidget(m_version);

    m_tagline = new QLabel(this);
    m_tagline->setWordWrap(true);
    root->addWidget(m_tagline);

    root->addSpacing(8);
    m_copyright = new QLabel(this);
    m_copyright->setWordWrap(true);
    root->addWidget(m_copyright);

    root->addStretch(1);
    m_licensesButton = new QPushButton(this);
    connect(m_licensesButton, &QPushButton::clicked, this, &AboutWindow::openLicenses);
    root->addWidget(m_licensesButton, 0, Qt::AlignLeft);

    if (m_localization) {
        connect(m_localization, &Localization::languageChanged, this, &AboutWindow::retranslate);
    }
    retranslate();
}

void AboutWindow::retranslate()
{
    setWindowTitle(QStringLiteral("Potato"));
    if (!m_localization) return;
    m_version->setText(QStringLiteral("%1 %2")
                           .arg(m_localization->text(TextKey::AboutVersionLabel),
                                QCoreApplication::applicationVersion()));
    m_tagline->setText(m_localization->text(TextKey::AboutTagline));
    m_copyright->setText(m_localization->text(TextKey::AboutCopyright));
    m_licensesButton->setText(m_localization->text(TextKey::AboutViewLicenses));
}

void AboutWindow::openLicenses()
{
    // Bundled at build time into Contents/Resources (see CMakeLists POST_BUILD).
    const QString notices = QDir::cleanPath(
        QDir(QCoreApplication::applicationDirPath())
            .filePath(QStringLiteral("../Resources/THIRD_PARTY_NOTICES.md")));
    if (QFileInfo::exists(notices)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(notices));
    }
}
