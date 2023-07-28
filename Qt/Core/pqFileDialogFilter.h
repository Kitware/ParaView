// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqFileDialogFilter_h
#define pqFileDialogFilter_h

#include "pqCoreModule.h"
#include <QRegExp>
#include <QSortFilterProxyModel>
class pqFileDialogModel;

class PQCORE_EXPORT pqFileDialogFilter : public QSortFilterProxyModel
{
  Q_OBJECT

public:
  pqFileDialogFilter(pqFileDialogModel* sourceModel, QObject* Parent = nullptr);
  ~pqFileDialogFilter() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void setFilter(const QString& filter);
  void setShowHidden(const bool& hidden);
  bool getShowHidden() { return showHidden; };
  QRegExp const& getWildcards() const { return Wildcards; }

protected:
  bool filterAcceptsRow(int row_source, const QModelIndex& source_parent) const override;
  bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

  pqFileDialogModel* Model;
  QRegExp Wildcards;
  bool showHidden;
};

#endif // !pqFileDialogFilter_h
