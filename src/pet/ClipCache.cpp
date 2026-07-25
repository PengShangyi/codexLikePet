#include "pet/ClipCache.h"

#include <QFileInfo>

#include <algorithm>

ClipCache::ClipCache(int capacity)
    : m_capacity(std::max(1, capacity))
{
}

QString ClipCache::keyFor(const QString &filePath, const QVector<int> &durationsMs)
{
    QStringList parts;
    parts.reserve(durationsMs.size());
    for (const int duration : durationsMs) parts.append(QString::number(duration));
    return QFileInfo(filePath).absoluteFilePath() + QLatin1Char('|')
        + parts.join(QLatin1Char(','));
}

QSharedPointer<AnimationClip> ClipCache::load(const QString &filePath,
                                              const QVector<int> &durationsMs,
                                              QString *error)
{
    const QString key = keyFor(filePath, durationsMs);
    if (auto existing = m_entries.value(key)) {
        touch(key);
        return existing;
    }

    auto clip = QSharedPointer<AnimationClip>::create();
    if (!clip->load(QFileInfo(filePath).absoluteFilePath(), durationsMs)) {
        if (error) *error = clip->errorString();
        return {};
    }

    while (m_entries.size() >= m_capacity && !m_lru.isEmpty()) {
        m_entries.remove(m_lru.takeFirst());
    }
    m_entries.insert(key, clip);
    m_lru.append(key);
    return clip;
}

bool ClipCache::contains(const QString &filePath, const QVector<int> &durationsMs) const
{
    return m_entries.contains(keyFor(filePath, durationsMs));
}

int ClipCache::size() const
{
    return m_entries.size();
}

void ClipCache::clear()
{
    m_entries.clear();
    m_lru.clear();
}

void ClipCache::touch(const QString &key)
{
    m_lru.removeAll(key);
    m_lru.append(key);
}
