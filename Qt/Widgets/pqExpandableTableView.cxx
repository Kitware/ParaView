/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#include "pqExpandableTableView.h"
#include "pqQtDeprecated.h"

#include <QAbstractItemDelegate>
#include <QApplication>
#include <QClipboard>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QMimeData>
#include <QRegExp>

//-----------------------------------------------------------------------------
pqExpandableTableView::pqExpandableTableView(QWidget* parentObject)
  : Superclass(parentObject)
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
void pqExpandableTableView::keyPressEvent(QKeyEvent* e)
{
  if (e->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_V)
  {
    // Get text from the clipboard. Text is expected to be tabular in form,
    // delimited by arbitrary whitespace.
    const QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();

    int numModelRows = this->model()->rowCount();
    int numModelColumns = this->model()->columnCount();

    if (mimeData->hasText())
    {
      // Split the lines in the text
      QString text = mimeData->text();
      QStringList lines = text.split("\n", PV_QT_SKIP_EMPTY_PARTS);
      for (int row = 0; row < std::min(lines.size(), numModelRows); ++row)
      {
        // Split within each line
        QStringList items = lines[row].split(QRegExp("\\s+"));

        // Set the data in the table
        for (int column = 0; column < std::min(items.size(), numModelColumns); ++column)
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
