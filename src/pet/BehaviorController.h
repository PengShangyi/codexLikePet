#pragma once

#include "pet/WindowPlacement.h"

#include <QObject>

enum class BehaviorState {
    Idle,
    Typing,
    EdgeLeft,
    EdgeRight,
    EdgeBottom,
    ClickReaction,
    DraggingLeft,
    DraggingRight,
};

class BehaviorController final : public QObject
{
    Q_OBJECT

public:
    explicit BehaviorController(QObject *parent = nullptr);

    BehaviorState state() const;
    SnapEdge snapEdge() const;
    bool typingActive() const;

public slots:
    void beginDrag();
    void setDragDirection(HorizontalDragDirection direction);
    void endDrag(SnapEdge edge);
    void setSnapEdge(SnapEdge edge);
    void setTypingActive(bool active);
    void triggerClick();
    void finishClickReaction();

signals:
    void stateChanged(BehaviorState state);

private:
    BehaviorState resolvedState() const;
    void updateState();

    BehaviorState m_state = BehaviorState::Idle;
    SnapEdge m_snapEdge = SnapEdge::None;
    HorizontalDragDirection m_dragDirection = HorizontalDragDirection::Right;
    bool m_dragging = false;
    bool m_typing = false;
    bool m_clickReaction = false;
};

Q_DECLARE_METATYPE(BehaviorState)
