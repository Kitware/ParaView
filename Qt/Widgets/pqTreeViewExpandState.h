/*=========================================================================

   Program: ParaView
   Module:  pqTreeViewExpandState.h

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
#ifndef pqTreeViewExpandState_h
#define pqTreeViewExpandState_h

#include "pqWidgetsModule.h" // for exports
#include <QObject>
#include <QScopedPointer> // for ivar
class QTreeView;

/**
 * @class pqTreeViewExpandState
 * @brief save/restore expand-state for items in a tree view.
 *
 * pqTreeViewExpandState is a helper class that can be used to save/restore
 * expansion state for nodes in the tree view even after the view's model has been
 * reset or severely modified.
 *
 * Example usage:
 * @code{cpp}
 *
 *  pqTreeViewExpandState helper;
 *  helper.save(treeView);
 *
 *  treeView->model()->reset();
 *  // do other changes to model.
 *
 *  // optional default expand state.
 *  treeView->expandToDepth(1);
 *
 *  helper.restore(treeView);
 *
 * @endcode
 *
 * Applications often want to have a default expand state for tree views. To do
 * that, simply call `QTreeView::expand`, `QTreeView::expandAll`, or
 * `QTreeView::expandToDepth` before calling `restore`. pqTreeViewExpandState
 * ensures that it doesn't not save (and hence restore) state for the tree if
 * the tree has only 1 or less nodes when `save` is called.
 *
 */
class PQWIDGETS_EXPORT pqTreeViewExpandState : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqTreeViewExpandState(QObject* parent = nullptr);
  virtual ~pqTreeViewExpandState();

  void save(QTreeView* treeView);
  void restore(QTreeView* treeView);

private:
  Q_DISABLE_COPY(pqTreeViewExpandState);
  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
