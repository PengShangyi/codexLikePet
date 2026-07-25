#include "quotes/QuoteProvider.h"

#include <QLocale>
#include <QTimer>

FixedQuoteProvider::FixedQuoteProvider(QString text, QObject *parent)
    : QuoteProvider(parent)
    , m_text(std::move(text))
{
}

void FixedQuoteProvider::requestQuote(const QuoteContext &, const QUuid &requestId)
{
    const Quote quote{m_text, {}, {}};
    QTimer::singleShot(0, this, [this, requestId, quote] { emit quoteReady(requestId, quote); });
}

LocalQuoteProvider::LocalQuoteProvider(ChineseLanguageResolver usesChinese, QObject *parent)
    : QuoteProvider(parent)
    , m_usesChinese(std::move(usesChinese))
{
}

void LocalQuoteProvider::requestQuote(const QuoteContext &context, const QUuid &requestId)
{
    const bool usesChinese = m_usesChinese
        ? m_usesChinese()
        : QLocale::system().language() == QLocale::Chinese;
    const QStringList messages = messagesFor(context, usesChinese);
    const QString sequenceKey = QStringLiteral("%1|%2|%3")
                                    .arg(usesChinese ? QStringLiteral("zh") : QStringLiteral("en"),
                                         context.trigger,
                                         context.environmentKey);
    int &nextIndex = m_nextMessageIndex[sequenceKey];
    const Quote quote{messages.at(nextIndex % messages.size()), {}, {}};
    nextIndex = (nextIndex + 1) % messages.size();
    QTimer::singleShot(0, this, [this, requestId, quote] { emit quoteReady(requestId, quote); });
}

QStringList LocalQuoteProvider::messagesFor(const QuoteContext &context, bool usesChinese)
{
    QStringList messages;
    const bool night = context.environmentKey == QStringLiteral("night")
        || context.environmentKey.endsWith(QStringLiteral("-night"));
    if (night) {
        messages.append(usesChinese ? QStringLiteral("夜深啦，也别忘了休息。")
                                    : QStringLiteral("It’s getting late—remember to rest, too."));
    } else {
        messages.append(usesChinese ? QStringLiteral("今天也一起慢慢来。")
                                    : QStringLiteral("Let’s take today one step at a time."));
    }

    if (context.environmentKey.startsWith(QStringLiteral("spring"))) {
        messages.append(usesChinese ? QStringLiteral("春天适合开始一点新东西。")
                                    : QStringLiteral("Spring is a lovely time to begin something new."));
    } else if (context.environmentKey.startsWith(QStringLiteral("summer"))) {
        messages.append(usesChinese ? QStringLiteral("夏日能量已送达！")
                                    : QStringLiteral("A little summer energy, delivered!"));
    } else if (context.environmentKey.startsWith(QStringLiteral("autumn"))) {
        messages.append(usesChinese ? QStringLiteral("秋天也要稳稳地向前走。")
                                    : QStringLiteral("Let’s keep moving gently through autumn."));
    } else if (context.environmentKey.startsWith(QStringLiteral("winter"))) {
        messages.append(usesChinese ? QStringLiteral("冬日里，有陪伴就很暖。")
                                    : QStringLiteral("A little company makes winter warmer."));
    }

    messages.append(usesChinese ? QStringLiteral("摸摸收到！")
                                : QStringLiteral("Pat received!"));
    messages.append(usesChinese ? QStringLiteral("我在这儿陪你。")
                                : QStringLiteral("I’m right here with you."));
    messages.append(usesChinese ? QStringLiteral("给你一点土豆能量。")
                                : QStringLiteral("Sending you a little potato energy."));
    return messages;
}
