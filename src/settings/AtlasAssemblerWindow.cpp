#include "settings/AtlasAssemblerWindow.h"

#include "resources/PackagePolicy.h"
#include "settings/AtlasGridPreview.h"
#include "ui/InlineBanner.h"
#include "ui/SettingsCard.h"
#include "ui/SettingsPage.h"
#include "ui/SettingsRow.h"
#include "ui/Theme.h"
#include "ui/ValueSlider.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QImageReader>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>

namespace {

// One coalescing window for the tolerance slider, same shape as the settings
// window's geometry save. A drag would otherwise recompose eleven strips per step.
constexpr int kRecomposeDelayMs = 150;

// Suffixes tried when matching a row name to a file in "Fill from folder".
const QStringList &imageSuffixes()
{
    static const QStringList suffixes{QStringLiteral("png"), QStringLiteral("webp"),
                                      QStringLiteral("jpg"), QStringLiteral("jpeg")};
    return suffixes;
}

TextKey rowNameKey(int row)
{
    switch (row) {
    case 0: return TextKey::AssemblerRowIdle;
    case 1: return TextKey::AssemblerRowRunningRight;
    case 2: return TextKey::AssemblerRowRunningLeft;
    case 3: return TextKey::AssemblerRowWaving;
    case 4: return TextKey::AssemblerRowJumping;
    case 5: return TextKey::AssemblerRowFailed;
    case 6: return TextKey::AssemblerRowWaiting;
    case 7: return TextKey::AssemblerRowRunning;
    case 8: return TextKey::AssemblerRowReview;
    case 9: return TextKey::AssemblerRowLookA;
    default: return TextKey::AssemblerRowLookB;
    }
}

QString swatchLabel(const QColor &colour)
{
    // The hex is the label. A colour patch alone would be unreadable to a screen
    // reader, and the value is what the prompt template names, so it has to be
    // checkable against it by eye.
    return colour.name(QColor::HexRgb).toUpper();
}

}  // namespace

AtlasAssemblerWindow::AtlasAssemblerWindow(Localization *localization, QWidget *parent)
    : QWidget(parent, Qt::Window)
    , m_localization(localization)
    , m_theme(new ThemeWatcher(this))
    , m_recomposeTimer(new QTimer(this))
{
    setAttribute(Qt::WA_QuitOnClose, false);
    setMinimumSize(720, 620);
    resize(820, 900);

    m_recomposeTimer->setSingleShot(true);
    m_recomposeTimer->setInterval(kRecomposeDelayMs);
    connect(m_recomposeTimer, &QTimer::timeout, this, &AtlasAssemblerWindow::recompose);

    buildUi();
    if (m_localization) {
        connect(m_localization, &Localization::languageChanged, this,
                &AtlasAssemblerWindow::retranslate);
    }
    connect(m_theme, &ThemeWatcher::schemeChanged, this, &AtlasAssemblerWindow::applyTheme);
    retranslate();
    updateActionState();
}

