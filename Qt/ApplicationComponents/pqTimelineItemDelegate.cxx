// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqTimelineItemDelegate.h"

#include "pqAnimationManager.h"
#include "pqAnimationScene.h"
#include "pqCoreUtilities.h"
#include "pqDoubleLineEdit.h"
#include "pqPVApplicationCore.h"
#include "pqPropertyLinks.h"
#include "pqTimelineModel.h"
#include "pqTimelinePainter.h"

#include "vtkCommand.h"
#include "vtkPVGeneralSettings.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"

#include <QAbstractScrollArea>
#include <QCoreApplication>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QString>
#include <QToolButton>

#include <algorithm>
#include <cmath>

constexpr int MOUSE_WHEEL_STEPS_FACTOR = 120;
constexpr double ZOOM_BASE = 1.5;

// this struct is responsible for drawing elements in timelines.
//
struct pqTimelineItemDelegate::pqInternals
{
  pqTimelineItemDelegate* Self;

  // Handle property links with the scene.
  pqPropertyLinks SceneLinks;

  // widgets to edit start/end time properties
  QToolButton* LockStart;
  QToolButton* LockEnd;
  pqDoubleLineEdit* EditStart;
  pqDoubleLineEdit* EditEnd;
  QDoubleValidator* StartValidator;
  QDoubleValidator* EndValidator;

  enum class InteractionMode
  {
    None,
    Scroll,
    GrabMark
  };

  InteractionMode Interaction = InteractionMode::None;
  double ClickedTime = 0.;

  void endInteraction() { this->Interaction = InteractionMode::None; }

  bool startGrabMark()
  {
    if (this->Interaction == InteractionMode::None)
    {
      this->Interaction = InteractionMode::GrabMark;
      return true;
    }
    return false;
  }

  bool startScroll()
  {
    if (this->Interaction == InteractionMode::None)
    {
      this->Interaction = InteractionMode::Scroll;
      return true;
    }
    return false;
  }

  bool isScrolling() { return this->Interaction == InteractionMode::Scroll; }

  bool isGrabbing() { return this->Interaction == InteractionMode::GrabMark; }

  QString lockTooltip() const
  {
    return QCoreApplication::translate("pqTimelineItemDelegate",
      "%1 is unlocked. Lock to avoid auto-update when adding/removing time sources.");
  }

  QString unlockTooltip() const
  {
    return QCoreApplication::translate("pqTimelineItemDelegate",
      "%1 is locked. Unlock to allow auto-update when adding/removing time sources.");
  }

  pqInternals(pqTimelineItemDelegate* self, QWidget* parentWidget)
    : Self(self)
  {
    auto scrollArea = dynamic_cast<QAbstractScrollArea*>(parentWidget);
    this->LockStart = new QToolButton(scrollArea->viewport());
    this->LockStart->setObjectName("LockStart");
    this->LockStart->setToolTip(this->lockTooltip().arg(tr("Start time")));
    this->LockStart->setCheckable(true);
    this->LockStart->setIcon(QIcon(":/pqWidgets/Icons/pqLock.svg"));
    QObject::connect(this->LockStart, &QToolButton::clicked, [&]() {
      if (!this->LockStart->isChecked())
      {
        this->LockStart->setIcon(QIcon(":/pqWidgets/Icons/pqLock.svg"));
        this->LockStart->setToolTip(this->lockTooltip().arg(tr("Start time")));
      }
      else
      {
        this->LockStart->setIcon(QIcon(":/pqWidgets/Icons/pqUnlock.svg"));
        this->LockStart->setToolTip(this->unlockTooltip().arg(tr("Start time")));
      }
      this->Self->TimelinePainter->setSceneLockStart(this->LockStart->isChecked());
    });

    this->LockEnd = new QToolButton(scrollArea->viewport());
    this->LockEnd->setObjectName("LockEnd");
    this->LockEnd->setToolTip(this->lockTooltip().arg(tr("End Time")));
    this->LockEnd->setCheckable(true);
    this->LockEnd->setIcon(QIcon(":/pqWidgets/Icons/pqLock.svg"));
    QObject::connect(this->LockEnd, &QToolButton::clicked, [&]() {
      if (!this->LockEnd->isChecked())
      {
        this->LockEnd->setIcon(QIcon(":/pqWidgets/Icons/pqLock.svg"));
        this->LockEnd->setToolTip(this->lockTooltip().arg(tr("End time")));
      }
      else
      {
        this->LockEnd->setIcon(QIcon(":/pqWidgets/Icons/pqUnlock.svg"));
        this->LockEnd->setToolTip(this->unlockTooltip().arg(tr("End time")));
      }

      this->Self->TimelinePainter->setSceneLockEnd(this->LockEnd->isChecked());
    });

    this->EditStart = new pqDoubleLineEdit(parentWidget);
    this->EditStart->setObjectName("StartTime");
    this->EditStart->hide();
    this->StartValidator = new QDoubleValidator(parentWidget);
    this->EditStart->setValidator(this->StartValidator);

    this->EditEnd = new pqDoubleLineEdit(parentWidget);
    this->EditEnd->setObjectName("EndTime");
    this->EditEnd->hide();
    this->EndValidator = new QDoubleValidator(parentWidget);
    this->EditEnd->setValidator(this->EndValidator);

    this->updateValidators();

    QObject::connect(
      this->EditStart, &pqDoubleLineEdit::textChanged, [&]() { this->EditStart->adjustSize(); });
    QObject::connect(
      this->EditEnd, &pqDoubleLineEdit::textChanged, [&]() { this->EditEnd->adjustSize(); });
  }

