#include "environment/EnvironmentClock.h"

QDateTime SystemEnvironmentClock::now() const
{
    return QDateTime::currentDateTime();
}
