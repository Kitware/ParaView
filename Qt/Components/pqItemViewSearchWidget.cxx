// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqItemViewSearchWidget.h"

#include "pqHighlightItemDelegate.h"
#include "pqWaitCursor.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QApplication>
#include <QEvent>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QModelIndex>
#include <QPointer>
#include <QShowEvent>
#include <QStyle>

#include "ui_pqItemViewSearchWidget.h"

//-----------------------------------------------------------------------------
class pqItemViewSearchWidget::PIMPL : public Ui::pqItemViewSearchWidget
{
public:
  PIMPL(QWidget* parentW)
  {
    this->BaseWidget = parentW ? qobject_cast<QAbstractItemView*>(parentW) : nullptr;
    ;
    this->RedPal.setColor(QPalette::Base, QColor(240, 128, 128));
    this->WhitePal.setColor(QPalette::Base, QColor(Qt::white));
    this->Highlighter = new pqHighlightItemDelegate(QColor(175, 166, 238));
    this->UnHighlighter = new pqHighlightItemDelegate(QColor(Qt::white));
  }
  ~PIMPL() = default;

  QString SearchString;
  QModelIndex CurrentFound;
  QPointer<QAbstractItemView> BaseWidget;
  QPalette RedPal;
  QPalette WhitePal;
  QPointer<pqHighlightItemDelegate> Highlighter;
  QPointer<QStyledItemDelegate> UnHighlighter;
};

pqItemViewSearchWidget::pqItemViewSearchWidget(QWidget* parentW)
  : Superclass(parentW->parentWidget(), Qt::Dialog | Qt::FramelessWindowHint)
{
  this->Private = new pqItemViewSearchWidget::PIMPL(parentW);
  this->Private->setupUi(this);
  QObject::connect(
    this->Private->lineEditSearch, SIGNAL(textEdited(QString)), this, SLOT(updateSearch(QString)));
  QObject::connect(
    this->Private->checkBoxMattchCase, SIGNAL(toggled(bool)), this, SLOT(updateSearch()));
  QObject::connect(this->Private->nextButton, SIGNAL(clicked()), this, SLOT(findNext()));
  QObject::connect(this->Private->previousButton, SIGNAL(clicked()), this, SLOT(findPrevious()));
  this->installEventFilter(this);
  this->Private->lineEditSearch->installEventFilter(this);
  this->setAttribute(Qt::WA_DeleteOnClose, true);
  this->setFocusPolicy(Qt::StrongFocus);
}

// -------------------------------------------------------------------------
pqItemViewSearchWidget::~pqItemViewSearchWidget()
{
  this->Private->lineEditSearch->removeEventFilter(this);
  if (this->Private->CurrentFound.isValid() && this->Private->BaseWidget)
  {
    this->Private->BaseWidget->setItemDelegateForRow(
      this->Private->CurrentFound.row(), this->Private->UnHighlighter);
  }
  delete this->Private;
}

// -------------------------------------------------------------------------
void pqItemViewSearchWidget::setBaseWidget(QWidget* widget)
{
  this->Private->BaseWidget = widget ? qobject_cast<QAbstractItemView*>(widget) : nullptr;
  ;
}

// -------------------------------------------------------------------------
void pqItemViewSearchWidget::showSearchWidget()
{
  if (!this->Private->BaseWidget)
  {
    return;
  }
  QPoint mappedPoint = this->Private->BaseWidget->parentWidget()->childrenRect().topLeft();
  mappedPoint.setX(0);
  mappedPoint = this->Private->BaseWidget->mapToGlobal(mappedPoint);
  mappedPoint = this->mapFromGlobal(mappedPoint);
  this->setGeometry(mappedPoint.x(), mappedPoint.y() - 2 * this->height(),
    this->Private->BaseWidget->width(), this->height());
  this->setModal(false);
  this->show();
  this->raise();
  this->activateWindow();
}

//-----------------------------------------------------------------------------
bool pqItemViewSearchWidget::eventFilter(QObject* obj, QEvent* anyevent)
{
  if (anyevent->type() == QEvent::KeyPress)
  {
    QKeyEvent* e = dynamic_cast<QKeyEvent*>(anyevent);
    if (e && e->modifiers() == Qt::AltModifier)
    {
      this->keyPressEvent(e);
      return true;
    }
  }
  else if (anyevent->type() == QEvent::WindowDeactivate)
  {
    if (obj == this && !this->isActiveWindow())
    {
      anyevent->accept();
      this->close();
      return true;
    }
  }
  return this->Superclass::eventFilter(obj, anyevent);
}

