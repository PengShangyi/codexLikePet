#include "resources/PetPackageImporter.h"

#include "resources/ArchiveExtractor.h"
#include "resources/PetPackageValidator.h"

#include <QFileInfo>
#include <QTemporaryDir>

PetPackageImporter::PetPackageImporter(PetStore store)
    : m_store(std::move(store))
{
}

PetImportResult PetPackageImporter::importPath(const QString &path) const
{
    PetImportResult result;
    const QFileInfo info(path);
    QString validationRoot;
    QTemporaryDir extracted;

    if (info.isDir()) {
        validationRoot = info.absoluteFilePath();
    } else if (info.isFile() && info.suffix().compare(QStringLiteral("potatopet"), Qt::CaseInsensitive) == 0) {
        if (!extracted.isValid()) {
            result.error = QStringLiteral("Unable to create secure extraction storage");
            return result;
        }
        if (!ArchiveExtractor().extractPotatoPackage(info.absoluteFilePath(), extracted.path(), &result.error)) {
            return result;
        }
        validationRoot = extracted.path();
    } else {
        result.error = QStringLiteral("Choose a pet directory or .potatopet archive");
        return result;
    }

    result.validation = PetPackageValidator().validateDirectory(validationRoot);
    if (!result.validation.isValid()) {
        result.error = result.validation.errorMessages().join(QLatin1Char('\n'));
        return result;
    }
    if (!m_store.install(result.validation, &result.installedPath, &result.error)) {
        return result;
    }
    result.success = true;
    return result;
}
