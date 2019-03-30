/*=========================================================================

   Program: ParaView
   Module:  pqViewFrame.cxx

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
#include "pqViewFrame.h"
#include "ui_pqViewFrame.h"

#include "pqSetName.h"
#include "pqView.h"
#include <QAction>
#include <QApplication>
#include <QDrag>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

#include <cassert>

#include "vtksys/SystemInformation.hxx"

const int ICON_SIZE = 12;

class pqViewFrame::pqInternals
{
public:
  Ui::pqViewFrame Ui;
  pqInternals(pqViewFrame* self) { this->Ui.setupUi(self); }
};

//-----------------------------------------------------------------------------
pqViewFrame::pqViewFrame(QWidget* parentObject)
  : Superclass(parentObject)
  , DecorationsVisible(true)
  , TitleBarVisible(true)
  , BorderVisible(false)
  , Buttons(NoButton)
  , UniqueID(QUuid::createUuid())
  , Internals(new pqViewFrame::pqInternals(this))
{
  Ui::pqViewFrame& ui = this->Internals->Ui;

  this->ToolBar = new QToolBar(this);
  this->ToolBar->setObjectName("ToolBar");
  this->ToolBar->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
  this->ToolBar->layout()->setSpacing(0);
  this->ToolBar->layout()->setContentsMargins(0, 0, 0, 0);

  this->connect(ui.TitleBar, SIGNAL(customContextMenuRequested(const QPoint&)),
    SLOT(contextMenuRequested(const QPoint&)));

  // To handle drag/drop event.
  ui.TitleBar->installEventFilter(this);

  // Create standard buttons.
  this->StandardToolButtons[SplitVertical] = this->createButton(
    new QAction(QIcon(":/pqWidgets/Icons/pqSplitViewV12.png"), "Split Vertical", this)
    << pqSetName("SplitVertical"));
  this->StandardToolButtons[SplitHorizontal] = this->createButton(
    new QAction(QIcon(":/pqWidgets/Icons/pqSplitViewH12.png"), "Split Horizontal", this)
    << pqSetName("SplitHorizontal"));
  this->StandardToolButtons[Maximize] = this->createButton(
    new QAction(
      QIcon(this->style()->standardPixmap(QStyle::SP_TitleBarMaxButton)), "Maximize", this)
    << pqSetName("Maximize"));
  this->StandardToolButtons[Restore] = this->createButton(
    new QAction(
      QIcon(this->style()->standardPixmap(QStyle::SP_TitleBarNormalButton)), "Restore", this)
    << pqSetName("Minimize"));
  this->StandardToolButtons[Close] = this->createButton(
    new QAction(QIcon(this->style()->standardPixmap(QStyle::SP_TitleBarCloseButton)), "Close", this)
    << pqSetName("Close"));

  // Setup the title bar.
  ui.TitleBarLayout->insertWidget(0, this->ToolBar);
  foreach (QToolButton* button, this->StandardToolButtons)
  {
    ui.TitleBarLayout->addWidget(button);
  }

  this->ContextMenu = new QMenu(ui.TitleBar), this->ContextMenu->setObjectName("FrameContextMenu");
  this->ContextMenu->addAction(this->StandardToolButtons[SplitHorizontal]->defaultAction());
  this->ContextMenu->addAction(this->StandardToolButtons[SplitVertical]->defaultAction());
  this->ContextMenu->addAction(this->StandardToolButtons[Close]->defaultAction());

  this->setBorderColor(QColor("blue"));
  this->Internals->Ui.TitleLabel->installEventFilter(this);

  this->setStandardButtons(SplitVertical | SplitHorizontal | Maximize | Close);
}

//-----------------------------------------------------------------------------
pqViewFrame::~pqViewFrame()
{
}

//-----------------------------------------------------------------------------
void pqViewFrame::setBorderColor(const QColor& clr)
{
  if (this->BorderColor != clr)
  {
    this->BorderColor = clr;
    this->updateComponentVisibilities();
  }
}

//-----------------------------------------------------------------------------
void pqViewFrame::setCentralWidget(QWidget* widget, pqView* view)
{
  this->CentralWidget = widget;
  Ui::pqViewFrame& ui = this->Internals->Ui;
  while (QLayoutItem* item = ui.CentralWidgetFrameLayout->takeAt(0))
  {
    delete item;
  }
  if (this->CentralWidget)
  {
    ui.CentralWidgetFrameLayout->addWidget(this->CentralWidget);
  }
  if (view)
  {
    this->connect(view, SIGNAL(nameChanged(pqServerManagerModelItem*)),
      SLOT(onViewNameChanged(pqServerManagerModelItem*)));
  }
}

//-----------------------------------------------------------------------------
void pqViewFrame::onViewNameChanged(pqServerManagerModelItem* item)
{
  pqView* view = dynamic_cast<pqView*>(item);
  if (view)
  {
    this->setTitle(view->getSMName());
  }
}

//-----------------------------------------------------------------------------
QWidget* pqViewFrame::centralWidget() const
{
  return this->CentralWidget;
}

//-----------------------------------------------------------------------------
void pqViewFrame::setTitle(const QString& text)
{
  this->PlainTitle = text;
  this->updateComponentVisibilities();
}

//-----------------------------------------------------------------------------
QString pqViewFrame::title() const
{
  return this->Internals->Ui.TitleLabel->text();
}

//-----------------------------------------------------------------------------
void pqViewFrame::updateComponentVisibilities()
{
  bool showTitleBar = this->DecorationsVisible && this->TitleBarVisible;
  bool showBorder = this->DecorationsVisible && this->BorderVisible;

  Ui::pqViewFrame& ui = this->Internals->Ui;
  ui.TitleBar->setVisible(showTitleBar);
  if (showBorder)
  {
    ui.CentralWidgetFrame->setForegroundRole(QPalette::WindowText);
    ui.CentralWidgetFrame->setStyleSheet(
      QString("QFrame#CentralWidgetFrame { color: rgb(%1, %2, %3); }")
        .arg(this->BorderColor.red())
        .arg(this->BorderColor.green())
        .arg(this->BorderColor.blue()));
    ui.TitleLabel->setText(QString("<b><u>%1</u></b>").arg(this->PlainTitle));
  }
  else
  {
    ui.CentralWidgetFrame->setForegroundRole(QPalette::Window);
    ui.CentralWidgetFrame->setStyleSheet(QString());
    ui.TitleLabel->setText(this->PlainTitle);
  }
  if (this->DecorationsVisible)
  {
    ui.CentralWidgetFrame->setFrameStyle(QFrame::Plain | QFrame::Box);
    ui.CentralWidgetFrameLayout->setContentsMargins(1, 1, 1, 1);
  }
  else
  {
    ui.CentralWidgetFrame->setFrameStyle(QFrame::NoFrame);
    ui.CentralWidgetFrameLayout->setContentsMargins(0, 0, 0, 0);
  }
}

//-----------------------------------------------------------------------------
void pqViewFrame::setStandardButtons(StandardButtons buttons)
{
  if (this->Buttons != buttons)
  {
    this->Buttons = buttons;
    for (StandardToolButtonsMap::iterator iter = this->StandardToolButtons.begin();
         iter != this->StandardToolButtons.end(); ++iter)
    {
      iter.value()->setVisible(this->Buttons & iter.key());
    }
  }
}

//-----------------------------------------------------------------------------
QToolButton* pqViewFrame::createButton(QAction* action)
{
  QToolButton* toolButton = new QToolButton(this);
  toolButton->setDefaultAction(action);
  toolButton->setObjectName(action->objectName());
  toolButton->setIcon(action->icon());
  toolButton->setIconSize(QSize(ICON_SIZE, ICON_SIZE));

  QObject::connect(
    toolButton, SIGNAL(triggered(QAction*)), this, SLOT(buttonClicked()), Qt::QueuedConnection);
  return toolButton;
}

//-----------------------------------------------------------------------------
void pqViewFrame::buttonClicked()
{
  QToolButton* toolButton = qobject_cast<QToolButton*>(this->sender());
  if (toolButton)
  {
    StandardButton key = this->StandardToolButtons.key(toolButton, NoButton);
    if (key != NoButton)
    {
      emit this->buttonPressed(key);
    }
  }
}

//-----------------------------------------------------------------------------
void pqViewFrame::addTitleBarAction(QAction* action)
{
  this->ToolBar->addAction(action);
}

//-----------------------------------------------------------------------------
QAction* pqViewFrame::addTitleBarAction(const QString& titlestr)
{
  QAction* action = new QAction(titlestr, this);
  action->setObjectName(titlestr);
  this->addTitleBarAction(action);
  return action;
}

//-----------------------------------------------------------------------------
QAction* pqViewFrame::addTitleBarAction(const QIcon& icon, const QString& titlestr)
{
  QAction* action = new QAction(icon, titlestr, this);
  action->setObjectName(titlestr);
  this->addTitleBarAction(action);
  return action;
}

//-----------------------------------------------------------------------------
QAction* pqViewFrame::addTitleBarSeparator()
{
  QAction* separator = new QAction(this);
  separator->setSeparator(true);
  this->addTitleBarAction(separator);
  return separator;
}

//-----------------------------------------------------------------------------
void pqViewFrame::removeTitleBarActions()
{
  this->ToolBar->clear();
}

//-----------------------------------------------------------------------------
void pqViewFrame::contextMenuRequested(const QPoint& point)
{
  this->setFocus(Qt::OtherFocusReason);

  Ui::pqViewFrame& ui = this->Internals->Ui;
  this->ContextMenu->exec(ui.TitleBar->mapToGlobal(point));
}

//-----------------------------------------------------------------------------
bool pqViewFrame::eventFilter(QObject* caller, QEvent* evt)
{
  Q_UNUSED(caller);
  if (evt->type() == QEvent::MouseButtonPress)
  {
    QMouseEvent* mouseEvent = reinterpret_cast<QMouseEvent*>(evt);
    if (mouseEvent->button() == Qt::LeftButton)
    {
      this->DragStartPosition = mouseEvent->pos();
    }
  }
  else if (evt->type() == QEvent::MouseMove)
  {
    QMouseEvent* mouseEvent = reinterpret_cast<QMouseEvent*>(evt);
    if ((mouseEvent->buttons() & Qt::LeftButton) &&
      (mouseEvent->pos() - this->DragStartPosition).manhattanLength() >=
        QApplication::startDragDistance())
    {
      this->drag();
      return true;
    }
  }
  else if (evt->type() == QEvent::DragEnter)
  {
    QDragEnterEvent* de = reinterpret_cast<QDragEnterEvent*>(evt);
    this->dragEnter(de);
    return true;
  }
  else if (evt->type() == QEvent::Drop)
  {
    QDropEvent* de = reinterpret_cast<QDropEvent*>(evt);
    this->drop(de);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void pqViewFrame::drag()
{
  vtksys::SystemInformation sysInfo;
  QPixmap pixmap(":/pqWidgets/Icons/pqWindow16.png");

  QMimeData* mimeData = new QMimeData;
  mimeData->setText(QString("application/paraview3/%1").arg(sysInfo.GetProcessId()));

  QPointer<QDrag> dragObj = new QDrag(this);
  dragObj->setMimeData(mimeData);
  dragObj->setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));
  dragObj->setPixmap(pixmap);
  if (dragObj->exec() == Qt::MoveAction)
  {
    // Let the target know that the drag operation has concluded.
    emit this->finishDrag(this);
  }
  // It seems we are not supposed to call delete on QDrag. It gets deleted on
  // its own. Calling delete was causing segfaults on Linux in obscure call
  // stack.
  // delete dragObj.
}

//-----------------------------------------------------------------------------
void pqViewFrame::dragEnter(QDragEnterEvent* evt)
{
  vtksys::SystemInformation sysInfo;
  QString mimeType = QString("application/paraview3/%1").arg(sysInfo.GetProcessId());
  if (evt->source() != this && evt->mimeData()->hasText() && evt->mimeData()->text() == mimeType)
  {
    evt->acceptProposedAction();
  }
}

//-----------------------------------------------------------------------------
void pqViewFrame::drop(QDropEvent* evt)
{
  vtksys::SystemInformation sysInfo;
  pqViewFrame* source = qobject_cast<pqViewFrame*>(evt->source());
  QString mimeType = QString("application/paraview3/%1").arg(sysInfo.GetProcessId());
  if (source && source != this && evt->mimeData()->hasText() && evt->mimeData()->text() == mimeType)
  {
    this->connect(source, SIGNAL(finishDrag(pqViewFrame*)), SLOT(finishedDrag(pqViewFrame*)));
    evt->acceptProposedAction();
  }
}

//-----------------------------------------------------------------------------
void pqViewFrame::finishedDrag(pqViewFrame* source)
{
  assert(source != NULL);
  emit this->swapPositions(source->uniqueID().toString());
}

//-----------------------------------------------------------------------------
QMenu* pqViewFrame::contextMenu() const
{
  return this->ContextMenu;
}
