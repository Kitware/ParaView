/*=========================================================================

   Program: ParaView
   Module:  TreeViewSelectionAndCheckState.cxx

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
// This test is intended to test code added to pqTreeView to support use-cases
// report is issue paraview/paraview#18157.

#include "QTestApp.h"
#include "pqTreeView.h"

#include <QItemSelectionModel>
#include <QSet>
#include <QStandardItemModel>
#include <QtDebug>

namespace
{
QRect itemRect(QTreeView& view, const QModelIndex& idx)
{
  return view.visualRect(idx);
}

QPoint itemCenter(QTreeView& view, const QModelIndex& idx)
{
  // qDebug() << "center " << itemRect(view, idx).center();
  return itemRect(view, idx).center();
}

QPoint itemCheckbox(QTreeView& view, const QModelIndex& idx)
{
  const QRect rect = itemRect(view, idx);
  return QPoint(rect.left() + 5, rect.center().y());
}

QSet<QString> checkedItemNames(const QList<QStandardItem*>& items)
{
  QSet<QString> names;
  for (QStandardItem* item : items)
  {
    if (item->data(Qt::CheckStateRole).value<Qt::CheckState>() == Qt::Checked)
    {
      names.insert(item->text());
    }
  }
  return names;
}

#define PQVERIFY2(x, y)                                                                            \
  if (!(x))                                                                                        \
  {                                                                                                \
    qCritical() << "Failed test: " << y;                                                           \
    return EXIT_FAILURE;                                                                           \
  }
}

int TreeViewSelectionAndCheckState(int argc, char* argv[])
{
  QTestApp app(argc, argv);

  QStandardItemModel model;

  auto parentItem = model.invisibleRootItem();
  QMap<QString, QStandardItem*> itemsMap;
  QMap<QString, QModelIndex> itemIndexMap;
  for (int cc = 0; cc < 10; cc++)
  {
    auto txt = QString("item %0").arg(cc + 1);
    auto item = new QStandardItem(txt);
    item->setFlags(
      (item->flags() | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable) & (~Qt::ItemIsEditable));
    item->setData(Qt::Unchecked, Qt::CheckStateRole);
    parentItem->appendRow(item);

    itemsMap[txt] = item;
    itemIndexMap[txt] = model.indexFromItem(item);
  }
  model.setHeaderData(0, Qt::Horizontal, "Data", Qt::DisplayRole);

  pqTreeView view;
  view.setModel(&model);
  view.setWindowTitle("TreeViewSelectionAndCheckState");
  view.setMinimumWidth(400);
  view.setMinimumHeight(600);
  view.setRootIsDecorated(false);
  view.setUniformRowHeights(true);
  view.setSelectionMode(QAbstractItemView::ExtendedSelection);

  view.show();
  QTestApp::delay(1000);

  // let's click on "item 2"
  QTestApp::mouseClick(
    view.viewport(), itemCenter(view, itemIndexMap["item 2"]), Qt::LeftButton, Qt::NoModifier, 20);
  PQVERIFY2(view.selectionModel()->selectedIndexes() == QModelIndexList{ itemIndexMap["item 2"] },
    "ensure item 2 is the only item selected.");

  // shift + click "item 4"
  QTestApp::mouseClick(view.viewport(), itemCenter(view, itemIndexMap["item 4"]), Qt::LeftButton,
    Qt::ShiftModifier, 20);
  PQVERIFY2(view.selectionModel()->selectedIndexes() ==
      QModelIndexList({ itemIndexMap["item 2"], itemIndexMap["item 3"], itemIndexMap["item 4"] }),
    "ensure items [2-4] are selected.");

  // ctrl + click "item 7"
  QTestApp::mouseClick(view.viewport(), itemCenter(view, itemIndexMap["item 7"]), Qt::LeftButton,
    Qt::ControlModifier, 20);
  PQVERIFY2(view.selectionModel()->selectedIndexes() ==
      QModelIndexList({ itemIndexMap["item 2"], itemIndexMap["item 3"], itemIndexMap["item 4"],
        itemIndexMap["item 7"] }),
    "ensure items [2-4, 7] are selected.");

  // check item 10
  QTestApp::mouseClick(view.viewport(), itemCheckbox(view, itemIndexMap["item 10"]), Qt::LeftButton,
    Qt::NoModifier, 20);
  PQVERIFY2(view.selectionModel()->selectedIndexes() ==
      QModelIndexList({ itemIndexMap["item 2"], itemIndexMap["item 3"], itemIndexMap["item 4"],
        itemIndexMap["item 7"] }),
    "ensure items [2-4, 7] are selected (selection should stay unchanged).");
  PQVERIFY2(checkedItemNames(itemsMap.values()) == QSet<QString>({ "item 10" }),
    "ensure item 10 is checked.");

  // check item 2
  QTestApp::mouseClick(view.viewport(), itemCheckbox(view, itemIndexMap["item 2"]), Qt::LeftButton,
    Qt::NoModifier, 20);
  PQVERIFY2(view.selectionModel()->selectedIndexes() ==
      QModelIndexList({ itemIndexMap["item 2"], itemIndexMap["item 3"], itemIndexMap["item 4"],
        itemIndexMap["item 7"] }),
    "ensure items [2-4, 7] are selected (selection should stay unchanged).");
  PQVERIFY2(checkedItemNames(itemsMap.values()) ==
      QSet<QString>({ "item 10", "item 2", "item 3", "item 4", "item 7" }),
    "ensure item 10 and [2-4], 7 are checked.");

  // uncheck item 4
  QTestApp::mouseClick(view.viewport(), itemCheckbox(view, itemIndexMap["item 4"]), Qt::LeftButton,
    Qt::NoModifier, 20);
  PQVERIFY2(view.selectionModel()->selectedIndexes() ==
      QModelIndexList({ itemIndexMap["item 2"], itemIndexMap["item 3"], itemIndexMap["item 4"],
        itemIndexMap["item 7"] }),
    "ensure items [2-4, 7] are selected (selection should stay unchanged).");
  PQVERIFY2(checkedItemNames(itemsMap.values()) == QSet<QString>({ "item 10" }),
    "ensure item 10 is checked (others get unchecked).");

  return app.exec();
}
