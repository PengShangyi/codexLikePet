#include "settings/PetGuideWindow.h"

#include <QAbstractTextDocumentLayout>
#include <QClipboard>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextDocument>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

#include <cmath>

namespace {

// English in both UI languages, on purpose — see the class comment. Line lengths
// are kept short because the boxes do not word-wrap: wrapping would make the
// height calculation below undercount, and a prompt that reflows differently
// from what the user copies is harder to check against.
const auto kBasePrompt = QStringLiteral(
    "A single character reference of <name>, a <style> desktop mascot:\n"
    "<description>.\n"
    "\n"
    "Full body, facing the viewer, standing at rest, neutral friendly\n"
    "expression. Centred in frame with even margins; no limb, tail or prop\n"
    "is cropped. Clean readable silhouette, even neutral lighting, one\n"
    "character only.\n"
    "\n"
    "Background: flat solid chroma key, pure green #00B140, no gradient and\n"
    "no background shadow.\n"
    "Do not draw: text, labels, watermarks, borders, frames, grid lines,\n"
    "drop shadows, or a second character.\n"
    "\n"
    "Square image, 1024x1024.");

const auto kRowPrompt = QStringLiteral(
    "Use the attached image as the exact character reference.\n"
    "Draw ONE horizontal animation strip of <name>.\n"
    "\n"
    "State: <state>\n"
    "Frames: <count>, evenly spaced left to right, reading as one loop.\n"
    "\n"
    "Hard requirements\n"
    "- Exactly <count> equal-width cells in ONE row. No extra frames and\n"
    "  no second row.\n"
    "- Each cell is 192:208 in aspect ratio, very slightly taller than wide.\n"
    "- Same character size and same ground baseline in every frame. Only the\n"
    "  pose changes; the character never drifts between frames.\n"
    "- Identical style, palette, markings, proportions and props as the\n"
    "  reference image.\n"
    "- Background: flat solid chroma key, pure green #00B140, filling every\n"
    "  cell edge to edge.\n"
    "- Leave a small margin inside each cell; nothing touches a cell edge.\n"
    "- Do not draw: text, numbers, labels, borders, boxes, guide lines, drop\n"
    "  shadows, or onion-skin ghosting of neighbouring frames.\n"
    "\n"
    "Output one wide strip with an aspect ratio of (<count> x 192) : 208.\n"
    "Exact pixel dimensions are normalised later, during assembly.");

// Matches docs/PET_AUTHORING.md. Not translated: these are file keys.
const auto kManifest = QStringLiteral(
    "{\n"
    "  \"id\": \"my-pet\",\n"
    "  \"displayName\": \"My Pet\",\n"
    "  \"description\": \"A short description.\",\n"
    "  \"spriteVersionNumber\": 2,\n"
    "  \"spritesheetPath\": \"spritesheet.webp\"\n"
    "}");

// Above every snippet's natural height, so today it never binds: the page
// already scrolls, a scrollbar inside a scrolled page is worse than a longer
// page, and a box one line short of its content reads as broken rather than as
// scrollable. The cap is only a guard against a snippet growing unnoticed.
constexpr int kMaxSnippetHeight = 600;
constexpr int kCopyFeedbackMs = 1500;

// Both live under the app bundle; see the POST_BUILD steps in CMakeLists.txt.
const auto kAuthoringDoc = QStringLiteral("Documentation/PET_AUTHORING.md");
const auto kHatchPetSkill = QStringLiteral("Skills/hatch-pet");

QString bundledResourcePath(const QString &relativePath)
{
    return QDir::cleanPath(QDir(QCoreApplication::applicationDirPath())
                               .filePath(QStringLiteral("../Resources/") + relativePath));
}

}  // namespace

