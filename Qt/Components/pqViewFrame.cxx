/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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

#include "pqSetName.h"

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

#if WIN32
# include "process.h"
# define getpid _getpid
#else
# include "unistd.h"
#endif

#define PEN_WIDTH 2
const int ICON_SIZE = 12;

//-----------------------------------------------------------------------------
pqViewFrame::pqViewFrame(QWidget* parentObject)
  : Superclass(parentObject),
  DecorationsVisible(true),
  TitleBarVisible(true),
  BorderVisible(false),
  BorderColor(QColor("blue")),
  Buttons(SplitVertical | SplitHorizontal | Maximize | Close),
  TitleBar(new QWidget(this)),
  ToolBar (new QToolBar(this)),
  TitleLabel(new QLabel(this)),
  ContextMenu(new QMenu(this->TitleBar)),
  UniqueID(QUuid::createUuid())
{
  this->ToolBar->setIconSize(QSize (ICON_SIZE, ICON_SIZE));
  QLayout* tbLayout = this->ToolBar->layout ();
  tbLayout->setSpacing (0);
  tbLayout->setContentsMargins (0, 0, 0, 0);

  // to allow an empty frame to work with the focus stuff
  this->setFocusPolicy(Qt::ClickFocus);

  QVBoxLayout* vbox = new QVBoxLayout(this);
  this->setLayout(vbox);

  this->TitleBar->setObjectName("TitleBar");
  this->TitleBar->setAcceptDrops(true);
  this->TitleBar->setContextMenuPolicy(Qt::CustomContextMenu);
  QObject::connect(
    this->TitleBar, SIGNAL(customContextMenuRequested(const QPoint&)),
    this, SLOT(contextMenuRequested(const QPoint&)));

// limits the titlebar's height.
  this->TitleBar->installEventFilter(this);

  // Create standard buttons.
  this->StandardToolButtons[SplitVertical] =
    this->createButton(new QAction(
        QIcon(":/pqWidgets/Icons/pqSplitViewV12.png"), "Split Vertical", this)
      << pqSetName("SplitVertical"));
  this->StandardToolButtons[SplitHorizontal] =
    this->createButton(new QAction(
        QIcon(":/pqWidgets/Icons/pqSplitViewH12.png"), "Split Horizontal", this)
      << pqSetName("SplitHorizontal"));
  this->StandardToolButtons[Maximize] =
    this->createButton(new QAction(
        QIcon(this->style()->standardPixmap(QStyle::SP_TitleBarMaxButton)),
        "Maximize", this) << pqSetName("Maximize"));
  this->StandardToolButtons[Restore] =
    this->createButton(new QAction(
        QIcon(this->style()->standardPixmap(QStyle::SP_TitleBarNormalButton)),
        "Restore", this) << pqSetName("Minimize"));
  this->StandardToolButtons[Close] =
    this->createButton(new QAction(
        QIcon(this->style()->standardPixmap(QStyle::SP_TitleBarCloseButton)),
        "Close", this) << pqSetName("Close"));

  this->updateTitleBar();
  this->updateLayout();

  this->ContextMenu->setObjectName("FrameContextMenu");
  this->ContextMenu->addAction(
    this->StandardToolButtons[SplitHorizontal]->defaultAction());
  this->ContextMenu->addAction(
    this->StandardToolButtons[SplitVertical]->defaultAction());
  this->ContextMenu->addAction(
    this->StandardToolButtons[Close]->defaultAction());
}

//-----------------------------------------------------------------------------
pqViewFrame::~pqViewFrame()
{
  delete this->TitleBar;
}

//-----------------------------------------------------------------------------
void pqViewFrame::setCentralWidget(QWidget* widget)
{
  this->CentralWidget = widget;
  this->updateLayout();
}

//-----------------------------------------------------------------------------
QWidget* pqViewFrame::centralWidget() const
{
  return this->CentralWidget;
}

//-----------------------------------------------------------------------------
void pqViewFrame::setTitle(const QString& text)
{
  this->TitleLabel->setText(text);
}

