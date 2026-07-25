#include "pet/BehaviorController.h"
#include "quotes/SpeechBubble.h"
#include "quotes/QuoteProvider.h"

#include <QSignalSpy>
#include <QTest>

class BehaviorTest final : public QObject
{
    Q_OBJECT

private slots:
    // Priority, highest to lowest: Dragging > ClickReaction > Typing > Edge > Idle.
    void enforcesThePriorityOrder()
    {
        BehaviorController behavior;

        // An edge pose shows when nothing more urgent is active...
        behavior.setSnapEdge(SnapEdge::Left);
        QCOMPARE(behavior.state(), BehaviorState::EdgeLeft);
        // ...but typing outranks it, so activity stays visible while parked.
        behavior.setTypingActive(true);
        QCOMPARE(behavior.state(), BehaviorState::Typing);
        // A click reaction outranks typing.
        behavior.triggerClick();
        QCOMPARE(behavior.state(), BehaviorState::ClickReaction);
        // Dragging outranks everything.
        behavior.beginDrag();
        behavior.setDragDirection(HorizontalDragDirection::Right);
        QCOMPARE(behavior.state(), BehaviorState::DraggingRight);

        // Releasing onto an edge while still typing keeps typing on top.
        behavior.endDrag(SnapEdge::Bottom);
        QCOMPARE(behavior.state(), BehaviorState::Typing);
        // A transient click while typing resolves back to typing afterwards.
        behavior.triggerClick();
        QCOMPARE(behavior.state(), BehaviorState::ClickReaction);
        behavior.finishClickReaction();
        QCOMPARE(behavior.state(), BehaviorState::Typing);

        // Typing stops -> fall back to the snapped edge, then to idle.
        behavior.setTypingActive(false);
        QCOMPARE(behavior.state(), BehaviorState::EdgeBottom);
        behavior.setSnapEdge(SnapEdge::None);
        QCOMPARE(behavior.state(), BehaviorState::Idle);
    }

    void typingOverridesEveryEdgeAndRevertsWhenItStops()
    {
        struct Case { SnapEdge edge; BehaviorState pose; };
        for (const Case c : {Case{SnapEdge::Left, BehaviorState::EdgeLeft},
                             Case{SnapEdge::Right, BehaviorState::EdgeRight},
                             Case{SnapEdge::Bottom, BehaviorState::EdgeBottom}}) {
            BehaviorController behavior;
            behavior.setSnapEdge(c.edge);
            QCOMPARE(behavior.state(), c.pose);
            behavior.setTypingActive(true);
            QCOMPARE(behavior.state(), BehaviorState::Typing);
            behavior.setTypingActive(false);
            QCOMPARE(behavior.state(), c.pose);
        }
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

    void fixedProviderReturnsTheConfiguredFixture()
    {
        qRegisterMetaType<Quote>();
        FixedQuoteProvider provider(QStringLiteral("fixture quote"));
        QSignalSpy spy(&provider, &QuoteProvider::quoteReady);
        provider.requestQuote({QStringLiteral("potato"), QStringLiteral("click"), {}},
                              QUuid::createUuid());
        QTRY_COMPARE(spy.count(), 1);
        const Quote quote = spy.first().at(1).value<Quote>();
        QCOMPARE(quote.text, QStringLiteral("fixture quote"));
        QVERIFY(quote.sourceUrl.isEmpty());
        QVERIFY(quote.attribution.isEmpty());
    }

    void localProviderUsesLanguageAndEnvironmentWithoutNetworkMetadata()
    {
        qRegisterMetaType<Quote>();
        bool usesChinese = false;
        LocalQuoteProvider provider([&usesChinese] { return usesChinese; });
        QSignalSpy quoteSpy(&provider, &QuoteProvider::quoteReady);
        const QuoteContext winterNight{QStringLiteral("potato"),
                                       QStringLiteral("click"),
                                       QStringLiteral("winter-night")};

        provider.requestQuote(winterNight, QUuid::createUuid());
        QTRY_COMPARE(quoteSpy.count(), 1);
        const Quote first = quoteSpy.takeFirst().at(1).value<Quote>();
        QCOMPARE(first.text, QStringLiteral("It’s getting late—remember to rest, too."));
        QVERIFY(first.sourceUrl.isEmpty());
        QVERIFY(first.attribution.isEmpty());

        provider.requestQuote(winterNight, QUuid::createUuid());
        QTRY_COMPARE(quoteSpy.count(), 1);
        const Quote second = quoteSpy.takeFirst().at(1).value<Quote>();
        QCOMPARE(second.text, QStringLiteral("A little company makes winter warmer."));
        QVERIFY(second.text != first.text);

        usesChinese = true;
        provider.requestQuote(winterNight, QUuid::createUuid());
        QTRY_COMPARE(quoteSpy.count(), 1);
        const Quote translated = quoteSpy.takeFirst().at(1).value<Quote>();
        QCOMPARE(translated.text, QStringLiteral("夜深啦，也别忘了休息。"));
    }
};

QTEST_MAIN(BehaviorTest)

#include "test_behavior.moc"
