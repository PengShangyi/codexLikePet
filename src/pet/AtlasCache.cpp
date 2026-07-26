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

void AtlasCache::purgeUnreferenced()
{
    // QSharedPointer exposes no reference count, so ask a QWeakPointer instead:
    // drop the cache's reference and see whether the atlas is still alive. If it is,
    // something else owns it and the entry goes back; if it is not, the cache was
    // its last owner and the 13.4MB has already been returned.
    //
    // Iterating over a copy of the key order, because take() rehashes and mutating
    // m_entries under its own iterator is undefined. And m_lru must lose the key
    // too: the eviction loop in load() pops from the front while only checking
    // m_entries.size(), so a stale key there makes it discard a *live* entry to make
    // room that already exists.
    const QStringList keys = m_lru;
    for (const QString &key : keys) {
        QSharedPointer<PetAtlas> entry = m_entries.take(key);
        QWeakPointer<PetAtlas> observer = entry;
        entry.clear();
        if (QSharedPointer<PetAtlas> survivor = observer.lock()) {
            m_entries.insert(key, survivor);
        } else {
            m_lru.removeAll(key);
        }
    }
}

void AtlasCache::touch(const QString &absolutePath)
{
    m_lru.removeAll(absolutePath);
    m_lru.append(absolutePath);
}
