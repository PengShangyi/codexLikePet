#pragma once

#include <QObject>

class AppSettings;
class SystemActivitySource;

class MotionController final : public QObject
{
    Q_OBJECT

public:
    MotionController(AppSettings *settings,
                     SystemActivitySource *systemActivity,
                     QObject *parent = nullptr);

    bool reducedMotion() const;

signals:
    void reducedMotionChanged(bool reduced);

private:
    void recompute();

    AppSettings *m_settings;
    SystemActivitySource *m_systemActivity;
    bool m_reducedMotion = false;
};
