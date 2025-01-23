// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#define NOMINMAX

#include "pqExpandableTableView.h"
#include "pqQtDeprecated.h"

#include <QAbstractItemDelegate>
#include <QApplication>
#include <QClipboard>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QMimeData>
#include <QRegularExpression>

//-----------------------------------------------------------------------------
pqExpandableTableView::pqExpandableTableView(QWidget* parentObject)
  : Superclass(parentObject)
  , PasteEnabled(true)
  , MoveToNextEditableItem(false)
{
}

//-----------------------------------------------------------------------------
pqExpandableTableView::~pqExpandableTableView() = default;

//-----------------------------------------------------------------------------
QModelIndex pqExpandableTableView::moveCursor(
  CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
  QModelIndex idx = this->Superclass::moveCursor(cursorAction, modifiers);
  if (this->MoveToNextEditableItem && this->selectionModel())
  {
    // check to see if this idx is editable. If yes, we're all good. Otherwise
    // skip to an editable index.

    QItemSelectionModel::SelectionFlags sflags = QItemSelectionModel::ClearAndSelect;
    switch (this->selectionBehavior())
    {
      case QAbstractItemView::SelectRows:
        sflags |= QItemSelectionModel::Rows;
        break;
      case QAbstractItemView::SelectColumns:
        sflags |= QItemSelectionModel::Columns;
        break;
      case QAbstractItemView::SelectItems:
      default:
        sflags |= QItemSelectionModel::NoUpdate;
    }

    while (idx.isValid() && !(idx.flags() & Qt::ItemIsEditable))
    {
      QPersistentModelIndex persistent(idx);
      this->selectionModel()->setCurrentIndex(persistent, sflags);
      idx = this->Superclass::moveCursor(cursorAction, modifiers);
    }
  }
  return idx;
}

//-----------------------------------------------------------------------------
void pqExpandableTableView::closeEditor(QWidget* editor, QAbstractItemDelegate::EndEditHint hint)
{
  if (hint == QAbstractItemDelegate::EditNextItem ||
    hint == QAbstractItemDelegate::EditPreviousItem)
  {
    QAbstractItemModel* curModel = this->model();
    QModelIndex idx = this->currentIndex();
    if (idx.isValid() && idx.row() == (curModel->rowCount() - 1) &&
      idx.column() == (curModel->columnCount() - 1) && hint == QAbstractItemDelegate::EditNextItem)
    {
      Q_EMIT this->editPastLastRow();
    }

    this->MoveToNextEditableItem = true;
    this->Superclass::closeEditor(editor, hint);
    this->MoveToNextEditableItem = false;
  }
  else
  {
    this->Superclass::closeEditor(editor, hint);
  }
}

//-----------------------------------------------------------------------------
void pqExpandableTableView::setPasteEnabled(bool enabled)
{
  this->PasteEnabled = enabled;
}

//-----------------------------------------------------------------------------
void pqExpandableTableView::keyPressEvent(QKeyEvent* e)
{
  if (this->PasteEnabled && e->matches(QKeySequence::Paste))
  {
    // Get text from the clipboard. Text is expected to be tabular in form,
    // delimited by arbitrary whitespace.
    const QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();

    qsizetype numModelRows = this->model()->rowCount();
    qsizetype numModelColumns = this->model()->columnCount();

    if (mimeData->hasText())
    {
      // Split the lines in the text
      QString text = mimeData->text();
      QStringList lines = text.split("\n", PV_QT_SKIP_EMPTY_PARTS);
      for (qsizetype row = 0; row < std::min<qsizetype>(lines.size(), numModelRows); ++row)
      {
        // Split within each line
        QStringList items = lines[row].split(QRegularExpression("\\s+"));

        // Set the data in the table
        for (qsizetype column = 0; column < std::min<qsizetype>(items.size(), numModelColumns);
             ++column)
        {
          QVariant value(items[column]);
          QModelIndex index = this->model()->index(row, column);
          this->model()->setData(index, value, Qt::EditRole);
        }
      }
    }
  }

  this->Superclass::keyPressEvent(e);
}
