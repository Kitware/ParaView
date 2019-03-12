/*=========================================================================

   Program: ParaView
   Module:  pqHierarchicalGridLayout.cxx

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
#include "pqHierarchicalGridLayout.h"

#include "pqHierarchicalGridWidget.h"

#include <QDebug>
#include <QLayoutItem>
#include <QScopedValueRollback>
#include <QVector>
#include <QWidget>

#include <cassert>
#include <cmath>
#include <functional>
#include <vector>

namespace
{
struct BTNode
{
  BTNode()
    : Invalid(true)
  {
  }
  BTNode(QLayoutItem* item)
    : Item(item)
    , Invalid(false)
  {
  }
  BTNode(Qt::Orientation direction, double fraction)
    : Direction(direction)
    , Fraction(fraction)
    , IsSplitter(true)
    , Invalid(false)
  {
  }

  QWidget* widget() const { return this->Item ? this->Item->widget() : nullptr; }

  QLayoutItem* Item = nullptr;
  Qt::Orientation Direction = Qt::Horizontal;
  double Fraction = 0.5;
  bool IsSplitter = false;
  bool Invalid = true;
};

template <typename T>
T get_child0(T location)
{
  return 2 * location + 1;
}

template <typename T>
T get_child1(T location)
{
  return 2 * location + 2;
}

template <typename T>
T get_parent(T location)
{
  return (location > 0 ? ((location - 1) / 2) : -1);
}
}

//-----------------------------------------------------------------------------
class pqHierarchicalGridLayout::pqInternals
{
  int MaximizedLocation = 0;

public:
  // Sequential binary tree representation where each node is represented by a
  // BTNode.
  std::vector<BTNode> SBTree;

  // Keeps track of the QLayoutItem's added to this layout.
  QVector<QLayoutItem*> Items;

  // Keeps track of split regions of pqHierarchicalGridWidget to support
  // interactive resizing.
  QVector<pqHierarchicalGridWidget::SplitterInfo> Splitters;

  // Set to true in `pqHierarchicalGridLayout::rearrange`
  bool Rearranging = false;

  void setGeometry(const QRect& rect, int spacing)
  {
    int root = 0;
    if (this->MaximizedLocation >= 0 &&
      this->MaximizedLocation < static_cast<int>(this->SBTree.size()))
    {
      root = this->MaximizedLocation;
    }

    // reset all splitters
    this->Splitters.clear();
    this->hide(0, root);
    this->setGeometry(rect, root, spacing);
  }

  void maximize(int location)
  {
    if (location < 0 || location >= static_cast<int>(this->SBTree.size()) ||
      this->SBTree[location].Invalid)
    {
      qWarning() << "unknown cell specified : " << location;
    }
    else
    {
      this->MaximizedLocation = location;
    }
  }

private:
  void hide(int start, int end)
  {
    if (start == end || start >= static_cast<int>(this->SBTree.size()) ||
      this->SBTree[start].Invalid)
    {
      return;
    }
    const auto& node = this->SBTree[start];
    if (node.IsSplitter)
    {
      this->hide(get_child0(start), end);
      this->hide(get_child1(start), end);
    }
    else if (auto wdg = node.widget())
    {
      wdg->hide();
    }
    else if (node.Item && !node.Item->isEmpty())
    {
      qWarning("currently unsupported; please contact the developers.");
    }
  }

  void setGeometry(const QRect& rect, int location, int spacing)
  {
    if (location < 0 || location >= static_cast<int>(this->SBTree.size()))
    {
      return;
    }
    const auto& node = this->SBTree[location];
    if (node.Invalid)
    {
      return;
    }

    if (node.IsSplitter)
    {
      pqHierarchicalGridWidget::SplitterInfo sinfo;
      sinfo.Direction = node.Direction;
      sinfo.Bounds = rect;
      sinfo.Location = location;

      // split rect and pass to children.
      if (node.Direction == Qt::Horizontal)
      {
        const int width0 = static_cast<int>(std::ceil((rect.width() - spacing) * node.Fraction));
        const QRect lRect = QRect(rect.x(), rect.y(), width0, rect.height());
        const QRect rRect = QRect(
          rect.x() + width0 + spacing, rect.y(), rect.width() - width0 - spacing, rect.height());
        this->setGeometry(lRect, get_child0(location), spacing);
        this->setGeometry(rRect, get_child1(location), spacing);

        sinfo.Position = width0 + spacing / 2;
      }
      else
      {
        const int height0 = static_cast<int>(std::ceil((rect.height() - spacing) * node.Fraction));
        const QRect tRect = QRect(rect.x(), rect.y(), rect.width(), height0);
        const QRect bRect = QRect(
          rect.x(), rect.y() + height0 + spacing, rect.width(), rect.height() - height0 - spacing);
        this->setGeometry(tRect, get_child0(location), spacing);
        this->setGeometry(bRect, get_child1(location), spacing);

        sinfo.Position = height0 + spacing / 2;
      }

      this->Splitters.push_back(sinfo);
    }
    else if (node.Item != nullptr)
    {
      if (auto wdg = node.widget())
      {
        wdg->show();
      }
      node.Item->setGeometry(rect);
    }
  }
};

//-----------------------------------------------------------------------------
pqHierarchicalGridLayout::pqHierarchicalGridLayout(QWidget* parentObject)
  : Superclass(parentObject)
  , Internals(new pqHierarchicalGridLayout::pqInternals())
{
}

//-----------------------------------------------------------------------------
pqHierarchicalGridLayout::~pqHierarchicalGridLayout()
{
  auto& internals = *this->Internals;
  for (auto item : internals.Items)
  {
    delete item;
  }
}

//-----------------------------------------------------------------------------
void pqHierarchicalGridLayout::addItem(QLayoutItem* item)
{
  auto& internals = *this->Internals;

  // current implementation doesn't support adding non-QWidget items to the
  // layout. In future, we may support it, if needed.
  assert(item->widget() != nullptr);

  internals.Items.push_back(item);

  // skip adding to binary tree if we're in middle of rearrangement.
  if (internals.Rearranging)
  {
    return;
  }

  // now add the item to the binary tree.

  // 1. If tree is empty, simply add the item as the root node.
  if (internals.SBTree.size() == 0)
  {
    internals.SBTree.emplace_back(BTNode(item));
    return;
  }

  // 2. Find an empty leaf node and place the item in it.
  for (auto& node : internals.SBTree)
  {
    if (node.Invalid == false && node.IsSplitter == false && node.Item == nullptr)
    {
      node.Item = item;
      return;
    }
  }

  // 3. Find an invalid node, that is node that is not yet reachable from the root
  //    and split its parent. That's an easy way to keep the tree balanced.
  for (size_t cc = 1, max = internals.SBTree.size(); cc < max; ++cc)
  {
    auto& node = internals.SBTree[cc];
    if (node.Invalid)
    {
      const auto parentIdx = get_parent(cc);
      assert(internals.SBTree[parentIdx].Invalid == false &&
        internals.SBTree[parentIdx].IsSplitter == false);

      this->splitAny(static_cast<int>(parentIdx), 0.5);
      const auto child1 = get_child1(parentIdx);
      assert(internals.SBTree[child1].Invalid == false && internals.SBTree[child1].Item == nullptr);
      internals.SBTree[child1].Item = item;
      return;
    }
  }

  // we have a fully-balanced tree, just split the last cell and try again.
  const int last_cell = static_cast<int>(internals.SBTree.size() - 1);
  this->splitAny(last_cell, 0.5);

  const auto child1 = get_child1(last_cell);
  assert(internals.SBTree[child1].Invalid == false && internals.SBTree[child1].Item == nullptr);
  internals.SBTree[child1].Item = item;
}

//-----------------------------------------------------------------------------
QLayoutItem* pqHierarchicalGridLayout::itemAt(int index) const
{
  auto& internals = *this->Internals;
  if (index >= 0 && index < internals.Items.size())
  {
    return internals.Items[index];
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
QLayoutItem* pqHierarchicalGridLayout::takeAt(int index)
{
  auto& internals = *this->Internals;
  if (index >= 0 && index < internals.Items.size())
  {
    auto item = internals.Items.takeAt(index);

    // remove item from the SBTree.
    for (auto& node : internals.SBTree)
    {
      if (node.Item == item)
      {
        node.Item = nullptr;
        break;
      }
    }
    return item;
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
int pqHierarchicalGridLayout::count() const
{
  auto& internals = *this->Internals;
  return internals.Items.size();
}

//-----------------------------------------------------------------------------
QSize pqHierarchicalGridLayout::minimumSize() const
{
  // todo: maybe scan the views, not sure what's a good min size.
  return QSize(200, 200);
}

//-----------------------------------------------------------------------------
void pqHierarchicalGridLayout::setGeometry(const QRect& rect)
{
  this->Superclass::setGeometry(rect);
  auto& internals = *this->Internals;
  internals.setGeometry(rect, this->spacing());

  // since layout may have changed, let the pqHierarchicalGridWidget know about
  // the splitter locations to support interactive resizing of the layout.
  if (auto p = qobject_cast<pqHierarchicalGridWidget*>(this->parentWidget()))
  {
    p->setSplitters(internals.Splitters);
  }
}

//-----------------------------------------------------------------------------
QSize pqHierarchicalGridLayout::sizeHint() const
{
  return QSize();
}

//-----------------------------------------------------------------------------
bool pqHierarchicalGridLayout::isLocationValid(int location) const
{
  const auto& internals = *this->Internals;
  if (location >= 0 && location < static_cast<int>(internals.SBTree.size()))
  {
    return (internals.SBTree[location].Invalid == false);
  }

  return false;
}

//-----------------------------------------------------------------------------
void pqHierarchicalGridLayout::splitAny(int location, double splitFraction)
{
  if (!this->isLocationValid(location))
  {
    qCritical() << "invalid location specified `" << location << "`.";
  }
  else if (location == 0)
  {
    this->splitHorizontal(location, splitFraction);
  }
  else
  {
    auto& internals = *this->Internals;
    const auto parentIdx = get_parent(location);
    assert(internals.SBTree[parentIdx].Invalid == false && internals.SBTree[parentIdx].IsSplitter);

    this->split(location,
      (internals.SBTree[parentIdx].Direction == Qt::Horizontal) ? Qt::Vertical : Qt::Horizontal,
      splitFraction);
  }
}

//-----------------------------------------------------------------------------
void pqHierarchicalGridLayout::setSplitFraction(int location, double splitFraction)
{
  if (!this->isLocationValid(location))
  {
    qCritical() << "invalid location specified `" << location << "`.";
  }
  else
  {
    auto& internals = *this->Internals;
    auto& node = internals.SBTree[location];
    if (node.IsSplitter)
    {
      node.Fraction = splitFraction;
      this->invalidate();
    }
  }
}

//-----------------------------------------------------------------------------
void pqHierarchicalGridLayout::split(int location, Qt::Orientation direction, double splitFraction)
{
  if (!this->isLocationValid(location))
  {
    qCritical() << "invalid location specified `" << location << "`.";
    return;
  }

  auto& internals = *this->Internals;
  assert(location >= 0 && location < static_cast<int>(internals.SBTree.size()));

  auto& node = internals.SBTree[location];

  // if node.Invalid, we're splitting an unreachable location.
  assert(node.Invalid == false);
  if (node.IsSplitter)
  {
    node.Direction = direction;
    node.Fraction = splitFraction;
  }
  else
  {
    const int child0 = get_child0(location);
    const int child1 = get_child1(location);
    if (static_cast<int>(internals.SBTree.size()) <= child1)
    {
      internals.SBTree.resize(child1 + 1);
    }
    assert(internals.SBTree[child0].Invalid && internals.SBTree[child1].Invalid);

    internals.SBTree[child0] = std::move(internals.SBTree[location]);
    internals.SBTree[child1] = BTNode(nullptr);
    internals.SBTree[location] = BTNode(direction, splitFraction);
  }

  this->invalidate();
}

//-----------------------------------------------------------------------------
void pqHierarchicalGridLayout::maximize(int location)
{
  auto& internals = *this->Internals;
  internals.maximize(location);
  this->invalidate();
}

//-----------------------------------------------------------------------------
QVector<QWidget*> pqHierarchicalGridLayout::rearrange(
  const QVector<pqHierarchicalGridLayout::Item>& litems)
{
  auto& internals = *this->Internals;
  QScopedValueRollback<bool> rollback(internals.Rearranging, true);

  // build a map of all QWidget instances and their QLayoutItem's we know
  // currently.
  std::map<QWidget*, QLayoutItem*> w2l;

  auto updateMap = [&internals, &w2l](QWidget* current) {
    for (auto litem : internals.Items)
    {
      if (auto wdg = litem->widget())
      {
        if (current == nullptr || wdg == current)
        {
          w2l[wdg] = litem;
        }
      }
    }
  };
  updateMap(nullptr);

  std::vector<BTNode> bstree(litems.size());
  std::function<void(int)> build = [&build, &litems, &bstree, &w2l, this, &updateMap](int index) {
    if (index < 0 || index >= litems.count())
    {
      return;
    }

    const auto& litem = litems[index];
    if (litem.Fraction >= 0.0)
    {
      bstree[index] = BTNode(litem.Direction, litem.Fraction);
      build(get_child0(index));
      build(get_child1(index));
    }
    else if (litem.Widget != nullptr)
    {
      auto iter = w2l.find(litem.Widget);
      if (iter == w2l.end())
      {
        this->addWidget(litem.Widget);
        updateMap(litem.Widget);
        iter = w2l.find(litem.Widget);
      }
      if (iter != w2l.end())
      {
        assert(iter->second != nullptr); // same widget cannot be added twice to the tree.
        bstree[index] = BTNode(iter->second);
        iter->second = nullptr;
      }
    }
  };
  build(0);

  internals.SBTree.clear();
  internals.SBTree = std::move(bstree);

  QVector<QWidget*> removed;

  // delete QLayoutItem's for all QWidget's that are no longer placed in
  // the layout and return the QWidget instances back to the caller.
  for (auto& pair : w2l)
  {
    if (pair.second != nullptr)
    {
      removed.push_back(pair.first);

      // delete the QLayoutItem associated with the widget
      // we're passing back to the caller as unclaimed.
      this->removeItem(pair.second);
      delete pair.second;

      pair.second = nullptr;
    }
  }

  // iterate of QWidget's provided in the input argument that were potentially
  // unreachable to bad binary tree specification and return those in the
  // removed list as well.
  for (auto& litem : litems)
  {
    if (litem.Widget && w2l.find(litem.Widget) == w2l.end())
    {
      removed.push_back(litem.Widget);
    }
  }

  this->invalidate();
  return removed;
}
