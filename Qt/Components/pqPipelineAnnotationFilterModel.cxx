// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqPipelineAnnotationFilterModel.h"

#include "pqApplicationCore.h"
#include "pqPipelineModel.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"

#include <QApplication>
#include <QString>
#include <QtDebug>

pqPipelineAnnotationFilterModel::pqPipelineAnnotationFilterModel(QObject* p)
  : QSortFilterProxyModel(p)
{
  this->FilterAnnotation = this->FilterSession = false;
}
//-----------------------------------------------------------------------------

bool pqPipelineAnnotationFilterModel::filterAcceptsRow(
  int sourceRow, const QModelIndex& sourceParent) const
{
  QModelIndex sourceIndex = sourceModel()->index(sourceRow, 1, sourceParent);
  return (this->FilterAnnotation
             ? sourceModel()->data(sourceIndex, pqPipelineModel::AnnotationFilterRole).toBool()
             : true) &&
    (this->FilterSession
        ? sourceModel()->data(sourceIndex, pqPipelineModel::SessionFilterRole).toBool()
        : true);
}
//-----------------------------------------------------------------------------

bool pqPipelineAnnotationFilterModel::lessThan(
  const QModelIndex& left, const QModelIndex& right) const
{
  Q_UNUSED(left);
  Q_UNUSED(right);
  return true; // We do not sort, we only filter
}
//-----------------------------------------------------------------------------

void pqPipelineAnnotationFilterModel::setAnnotationFilterMatching(bool matching)
{
  pqPipelineModel* model = qobject_cast<pqPipelineModel*>(this->sourceModel());
  if (model)
  {
    this->beginResetModel();
    model->setAnnotationFilterMatching(matching);
    this->endResetModel();
  }
}

void pqPipelineAnnotationFilterModel::enableAnnotationFilter(const QString& annotationKey)
{
  pqPipelineModel* model = qobject_cast<pqPipelineModel*>(this->sourceModel());
  if (model)
  {
    this->beginResetModel();
    this->FilterAnnotation = true;
    model->enableFilterAnnotationKey(annotationKey);
    this->endResetModel();
  }
  else
  {
    this->FilterAnnotation = false;
  }
}
//-----------------------------------------------------------------------------

void pqPipelineAnnotationFilterModel::disableAnnotationFilter()
{
  this->FilterAnnotation = false;
  pqPipelineModel* model = qobject_cast<pqPipelineModel*>(this->sourceModel());
  if (model)
  {
    this->beginResetModel();
    model->disableFilterAnnotationKey();
    this->endResetModel();
  }
}
//-----------------------------------------------------------------------------

void pqPipelineAnnotationFilterModel::enableSessionFilter(vtkSession* session)
{
  pqPipelineModel* model = qobject_cast<pqPipelineModel*>(this->sourceModel());
  if (model)
  {
    this->beginResetModel();
    this->FilterSession = true;
    model->enableFilterSession(session);
    this->endResetModel();
  }
  else
  {
    this->FilterSession = false;
  }
}
//-----------------------------------------------------------------------------

void pqPipelineAnnotationFilterModel::disableSessionFilter()
{
  this->FilterSession = false;
  pqPipelineModel* model = qobject_cast<pqPipelineModel*>(this->sourceModel());
  if (model)
  {
    this->beginResetModel();
    model->disableFilterSession();
    this->endResetModel();
  }
}