void AtlasAssemblerWindow::buildUi()
{
    setObjectName(QStringLiteral("petGuideRoot"));
    setAttribute(Qt::WA_StyledBackground, true);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *page = new SettingsPage(this);
    root->addWidget(page, 1);

    auto *intro = new QLabel(page);
    intro->setObjectName(QStringLiteral("petGuideText"));
    intro->setWordWrap(true);
    m_introLabel = intro;
    page->addContent(intro);

    // --- rows ---------------------------------------------------------------
    auto *rowsCard = new SettingsCard(TextKey::AssemblerSectionRows, m_localization, page);
    rowsCard->setObjectName(QStringLiteral("assemblerRowsCard"));

    auto *strip = new QWidget(rowsCard);
    auto *stripLayout = new QHBoxLayout(strip);
    stripLayout->setContentsMargins(0, 0, 0, 0);
    m_fillFromFolder = new QPushButton(strip);
    m_fillFromFolder->setObjectName(QStringLiteral("assemblerFillFromFolderButton"));
    connect(m_fillFromFolder, &QPushButton::clicked, this, &AtlasAssemblerWindow::fillFromFolder);
    stripLayout->addWidget(m_fillFromFolder);
    stripLayout->addStretch(1);
    rowsCard->addContent(strip, false);

    for (const AtlasComposer::RowSpec &spec : AtlasComposer::rows()) {
        Row &row = m_rows[static_cast<size_t>(spec.row)];
        row.nameKey = rowNameKey(spec.row);

        auto *control = new QWidget(rowsCard);
        auto *layout = new QHBoxLayout(control);
        layout->setContentsMargins(0, 0, 0, 0);
        row.count = new QLabel(control);
        row.count->setObjectName(QStringLiteral("rowValue"));
        row.fileName = new QLabel(control);
        row.fileName->setObjectName(QStringLiteral("rowDescription"));
        row.choose = new QPushButton(control);
        row.choose->setObjectName(QStringLiteral("assemblerChoose%1").arg(spec.row));
        connect(row.choose, &QPushButton::clicked, this, [this, index = spec.row] {
            chooseStrip(index);
        });
        layout->addWidget(row.count);
        layout->addWidget(row.fileName, 1);
        layout->addWidget(row.choose);

        auto *settingsRow = new SettingsRow(row.nameKey, control, m_localization, rowsCard);
        settingsRow->setObjectName(QStringLiteral("assemblerRow%1").arg(spec.row));
        rowsCard->addRow(settingsRow);
    }
    page->addCard(rowsCard);

    // --- preview and report -------------------------------------------------
    //
    // Between the rows and the background controls, not after everything. The preview
    // and the tolerance slider are the pair a user works with together -- nudge the
    // tolerance, look at what happened -- so they have to be able to be on screen at
    // the same time. Putting the preview last, after two more cards, meant scrolling
    // away from the control to see its effect.
    m_banner = new InlineBanner(page);
    m_banner->setObjectName(QStringLiteral("assemblerBanner"));
    page->addContent(m_banner);

    m_preview = new AtlasGridPreview(page);
    m_preview->setObjectName(QStringLiteral("assemblerPreview"));
    page->addStretchingContent(m_preview);

    // --- background removal -------------------------------------------------
    auto *keyCard = new SettingsCard(TextKey::AssemblerSectionKey, m_localization, page);
    keyCard->setObjectName(QStringLiteral("assemblerKeyCard"));

    auto *keyControl = new QWidget(keyCard);
    auto *keyLayout = new QHBoxLayout(keyControl);
    keyLayout->setContentsMargins(0, 0, 0, 0);
    m_swatch = new QPushButton(keyControl);
    m_swatch->setObjectName(QStringLiteral("assemblerChromaKeyButton"));
    connect(m_swatch, &QPushButton::clicked, this, [this] {
        const QColor chosen = QColorDialog::getColor(m_chromaKey, this,
                                                     m_localization
                                                         ? m_localization->text(TextKey::AssemblerChromaKey)
                                                         : QString());
        if (!chosen.isValid()) return;
        m_chromaKey = chosen;
        m_swatch->setText(swatchLabel(m_chromaKey));
        scheduleRecompose();
    });
    m_detectKey = new QPushButton(keyControl);
    m_detectKey->setObjectName(QStringLiteral("assemblerDetectKeyButton"));
    connect(m_detectKey, &QPushButton::clicked, this,
            &AtlasAssemblerWindow::detectKeyFromFirstStrip);
    keyLayout->addWidget(m_swatch);
    keyLayout->addWidget(m_detectKey);
    keyLayout->addStretch(1);
    keyCard->addRow(new SettingsRow(TextKey::AssemblerChromaKey, keyControl, m_localization, keyCard));

    m_tolerance = new ValueSlider(0, 180, [](int value) { return QString::number(value); });
    m_tolerance->setValue(AtlasComposer::Options{}.keyTolerance);
    connect(m_tolerance, &ValueSlider::valueChanged, this, [this](int) { scheduleRecompose(); });
    keyCard->addRow(new SettingsRow(TextKey::AssemblerKeyTolerance, m_tolerance, m_localization, keyCard));

    m_despill = new QCheckBox(keyCard);
    m_despill->setText(QString());
    m_despill->setMinimumWidth(1);
    m_despill->setChecked(true);
    connect(m_despill, &QCheckBox::toggled, this, [this](bool) { scheduleRecompose(); });
    keyCard->addRow(new SettingsRow(TextKey::AssemblerDespill, m_despill, m_localization, keyCard));
    page->addCard(keyCard);

    // --- pet details --------------------------------------------------------
    auto *petCard = new SettingsCard(TextKey::AssemblerSectionPet, m_localization, page);
    petCard->setObjectName(QStringLiteral("assemblerPetCard"));

    m_displayName = new QLineEdit(petCard);
    m_displayName->setObjectName(QStringLiteral("assemblerDisplayName"));
    m_petId = new QLineEdit(petCard);
    m_petId->setObjectName(QStringLiteral("assemblerPetId"));
    connect(m_displayName, &QLineEdit::textEdited, this, [this](const QString &text) {
        // Suggest an id until the user takes it over, then leave it alone.
        if (!m_petIdEdited) {
            const QSignalBlocker blocker(m_petId);
            m_petId->setText(PackagePolicy::suggestPetId(text));
        }
        updateActionState();
    });
    connect(m_petId, &QLineEdit::textEdited, this, [this](const QString &) {
        m_petIdEdited = true;
        updateActionState();
    });
    petCard->addRow(new SettingsRow(TextKey::AssemblerDisplayName, m_displayName, m_localization, petCard));
    petCard->addRow(new SettingsRow(TextKey::AssemblerPetId, m_petId, m_localization, petCard));
    page->addCard(petCard);

    // --- actions ------------------------------------------------------------
    auto *actions = new QWidget(this);
    actions->setObjectName(QStringLiteral("petGuideActions"));
    actions->setAttribute(Qt::WA_StyledBackground, true);
    auto *actionLayout = new QHBoxLayout(actions);
    actionLayout->setContentsMargins(24, 12, 24, 16);
    m_composeButton = new QPushButton(actions);
    m_composeButton->setObjectName(QStringLiteral("assemblerComposeButton"));
    connect(m_composeButton, &QPushButton::clicked, this, &AtlasAssemblerWindow::recompose);
    m_installButton = new QPushButton(actions);
    m_installButton->setObjectName(QStringLiteral("assemblerInstallButton"));
    m_installButton->setDefault(true);
    connect(m_installButton, &QPushButton::clicked, this, [this] {
        if (!m_result.isInstallable()) return;
        emit installRequested(m_result.atlas,
                              {m_petId->text().trimmed(), m_displayName->text().trimmed(), QString()});
    });
    m_closeButton = new QPushButton(actions);
    m_closeButton->setObjectName(QStringLiteral("assemblerCloseButton"));
    connect(m_closeButton, &QPushButton::clicked, this, &QWidget::hide);
    actionLayout->addWidget(m_composeButton);
    actionLayout->addWidget(m_installButton);
    actionLayout->addStretch(1);
    actionLayout->addWidget(m_closeButton);
    root->addWidget(actions);
}

void AtlasAssemblerWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (!m_themeApplied) applyTheme();
}

void AtlasAssemblerWindow::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    // Paths are cheap and worth keeping so reopening the window does not lose the
    // user's work; the pixels are not. Eleven strips and a 14 MB atlas would
    // otherwise stay resident for the rest of the session, since this window is
    // built on first use and never destroyed. Re-decoding costs about 10ms a strip.
    for (Row &row : m_rows) row.strip = QImage();
    m_result = {};
    m_composed = false;
    m_preview->setAtlas(QImage());
    updateActionState();
}

void AtlasAssemblerWindow::applyTheme()
{
    m_themeApplied = true;
    setStyleSheet(Theme::styleSheet(m_theme->scheme()));
    m_preview->setColorScheme(m_theme->scheme());
}

void AtlasAssemblerWindow::setStrip(int row, const QString &path)
{
    if (row < 0 || row >= PetAtlas::Rows) return;
    m_rows[static_cast<size_t>(row)].path = path;
    m_rows[static_cast<size_t>(row)].strip = QImage();
    refreshRowLabels();
    scheduleRecompose();
}

bool AtlasAssemblerWindow::hasDecodedStrip(int row) const
{
    if (row < 0 || row >= PetAtlas::Rows) return false;
    return !m_rows[static_cast<size_t>(row)].strip.isNull();
}

void AtlasAssemblerWindow::chooseStrip(int row)
{
    if (!m_localization) return;
    const QString filter = QStringLiteral("Images (*.png *.webp *.jpg *.jpeg)");
    const QString path = QFileDialog::getOpenFileName(
        this, m_localization->text(TextKey::AssemblerChooseStripTitle), {}, filter);
    if (path.isEmpty()) return;
    setStrip(row, path);
}

