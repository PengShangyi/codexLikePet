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

private:
    void touch(const QString &absolutePath);

    int m_capacity;
    QHash<QString, QSharedPointer<PetAtlas>> m_entries;
    QStringList m_lru;
};
