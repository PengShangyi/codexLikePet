#pragma once

#include "resources/PetPackage.h"

#include <QString>

class PetStore final
{
public:
    explicit PetStore(QString petsRoot = {});

    QString petsRoot() const;
    bool install(const PackageValidationResult &validation,
                 QString *installedPath,
                 QString *error) const;
    bool remove(const QString &petId, QString *error) const;

    static QString defaultPetsRoot();

private:
    static bool copyDirectory(const QString &source, const QString &destination, QString *error);

    QString m_petsRoot;
};