//-----------------------------------------------------------------------------
QString pqViewFrame::title() const
{
  return this->TitleLabel->text();
}

//-----------------------------------------------------------------------------
void pqViewFrame::updateLayout()
{
  QVBoxLayout* vbox = new QVBoxLayout();
  if ((!this->TitleBarVisible && !this->BorderVisible) ||
    !this->DecorationsVisible)
    {
    vbox->setMargin(0);
    vbox->setSpacing(0);
    }
  else
    {
    vbox->setMargin(PEN_WIDTH);
    vbox->setSpacing(PEN_WIDTH);
    }

  if (this->TitleBarVisible && this->DecorationsVisible)
    {
    vbox->addWidget(this->TitleBar);
    }

  if (this->CentralWidget)
    {
    vbox->addWidget(this->CentralWidget);
    }
  else
    {
    vbox->addStretch();
    }

  delete this->layout();
  this->setLayout(vbox);

  // ensure that the frame is repainted.
  this->update();
}

//-----------------------------------------------------------------------------
void pqViewFrame::updateTitleBar()
{
  QHBoxLayout* hbox = new QHBoxLayout(); 
  hbox->setMargin(0);
  hbox->setSpacing(0);
  hbox->addWidget (this->ToolBar);
  hbox->addStretch();

  this->TitleLabel->setAlignment(Qt::AlignRight);
  this->TitleLabel->setIndent(10);
  hbox->addWidget(this->TitleLabel);

  foreach (QToolButton* button, this->StandardToolButtons)
    {
    button->hide();
    }
  if (this->standardButtons() & SplitHorizontal)
    {
    QToolButton* button = this->StandardToolButtons[SplitHorizontal];
    hbox->addWidget(button);
    button->show();
    }
  if (this->standardButtons() & SplitVertical)
    {
    QToolButton* button = this->StandardToolButtons[SplitVertical];
    hbox->addWidget(button);
    button->show();
    }

  if (this->standardButtons() & Maximize)
    {
    QToolButton* button = this->StandardToolButtons[Maximize];
    hbox->addWidget(button);
    button->show();
    }
  if (this->standardButtons() & Restore)
    {
    QToolButton* button = this->StandardToolButtons[Restore];
    hbox->addWidget(button);
    button->show();
    }
  if (this->standardButtons() & Close)
    {
    QToolButton* button = this->StandardToolButtons[Close];
    hbox->addWidget(button);
    button->show();
    }

  delete this->TitleBar->layout();
  this->TitleBar->setLayout(hbox);
}

//-----------------------------------------------------------------------------
void pqViewFrame::paintEvent(QPaintEvent* evt)
{
  this->Superclass::paintEvent(evt);

  if (this->BorderVisible && this->DecorationsVisible)
    {
    QPainter painter(this);
    QPen pen;
    pen.setColor(this->BorderColor);
    pen.setWidth(PEN_WIDTH);
    painter.setPen(pen);
    QRect borderRect = this->contentsRect();
    if (this->isTitleBarVisible())
      {
      QLayoutItem* titlebar = this->layout()->itemAt(0);
      borderRect.adjust(-PEN_WIDTH/2+2, 
        titlebar->geometry().height()+4-PEN_WIDTH/2, 
        PEN_WIDTH/2-2, 
        PEN_WIDTH/2-2);
      }
    else
      {
      borderRect.adjust(
        -PEN_WIDTH/2+2, PEN_WIDTH/2, PEN_WIDTH/2-2, PEN_WIDTH/2-2);
      }
    painter.drawRect(borderRect);
    }
}

//-----------------------------------------------------------------------------
void pqViewFrame::setStandardButtons(StandardButtons buttons)
{
  if (this->Buttons != buttons)
    {
    this->Buttons = buttons;
    this->updateTitleBar();
    }
}

