#include "pet/BehaviorController.h"

BehaviorController::BehaviorController(QObject *parent)
    : QObject(parent)
{
}

BehaviorState BehaviorController::state() const { return m_state; }
SnapEdge BehaviorController::snapEdge() const { return m_snapEdge; }
bool BehaviorController::typingActive() const { return m_typing; }

void BehaviorController::beginDrag()
{
    m_dragging = true;
    m_snapEdge = SnapEdge::None;
    m_clickReaction = false;
    updateState();
}

void BehaviorController::setDragDirection(HorizontalDragDirection direction)
{
    if (direction == HorizontalDragDirection::None) return;
    m_dragDirection = direction;
    if (m_dragging) updateState();
}

void BehaviorController::endDrag(SnapEdge edge)
{
    m_dragging = false;
    m_snapEdge = edge;
    updateState();
}

void BehaviorController::setSnapEdge(SnapEdge edge)
{
    m_snapEdge = edge;
    updateState();
}

void BehaviorController::setTypingActive(bool active)
{
    m_typing = active;
    updateState();
}

void BehaviorController::triggerClick()
{
    if (m_dragging) return;
    m_clickReaction = true;
    updateState();
}

void BehaviorController::finishClickReaction()
{
    if (!m_clickReaction) return;
    m_clickReaction = false;
    updateState();
}

BehaviorState BehaviorController::resolvedState() const
{
    if (m_dragging) {
        return m_dragDirection == HorizontalDragDirection::Left
            ? BehaviorState::DraggingLeft
            : BehaviorState::DraggingRight;
    }
    if (m_clickReaction) return BehaviorState::ClickReaction;
    if (m_snapEdge == SnapEdge::Left) return BehaviorState::EdgeLeft;
    if (m_snapEdge == SnapEdge::Right) return BehaviorState::EdgeRight;
    if (m_snapEdge == SnapEdge::Bottom) return BehaviorState::EdgeBottom;
    if (m_typing) return BehaviorState::Typing;
    return BehaviorState::Idle;
}

void BehaviorController::updateState()
{
    const BehaviorState next = resolvedState();
    if (next == m_state) return;
    m_state = next;
    emit stateChanged(next);
}
