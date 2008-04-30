/*=========================================================================

   Program: ParaView
   Module:    pqMultiViewFrame.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

#ifndef _pqMultiViewFrame_h
#define _pqMultiViewFrame_h

#include <QWidget>
#include "pqComponentsExport.h"
#include "ui_pqMultiViewFrameMenu.h"
#include <QUuid>

class QMenu;

/// a holder for a widget in a multiview
class PQCOMPONENTS_EXPORT pqMultiViewFrame : public QWidget, public Ui::MultiViewFrameMenu
{
  Q_OBJECT
  Q_PROPERTY(bool menuAutoHide READ menuAutoHide WRITE setMenuAutoHide)
  Q_PROPERTY(bool active WRITE setActive READ active)
  Q_PROPERTY(QColor borderColor WRITE setBorderColor READ borderColor)
public:
  pqMultiViewFrame(QWidget* parent = NULL);
  ~pqMultiViewFrame();

  /// sets the window title in the title bar and the widget.
  void setTitle(const QString& title);

  /// whether the menu is auto hidden
  bool menuAutoHide() const;
  /// whether the menu is auto hidden
  void setMenuAutoHide(bool);

  /// set the main widget for this holder
  void setMainWidget(QWidget*);
  /// get the main widget for this holder
  QWidget* mainWidget();
 
  /// Returns the QWidget that is used as the 
  /// main widget in absence of any main widget.
  /// This can be used to setup a UI to show
  /// when there is not main widget present.
  QWidget* emptyMainWidget() const
    { return this->EmptyMainWidget; }

  /// get whether active, if active, a border is drawn
  bool active() const;
  /// get the color of the border
  QColor borderColor() const;

  void hideMenu(bool hide);

  QUuid uniqueID() const;

  QMenu* getContextMenu() const
    { return this->ContextMenu; }

  void addTitlebarAction(QAction* action);
  void removeTitlebarAction(QAction* action);
  QAction* getAction(QString name);

public slots:

  /// close this frame, emits closePressed() so receiver does the actual remove
  void close();
  /// maximize this frame, emits maximizePressed() so receiver does the actual maximize
  void maximize();
  /// restores this frame, emits restoredPRessed() so receiver does the actual resotre
  void restore();
  /// split this frame vertically, emits splitVerticalPressed so receiver does the actual split
  void splitVertical();
  /// split this frame horizontally, emits splitVerticalPressed so receiver does the actual split
  void splitHorizontal();
  /// sets the border color
  void setBorderColor(QColor);
  /// sets whether this frame is active.  if active, a border is drawn
  void setActive(bool);

  /// hides the frame decorations.
  void hideDecorations();

  /// shows the frame decorations.
  void showDecorations();
signals:
  /// signal active state changed
  void activeChanged(bool);
  /// signal close pressed
  void closePressed();
  /// signal maximize pressed
  void maximizePressed();
  /// signal restore pressed
  void restorePressed();
  /// signal split vertical pressed
  void splitVerticalPressed();
  /// signal split horizontal pressed
  void splitHorizontalPressed();
  /// drag start event
  void dragStart(pqMultiViewFrame*);
  /// drag enter event
  void dragEnter(pqMultiViewFrame*,QDragEnterEvent*);
  /// drag move event
  void dragMove(pqMultiViewFrame*,QDragMoveEvent*);
  /// drop event
  void drop(pqMultiViewFrame*,QDropEvent*);
  /// fired before the context menu is shown for this frame.
  void contextMenuRequested();
  // Signal emmited before the main widget gets changed for this frame
  void mainWidgetPreChange(pqMultiViewFrame*);
  /// Signal emitted after the main widget has changed for this frame
  void mainWidgetChanged(pqMultiViewFrame*);

protected slots:
  /// called when a context menu is requested.
  void onCustomContextMenuRequested(const QPoint& point);

protected:
  void paintEvent(QPaintEvent* e);
  bool eventFilter(QObject*, QEvent* e);
  bool event(QEvent* e);

private:
  QWidget* EmptyMainWidget;
  QMenu* ContextMenu;
  bool AutoHide;
  bool Active;
  bool MenuHidden;
  QColor Color;
  QWidget* Menu;
  QPoint DragStartPosition;
  QUuid UniqueID;
  QList<QAction*> TitlebarActions;
};

#endif //_pqMultiViewFrame_h

