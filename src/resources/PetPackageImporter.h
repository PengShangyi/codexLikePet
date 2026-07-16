#pragma once

#include "resources/PetPackage.h"
#include "resources/PetStore.h"

struct PetImportResult {
    bool success = false;
    QString installedPath;
    QString error;
    PackageValidationResult validation;
};

class PetPackageImporter final
{
public:
    explicit PetPackageImporter(PetStore store = PetStore());

    PetImportResult importPath(const QString &path) const;

private:
    PetStore m_store;
};
