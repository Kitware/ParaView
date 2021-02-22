/*=========================================================================

   Program: ParaView
   Module:  pqTreeViewExpandState.cxx

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
#include "pqTreeViewExpandState.h"

#include <QAbstractItemModel>
#include <QTreeView>

class pqTreeViewExpandState::pqInternals
{
  class CNode
  {
  public:
    bool Expanded;
    std::vector<CNode> Children;

    CNode()
      : Expanded(false)
    {
    }

    void save(
      QTreeView* treeView, QAbstractItemModel* model, const QModelIndex& idx, bool is_root = false)
    {
      if (is_root || treeView->isExpanded(idx))
      {
        this->Expanded = true;
        int childCount = model->rowCount(idx);
        this->Children.resize(childCount);
        for (int cc = 0; cc < childCount; ++cc)
        {
          this->Children[cc].save(treeView, model, model->index(cc, 0, idx));
        }
      }
      else
      {
        this->Expanded = false;
        // don't care about expanded state for children if parent is not expanded.
        this->Children.clear();
      }
    }

    void restore(QTreeView* treeView, QAbstractItemModel* model, const QModelIndex& idx)
    {
      if (this->Expanded)
      {
        treeView->expand(idx);
        int childCount = model->rowCount(idx);
        for (int cc = 0, max = qMin(childCount, static_cast<int>(this->Children.size())); cc < max;
             ++cc)
        {
          this->Children[cc].restore(treeView, model, model->index(cc, 0, idx));
        }
      }
      else
      {
        treeView->collapse(idx);
      }
    }
  };

  CNode* Root;

public:
  void save(QTreeView* treeView)
  {
    delete this->Root;
    this->Root = nullptr;

    QAbstractItemModel* model = treeView ? treeView->model() : nullptr;
    if (!model)
    {
      return;
    }

    QModelIndex rootIndex = treeView->rootIndex();

    // let's determine if the tree is non-trivial.
    // A tree is trivial if it has only 1 node.
    bool trivial_tree = (model->rowCount(rootIndex) < 1) ||
      (model->rowCount(rootIndex) == 1 &&
                          model->hasChildren(model->index(0, 0, rootIndex)) == false);

    // For trivial trees, we don't save any state, that way it's restore is
    // unaffected. This allows the applications to have a default expansion
    // state for tree views.
    if (trivial_tree == false)
    {
      this->Root = new CNode();
      this->Root->save(treeView, model, rootIndex, true);
    }
  }

  void restore(QTreeView* treeView)
  {
    QAbstractItemModel* model = treeView ? treeView->model() : nullptr;
    if (model == nullptr || this->Root == nullptr)
    {
      return;
    }

    this->Root->restore(treeView, model, treeView->rootIndex());
  }

  pqInternals()
    : Root(nullptr)
  {
  }
  ~pqInternals() { delete this->Root; }
};

//-----------------------------------------------------------------------------
pqTreeViewExpandState::pqTreeViewExpandState(QObject* parentObject)
  : Superclass(parentObject)
  , Internals(new pqTreeViewExpandState::pqInternals())
{
}

//-----------------------------------------------------------------------------
pqTreeViewExpandState::~pqTreeViewExpandState() = default;

//-----------------------------------------------------------------------------
void pqTreeViewExpandState::save(QTreeView* treeView)
{
  this->Internals->save(treeView);
}

//-----------------------------------------------------------------------------
void pqTreeViewExpandState::restore(QTreeView* treeView)
{
  this->Internals->restore(treeView);
}
