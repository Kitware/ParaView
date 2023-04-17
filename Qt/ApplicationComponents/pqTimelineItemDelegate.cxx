/*=========================================================================

   Program: ParaView
   Module:  pqTimelineItemDelegate.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/

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

    // Avoid Start == End. As validator is an inclusive range, use some epsilon.
    QObject::connect(this->EditStart, &pqDoubleLineEdit::editingFinished, [&]() {
      this->EndValidator->setBottom(
        this->EditStart->text().toDouble() + std::numeric_limits<double>::epsilon());
    });
    QObject::connect(this->EditEnd, &pqDoubleLineEdit::editingFinished, [&]() {
      this->StartValidator->setTop(
        this->EditStart->text().toDouble() - std::numeric_limits<double>::epsilon());
    });
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
  this->connect(this->Internals->EditStart, &pqDoubleLineEdit::editingFinished,
    this->Internals->EditStart, &pqDoubleLineEdit::hide);
  this->connect(this->Internals->EditStart, &pqDoubleLineEdit::editingFinished, this,
    &pqTimelineItemDelegate::needsRepaint);
  this->connect(this->Internals->EditEnd, &pqDoubleLineEdit::editingFinished,
    this->Internals->EditEnd, &pqDoubleLineEdit::hide);
  this->connect(this->Internals->EditEnd, &pqDoubleLineEdit::editingFinished, this,
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

  if (!this->TimelinePainter->hasStartEndLabels())
  {
    return;
  }

  // update Start/End widgets on TIME item only
  if (index.data(pqTimelineItemRole::TYPE) == pqTimelineTrack::TIME)
  {
    auto startRect = this->TimelinePainter->getStartLabelRect();
    int buttonSide = startRect.height();
    auto buttonRect = QRect(startRect.right(), startRect.top(), buttonSide, buttonSide);
    this->Internals->LockStart->setGeometry(buttonRect);

    auto endRect = this->TimelinePainter->getEndLabelRect();
    buttonSide = endRect.height();
    buttonRect = QRect(endRect.left() - buttonSide, endRect.top(), buttonSide, buttonSide);
    this->Internals->LockEnd->setGeometry(buttonRect);
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
bool pqTimelineItemDelegate::editorEvent(
  QEvent* event, QAbstractItemModel*, const QStyleOptionViewItem&, const QModelIndex&)
{
  auto mouseEvent = dynamic_cast<QMouseEvent*>(event);
  if (mouseEvent && this->TimelinePainter->hasStartEndLabels())
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

  this->Internals->EditStart->hide();
  this->Internals->EditEnd->hide();
  return Superclass::event(event);
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

  this->Internals->StartValidator->setTop(
    vtkSMPropertyHelper(sceneProxy->GetProperty("EndTime")).GetAsDouble() -
    std::numeric_limits<double>::epsilon());
  this->Internals->EndValidator->setBottom(
    vtkSMPropertyHelper(sceneProxy->GetProperty("StartTime")).GetAsDouble() +
    std::numeric_limits<double>::epsilon());

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
  Q_EMIT this->needsRepaint();
}