//-----------------------------------------------------------------------------
void pqItemViewSearchWidget::keyPressEvent(QKeyEvent* e)
{
  if ((e->key() == Qt::Key_Escape))
  {
    e->accept();
    this->accept();
  }
  else if ((e->modifiers() == Qt::AltModifier))
  {
    e->accept();
    if (e->key() == Qt::Key_N)
    {
      this->findNext();
    }
    else if (e->key() == Qt::Key_P)
    {
      this->findPrevious();
    }
  }
}
//-----------------------------------------------------------------------------
void pqItemViewSearchWidget::showEvent(QShowEvent* e)
{
  this->activateWindow();
  this->Private->lineEditSearch->setFocus();
  this->Superclass::showEvent(e);
}

//-----------------------------------------------------------------------------
void pqItemViewSearchWidget::updateSearch(QString searchText)
{
  this->Private->SearchString = searchText;
  QModelIndex current;
  if (this->Private->CurrentFound.isValid())
  {
    this->Private->BaseWidget->setItemDelegateForRow(
      this->Private->CurrentFound.row(), this->Private->UnHighlighter);
  }
  this->Private->CurrentFound = current;
  QAbstractItemView* theView = this->Private->BaseWidget;
  if (!theView || this->Private->SearchString.size() == 0)
  {
    this->Private->lineEditSearch->setPalette(this->Private->WhitePal);
    return;
  }
  const QString searchString = this->Private->SearchString;
  // Loop through all the model indices in the model
  QAbstractItemModel* viewModel = theView->model();

  for (int r = 0; r < viewModel->rowCount(); r++)
  {
    for (int c = 0; c < viewModel->columnCount(); c++)
    {
      current = viewModel->index(r, c);
      if (this->searchModel(viewModel, current, searchString))
      {
        return;
      }
    }
  }

  this->Private->lineEditSearch->setPalette(this->Private->RedPal);
}
//-----------------------------------------------------------------------------
bool pqItemViewSearchWidget::searchModel(const QAbstractItemModel* M, const QModelIndex& curIdx,
  const QString& searchString, ItemSearchType searchType) const
{
  bool found = false;
  if (!curIdx.isValid())
  {
    return found;
  }
  pqWaitCursor wCursor;

  if (searchType == Previous && M->hasChildren(curIdx))
  {
    QModelIndex current;
    // Search curIdx index's children
    for (int r = M->rowCount(curIdx) - 1; r >= 0 && !found; r--)
    {
      for (int c = M->columnCount(curIdx) - 1; c >= 0 && !found; c--)
      {
        current = M->index(r, c, curIdx);
        found = this->searchModel(M, current, searchString, searchType);
      }
    }
  }

  if (found)
  {
    return found;
  }
  // Try to match the curIdx index itself
  if (this->matchString(M, curIdx, searchString))
  {
    return true;
  }

  if (searchType != Previous && M->hasChildren(curIdx))
  {
    QModelIndex current;
    // Search curIdx index's children
    for (int r = 0; r < M->rowCount(curIdx) && !found; r++)
    {
      for (int c = 0; c < M->columnCount(curIdx) && !found; c++)
      {
        current = M->index(r, c, curIdx);
        found = this->searchModel(M, current, searchString, searchType);
      }
    }
  }

  return found;
}
//-----------------------------------------------------------------------------
void pqItemViewSearchWidget::findNext()
{
  QAbstractItemView* theView = this->Private->BaseWidget;
  if (!theView || this->Private->SearchString.size() == 0)
  {
    return;
  }
  const QString searchString = this->Private->SearchString;

  // Loop through all the model indices in the model
  QAbstractItemModel* viewModel = theView->model();
  QModelIndex current, start = this->Private->CurrentFound;
  if (start.isValid())
  {
    this->Private->BaseWidget->setItemDelegateForRow(start.row(), this->Private->UnHighlighter);
    // search the rest of this index -- horizontally
    int r = start.row();
    int numCols = viewModel->columnCount(start.parent());
    for (int c = start.column() + 1; c < numCols; c++)
    {
      current = start.sibling(r, c);
      if (this->searchModel(viewModel, current, searchString))
      {
        return;
      }
    }

    // Search all the children
    if (viewModel->hasChildren(start))
    {
      for (r = 0; r < viewModel->rowCount(start); r++)
      {
        for (int c = 0; c < viewModel->columnCount(start); c++)
        {
          current = viewModel->index(r, c, start);
          if (this->searchModel(viewModel, current, searchString))
          {
            return;
          }
        }
      }
    }

    // search the siblings of this index and
    // need to recursive up parents until root
    QModelIndex pidx = start.parent();
    QModelIndex tmpIdx = start;
    while (pidx.isValid())
    {
      for (r = tmpIdx.row() + 1; r < viewModel->rowCount(pidx); r++)
      {
        for (int c = 0; c < viewModel->columnCount(pidx); c++)
        {
          current = viewModel->index(r, c, pidx);
          if (this->searchModel(viewModel, current, searchString))
          {
            return;
          }
        }
      }
      tmpIdx = pidx;
      pidx = pidx.parent();
    }

    // If not found, start from next row
    int numRows = viewModel->rowCount();
    for (r = start.row() + 1; r < numRows; r++)
    {
      for (int c = 0; c < viewModel->columnCount(); c++)
      {
        current = viewModel->index(r, c);
        if (this->searchModel(viewModel, current, searchString))
        {
          return;
        }
      }
    }

    // If still not found, start from (0,0)
    for (r = 0; r <= start.row(); r++)
    {
      for (int c = 0; c <= start.column(); c++)
      {
        current = viewModel->index(r, c);
        if (this->searchModel(viewModel, current, searchString))
        {
          return;
        }
      }
    }

    this->Private->lineEditSearch->setPalette(this->Private->RedPal);
  }
  else
  {
    this->updateSearch();
  }
}
//-----------------------------------------------------------------------------
void pqItemViewSearchWidget::updateSearch()
{
  this->updateSearch(this->Private->SearchString);
}
//-----------------------------------------------------------------------------
void pqItemViewSearchWidget::findPrevious()
{
  QAbstractItemView* theView = this->Private->BaseWidget;
  if (!theView || this->Private->SearchString.size() == 0)
  {
    return;
  }
  const QString searchString = this->Private->SearchString;

  // Loop through all the model indices in the model
  QAbstractItemModel* viewModel = theView->model();
  QModelIndex current, start = this->Private->CurrentFound;
  if (start.isValid())
  {
    this->Private->BaseWidget->setItemDelegateForRow(start.row(), this->Private->UnHighlighter);
    // search the rest of this index
    int r = start.row();
    for (int c = start.column() - 1; c >= 0; c--)
    {
      current = start.sibling(r, c);
      if (this->searchModel(viewModel, current, searchString, Previous))
      {
        return;
      }
    }

    // search the siblings of this index
    // need to recursive up parents until root
    QModelIndex pidx = start.parent();
    QModelIndex tmpIdx = start;
    while (pidx.isValid())
    {
      for (r = tmpIdx.row() - 1; r >= 0; r--)
      {
        for (int c = viewModel->columnCount(pidx) - 1; c >= 0; c--)
        {
          current = viewModel->index(r, c, pidx);
          if (this->searchModel(viewModel, current, searchString, Previous))
          {
            return;
          }
        }
      }
      // Try to match the parent index itself
      if (this->matchString(viewModel, pidx, searchString))
      {
        return;
      }
      tmpIdx = pidx;
      pidx = pidx.parent();
    }

    // If not found, start from previous row
    for (r = start.row() - 1; r >= 0; r--)
    {
      for (int c = viewModel->columnCount() - 1; c >= 0; c--)
      {
        current = viewModel->index(r, c);
        if (this->searchModel(viewModel, current, searchString, Previous))
        {
          return;
        }
      }
    }
    // If still not found, start from the end(rowCount,columnCount)
    for (r = viewModel->rowCount() - 1; r >= start.row(); r--)
    {
      for (int c = viewModel->columnCount() - 1; c >= 0; c--)
      {
        current = viewModel->index(r, c);
        if (this->searchModel(viewModel, current, searchString, Previous))
        {
          return;
        }
      }
    }

    this->Private->lineEditSearch->setPalette(this->Private->RedPal);
  }
  else
  {
    this->updateSearch();
  }
}

//-----------------------------------------------------------------------------
bool pqItemViewSearchWidget::matchString(
  const QAbstractItemModel* M, const QModelIndex& curIdx, const QString& searchString) const
{ // Try to match the curIdx index itself
  QString strText = M->data(curIdx, Qt::DisplayRole).toString();
  Qt::CaseSensitivity cs =
    this->Private->checkBoxMattchCase->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
  if (strText.contains(searchString, cs))
  {
    this->Private->CurrentFound = curIdx;
    this->Private->BaseWidget->setItemDelegateForRow(
      this->Private->CurrentFound.row(), this->Private->Highlighter);
    this->Private->BaseWidget->scrollTo(this->Private->CurrentFound);
    this->Private->lineEditSearch->setPalette(this->Private->WhitePal);
    return true;
  }
  return false;
}
