#pragma once

#include <QString>

class ArchiveExtractor final
{
public:
    static constexpr qint64 MaximumArchiveBytes = 256LL * 1024 * 1024;
    static constexpr qint64 MaximumExpandedBytes = 512LL * 1024 * 1024;
    static constexpr int MaximumEntries = 256;

    bool extractPotatoPackage(const QString &archivePath,
                              const QString &destinationPath,
                              QString *error) const;

private:
    static bool isSafeEntryName(const QString &entryName, QString *cleanPath);
    static bool isAllowedFileName(const QString &path);
};