void AtlasAssemblerWindow::fillFromFolder()
{
    if (!m_localization) return;
    const QString folder = QFileDialog::getExistingDirectory(
        this, m_localization->text(TextKey::AssemblerFillFromFolderTitle));
    if (folder.isEmpty()) return;

    const QDir dir(folder);
    int matched = 0;
    for (const AtlasComposer::RowSpec &spec : AtlasComposer::rows()) {
        // The row's contract name, then row<N>. Deliberately narrower than the
        // skill's glob soup: the prompt tells the user the name, so this only has to
        // cover what the prompt asked for plus the obvious numeric fallback.
        QStringList candidates{QString(spec.name), QStringLiteral("row%1").arg(spec.row)};
        if (spec.row == 9) candidates << QStringLiteral("look-000-to-157.5");
        if (spec.row == 10) candidates << QStringLiteral("look-180-to-337.5");

        bool found = false;
        for (const QString &stem : candidates) {
            for (const QString &suffix : imageSuffixes()) {
                const QString name = stem + QLatin1Char('.') + suffix;
                // Case-insensitively, because a file picked out of Finder may not
                // match the prompt's casing.
                const QStringList hits = dir.entryList({name}, QDir::Files);
                if (hits.isEmpty()) continue;
                m_rows[static_cast<size_t>(spec.row)].path = dir.filePath(hits.first());
                m_rows[static_cast<size_t>(spec.row)].strip = QImage();
                found = true;
                break;
            }
            if (found) break;
        }
        if (found) ++matched;
    }

    refreshRowLabels();
    m_banner->setMessage(m_localization->text(TextKey::AssemblerFilledCount)
                             .arg(matched)
                             .arg(PetAtlas::Rows),
                         matched == PetAtlas::Rows ? InlineBanner::Severity::Info
                                                   : InlineBanner::Severity::Error);
    scheduleRecompose();
}

void AtlasAssemblerWindow::detectKeyFromFirstStrip()
{
    for (const AtlasComposer::RowSpec &spec : AtlasComposer::rows()) {
        const QImage strip = stripFor(spec.row);
        if (strip.isNull()) continue;
        m_chromaKey = AtlasComposer::detectChromaKey(strip);
        m_swatch->setText(swatchLabel(m_chromaKey));
        scheduleRecompose();
        return;
    }
}

QImage AtlasAssemblerWindow::stripFor(int row)
{
    Row &entry = m_rows[static_cast<size_t>(row)];
    if (!entry.strip.isNull() || entry.path.isEmpty()) return entry.strip;
    QImageReader reader(entry.path);
    reader.setAutoTransform(true);
    entry.strip = reader.read();
    return entry.strip;
}

AtlasComposer::Options AtlasAssemblerWindow::options() const
{
    AtlasComposer::Options options;
    options.chromaKey = m_chromaKey;
    options.keyTolerance = m_tolerance->value();
    options.despill = m_despill->isChecked() ? AtlasComposer::DespillEdges::On
                                             : AtlasComposer::DespillEdges::Off;
    return options;
}

void AtlasAssemblerWindow::scheduleRecompose()
{
    m_composed = false;
    updateActionState();
    m_recomposeTimer->start();
}

void AtlasAssemblerWindow::recompose()
{
    m_recomposeTimer->stop();

    QVector<AtlasComposer::RowInput> inputs;
    for (const AtlasComposer::RowSpec &spec : AtlasComposer::rows()) {
        const QImage strip = stripFor(spec.row);
        if (strip.isNull()) continue;
        inputs.append({spec.row, strip});
    }

    m_result = AtlasComposer::compose(inputs, options());
    m_composed = true;
    m_preview->setAtlas(m_result.atlas);

    QSet<int> flagged;
    QStringList lines;
    for (const AtlasComposer::Issue &issue : m_result.issues) {
        if (issue.row >= 0 && issue.column >= 0) {
            flagged.insert(issue.row * PetAtlas::Columns + issue.column);
        }
        const QString line = describe(issue);
        if (!line.isEmpty() && !lines.contains(line)) lines << line;
    }
    m_preview->setFlaggedCells(flagged);

    if (m_localization) {
        if (lines.isEmpty()) {
            m_banner->setMessage(QStringLiteral("%1 %2")
                                     .arg(m_localization->text(TextKey::AssemblerReady),
                                          m_localization->text(TextKey::AssemblerScaleReadout)
                                              .arg(qRound(m_result.scale * 100))),
                                 InlineBanner::Severity::Info);
        } else {
            m_banner->setMessage(lines.join(QLatin1Char('\n')),
                                 m_result.hasBlockingIssue() ? InlineBanner::Severity::Error
                                                             : InlineBanner::Severity::Info);
        }
    }
    updateActionState();
}

