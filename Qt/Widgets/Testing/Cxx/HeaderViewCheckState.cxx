/*=========================================================================

   Program: ParaView
   Module:  HeaderViewCheckState.cxx

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
#include <QApplication>

#include <QPointer>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTableView>
#include <QTimer>
#include <QtDebug>

#include "pqHeaderView.h"

#include <QTestEventList>

#define PQVERIFY2(x, y)                                                                            \
  if (!(x))                                                                                        \
  {                                                                                                \
    qCritical() << "Failed test: " << y;                                                           \
    return EXIT_FAILURE;                                                                           \
  }

/*
 * This tests tests clickability of pqHeaderView.
 * To use pqHeaderView, one simply needs to connect the header to a view
 * with model that respects Qt::CheckStateRole for setHeaderData and headerData
 * calls.
 */
int HeaderViewCheckState(int argc, char* argv[])
{
  QApplication app(argc, argv);

  static const int checkable_column = 1;

  QStandardItemModel model(10, 3);
  for (int row = 0; row < 10; ++row)
  {
    for (int col = 0; col < 3; ++col)
    {
      auto item = new QStandardItem(QString("%0, %1").arg(row).arg(col));
      model.setItem(row, col, item);
    }
    model.item(row, checkable_column)->setCheckable(true);
    model.item(row, checkable_column)->setCheckState((row % 3) == 0 ? Qt::Checked : Qt::Unchecked);
  }
  for (int col = 0; col < 3; ++col)
  {
    model.setHeaderData(col, Qt::Horizontal, QString("%1").arg(col + 1), Qt::DisplayRole);
  }

  // this make pqHeaderView in a operate in checkable-mode for this section.
  model.setHeaderData(checkable_column, Qt::Horizontal, Qt::PartiallyChecked, Qt::CheckStateRole);

  auto* self = &model;
  QObject::connect(&model, &QStandardItemModel::headerDataChanged,
    [=](Qt::Orientation orientation, int first, int last) {
      if (first <= checkable_column && last >= checkable_column && orientation == Qt::Horizontal)
      {
        QVariant checkState = self->headerData(checkable_column, orientation, Qt::CheckStateRole);
        for (int row = 0; row < self->rowCount(); ++row)
        {
          self->item(row, checkable_column)->setCheckState(checkState.value<Qt::CheckState>());
        }
      }
    });

  QTableView view;
  view.setMinimumWidth(400);
  view.setMinimumHeight(600);
  view.setModel(&model);
  view.setWindowTitle("HeaderViewCheckState");

  QPointer<QHeaderView> oldheader = view.horizontalHeader();
  auto pqheader = new pqHeaderView(Qt::Horizontal, &view);
  view.setHorizontalHeader(pqheader);
  delete oldheader;

  view.show();
  QTest::qWait(500);

  // click on checkbox in header and ensure all items get checked.
  QTestEventList events0;
  events0.addMouseClick(Qt::LeftButton, Qt::NoModifier, pqheader->lastCheckRect().center());
  events0.addDelay(500);
  events0.simulate(pqheader->viewport());

  for (int row = 0; row < 10; ++row)
  {
    auto item = model.item(row, checkable_column);
    PQVERIFY2(item->checkState() == Qt::Checked, QString("row %1 should be `checked`.").arg(row));
  }

  // now click on header section (not in checkbox) and ensure nothing changes.
  QTestEventList events1;

  const auto checkRect = pqheader->lastCheckRect();
  events1.addMouseClick(
    Qt::LeftButton, Qt::NoModifier, QPoint(checkRect.right() + 20, checkRect.center().y()));
  events1.addDelay(500);
  events1.simulate(pqheader->viewport());
  for (int row = 0; row < 10; ++row)
  {
    auto item = model.item(row, checkable_column);
    PQVERIFY2(
      item->checkState() == Qt::Checked, QString("row %1 should remain `checked`.").arg(row));
  }

  // now change header mode to respect clicks on the entire section.
  pqheader->setToggleCheckStateOnSectionClick(true);
  events1.simulate(pqheader->viewport());
  for (int row = 0; row < 10; ++row)
  {
    auto item = model.item(row, checkable_column);
    PQVERIFY2(
      item->checkState() == Qt::Unchecked, QString("row %1 should get `unchecked`.").arg(row));
  }

  return app.arguments().indexOf("--exit") == -1 ? app.exec() : EXIT_SUCCESS;
}
