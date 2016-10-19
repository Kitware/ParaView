/*=========================================================================

   Program: ParaView
   Module:    pqTreeViewSelectionHelper.h

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
#ifndef pqTreeViewSelectionHelper_h
#define pqTreeViewSelectionHelper_h

#include "pqWidgetsModule.h"
#include <QItemSelection>
#include <QObject>
#include <QTreeView>

// pqTreeViewSelectionHelper is analogous to pqTreeWidgetSelectionHelper.
// We let pqTreeWidgetSelectionHelper be since the check state handling for the
// tree view isn't as graceful as the tree widget.
class PQWIDGETS_EXPORT pqTreeViewSelectionHelper : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqTreeViewSelectionHelper(QTreeView* view);
  virtual ~pqTreeViewSelectionHelper();

protected slots:
  void onClicked(QModelIndex idx);
  void onPressed(QModelIndex idx);
  void showContextMenu(const QPoint&);
  void saveSelection();

private:
  Q_DISABLE_COPY(pqTreeViewSelectionHelper)

  void setSelectedItemsCheckState(Qt::CheckState state);

  QTreeView* TreeView;
  QItemSelection PrevSelection;
  QItemSelection CurrentSelection;
  int PressState;
};

#endif