QString AtlasAssemblerWindow::describe(const AtlasComposer::Issue &issue) const
{
    if (!m_localization) return {};
    const QString rowName = issue.row >= 0 && issue.row < PetAtlas::Rows
        ? m_localization->text(rowNameKey(issue.row))
        : QString();
    // Frames are numbered from one for the reader; the contract counts from zero, but
    // nothing the user typed did.
    const int frame = issue.column + 1;

    switch (issue.problem) {
    case AtlasComposer::Problem::MissingRow:
        return m_localization->text(TextKey::AssemblerProblemMissingRow).arg(rowName);
    case AtlasComposer::Problem::StripTooSmall:
        return m_localization->text(TextKey::AssemblerProblemStripTooSmall).arg(rowName);
    case AtlasComposer::Problem::EmptyFrame:
        return m_localization->text(TextKey::AssemblerProblemEmptyFrame).arg(rowName).arg(frame);
    case AtlasComposer::Problem::OccupancyFailed:
        return m_localization->text(TextKey::AssemblerProblemOccupancy);
    case AtlasComposer::Problem::UnexpectedStripAspect:
        return m_localization->text(TextKey::AssemblerProblemStripAspect).arg(rowName);
    case AtlasComposer::Problem::OutlierFrame:
        return m_localization->text(TextKey::AssemblerProblemOutlierFrame).arg(rowName).arg(frame);
    case AtlasComposer::Problem::FrameDoesNotFit:
        return m_localization->text(TextKey::AssemblerProblemDoesNotFit).arg(rowName).arg(frame);
    }
    return {};
}

void AtlasAssemblerWindow::refreshRowLabels()
{
    if (!m_localization) return;
    for (const AtlasComposer::RowSpec &spec : AtlasComposer::rows()) {
        Row &row = m_rows[static_cast<size_t>(spec.row)];
        row.count->setText(QStringLiteral("%1").arg(AtlasComposer::frameCount(spec.row)));
        row.fileName->setText(row.path.isEmpty() ? m_localization->text(TextKey::AssemblerNoStrip)
                                                 : QFileInfo(row.path).fileName());
        row.choose->setAccessibleName(QStringLiteral("%1 — %2")
                                          .arg(m_localization->text(TextKey::AssemblerChoose),
                                               m_localization->text(row.nameKey)));
    }
    updateActionState();
}

void AtlasAssemblerWindow::updateActionState()
{
    bool anyChosen = false;
    for (const Row &row : m_rows) {
        if (!row.path.isEmpty()) anyChosen = true;
    }
    // Live whenever there is anything to compose, and deliberately NOT gated on the
    // debounce timer: that made the button go dead for 150ms after every strip the
    // user chose, which is exactly when they are most likely to reach for it. Pressing
    // it cancels the pending recompose and does the work now.
    m_composeButton->setEnabled(anyChosen);

    // Install is gated on m_composed instead, which scheduleRecompose() clears -- so a
    // pending recompose blocks the install without touching the compose button. Only
    // enabled when all three things the validator will check already hold, so a
    // predictable failure can never be triggered from here.
    const bool named = !m_displayName->text().trimmed().isEmpty();
    const bool identified = PackagePolicy::isValidPetId(m_petId->text().trimmed());
    m_installButton->setEnabled(m_composed && m_result.isInstallable() && named && identified);
}

void AtlasAssemblerWindow::setInstallReport(const QString &report, bool error)
{
    if (!m_localization) return;
    m_banner->setMessage(report.isEmpty()
                             ? QString()
                             : QStringLiteral("%1: %2")
                                   .arg(m_localization->text(TextKey::ValidationReport), report),
                         error ? InlineBanner::Severity::Error : InlineBanner::Severity::Info);
}

void AtlasAssemblerWindow::retranslate()
{
    if (!m_localization) return;
    setWindowTitle(m_localization->text(TextKey::AssemblerTitle));
    m_introLabel->setText(m_localization->text(TextKey::AssemblerIntro));
    m_fillFromFolder->setText(m_localization->text(TextKey::AssemblerFillFromFolder));
    m_fillFromFolder->setAccessibleName(m_localization->text(TextKey::AssemblerFillFromFolder));
    m_swatch->setText(swatchLabel(m_chromaKey));
    m_swatch->setAccessibleName(m_localization->text(TextKey::AssemblerChromaKey));
    m_detectKey->setText(m_localization->text(TextKey::AssemblerDetectKey));
    m_detectKey->setAccessibleName(m_localization->text(TextKey::AssemblerDetectKey));
    for (Row &row : m_rows) row.choose->setText(m_localization->text(TextKey::AssemblerChoose));
    m_composeButton->setText(m_localization->text(TextKey::AssemblerCompose));
    m_composeButton->setAccessibleName(m_localization->text(TextKey::AssemblerCompose));
    m_installButton->setText(m_localization->text(TextKey::AssemblerInstall));
    m_installButton->setAccessibleName(m_localization->text(TextKey::AssemblerInstall));
    m_closeButton->setText(m_localization->text(TextKey::Close));
    m_closeButton->setAccessibleName(m_localization->text(TextKey::Close));
    m_preview->setAccessibleName(m_localization->text(TextKey::AssemblerTitle));

    // The cards and rows own their own labels; walking the page covers them.
    for (SettingsCard *card : findChildren<SettingsCard *>()) card->retranslate();
    refreshRowLabels();
}
