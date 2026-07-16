#include "resources/PetPackage.h"

bool PackageValidationResult::isValid() const
{
    for (const PackageIssue &issue : issues) {
        if (issue.severity == PackageIssueSeverity::Error) {
            return false;
        }
    }
    return true;
}

QStringList PackageValidationResult::errorMessages() const
{
    QStringList messages;
    for (const PackageIssue &issue : issues) {
        if (issue.severity == PackageIssueSeverity::Error) {
            messages.append(issue.message);
        }
    }
    return messages;
}

QString renderModeToString(RenderMode mode)
{
    return mode == RenderMode::Nearest ? QStringLiteral("nearest") : QStringLiteral("smooth");
}
