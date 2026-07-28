#pragma once

#include "pet/AtlasComposer.h"
#include "pet/PetAtlas.h"
#include "resources/PetPackageWriter.h"
#include "settings/Localization.h"

#include <QImage>
#include <QColor>
#include <QString>
#include <QVector>
#include <QWidget>

#include <array>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QShowEvent;
class QTimer;
class QHideEvent;
class AtlasGridPreview;
class InlineBanner;
class ThemeWatcher;
class ValueSlider;

// Turns eleven authored strips into an installed pet, so the assembly step no longer
// needs Python, Codex and a terminal. Opened from step 3 of PetGuideWindow, which is
// where the guide used to hand the job over.
//
// Everything hard is in AtlasComposer; this is the view. It decodes what the user
// picks, maps the composer's issue codes onto TextKeys, and emits the finished atlas.
// It deliberately does not validate or install: it emits installRequested and
// AppController runs the existing PetPackageImporter, which already validates at Full
// depth. One install path, not two -- and this window never links potato_package's
// validator as a result.
class AtlasAssemblerWindow final : public QWidget
{
    Q_OBJECT

public:
    explicit AtlasAssemblerWindow(Localization *localization, QWidget *parent = nullptr);

    // Also the test seam: QFileDialog cannot be driven offscreen.
    void setStrip(int row, const QString &path);
    bool hasDecodedStrip(int row) const;

    // The import verdict, pushed back by AppController.
    void setInstallReport(const QString &report, bool error);

signals:
    void installRequested(const QImage &atlas, const PetPackageWriter::PetInfo &info);

protected:
    void showEvent(QShowEvent *event) override;
    // Drops the decoded strips and the composed atlas. Eleven strips plus a 14 MB
    // atlas would otherwise stay resident for the session, because this window is
    // built on first use and then never destroyed.
    void hideEvent(QHideEvent *event) override;

private:
    struct Row {
        QPushButton *choose = nullptr;
        QLabel *fileName = nullptr;
        QLabel *count = nullptr;
        QString path;
        QImage strip;  // decoded lazily; cleared on hide
        TextKey nameKey;
    };

    void buildUi();
    void retranslate();
    void applyTheme();

    void chooseStrip(int row);
    void fillFromFolder();
    void detectKeyFromFirstStrip();
    // Coalesced: a tolerance drag would otherwise recompose on every step.
    void scheduleRecompose();
    void recompose();
    void refreshRowLabels();
    void updateActionState();
    QImage stripFor(int row);
    AtlasComposer::Options options() const;
    QString describe(const AtlasComposer::Issue &issue) const;

    Localization *m_localization;
    ThemeWatcher *m_theme;
    bool m_themeApplied = false;

    std::array<Row, PetAtlas::Rows> m_rows{};
    QPushButton *m_fillFromFolder = nullptr;
    QPushButton *m_swatch = nullptr;
    QPushButton *m_detectKey = nullptr;
    ValueSlider *m_tolerance = nullptr;
    QCheckBox *m_despill = nullptr;
    QLineEdit *m_displayName = nullptr;
    QLineEdit *m_petId = nullptr;
    // Stops the suggested id from overwriting one the user typed.
    bool m_petIdEdited = false;
    QLabel *m_introLabel = nullptr;
    AtlasGridPreview *m_preview = nullptr;
    InlineBanner *m_banner = nullptr;
    QPushButton *m_composeButton = nullptr;
    QPushButton *m_installButton = nullptr;
    QPushButton *m_closeButton = nullptr;
    QTimer *m_recomposeTimer = nullptr;

    QColor m_chromaKey = AtlasComposer::defaultChromaKey();
    AtlasComposer::Result m_result;
    bool m_composed = false;
};
