#include "pet/BehaviorController.h"
#include "quotes/SpeechBubble.h"
#include "quotes/QuoteProvider.h"

#include <QSignalSpy>
#include <QTest>

class BehaviorTest final : public QObject
{
    Q_OBJECT

private slots:
    void enforcesThePriorityOrder()
    {
        BehaviorController behavior;
        behavior.setTypingActive(true);
        QCOMPARE(behavior.state(), BehaviorState::Typing);
        behavior.setSnapEdge(SnapEdge::Left);
        QCOMPARE(behavior.state(), BehaviorState::EdgeLeft);
        behavior.triggerClick();
        QCOMPARE(behavior.state(), BehaviorState::ClickReaction);
        behavior.beginDrag();
        behavior.setDragDirection(HorizontalDragDirection::Right);
        QCOMPARE(behavior.state(), BehaviorState::DraggingRight);
        behavior.endDrag(SnapEdge::Bottom);
        QCOMPARE(behavior.state(), BehaviorState::EdgeBottom);
        behavior.triggerClick();
        behavior.finishClickReaction();
        QCOMPARE(behavior.state(), BehaviorState::EdgeBottom);
        behavior.setSnapEdge(SnapEdge::None);
        QCOMPARE(behavior.state(), BehaviorState::Typing);
    }

    void keepsSpeechBubbleInsideThePrimaryGeometry()
    {
        const QRect available(0, 25, 1000, 700);
        const QSize bubble(240, 80);
        const QPoint topPlacement = SpeechBubble::placementFor(bubble, QRect(300, 30, 192, 208), available);
        QVERIFY(available.contains(QRect(topPlacement, bubble)));
        const QPoint edgePlacement = SpeechBubble::placementFor(bubble, QRect(900, 300, 100, 208), available);
        QVERIFY(available.contains(QRect(edgePlacement, bubble)));
    }

    void speechBubbleFollowsThePetWindowLevelPreference()
    {
        SpeechBubble bubble;
        QVERIFY(bubble.windowFlags().testFlag(Qt::WindowStaysOnTopHint));
        bubble.setAlwaysOnTop(false);
        QVERIFY(!bubble.windowFlags().testFlag(Qt::WindowStaysOnTopHint));
        bubble.setAlwaysOnTop(true);
        QVERIFY(bubble.windowFlags().testFlag(Qt::WindowStaysOnTopHint));
    }

    void fixedProviderReturnsOnlyThePlaceholder()
    {
        qRegisterMetaType<Quote>();
        FixedQuoteProvider provider;
        QSignalSpy spy(&provider, &QuoteProvider::quoteReady);
        provider.requestQuote({QStringLiteral("potato"), QStringLiteral("click"), {}},
                              QUuid::createUuid());
        QTRY_COMPARE(spy.count(), 1);
        const Quote quote = spy.first().at(1).value<Quote>();
        QCOMPARE(quote.text, QStringLiteral("test balabala"));
        QVERIFY(quote.sourceUrl.isEmpty());
        QVERIFY(quote.attribution.isEmpty());
    }
};

QTEST_MAIN(BehaviorTest)

#include "test_behavior.moc"
