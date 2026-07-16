#pragma once

#include <QString>

class LoginItemController
{
public:
    virtual ~LoginItemController() = default;
    virtual bool setEnabled(bool enabled, QString *error) = 0;
};
