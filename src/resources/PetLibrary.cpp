#include "resources/PetLibrary.h"

#include "resources/PetPackageValidator.h"
#include "resources/PetStore.h"

#include <QCoreApplication>
#include <QDir>

#include <algorithm>

PetLibrary::PetLibrary(QString builtInRoot, QString userRoot, QObject *parent)
    : QObject(parent)
    , m_builtInRoot(builtInRoot.isEmpty() ? defaultBuiltInRoot() : QDir::cleanPath(builtInRoot))
    , m_userRoot(userRoot.isEmpty() ? PetStore::defaultPetsRoot() : QDir::cleanPath(userRoot))
{
}

void PetLibrary::refresh()
{
    QHash<QString, PetRecord> records;
    scanRoot(m_builtInRoot, true, &records);
    // User packages intentionally override a built-in package with the same id;
    // deleting the custom package reveals the built-in package again.
    scanRoot(m_userRoot, false, &records);
    m_pets = records.values();
    std::sort(m_pets.begin(), m_pets.end(), [](const PetRecord &left, const PetRecord &right) {
        if (left.builtIn != right.builtIn) return left.builtIn;
        return left.package.displayName.localeAwareCompare(right.package.displayName) < 0;
    });
    emit changed();
}

QVector<PetRecord> PetLibrary::pets() const
{
    return m_pets;
}

const PetRecord *PetLibrary::find(const QString &id) const
{
    for (const PetRecord &record : m_pets) {
        if (record.package.id == id) return &record;
    }
    return nullptr;
}

QString PetLibrary::firstAvailableId() const
{
    return m_pets.isEmpty() ? QString() : m_pets.first().package.id;
}

bool PetLibrary::removeCustomPet(const QString &id, QString *error)
{
    const PetRecord *record = find(id);
    if (!record) return true;
    if (record->builtIn) {
        *error = QStringLiteral("Built-in pets cannot be removed");
        return false;
    }
    if (!PetStore(m_userRoot).remove(id, error)) return false;
    refresh();
    return true;
}

QString PetLibrary::builtInRoot() const
{
    return m_builtInRoot;
}

QString PetLibrary::userRoot() const
{
    return m_userRoot;
}

QString PetLibrary::defaultBuiltInRoot()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../Resources/Pets"));
}

void PetLibrary::scanRoot(const QString &root,
                          bool builtIn,
                          QHash<QString, PetRecord> *records) const
{
    QDir directory(root);
    if (!directory.exists()) return;
    const QFileInfoList children = directory.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot,
                                                            QDir::Name | QDir::IgnoreCase);
    PetPackageValidator validator;
    for (const QFileInfo &child : children) {
        // Metadata depth: listing needs the manifest and the safety envelope, not
        // a pixel-level decode of every atlas and clip. The deep check runs at the
        // trust boundary (PetPackageImporter / PetStore::install); anything that
        // still fails to decode is caught by PetAtlas/AnimationClip::load and
        // surfaced through the pet-load error path. See ValidationDepth.
        const PackageValidationResult validation =
            validator.validateDirectory(child.absoluteFilePath(), ValidationDepth::Metadata);
        if (validation.isValid()) {
            records->insert(validation.package.id, {validation.package, builtIn});
        }
    }
}
