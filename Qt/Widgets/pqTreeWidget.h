/*=========================================================================

   Program: ParaView
   Module:    pqTreeWidget.h

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

#ifndef _pqTreeWidget_h
#define _pqTreeWidget_h

#include "pqWidgetsModule.h"
#include <QTreeWidget>

class vtkPVXMLElement;
class vtkSMProperty;
class vtkSMPropertyGroup;
/**
  A convenience QTreeWidget with extra features:
  1.  Automatic size hints based on contents
  2.  A check box added in a header if items have check boxes
  3.  Navigation through columns of top level items on Tab.
  4.  Signal emitted when user navigates beyond end of the table giving an
      opportunity to the lister to grow the table.
*/
class PQWIDGETS_EXPORT pqTreeWidget : public QTreeWidget
{
  typedef QTreeWidget Superclass;
  Q_OBJECT
public:
  pqTreeWidget(QWidget* p = NULL);
  ~pqTreeWidget();

  bool event(QEvent* e);

  /**
  * give a hint on the size
  */
  QSize sizeHint() const;
  QSize minimumSizeHint() const;

  void setMaximumRowCountBeforeScrolling(vtkSMPropertyGroup* smpropertygroup);
  void setMaximumRowCountBeforeScrolling(vtkSMProperty* smproperty);
  void setMaximumRowCountBeforeScrolling(vtkPVXMLElement* hints);
  void setMaximumRowCountBeforeScrolling(int val) { this->MaximumRowCountBeforeScrolling = val; }
  int maximumRowCountBeforeScrolling() const { return this->MaximumRowCountBeforeScrolling; }

public slots:
  void allOn();
  void allOff();

signals:
  /**
  * Fired when moveCursor takes the cursor beyond the last row.
  */
  void navigatedPastEnd();

protected slots:
  void doToggle(int col);
  void updateCheckState();
  void invalidateLayout();

private slots:
  void updateCheckStateInternal();

protected:
  QPixmap** CheckPixmaps;
  QPixmap pixmap(Qt::CheckState state, bool active);

  /**
  * Move the cursor in the way described by cursorAction,
  * using the information provided by the button modifiers.
  */
  virtual QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers);

  QTimer* Timer;

  int itemCount(QTreeWidgetItem* item) const;
  int MaximumRowCountBeforeScrolling;
};

#endif // !_pqTreeWidget_h