  void updateValidators()
  {
    pqAnimationManager* animationManager = pqPVApplicationCore::instance()->animationManager();
    pqAnimationScene* scene = animationManager->getActiveScene();
    if (!scene)
    {
      return;
    }

    auto timeRange = scene->getClockTimeRange();
    this->StartValidator->setTop(std::nextafter(timeRange.second, timeRange.second - 1));
    this->EndValidator->setBottom(std::nextafter(timeRange.first, timeRange.first + 1));
  }
};

//-----------------------------------------------------------------------------
pqTimelineItemDelegate::pqTimelineItemDelegate(QObject* parent, QWidget* parentWidget)
  : Superclass(parent)
  , Internals(new pqInternals(this, parentWidget))
  , TimelinePainter(new pqTimelinePainter(this))
{
  pqAnimationManager* animationManager = pqPVApplicationCore::instance()->animationManager();
  this->connect(animationManager, &pqAnimationManager::activeSceneChanged, this,
    &pqTimelineItemDelegate::setActiveSceneConnections);

  pqCoreUtilities::connect(
    vtkPVGeneralSettings::GetInstance(), vtkCommand::ModifiedEvent, this, SIGNAL(needsRepaint()));

  this->connect(
    this->Internals->LockStart, &QToolButton::clicked, this, &pqTimelineItemDelegate::needsRepaint);
  this->connect(
    this->Internals->LockEnd, &QToolButton::clicked, this, &pqTimelineItemDelegate::needsRepaint);
  this->connect(this->Internals->EditStart, &pqDoubleLineEdit::textChangedAndEditingFinished,
    this->Internals->EditStart, &pqDoubleLineEdit::hide);
  this->connect(this->Internals->EditStart, &pqDoubleLineEdit::textChangedAndEditingFinished, this,
    &pqTimelineItemDelegate::needsRepaint);
  this->connect(this->Internals->EditEnd, &pqDoubleLineEdit::textChangedAndEditingFinished,
    this->Internals->EditEnd, &pqDoubleLineEdit::hide);
  this->connect(this->Internals->EditEnd, &pqDoubleLineEdit::textChangedAndEditingFinished, this,
    &pqTimelineItemDelegate::needsRepaint);
}

//-----------------------------------------------------------------------------
pqTimelineItemDelegate::~pqTimelineItemDelegate() = default;

//-----------------------------------------------------------------------------
void pqTimelineItemDelegate::paint(
  QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  QStyleOptionViewItem itemOption = option;
  this->initStyleOption(&itemOption, index);

  this->TimelinePainter->paint(painter, index, itemOption);

  // update Start/End widgets on TIME item only
  if (index.data(pqTimelineItemRole::TYPE) == pqTimelineTrack::TIME)
  {
    auto startRect = this->TimelinePainter->getStartLabelRect();
    int buttonSide = startRect.height();
    auto buttonRect = QRect(startRect.right(), startRect.top(), buttonSide, buttonSide);
    this->Internals->LockStart->setGeometry(buttonRect);
    this->Internals->LockStart->setVisible(startRect.isValid());

    auto endRect = this->TimelinePainter->getEndLabelRect();
    buttonSide = endRect.height();
    buttonRect = QRect(endRect.left() - buttonSide, endRect.top(), buttonSide, buttonSide);
    this->Internals->LockEnd->setGeometry(buttonRect);
    this->Internals->LockEnd->setVisible(endRect.isValid());
  }

  Superclass::paint(painter, option, index);
}

