/*=========================================================================

   Program: ParaView
   Module:    pqTreeWidgetSelectionHelper.h

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
#ifndef pqTreeWidgetSelectionHelper_h
#define pqTreeWidgetSelectionHelper_h

#include "pqWidgetsModule.h"
#include <QItemSelection>
#include <QObject>

class QTreeWidget;
class QTreeWidgetItem;
/**
* pqTreeWidgetSelectionHelper enables multiple element selection and the
* toggling on then changing the checked state of the selected elements.
* Hence, the user can do things like selecting a subset of nodes and then
* (un)checking all of them etc. This cannot work in parallel with
* pqTreeWidgetCheckHelper, hence only once of the two must be used on the same
* tree widget.
* CAVEATS: This helper currently assumes that the 0-th column is checkable (if
* at all). This can be fixed if needed.
*/
class PQWIDGETS_EXPORT pqTreeWidgetSelectionHelper : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqTreeWidgetSelectionHelper(QTreeWidget* treeWidget);
  ~pqTreeWidgetSelectionHelper() override;

protected Q_SLOTS:
  void onItemPressed(QTreeWidgetItem* item, int column);
  void showContextMenu(const QPoint&);

private:
  Q_DISABLE_COPY(pqTreeWidgetSelectionHelper)

  void setSelectedItemsCheckState(Qt::CheckState state);

  QTreeWidget* TreeWidget;
  QItemSelection Selection;
  int PressState;
};

#endif
