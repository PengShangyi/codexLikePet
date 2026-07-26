#pragma once

#include "pet/PetAtlas.h"

#include <QHash>
#include <QSharedPointer>
#include <QStringList>

class AtlasCache final
{
public:
    explicit AtlasCache(int capacity = 2);

    QSharedPointer<PetAtlas> load(const QString &filePath, QString *error = nullptr);
    bool contains(const QString &filePath) const;
    int size() const;
    void clear();
    // Drops entries nothing outside the cache still holds. The capacity bounds how
    // many atlases the cache *indexes*, not how much memory the process keeps: after
    // a season or day/night rollover the previous variant is dead but still occupies
    // its slot until a third one arrives, which at 13.4MB an atlas is the largest
    // avoidable resident cost here. Entries something else is using are untouched,
    // so calling this is always safe -- it is a release, not an invalidation.
    void purgeUnreferenced();

private:
    void touch(const QString &absolutePath);

    int m_capacity;
    QHash<QString, QSharedPointer<PetAtlas>> m_entries;
    QStringList m_lru;
};
