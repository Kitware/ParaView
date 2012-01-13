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
#ifndef __pqViewFrame_h 
#define __pqViewFrame_h

#include "pqComponentsExport.h"
#include <QWidget>
#include <QPointer>
#include <QMap>
#include <QUuid>

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QMenu;
class QToolButton;

/// pqViewFrame is used to represent a frame for any ParaView view shown in the
/// pqMultiViewWidget. A frame has title-bar that can be used to show arbitrary
/// buttons, as well as a border that can be used to indicate if the frame is
/// active.
class PQCOMPONENTS_EXPORT pqViewFrame : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;
public:
  pqViewFrame(QWidget* parent=0);
  virtual ~pqViewFrame();

  /// Get/Set the window title. If not empty, the title is shown in the
  /// title-bar.
  void setTitle(const QString& text) { this->Title = text; }
  const QString& title() const { return this->Title; }

  /// Get/Set the central widget shown in this frame.
  void setCentralWidget(QWidget* widget);
  QWidget* centralWidget() const;

  /// Get/Set the border BorderColor. The border is only drawn when the
  /// borderVisibility is set to true.
  void setBorderColor(const QColor& clr) { this->BorderColor = clr; }
  const QColor& borderColor() const { return this->BorderColor; }

  /// Get/Set the border visibility.
  bool isBorderVisible() const { return this->BorderVisible; }

  /// Get whether the title-bar is shown.
  bool isTitleBarVisible() const { return this->TitleBarVisible; }

  enum StandardButton
    {
    NoButton          =0x0000,
    SplitVertical     =0x0001,
    SplitHorizontal   =0x0002,
    Maximize          =0x0004,
    Restore           =0x0008,
    Close             =0x0010
    };

  Q_DECLARE_FLAGS(StandardButtons, StandardButton);

  /// This holds the collection of standard buttons the frame should show in the
  /// title-bar.
  void setStandardButtons(StandardButtons buttons);
  StandardButtons standardButtons() const { return this->Buttons; }

  /// One can add arbitrary actions to be shown on the title bar.
  void addTitleBarAction(QAction* action);
  QAction* addTitleBarAction(const QString& title);
  QAction* addTitleBarAction(const QIcon& icon, const QString& title);

  /// remove all added custom title-bar actions.
  void removeTitleBarActions();

  /// Provides access to the context menu.
  QMenu* contextMenu() const
    { return this->ContextMenu; }

  /// provides access to the unique id assigned to the frame.
  QUuid uniqueID() const
    { return this->UniqueID; }

signals:
  /// fired when a standard button is pressed.
  void buttonPressed(int button);

  /// fired when one of the custom title-bar actions is triggered.
  void actionTriggered(QAction* action);

  /// Fired to indicate the positions for the two frames need to be swapped.
  void swapPositions(const QString& other);
  
public slots:
  /// set whether the border is visible.
  void setBorderVisibility(bool val) 
    {
    this->BorderVisible = val;
    this->updateLayout();
    }

  /// set whether the title-bar is visible.
  void setTitleBarVisibility(bool val)
    {
    this->TitleBarVisible = val;
    this->updateLayout();
    }

  void setDecorationsVisibility(bool val)
    {
    this->DecorationsVisible = val;
    this->updateLayout();
    }

  /// event filter to handle drag/drop events.
  virtual bool eventFilter(QObject*, QEvent*);

protected:
  /// updates the layout.
  void updateLayout();
  void updateTitleBar();
  void paintEvent(QPaintEvent* event);

  /// methods to manage drag-drop.
  void drag();
  void dragEnter(QDragEnterEvent*);
  void dragMove(QDragMoveEvent*);
  void drop(QDropEvent*);

protected slots:
  void buttonClicked();
  void contextMenuRequested(const QPoint&);

protected:
  bool DecorationsVisible;
  bool TitleBarVisible;
  bool BorderVisible;
  QColor BorderColor;
  QString Title;
  StandardButtons Buttons;
  QPointer<QWidget> CentralWidget;
  QPointer<QWidget> TitleBar;
  QMenu* ContextMenu;
  QUuid UniqueID;
  QPoint DragStartPosition;
  QList<QToolButton*> ToolButtons;
  QMap<StandardButton, QPointer<QToolButton> > StandardToolButtons;

private:
  Q_DISABLE_COPY(pqViewFrame)

  /// creates a tool button for the action.
  QToolButton* createButton(QAction* action);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(pqViewFrame::StandardButtons);

#endif
