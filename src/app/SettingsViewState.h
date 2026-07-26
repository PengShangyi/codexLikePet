#pragma once

#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QVector>

class AnimationClip;
class PetAtlas;
class SettingsWindow;
struct PetRecord;

// Holds what the settings window would be showing, so AppController can push
// state at it before the window exists.
//
// The window is built on first use rather than at startup: it is five pages of
// widgets, a preview widget and a QPlainTextEdit, and most sessions never open
// it. But AppController feeds it from eight different places, and sprinkling
// "if (m_settingsWindow)" through all of them would be easy to get half-right.
// This keeps the call sites in their current shape and puts the one branch here.
//
// Deliberately stores no QImage, QPixmap, PetAtlas or AnimationClip. The whole
// point is to use less memory when the window is closed, and a proxy that
// cached a preview atlas would pin 14MB to save building some widgets -- a lazy
// window that costs more than the eager one. The image-valued setters forward
// when attached and are dropped when not; AppController re-pushes them on attach
// from the package and atlas it already owns.
class SettingsViewState
{
public:
    // Replays everything recorded so far onto `window` and forwards from here on.
    void attach(SettingsWindow *window);
    bool isAttached() const { return m_window != nullptr; }
    SettingsWindow *window() const { return m_window; }

    void setPets(const QVector<PetRecord> &pets, const QString &selectedId);
    void setPreviewOptions(const QStringList &labels,
                           const QStringList &paths,
                           const QString &selectedPath);
    void setPreviewClipOptions(const QStringList &labels, const QStringList &keys);
    void setResourceSummary(const QString &summary);
    void setValidationReport(const QString &report, bool error);
    void setReducedMotion(bool reduced);
    void setEnvironmentSummary(const QString &summary);

    // Not recorded -- see the note above. No-ops while detached.
    void setPreviewAtlas(const QSharedPointer<PetAtlas> &atlas, bool smoothRendering);
    void setPreviewClip(const QSharedPointer<AnimationClip> &clip, bool smoothRendering);

private:
    SettingsWindow *m_window = nullptr;

    QVector<PetRecord> m_pets;
    QString m_selectedPetId;
    QStringList m_previewLabels;
    QStringList m_previewPaths;
    QString m_previewSelected;
    QStringList m_clipLabels;
    QStringList m_clipKeys;
    QString m_resourceSummary;
    QString m_validationReport;
    bool m_validationIsError = false;
    QString m_environmentSummary;
    bool m_reducedMotion = false;
};
