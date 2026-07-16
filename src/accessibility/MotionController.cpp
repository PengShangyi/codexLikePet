#include "accessibility/MotionController.h"

#include "platform/SystemActivitySource.h"
#include "settings/AppSettings.h"

MotionController::MotionController(AppSettings *settings,
                                   SystemActivitySource *systemActivity,
                                   QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_systemActivity(systemActivity)
{
    connect(settings, &AppSettings::motionPreferenceChanged, this, &MotionController::recompute);
    connect(systemActivity, &SystemActivitySource::reduceMotionChanged, this, &MotionController::recompute);
    recompute();
}

bool MotionController::reducedMotion() const { return m_reducedMotion; }

void MotionController::recompute()
{
    const MotionPreference preference = m_settings->motionPreference();
    const bool next = preference == MotionPreference::Reduce
        || (preference == MotionPreference::FollowSystem && m_systemActivity->systemReduceMotion());
    if (next == m_reducedMotion) return;
    m_reducedMotion = next;
    emit reducedMotionChanged(next);
}