PetGuideWindow::PetGuideWindow(Localization *localization, QWidget *parent)
    : QWidget(parent, Qt::Window)
    , m_localization(localization)
{
    setMinimumWidth(600);
    resize(660, 720);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // The guide is four screens of text, so the body scrolls while the action
    // row below stays reachable.
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *content = new QWidget(scroll);
    auto *column = new QVBoxLayout(content);
    column->setContentsMargins(24, 24, 24, 24);
    column->setSpacing(10);
    scroll->setWidget(content);
    root->addWidget(scroll, 1);

    m_title = new QLabel(content);
    QFont titleFont = m_title->font();
    titleFont.setPointSize(titleFont.pointSize() + 6);
    titleFont.setBold(true);
    m_title->setFont(titleFont);
    column->addWidget(m_title);

    m_intro = new QLabel(content);
    m_intro->setWordWrap(true);
    column->addWidget(m_intro);

    addSection(column, TextKey::PetGuideContractHeading, TextKey::PetGuideContractBody);

    m_placeholderNote = new QLabel(content);
    m_placeholderNote->setWordWrap(true);
    column->addSpacing(4);
    column->addWidget(m_placeholderNote);

    addSection(column, TextKey::PetGuideStep1Heading, TextKey::PetGuideStep1Body);
    addSnippet(column, TextKey::PetGuidePromptLabel, kBasePrompt);

    addSection(column, TextKey::PetGuideStep2Heading, TextKey::PetGuideStep2Body);
    addSnippet(column, TextKey::PetGuideRowPromptLabel, kRowPrompt);

    addSection(column, TextKey::PetGuideStep3Heading, TextKey::PetGuideStep3Body);

    addSection(column, TextKey::PetGuideStep4Heading, TextKey::PetGuideStep4Body);
    addSnippet(column, TextKey::PetGuideManifestLabel, kManifest);

    column->addStretch(1);

    auto *actions = new QWidget(this);
    auto *actionLayout = new QHBoxLayout(actions);
    actionLayout->setContentsMargins(24, 12, 24, 16);
    m_openDocButton = new QPushButton(actions);
    m_openDocButton->setObjectName(QStringLiteral("petGuideOpenDocButton"));
    m_revealSkillButton = new QPushButton(actions);
    m_revealSkillButton->setObjectName(QStringLiteral("petGuideRevealSkillButton"));
    // Disabled rather than silently inert when the bundle has not been built with
    // them, which is what a test or a loose binary sees.
    m_openDocButton->setEnabled(QFileInfo::exists(bundledResourcePath(kAuthoringDoc)));
    m_revealSkillButton->setEnabled(QFileInfo::exists(bundledResourcePath(kHatchPetSkill)));
    connect(m_openDocButton, &QPushButton::clicked, this, [this] {
        openBundledResource(kAuthoringDoc);
    });
    connect(m_revealSkillButton, &QPushButton::clicked, this, [this] {
        openBundledResource(kHatchPetSkill);
    });
    m_closeButton = new QPushButton(actions);
    m_closeButton->setObjectName(QStringLiteral("petGuideCloseButton"));
    m_closeButton->setDefault(true);
    connect(m_closeButton, &QPushButton::clicked, this, &QWidget::hide);
    actionLayout->addWidget(m_openDocButton);
    actionLayout->addWidget(m_revealSkillButton);
    actionLayout->addStretch(1);
    actionLayout->addWidget(m_closeButton);
    root->addWidget(actions);

    if (m_localization) {
        connect(m_localization, &Localization::languageChanged, this, &PetGuideWindow::retranslate);
    }
    retranslate();
}

void PetGuideWindow::addSection(QVBoxLayout *column, TextKey headingKey, TextKey bodyKey)
{
    QWidget *parent = column->parentWidget();
    auto *heading = new QLabel(parent);
    QFont headingFont = heading->font();
    headingFont.setBold(true);
    heading->setFont(headingFont);
    auto *body = new QLabel(parent);
    body->setWordWrap(true);
    column->addSpacing(6);
    column->addWidget(heading);
    column->addWidget(body);
    m_sections.append({heading, body, headingKey, bodyKey});
}

