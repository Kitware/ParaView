// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
  pqViewFrame(QWidget* parent = nullptr);
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
  void setCentralWidget(QWidget* widget, pqView* view = nullptr);
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
   * Internal signal, fired to notify the target pqViewFrame instance that the
   * drag operation has completed.
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

protected Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void buttonClicked();
  void contextMenuRequested(const QPoint&);

protected: // NOLINT(readability-redundant-access-specifiers)
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
  typedef QMap<StandardButton, QPointer<QToolButton>> StandardToolButtonsMap;
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
private Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void finishedDrag(pqViewFrame* source);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(pqViewFrame::StandardButtons);

#endif
