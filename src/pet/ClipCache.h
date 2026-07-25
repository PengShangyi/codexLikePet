#pragma once

#include "pet/AnimationClip.h"

#include <QHash>
#include <QSharedPointer>
#include <QStringList>
#include <QVector>

// LRU cache of decoded extension clips, mirroring AtlasCache. AppController held
// this as a bare QHash with no bound: it was cleared on pet and environment
// change, so it stayed small in practice, but nothing actually capped it the way
// the project contract caps atlases.
//
// The key covers the durations as well as the path, because the same strip can
// legitimately be declared with different per-frame timings by different
// variants, and those decode to clips that are not interchangeable.
class ClipCache final
{
public:
    explicit ClipCache(int capacity = 8);

    // Returns a cached clip when present, otherwise decodes and inserts it.
    // Returns null (and leaves the cache untouched) when the clip fails to load;
    // *error, when given, receives AnimationClip's message.
    QSharedPointer<AnimationClip> load(const QString &filePath,
                                       const QVector<int> &durationsMs,
                                       QString *error = nullptr);
    bool contains(const QString &filePath, const QVector<int> &durationsMs) const;
    int size() const;
    void clear();

private:
    static QString keyFor(const QString &filePath, const QVector<int> &durationsMs);
    void touch(const QString &key);

    int m_capacity;
    QHash<QString, QSharedPointer<AnimationClip>> m_entries;
    QStringList m_lru;
};