void PetGuideWindow::addSnippet(QVBoxLayout *column, TextKey labelKey, const QString &content)
{
    QWidget *parent = column->parentWidget();

    auto *captionRow = new QWidget(parent);
    auto *captionLayout = new QHBoxLayout(captionRow);
    captionLayout->setContentsMargins(0, 0, 0, 0);
    auto *label = new QLabel(captionRow);
    label->setWordWrap(true);
    auto *copyButton = new QPushButton(captionRow);
    captionLayout->addWidget(label, 1);
    captionLayout->addWidget(copyButton);
    column->addSpacing(2);
    column->addWidget(captionRow);

    auto *box = new QPlainTextEdit(parent);
    box->setPlainText(content);
    box->setReadOnly(true);
    // No wrapping: the text is pre-wrapped, and a reflowed prompt would no longer
    // match what the Copy button puts on the clipboard.
    box->setLineWrapMode(QPlainTextEdit::NoWrap);
    box->setTabChangesFocus(true);
    const QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    box->setFont(mono);

    // Sized to show every line, because a box one line short of its content reads
    // as broken rather than as scrollable.
    //
    // The line height comes from the document layout, not from
    // QFontMetrics::lineSpacing(): the two disagree by under a point for the
    // fixed-width system font, which is invisible on a short snippet and cost the
    // longest prompt its last line. The horizontal scrollbar is reserved rather
    // than measured because whether it appears depends on the window width, and
    // it would otherwise eat the last line only once the window was narrow.
    QTextDocument *document = box->document();
    const int lines = static_cast<int>(content.count(QLatin1Char('\n'))) + 1;
    const qreal blockHeight = document->documentLayout()->blockBoundingRect(document->firstBlock()).height();
    const qreal lineHeight = blockHeight > 0 ? blockHeight : box->fontMetrics().lineSpacing();
    const int chrome = 2 * box->frameWidth()
        + 2 * qRound(document->documentMargin())
        + box->horizontalScrollBar()->sizeHint().height();
    const int natural = static_cast<int>(std::ceil(lineHeight * lines)) + chrome;
    box->setFixedHeight(qMin(natural, kMaxSnippetHeight));
    column->addWidget(box);

    connect(copyButton, &QPushButton::clicked, this, [this, copyButton, content] {
        QGuiApplication::clipboard()->setText(content);
        if (!m_localization) return;
        copyButton->setText(m_localization->text(TextKey::PetGuideCopied));
        // Bound to the button, so a language switch or teardown cannot revert a
        // label that no longer exists.
        QTimer::singleShot(kCopyFeedbackMs, copyButton, [this, copyButton] {
            copyButton->setText(m_localization->text(TextKey::PetGuideCopy));
        });
    });

    m_snippets.append({label, copyButton, box, labelKey});
}

void PetGuideWindow::openBundledResource(const QString &relativePath)
{
    const QString path = bundledResourcePath(relativePath);
    if (!QFileInfo::exists(path)) return;
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void PetGuideWindow::retranslate()
{
    if (!m_localization) return;
    setWindowTitle(m_localization->text(TextKey::PetGuideTitle));
    m_title->setText(m_localization->text(TextKey::PetGuideTitle));
    m_intro->setText(m_localization->text(TextKey::PetGuideIntro));
    m_placeholderNote->setText(m_localization->text(TextKey::PetGuidePlaceholderNote));
    for (const Section &section : m_sections) {
        section.heading->setText(m_localization->text(section.headingKey));
        section.body->setText(m_localization->text(section.bodyKey));
    }
    for (const Snippet &snippet : m_snippets) {
        const QString caption = m_localization->text(snippet.labelKey);
        snippet.label->setText(caption);
        snippet.copyButton->setText(m_localization->text(TextKey::PetGuideCopy));
        // Neither the box nor the Copy button is owned by a SettingsRow, so both
        // are named here.
        snippet.copyButton->setAccessibleName(
            QStringLiteral("%1 — %2").arg(m_localization->text(TextKey::PetGuideCopy), caption));
        snippet.box->setAccessibleName(caption);
    }
    m_openDocButton->setText(m_localization->text(TextKey::PetGuideOpenDoc));
    m_revealSkillButton->setText(m_localization->text(TextKey::PetGuideRevealSkill));
    m_closeButton->setText(m_localization->text(TextKey::Close));
}
