/*=========================================================================

   Program:   ParaView
   Module:    pqPipelineAnnotationFilterModel.cxx

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