//-----------------------------------------------------------------------------
QSize pqTimelineItemDelegate::sizeHint(
  const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  auto size = Superclass::sizeHint(option, index);
  size.setHeight(size.height() * 2);
  return size;
}

//-----------------------------------------------------------------------------
bool pqTimelineItemDelegate::editorEvent(QEvent* event, QAbstractItemModel* model,
  const QStyleOptionViewItem& option, const QModelIndex& index)
{
  if (index.data(pqTimelineItemRole::TYPE) != pqTimelineTrack::TIME &&
    index.data(pqTimelineItemRole::TYPE) != pqTimelineTrack::ANIMATION &&
    index.data(pqTimelineItemRole::TYPE) != pqTimelineTrack::SOURCE)
  {
    return false;
  }

  auto mouseEvent = dynamic_cast<QMouseEvent*>(event);
  if (!mouseEvent)
  {
    return false;
  }

  // edit start / end time
  if (mouseEvent->type() == QEvent::MouseButtonDblClick &&
    this->TimelinePainter->hasStartEndLabels())
  {
    QRect startRect = this->TimelinePainter->getStartLabelRect();
    if (startRect.contains(mouseEvent->pos()))
    {
      this->Internals->EditStart->setGeometry(startRect);
      this->Internals->EditStart->show();
      return true;
    }
    QRect endRect = this->TimelinePainter->getEndLabelRect();
    if (endRect.contains(mouseEvent->pos()))
    {
      this->Internals->EditEnd->setGeometry(endRect);
      this->Internals->EditEnd->show();
      return true;
    }
  }

  double trackTime =
    this->TimelinePainter->indexTimeFromPosition(mouseEvent->pos().x(), option, index);
  double mouseTime = this->TimelinePainter->timeFromPosition(mouseEvent->pos().x());

  if (mouseEvent->type() == QEvent::MouseButtonPress && mouseEvent->button() == Qt::LeftButton)
  {
    if (mouseEvent->modifiers().testFlag(Qt::ControlModifier) && this->Internals->startScroll())
    {
      this->Internals->ClickedTime = mouseTime;
      return true;
    }
    else if (this->Internals->startGrabMark())
    {
      this->TimelinePainter->setSceneCurrentTime(trackTime);
      return true;
    }
  }

  if (mouseEvent->type() == QEvent::MouseMove)
  {
    if (this->Internals->isGrabbing())
    {
      this->TimelinePainter->setSceneCurrentTime(trackTime);
      Q_EMIT this->needsRepaint();
      return true;
    }
    else if (this->Internals->isScrolling() &&
      mouseEvent->modifiers().testFlag(Qt::ControlModifier))
    {
      pqAnimationManager* animationManager = pqPVApplicationCore::instance()->animationManager();
      pqAnimationScene* scene = animationManager->getActiveScene();
      auto displayRange = this->TimelinePainter->displayTimeRange();
      auto sceneRange = scene->getClockTimeRange();

      double timeScroll = mouseTime - this->Internals->ClickedTime;
      auto newStart = displayRange.first - timeScroll;
      auto newEnd = displayRange.second - timeScroll;

      if (newStart < sceneRange.first)
      {
        newStart = sceneRange.first;
        timeScroll = newStart - displayRange.first;
        newEnd = displayRange.second - timeScroll;
      }
      if (newEnd > sceneRange.second)
      {
        newEnd = sceneRange.second;
        timeScroll = newEnd - displayRange.second;
        newStart = displayRange.first - timeScroll;
      }

      this->TimelinePainter->setDisplayTimeRange(newStart, newEnd);
      Q_EMIT this->needsRepaint();
      return true;
    }
  }

  return Superclass::editorEvent(event, model, option, index);
}

