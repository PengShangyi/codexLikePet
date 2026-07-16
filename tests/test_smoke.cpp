#include <QCoreApplication>
#include <QTest>

class SmokeTest final : public QObject
{
    Q_OBJECT

private slots:
    void applicationMetadataCanBeConfigured()
    {
        QCoreApplication::setOrganizationDomain(QStringLiteral("com.peng"));
        QCoreApplication::setApplicationName(QStringLiteral("Potato"));
        QCOMPARE(QCoreApplication::organizationDomain(), QStringLiteral("com.peng"));
        QCOMPARE(QCoreApplication::applicationName(), QStringLiteral("Potato"));
    }
};

QTEST_GUILESS_MAIN(SmokeTest)

#include "test_smoke.moc"

