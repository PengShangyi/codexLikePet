#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QUuid>

struct QuoteContext {
    QString petId;
    QString trigger;
    QString environmentKey;
};

struct Quote {
    QString text;
    QString attribution;
    QUrl sourceUrl;
};

class QuoteProvider : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    ~QuoteProvider() override = default;
    virtual void requestQuote(const QuoteContext &context, const QUuid &requestId) = 0;

signals:
    void quoteReady(const QUuid &requestId, const Quote &quote);
    void quoteFailed(const QUuid &requestId, const QString &message);
};

class FixedQuoteProvider final : public QuoteProvider
{
    Q_OBJECT

public:
    explicit FixedQuoteProvider(QString text = QStringLiteral("test balabala"), QObject *parent = nullptr);
    void requestQuote(const QuoteContext &context, const QUuid &requestId) override;

private:
    QString m_text;
};

Q_DECLARE_METATYPE(QuoteContext)
Q_DECLARE_METATYPE(Quote)
