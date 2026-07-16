#include "quotes/QuoteProvider.h"

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
