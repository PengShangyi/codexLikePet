#include "resources/PetStore.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUuid>

PetStore::PetStore(QString petsRoot)
    : m_petsRoot(petsRoot.isEmpty() ? defaultPetsRoot() : QDir::cleanPath(petsRoot))
{
}

QString PetStore::petsRoot() const
{
    return m_petsRoot;
}

bool PetStore::install(const PackageValidationResult &validation,
                       QString *installedPath,
                       QString *error) const
{
    if (!validation.isValid()) {
        *error = QStringLiteral("Cannot install an invalid pet package");
        return false;
    }
    if (!QDir().mkpath(m_petsRoot)) {
        *error = QStringLiteral("Unable to create Potato pet storage");
        return false;
    }

    const QString token = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString stageName = QStringLiteral(".install-%1").arg(token);
    const QString backupName = QStringLiteral(".backup-%1").arg(token);
    const QString targetName = validation.package.id;
    const QString stagePath = QDir(m_petsRoot).filePath(stageName);
    const QString backupPath = QDir(m_petsRoot).filePath(backupName);
    const QString targetPath = QDir(m_petsRoot).filePath(targetName);

    if (!copyDirectory(validation.package.rootPath, stagePath, error)) {
        QDir(stagePath).removeRecursively();
        return false;
    }

    QDir root(m_petsRoot);
    const bool hadExisting = QFileInfo::exists(targetPath);
    if (hadExisting && !root.rename(targetName, backupName)) {
        QDir(stagePath).removeRecursively();
        *error = QStringLiteral("Unable to preserve the existing pet during update");
        return false;
    }
    if (!root.rename(stageName, targetName)) {
        if (hadExisting) {
            root.rename(backupName, targetName);
        }
        QDir(stagePath).removeRecursively();
        *error = QStringLiteral("Unable to activate the imported pet");
        return false;
    }
    if (hadExisting) {
        QDir(backupPath).removeRecursively();
    }
    if (installedPath) {
        *installedPath = targetPath;
    }
    return true;
}

bool PetStore::remove(const QString &petId, QString *error) const
{
    static const QRegularExpression safeId(QStringLiteral("^[a-z0-9][a-z0-9-]{0,63}$"));
    if (!safeId.match(petId).hasMatch()) {
        *error = QStringLiteral("Invalid pet id");
        return false;
    }
    const QString path = QDir(m_petsRoot).filePath(petId);
    if (!QFileInfo::exists(path)) {
        return true;
    }
    if (!QDir(path).removeRecursively()) {
        *error = QStringLiteral("Unable to remove pet");
        return false;
    }
    return true;
}

QString PetStore::defaultPetsRoot()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))
        .filePath(QStringLiteral("Potato/Pets"));
}

bool PetStore::copyDirectory(const QString &source, const QString &destination, QString *error)
{
    if (!QDir().mkpath(destination)) {
        *error = QStringLiteral("Unable to create install staging directory");
        return false;
    }
    QDirIterator iterator(source,
                          QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        const QFileInfo info = iterator.fileInfo();
        const QString relative = QDir(source).relativeFilePath(info.filePath());
        const QString output = QDir(destination).filePath(relative);
        if (info.isDir()) {
            if (!QDir().mkpath(output)) {
                *error = QStringLiteral("Unable to copy pet directory");
                return false;
            }
        } else {
            if (!QDir().mkpath(QFileInfo(output).absolutePath()) || !QFile::copy(info.filePath(), output)) {
                *error = QStringLiteral("Unable to copy pet file: %1").arg(relative);
                return false;
            }
        }
    }
    return true;
}
