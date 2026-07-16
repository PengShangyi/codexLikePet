#pragma once

#include "resources/PetPackage.h"

#include <QObject>
#include <QVector>

struct PetRecord {
    PetPackage package;
    bool builtIn = false;
};

class PetLibrary final : public QObject
{
    Q_OBJECT

public:
    explicit PetLibrary(QString builtInRoot = {}, QString userRoot = {}, QObject *parent = nullptr);

    void refresh();
    QVector<PetRecord> pets() const;
    const PetRecord *find(const QString &id) const;
    QString firstAvailableId() const;
    bool removeCustomPet(const QString &id, QString *error);

    QString builtInRoot() const;
    QString userRoot() const;
    static QString defaultBuiltInRoot();

signals:
    void changed();

private:
    void scanRoot(const QString &root, bool builtIn, QHash<QString, PetRecord> *records) const;

    QString m_builtInRoot;
    QString m_userRoot;
    QVector<PetRecord> m_pets;
};