//-----------------------------------------------------------------------------
QToolButton* pqViewFrame::createButton(QAction* action)
{
  QToolButton* toolButton = new QToolButton(this);
  toolButton->setDefaultAction(action);
  toolButton->setObjectName(action->objectName());
  toolButton->setIcon(action->icon());
  toolButton->setIconSize(QSize (ICON_SIZE, ICON_SIZE));

  QObject::connect(toolButton, SIGNAL(triggered(QAction*)),
    this, SLOT(buttonClicked()), Qt::QueuedConnection);
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
  this->ToolBar->addAction (action);
  this->updateTitleBar();
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
void pqViewFrame::removeTitleBarActions()
{
  this->ToolBar->clear();
}

//-----------------------------------------------------------------------------
void pqViewFrame::contextMenuRequested(const QPoint& point)
{
  this->setFocus(Qt::OtherFocusReason);
  this->ContextMenu->exec(this->TitleBar->mapToGlobal(point));
}

//-----------------------------------------------------------------------------
bool pqViewFrame::eventFilter(QObject* caller, QEvent* evt)
{
  if (evt->type() == QEvent::MouseButtonPress)
    {     
    QMouseEvent *mouseEvent=(QMouseEvent*)evt;
    if (mouseEvent->button() == Qt::LeftButton)
      {
      this->DragStartPosition = mouseEvent->pos();
      }
    }
  else if(evt->type() == QEvent::MouseMove)
    {
    QMouseEvent *mouseEvent=(QMouseEvent*)evt;
    if ((mouseEvent->buttons() & Qt::LeftButton) &&
      (mouseEvent->pos() - this->DragStartPosition).manhattanLength()
      >= QApplication::startDragDistance())
      {
      this->drag();
      }
    }
  else if(evt->type() == QEvent::DragEnter)
    {
    QDragEnterEvent* de=reinterpret_cast<QDragEnterEvent*>(evt);
    this->dragEnter(de);
    }
  else if(evt->type() == QEvent::DragMove)
    {
    QDragMoveEvent* de=reinterpret_cast<QDragMoveEvent*>(evt);
    this->dragMove(de);
    }
  else if(evt->type() == QEvent::Drop)
    {
    QDropEvent* de=reinterpret_cast<QDropEvent*>(evt);
    this->drop(de);
    }

  return this->Superclass::eventFilter(caller, evt);
}

//-----------------------------------------------------------------------------
void pqViewFrame::drag()
{
  QPixmap pixmap(":/pqWidgets/Icons/pqWindow16.png");

  QByteArray output;
  QDataStream dataStream(&output, QIODevice::WriteOnly);
  dataStream << this->uniqueID();

  QMimeData *mimeData = new QMimeData;
  QString mimeType = QString("application/paraview3/%1").arg(getpid());
  mimeData->setData(mimeType, output);

  QDrag *dragObj = new QDrag(this);
  dragObj->setMimeData(mimeData);
  dragObj->setHotSpot(QPoint(pixmap.width()/2, pixmap.height()/2));
  dragObj->setPixmap(pixmap);
  dragObj->start();
}

//-----------------------------------------------------------------------------
void pqViewFrame::dragEnter(QDragEnterEvent* evt)
{
  QString mimeType = QString("application/paraview3/%1").arg(getpid());
  if (evt->mimeData()->hasFormat(mimeType))
    {
    evt->accept();
    }
  else
    {
    evt->ignore();
    }
}

//-----------------------------------------------------------------------------
void pqViewFrame::dragMove(QDragMoveEvent* evt)
{
  QString mimeType = QString("application/paraview3/%1").arg(getpid());
  if (evt->mimeData()->hasFormat(mimeType))
    {
    evt->accept();
    }
  else
    {
    evt->ignore();
    }
}

//-----------------------------------------------------------------------------
void pqViewFrame::drop(QDropEvent* evt)
{
  QString mimeType = QString("application/paraview3/%1").arg(getpid());
  if (evt->mimeData()->hasFormat(mimeType))
    {
    QByteArray input= evt->mimeData()->data(mimeType);
    QDataStream dataStream(&input, QIODevice::ReadOnly);
    QUuid otherUID;
    dataStream>>otherUID;
    if (otherUID != this->uniqueID())
      {
      this->swapPositions(otherUID.toString());
      }
    evt->accept();
    }
  else
    {
    evt->ignore();
    }
}
