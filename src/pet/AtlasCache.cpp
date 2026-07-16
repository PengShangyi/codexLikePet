#include "pet/AtlasCache.h"

#include <QFileInfo>

#include <algorithm>

AtlasCache::AtlasCache(int capacity)
    : m_capacity(std::max(1, capacity))
{
}

QSharedPointer<PetAtlas> AtlasCache::load(const QString &filePath, QString *error)
{
    const QString absolutePath = QFileInfo(filePath).absoluteFilePath();
    if (auto existing = m_entries.value(absolutePath)) {
        touch(absolutePath);
        return existing;
    }

    auto atlas = QSharedPointer<PetAtlas>::create();
    if (!atlas->load(absolutePath)) {
        if (error) {
            *error = atlas->errorString();
        }
        return {};
    }

    while (m_entries.size() >= m_capacity && !m_lru.isEmpty()) {
        m_entries.remove(m_lru.takeFirst());
    }
    m_entries.insert(absolutePath, atlas);
    m_lru.append(absolutePath);
    return atlas;
}

bool AtlasCache::contains(const QString &filePath) const
{
    return m_entries.contains(QFileInfo(filePath).absoluteFilePath());
}

int AtlasCache::size() const
{
    return m_entries.size();
}

void AtlasCache::clear()
{
    m_entries.clear();
    m_lru.clear();
}

void AtlasCache::touch(const QString &absolutePath)
{
    m_lru.removeAll(absolutePath);
    m_lru.append(absolutePath);
}