//-----------------------------------------------------------------------------
bool pqTimelineItemDelegate::eventFilter(QObject* watched, QEvent* event)
{
  auto widget = dynamic_cast<QWidget*>(watched);
  auto mouseEvent = dynamic_cast<QMouseEvent*>(event);
  auto wheelEvent = dynamic_cast<QWheelEvent*>(event);
  if (!widget && !mouseEvent && !wheelEvent)
  {
    return false;
  }

  pqAnimationManager* animationManager = pqPVApplicationCore::instance()->animationManager();
  pqAnimationScene* scene = animationManager->getActiveScene();

  if (mouseEvent && event->type() == QEvent::MouseButtonRelease)
  {
    // if mouse is released outside of the widget, cancel changes
    if (!widget->geometry().contains(mouseEvent->pos()))
    {
      this->TimelinePainter->setSceneCurrentTime(scene->getAnimationTime());
      Q_EMIT this->needsRepaint();
    }
    else if (this->Internals->isGrabbing())
    {
      scene->setAnimationTime(this->TimelinePainter->getSceneCurrentTime());
    }

    this->Internals->endInteraction();
  }

  // zoom on ctrl (or cmd) + wheel
  if (wheelEvent && wheelEvent->modifiers().testFlag(Qt::ControlModifier) &&
    wheelEvent->buttons() == Qt::NoButton)
  {
    double steps = static_cast<double>(wheelEvent->angleDelta().y()) / MOUSE_WHEEL_STEPS_FACTOR;
    this->Zoom = std::max(1., this->Zoom * std::pow(ZOOM_BASE, steps));

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    double mousePos = wheelEvent->pos().x();
#else
    double mousePos = wheelEvent->position().x();
#endif

    double mouseTime = this->TimelinePainter->timeFromPosition(mousePos);
    auto start = scene->getClockTimeRange().first;
    auto end = scene->getClockTimeRange().second;

    double newStart = mouseTime - (mouseTime - start) / this->Zoom;
    double newEnd = mouseTime - (mouseTime - end) / this->Zoom;

    this->TimelinePainter->setDisplayTimeRange(newStart, newEnd);

    Q_EMIT this->needsRepaint();
    return true;
  }

  return this->Superclass::eventFilter(watched, event);
}

//-----------------------------------------------------------------------------
void pqTimelineItemDelegate::setActiveSceneConnections(pqAnimationScene* scene)
{
  if (!scene)
  {
    return;
  }

  auto sceneProxy = scene->getProxy();
  this->Internals->SceneLinks.clear();
  this->Internals->SceneLinks.addPropertyLink(this->Internals->LockStart, "checked",
    SIGNAL(toggled(bool)), sceneProxy, sceneProxy->GetProperty("LockStartTime"));
  this->Internals->SceneLinks.addPropertyLink(this->Internals->LockEnd, "checked",
    SIGNAL(toggled(bool)), sceneProxy, sceneProxy->GetProperty("LockEndTime"));

  this->Internals->SceneLinks.addPropertyLink(this->Internals->EditStart, "text",
    SIGNAL(editingFinished()), sceneProxy, sceneProxy->GetProperty("StartTime"));
  this->Internals->SceneLinks.addPropertyLink(this->Internals->EditEnd, "text",
    SIGNAL(editingFinished()), sceneProxy, sceneProxy->GetProperty("EndTime"));

  this->connect(scene, &pqAnimationScene::clockTimeRangesChanged, this,
    &pqTimelineItemDelegate::updateSceneTimeRange);
  this->connect(scene, &pqAnimationScene::animationTime, [&](double time) {
    this->TimelinePainter->setSceneCurrentTime(time);
    Q_EMIT this->needsRepaint();
  });

  this->updateSceneTimeRange();
  this->TimelinePainter->setSceneCurrentTime(scene->getAnimationTime());
}

//-----------------------------------------------------------------------------
void pqTimelineItemDelegate::updateSceneTimeRange()
{
  pqAnimationManager* animationManager = pqPVApplicationCore::instance()->animationManager();
  pqAnimationScene* scene = animationManager->getActiveScene();
  this->TimelinePainter->setSceneStartTime(scene->getClockTimeRange().first);
  this->TimelinePainter->setSceneEndTime(scene->getClockTimeRange().second);
  this->TimelinePainter->setDisplayTimeRange(
    scene->getClockTimeRange().first, scene->getClockTimeRange().second);
  this->Internals->updateValidators();
  Q_EMIT this->needsRepaint();
}
