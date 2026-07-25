#pragma once

#include <QObject>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QUuid>

#include <functional>

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
    explicit FixedQuoteProvider(QString text, QObject *parent = nullptr);
    void requestQuote(const QuoteContext &context, const QUuid &requestId) override;

private:
    QString m_text;
};

class LocalQuoteProvider final : public QuoteProvider
{
    Q_OBJECT

public:
    using ChineseLanguageResolver = std::function<bool()>;

    explicit LocalQuoteProvider(ChineseLanguageResolver usesChinese = {},
                                QObject *parent = nullptr);
    void requestQuote(const QuoteContext &context, const QUuid &requestId) override;

private:
    static QStringList messagesFor(const QuoteContext &context, bool usesChinese);

    ChineseLanguageResolver m_usesChinese;
    QHash<QString, int> m_nextMessageIndex;
};

Q_DECLARE_METATYPE(QuoteContext)
Q_DECLARE_METATYPE(Quote)
