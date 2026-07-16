#pragma once

#include "login/LoginItemController.h"

class MacLoginItemController final : public LoginItemController
{
public:
    bool setEnabled(bool enabled, QString *error) override;
};
