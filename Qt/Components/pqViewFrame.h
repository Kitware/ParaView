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
#ifndef pqViewFrame_h
#define pqViewFrame_h

#include "pqComponentsModule.h"
#include <QMap>
#include <QPointer>
#include <QScopedPointer>
#include <QUuid>
#include <QWidget>

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QFrame;
class QLabel;
class QMenu;
class QToolBar;
class QToolButton;
class pqServerManagerModelItem;
class pqView;

/**
* pqViewFrame is used to represent a frame for any ParaView view shown in the
* pqMultiViewWidget. A frame has title-bar that can be used to show arbitrary
* buttons, as well as a border that can be used to indicate if the frame is
* active.
*/
class PQCOMPONENTS_EXPORT pqViewFrame : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqViewFrame(QWidget* parent = 0);
  ~pqViewFrame() override;

  /**
  * Get/Set the window title. If not empty, the title is shown in the
  * title-bar.
  */
  void setTitle(const QString& text);
  QString title() const;

  /**
  * Get/Set the central widget shown in this frame. Similar to
  * QLayout::addWidget, this call takes the ownership of the widget and the
  * widget will be deleted with pqViewFrame is deleted or another widget is set
  * using setCentralWidget().
  */
  void setCentralWidget(QWidget* widget, pqView* view = 0);
  QWidget* centralWidget() const;

  /**
  * Get/Set the border BorderColor. The border is only drawn when the
  * borderVisibility is set to true.
  */
  void setBorderColor(const QColor& clr);
  const QColor& borderColor() const { return this->BorderColor; }

  /**
  * Get/Set the border visibility.
  */
  bool isBorderVisible() const { return this->BorderVisible; }

  /**
  * Get whether the title-bar is shown.
  */
  bool isTitleBarVisible() const { return this->TitleBarVisible; }

  enum StandardButton
  {
    NoButton = 0x0000,
    SplitHorizontal = 0x0001,
    SplitVertical = 0x0002,
    Maximize = 0x0004,
    Restore = 0x0008,
    Close = 0x0010
  };

  Q_DECLARE_FLAGS(StandardButtons, StandardButton);

  /**
  * This holds the collection of standard buttons the frame should show in the
  * title-bar.
  */
  void setStandardButtons(StandardButtons buttons);
  StandardButtons standardButtons() const { return this->Buttons; }

  /**
  * One can add arbitrary actions to be shown on the title bar.
  */
  void addTitleBarAction(QAction* action);
  QAction* addTitleBarAction(const QString& title);
  QAction* addTitleBarAction(const QIcon& icon, const QString& title);

  /**
  * One can add a separator between actions
  */
  QAction* addTitleBarSeparator();

  /**
  * remove all added custom title-bar actions.
  */
  void removeTitleBarActions();

  /**
  * Provides access to the context menu.
  */
  QMenu* contextMenu() const;

  /**
  * provides access to the unique id assigned to the frame.
  */
  QUuid uniqueID() const { return this->UniqueID; }

Q_SIGNALS:
  /**
  * fired when a standard button is pressed.
  */
  void buttonPressed(int button);

  /**
  * fired when one of the custom title-bar actions is triggered.
  */
  void actionTriggered(QAction* action);

  /**
  * Fired to indicate the positions for the two frames need to be swapped.
  */
  void swapPositions(const QString& other);

  /**
  * Internal signal, fired to notify the target
  * pqViewFrame instance that the drag operation has completed.
  */
  void finishDrag(pqViewFrame* source);

public Q_SLOTS:
  /**
  * set whether the border is visible.
  */
  void setBorderVisibility(bool val)
  {
    this->BorderVisible = val;
    this->updateComponentVisibilities();
  }

  /**
  * set whether the title-bar is visible.
  */
  void setTitleBarVisibility(bool val)
  {
    this->TitleBarVisible = val;
    this->updateComponentVisibilities();
  }

  void setDecorationsVisibility(bool val)
  {
    this->DecorationsVisible = val;
    this->updateComponentVisibilities();
  }

  void onViewNameChanged(pqServerManagerModelItem*);

  /**
  * event filter to handle drag/drop events.
  */
  bool eventFilter(QObject*, QEvent*) override;

protected:
  /**
  * Updates the visibilities for various components of the pqViewFrame based
  * on flags set on the instance.
  */
  virtual void updateComponentVisibilities();

  /**
  * methods to manage drag-drop.
  */
  void drag();
  void dragEnter(QDragEnterEvent*);
  void drop(QDropEvent*);

protected Q_SLOTS:
  void buttonClicked();
  void contextMenuRequested(const QPoint&);

protected:
  bool DecorationsVisible;
  bool TitleBarVisible;
  bool BorderVisible;
  QColor BorderColor;
  StandardButtons Buttons;
  QPointer<QToolBar> ToolBar;
  QPointer<QWidget> CentralWidget;
  QPointer<QMenu> ContextMenu;
  QUuid UniqueID;
  QPoint DragStartPosition;
  typedef QMap<StandardButton, QPointer<QToolButton> > StandardToolButtonsMap;
  StandardToolButtonsMap StandardToolButtons;
  QPalette PaletteWithBorder;
  QPalette PaletteWithoutBorder;
  QString PlainTitle;

private:
  Q_DISABLE_COPY(pqViewFrame)

  /**
  * creates a tool button for the action.
  */
  QToolButton* createButton(QAction* action);

  class pqInternals;
  const QScopedPointer<pqInternals> Internals;
private Q_SLOTS:
  void finishedDrag(pqViewFrame* source);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(pqViewFrame::StandardButtons);

#endif
