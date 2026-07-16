#pragma once

#include "environment/EnvironmentState.h"
#include "resources/PetPackage.h"

#include <QObject>
#include <QDate>
#include <QTime>

class AppSettings;
class EnvironmentClock;
class QTimer;
enum class Hemisphere;

class EnvironmentResolver final : public QObject
{
    Q_OBJECT

public:
    EnvironmentResolver(AppSettings *settings,
                        EnvironmentClock *clock,
                        QObject *parent = nullptr);

    VariantKey current() const;
    void start();
    void stop();
    void reevaluate();

    static Season seasonForDate(const QDate &date, Hemisphere hemisphere);
    static TimePhase phaseForTime(const QTime &time,
                                  const QTime &dayStart,
                                  const QTime &nightStart);
    static QString atlasRelativePath(const PetPackage &package, const VariantKey &key);

signals:
    void environmentChanged(const VariantKey &key);

private:
    AppSettings *m_settings;
    EnvironmentClock *m_clock;
    QTimer *m_timer;
    VariantKey m_current;
    bool m_initialized = false;
};
