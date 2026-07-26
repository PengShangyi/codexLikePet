#include "app/SettingsViewState.h"

// PetRecord is only forward declared in the header; QVector<PetRecord> needs the
// complete type to be constructed and destroyed here.
#include "resources/PetLibrary.h"
#include "settings/SettingsWindow.h"

void SettingsViewState::attach(SettingsWindow *window)
{
    m_window = window;
    if (!m_window) return;

    // Replay in the order AppController would have produced it, so the window
    // ends up in the same state as if it had been listening all along.
    m_window->setPets(m_pets, m_selectedPetId);
    m_window->setPreviewOptions(m_previewLabels, m_previewPaths, m_previewSelected);
    m_window->setPreviewClipOptions(m_clipLabels, m_clipKeys);
    m_window->setResourceSummary(m_resourceSummary);
    m_window->setEnvironmentSummary(m_environmentSummary);
    m_window->setReducedMotion(m_reducedMotion);
    // Last: a report is about the most recent action, so it must not be
    // overwritten by the settings replayed above.
    m_window->setValidationReport(m_validationReport, m_validationIsError);
}

void SettingsViewState::setPets(const QVector<PetRecord> &pets, const QString &selectedId)
{
    m_pets = pets;
    m_selectedPetId = selectedId;
    if (m_window) m_window->setPets(pets, selectedId);
}

void SettingsViewState::setPreviewOptions(const QStringList &labels,
                                          const QStringList &paths,
                                          const QString &selectedPath)
{
    m_previewLabels = labels;
    m_previewPaths = paths;
    m_previewSelected = selectedPath;
    if (m_window) m_window->setPreviewOptions(labels, paths, selectedPath);
}

void SettingsViewState::setPreviewClipOptions(const QStringList &labels, const QStringList &keys)
{
    m_clipLabels = labels;
    m_clipKeys = keys;
    if (m_window) m_window->setPreviewClipOptions(labels, keys);
}

void SettingsViewState::setResourceSummary(const QString &summary)
{
    m_resourceSummary = summary;
    if (m_window) m_window->setResourceSummary(summary);
}

void SettingsViewState::setValidationReport(const QString &report, bool error)
{
    m_validationReport = report;
    m_validationIsError = error;
    if (m_window) m_window->setValidationReport(report, error);
}

void SettingsViewState::setReducedMotion(bool reduced)
{
    m_reducedMotion = reduced;
    if (m_window) m_window->setReducedMotion(reduced);
}

void SettingsViewState::setEnvironmentSummary(const QString &summary)
{
    m_environmentSummary = summary;
    if (m_window) m_window->setEnvironmentSummary(summary);
}

void SettingsViewState::setPreviewAtlas(const QSharedPointer<PetAtlas> &atlas, bool smoothRendering)
{
    if (m_window) m_window->setPreviewAtlas(atlas, smoothRendering);
}

void SettingsViewState::setPreviewClip(const QSharedPointer<AnimationClip> &clip, bool smoothRendering)
{
    if (m_window) m_window->setPreviewClip(clip, smoothRendering);
}
