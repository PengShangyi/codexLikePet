#include "resources/ArchiveExtractor.h"

#include "resources/PackagePolicy.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>

#include <miniz.h>

#include <sys/stat.h>

namespace {
class ZipReader final
{
public:
    ~ZipReader()
    {
        if (initialized) {
            mz_zip_reader_end(&archive);
        }
    }

    mz_zip_archive archive{};
    bool initialized = false;
};
}

bool ArchiveExtractor::extractPotatoPackage(const QString &archivePath,
                                            const QString &destinationPath,
                                            QString *error) const
{
    const QFileInfo archiveInfo(archivePath);
    if (!archiveInfo.exists() || !archiveInfo.isFile() || archiveInfo.isSymLink()) {
        *error = QStringLiteral("Archive must be a regular file");
        return false;
    }
    if (archiveInfo.size() > MaximumArchiveBytes) {
        *error = QStringLiteral("Archive exceeds 256MiB");
        return false;
    }
    if (!QDir().mkpath(destinationPath)) {
        *error = QStringLiteral("Unable to create extraction directory");
        return false;
    }

    ZipReader reader;
    const QByteArray archiveName = QFile::encodeName(archiveInfo.absoluteFilePath());
    if (!mz_zip_reader_init_file(&reader.archive, archiveName.constData(), 0)) {
        *error = QStringLiteral("Unable to open .potatopet archive");
        return false;
    }
    reader.initialized = true;

    const mz_uint entryCount = mz_zip_reader_get_num_files(&reader.archive);
    if (entryCount > MaximumEntries) {
        *error = QStringLiteral("Archive contains more than 256 entries");
        return false;
    }

    qint64 expandedBytes = 0;
    QSet<QString> extractedPaths;
    for (mz_uint index = 0; index < entryCount; ++index) {
        mz_zip_archive_file_stat stat{};
        if (!mz_zip_reader_file_stat(&reader.archive, index, &stat)) {
            *error = QStringLiteral("Unable to inspect archive entry");
            return false;
        }
        const QString entryName = QString::fromUtf8(stat.m_filename);
        QString relativePath;
        if (!isSafeEntryName(entryName, &relativePath)) {
            *error = QStringLiteral("Unsafe archive path: %1").arg(entryName);
            return false;
        }
        const QString collisionKey = relativePath.normalized(QString::NormalizationForm_C)
                                         .toCaseFolded();
        if (extractedPaths.contains(collisionKey)) {
            *error = QStringLiteral("Duplicate archive path: %1").arg(entryName);
            return false;
        }
        extractedPaths.insert(collisionKey);

        const mode_t mode = static_cast<mode_t>(stat.m_external_attr >> 16);
        if ((mode & S_IFMT) == S_IFLNK) {
            *error = QStringLiteral("Symbolic links are not allowed in pet archives");
            return false;
        }
        if ((mode & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0) {
            *error = QStringLiteral("Executable files are not allowed in pet archives");
            return false;
        }

        const QString outputPath = QDir(destinationPath).filePath(relativePath);
        if (mz_zip_reader_is_file_a_directory(&reader.archive, index)) {
            if (!QDir().mkpath(outputPath)) {
                *error = QStringLiteral("Unable to create archive directory");
                return false;
            }
            continue;
        }
        if (!isAllowedFileName(relativePath)) {
            *error = QStringLiteral("Unsupported file in archive: %1").arg(relativePath);
            return false;
        }

        const quint64 entryBytes = static_cast<quint64>(stat.m_uncomp_size);
        if (entryBytes > static_cast<quint64>(MaximumExpandedBytes)
            || expandedBytes > MaximumExpandedBytes - static_cast<qint64>(entryBytes)) {
            *error = QStringLiteral("Archive expands beyond 512MiB");
            return false;
        }
        expandedBytes += static_cast<qint64>(entryBytes);
        if (!QDir().mkpath(QFileInfo(outputPath).absolutePath())) {
            *error = QStringLiteral("Unable to create archive parent directory");
            return false;
        }
        const QByteArray encodedOutput = QFile::encodeName(outputPath);
        if (!mz_zip_reader_extract_to_file(&reader.archive,
                                           index,
                                           encodedOutput.constData(),
                                           0)) {
            *error = QStringLiteral("Unable to extract archive entry: %1").arg(relativePath);
            return false;
        }
    }
    return true;
}

bool ArchiveExtractor::isSafeEntryName(const QString &entryName, QString *cleanPath)
{
    QString cleaned;
    if (!PackagePolicy::isPortableRelativePath(entryName, &cleaned)) return false;
    // Stricter than the directory validator on one point: an archive entry that
    // cleans to "." names the destination root itself, which is never a real
    // member and would otherwise be extracted over the extraction directory.
    if (cleaned == QStringLiteral(".") || PackagePolicy::escapesRoot(cleaned)) return false;
    *cleanPath = cleaned;
    return true;
}

bool ArchiveExtractor::isAllowedFileName(const QString &path)
{
    return PackagePolicy::isAllowedPackageFileName(path);
}
