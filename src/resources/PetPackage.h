#pragma once

#include <QHash>
#include <QString>
#include <QVector>

enum class RenderMode {
    Smooth,
    Nearest,
};

struct ClipDefinition {
    QString path;
    QVector<int> durationsMs;
};

struct PetPackage {
    QString rootPath;
    QString id;
    QString displayName;
    QString description;
    QString spriteSheetPath;
    RenderMode renderMode = RenderMode::Smooth;
    QHash<QString, QString> variants;
    QHash<QString, ClipDefinition> clips;
    QHash<QString, QHash<QString, ClipDefinition>> variantClips;
};

enum class PackageIssueSeverity {
    Warning,
    Error,
};

struct PackageIssue {
    PackageIssueSeverity severity = PackageIssueSeverity::Error;
    QString code;
    QString message;
    QString path;
};

struct PackageValidationResult {
    PetPackage package;
    QVector<PackageIssue> issues;

    bool isValid() const;
    QStringList errorMessages() const;
};

QString renderModeToString(RenderMode mode);
