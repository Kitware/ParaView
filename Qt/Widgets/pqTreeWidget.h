// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqTreeWidget_h
#define pqTreeWidget_h

#include "pqWidgetsModule.h"
#include <QTreeWidget>

class vtkPVXMLElement;
class vtkSMProperty;
class vtkSMPropertyGroup;
/**
 * @class pqTreeWidget
 * @brief a ParaView specific customization of QTreeWidget.
 *
 * A convenience QTreeWidget with extra features:
 * \li Automatic size hints based on contents
 * \li A check box added in a header if items have check boxes
 * \li Navigation through columns of top level items on Tab.
 * \li Signal emitted when user navigates beyond end of the table giving an
 *     opportunity to the lister to grow the table.
 * \li Avoid grabbing scroll focus: Wheel events are not handled by the widget
 *     unless the widget has focus. Together with change in focus policy to
 *     Qt::StrongFocus instead of the default Qt::WheelFocus, we improve the
 *     widget scroll behavior when nested in other scrollable panels.
 */
class PQWIDGETS_EXPORT pqTreeWidget : public QTreeWidget
{
  typedef QTreeWidget Superclass;
  Q_OBJECT
public:
  pqTreeWidget(QWidget* p = nullptr);
  ~pqTreeWidget() override;

  bool event(QEvent* e) override;

  /**
   * give a hint on the size
   */
  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

  void setMaximumRowCountBeforeScrolling(vtkSMPropertyGroup* smpropertygroup);
  void setMaximumRowCountBeforeScrolling(vtkSMProperty* smproperty);
  void setMaximumRowCountBeforeScrolling(vtkPVXMLElement* hints);
  void setMaximumRowCountBeforeScrolling(int val) { this->MaximumRowCountBeforeScrolling = val; }
  int maximumRowCountBeforeScrolling() const { return this->MaximumRowCountBeforeScrolling; }

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void allOn();
  void allOff();

Q_SIGNALS:
  /**
   * Fired when moveCursor takes the cursor beyond the last row.
   */
  void navigatedPastEnd();

protected Q_SLOTS:
  void doToggle(int col);
  void updateCheckState();
  void invalidateLayout();

private Q_SLOTS:
  void updateCheckStateInternal();

protected:
  QPixmap** CheckPixmaps;
  QPixmap pixmap(Qt::CheckState state, bool active);

  /**
   * Overridden to eat wheel events unless this->hasFocus().
   */
  void wheelEvent(QWheelEvent* event) override;

  /**
   * Move the cursor in the way described by cursorAction,
   * using the information provided by the button modifiers.
   */
  QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;

  QTimer* Timer;

  int itemCount(QTreeWidgetItem* item) const;
  int MaximumRowCountBeforeScrolling;
};

#endif // !pqTreeWidget_h
