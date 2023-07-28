// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqFileDialogFilter.h"

#include <QDateTime>
#include <QFileIconProvider>
#include <QIcon>
#include <QRegularExpression>
#include <QStringBuilder>

#include "pqFileDialogModel.h"

pqFileDialogFilter::pqFileDialogFilter(pqFileDialogModel* model, QObject* Parent)
  : QSortFilterProxyModel(Parent)
  , Model(model)
  , showHidden(false)
{
  this->setSourceModel(model);
  this->Wildcards.setPatternSyntax(QRegExp::RegExp2);
  this->Wildcards.setCaseSensitivity(Qt::CaseSensitive);
  this->setSortCaseSensitivity(Qt::CaseInsensitive);
}

pqFileDialogFilter::~pqFileDialogFilter() = default;

#include <cstdio>

void pqFileDialogFilter::setFilter(const QString& filter)
{
  QString f(filter);
  // if we have (...) in our filter, strip everything out but the contents of ()
  int start, end;
  end = filter.lastIndexOf(')');
  // we need to from the backwards in case the name of the filter is:
  // File Type (polydata) (*.ft)
  start = filter.lastIndexOf('(', end);
  if (start != -1 && end != -1)
  {
    f = f.mid(start + 1, end - start - 1);
  }
  // Now 'f' is going to be string like "*.ext1 *.ext2" or "spct* *pattern*"
  // etc. The following code converts it to a regular expression.
  QString pattern = ".*";
  if (f != "*")
  {
    f = f.trimmed();

    // convert all spaces into |
    f.replace(QRegularExpression("[\\s+;]+"), "|");

    QStringList strings = f.split("|");
    QStringList extensions_list, filepatterns_list;
    Q_FOREACH (QString string, strings)
    {
      if (string.startsWith("*."))
      {
        extensions_list.push_back(string.remove(0, 2));
      }
      else
      {
        filepatterns_list.push_back(string);
      }
    }

    QString extensions = extensions_list.join("|");
    QString filepatterns = filepatterns_list.join("|");

    extensions.replace(".", "\\.");
    extensions.replace("*", ".*");

    filepatterns.replace(".", "\\.");
    filepatterns.replace("*", ".*");

    // use non capturing(?:) for speed
    // accepts the pattern followed by up to 3 dot and number (for example can.e.4.1.001)
    QString postExtFileSeries("(\\.?\\d+){0,3}$");
    QString extGroup = ".*\\.(?:" % extensions % ")" % postExtFileSeries;
    QString fileGroup = "(?:" % filepatterns % ")" % postExtFileSeries;
    if (!extensions_list.empty() && !filepatterns_list.empty())
    {
      pattern = "(?:" % fileGroup % "|" % extGroup % ")";
    }
    else if (!extensions_list.empty())
    {
      pattern = extGroup;
    }
    else
    {
      pattern = fileGroup;
    }
  }

  this->Wildcards.setPattern(pattern);
  this->invalidateFilter();
}

void pqFileDialogFilter::setShowHidden(const bool& hidden)
{
  if (this->showHidden != hidden)
  {
    this->showHidden = hidden;
    this->invalidateFilter();
  }
}

bool pqFileDialogFilter::filterAcceptsRow(int row_source, const QModelIndex& source_parent) const
{
  QModelIndex idx = this->Model->index(row_source, 0, source_parent);
  QAbstractItemModel const* sourceModel = this->sourceModel();

  // hidden flag supersedes anything else
  if (this->Model->isHidden(idx) && !this->showHidden)
  {
    return false;
  }

  if (this->Model->isDir(idx))
  {
    QString str = sourceModel->data(idx).toString();
    return true;
  }

  int const rowCount = sourceModel->rowCount(idx);

  if (rowCount != 0)
  {
    for (int row = 0; row < rowCount; ++row)
    {
      QString str = sourceModel->data(sourceModel->index(row, 0, idx)).toString();
      if (!this->Wildcards.exactMatch(str))
      {
        return false;
      }
    }
    return true;
  }
  else
  {
    QString str = sourceModel->data(idx).toString();
    return this->Wildcards.exactMatch(str);
  }
}

bool pqFileDialogFilter::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
  // Compare two index for sorting purposes
  QModelIndex leftTypeIdx = this->sourceModel()->index(left.row(), 1, left.parent());
  QModelIndex rightTypeIdx = this->sourceModel()->index(right.row(), 1, right.parent());

  int leftType = this->sourceModel()->data(leftTypeIdx, Qt::UserRole).toInt();
  int rightType = this->sourceModel()->data(rightTypeIdx, Qt::UserRole).toInt();

  // Sanity Check
  if ((leftType != rightType) ||
    ((left.parent().isValid() && right.parent().isValid() && left.parent() != right.parent()) ||
      (left.parent().isValid() && !right.parent().isValid()) ||
      (!left.parent().isValid() && right.parent().isValid())))
  {
    return false;
  }
  // Compare File Size
  if (left.column() == 2)
  {
    qulonglong leftData = this->sourceModel()->data(left, Qt::UserRole).toULongLong();
    qulonglong rightData = this->sourceModel()->data(right, Qt::UserRole).toULongLong();
    return leftData < rightData;
  }
  // Compare Modification time
  else if (left.column() == 3)
  {
    QDateTime leftData = this->sourceModel()->data(left, Qt::UserRole).toDateTime();
    QDateTime rightData = this->sourceModel()->data(right, Qt::UserRole).toDateTime();
    return leftData < rightData;
  }
  // Other column (strings) are handled by the superclass
  return QSortFilterProxyModel::lessThan(left, right);
}
