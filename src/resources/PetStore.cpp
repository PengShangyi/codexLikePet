#include "resources/PetStore.h"

#include "resources/PetPackageValidator.h"

#include <QDir>
#include <QDirIterator>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUuid>

#include <utility>

namespace {
bool removePathWithoutFollowingLinks(const QString &path)
{
    const QFileInfo info(path);
    if (info.isSymLink() || info.isFile()) return QFile::remove(path);
    if (info.isDir()) return QDir(path).removeRecursively();
    return !info.exists();
}
}

PetStore::PetStore(QString petsRoot, std::function<bool()> activationGate)
    : m_petsRoot(petsRoot.isEmpty() ? defaultPetsRoot() : QDir::cleanPath(petsRoot))
    , m_activationGate(std::move(activationGate))
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
        removePathWithoutFollowingLinks(stagePath);
        return false;
    }
    // Full depth: this recheck exists to catch a package that changed between the
    // caller's validation and this copy, so it must not trust the earlier verdict.
    const PackageValidationResult staged =
        PetPackageValidator().validateDirectory(stagePath, ValidationDepth::Full);
    if (!staged.isValid() || staged.package.id != validation.package.id) {
        removePathWithoutFollowingLinks(stagePath);
        *error = staged.isValid()
            ? QStringLiteral("Pet package identity changed while it was being imported")
            : QStringLiteral("Copied pet package failed revalidation: %1")
                  .arg(staged.errorMessages().join(QLatin1Char('\n')));
        return false;
    }

    QDir root(m_petsRoot);
    const QFileInfo targetInfo(targetPath);
    const bool hadExisting = targetInfo.exists() || targetInfo.isSymLink();
    if (hadExisting && !root.rename(targetName, backupName)) {
        removePathWithoutFollowingLinks(stagePath);
        *error = QStringLiteral("Unable to preserve the existing pet during update");
        return false;
    }
    const bool activationAllowed = !m_activationGate || m_activationGate();
    if (!activationAllowed || !root.rename(stageName, targetName)) {
        const bool restored = !hadExisting || root.rename(backupName, targetName);
        removePathWithoutFollowingLinks(stagePath);
        *error = restored ? QStringLiteral("Unable to activate the imported pet; previous pet restored")
                          : QStringLiteral("Unable to activate the imported pet or restore the previous pet");
        return false;
    }
    if (hadExisting) {
        removePathWithoutFollowingLinks(backupPath);
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
    const QFileInfo info(path);
    if (!info.exists() && !info.isSymLink()) {
        return true;
    }
    if (!removePathWithoutFollowingLinks(path)) {
        *error = QStringLiteral("Unable to remove pet");
        return false;
    }
    return true;
}

QString PetStore::defaultPetsRoot()
{
    const QString applicationName = QCoreApplication::applicationName().isEmpty()
        ? QStringLiteral("Potato")
        : QCoreApplication::applicationName();
    return QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))
        .filePath(applicationName + QStringLiteral("/Pets"));
}

bool PetStore::copyDirectory(const QString &source, const QString &destination, QString *error)
{
    const QString canonicalSource = QFileInfo(source).canonicalFilePath();
    const QFileInfo destinationInfo(destination);
    const QString canonicalDestinationParent = QFileInfo(destinationInfo.absolutePath())
                                                   .canonicalFilePath();
    const QString absoluteDestination = canonicalDestinationParent.isEmpty()
        ? destinationInfo.absoluteFilePath()
        : QDir(canonicalDestinationParent).filePath(destinationInfo.fileName());
    if (!canonicalSource.isEmpty()
        && (absoluteDestination == canonicalSource
            || absoluteDestination.startsWith(canonicalSource + QDir::separator()))) {
        *error = QStringLiteral("Install staging cannot be created inside the imported package");
        return false;
    }
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
