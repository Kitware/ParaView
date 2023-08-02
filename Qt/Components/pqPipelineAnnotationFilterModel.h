// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

/**
 * \file pqPipelineAnnotationFilterModel.h
 * \date 9/22/2011
 */

#ifndef pqPipelineAnnotationFilterModel_h
#define pqPipelineAnnotationFilterModel_h

#include "pqComponentsModule.h"
#include <QSortFilterProxyModel>

class vtkSession;

/**
 * \class pqPipelineAnnotationFilterModel
 * \brief
 *   The pqPipelineAnnotationFilterModel class is used to filter a tree
 *    representation of the pipeline graph by using proxy annotation.
 *
 * It use a pqPipelineModel as source model
 */

class PQCOMPONENTS_EXPORT pqPipelineAnnotationFilterModel : public QSortFilterProxyModel
{
  Q_OBJECT;

public:
  pqPipelineAnnotationFilterModel(QObject* parent = nullptr);
  ~pqPipelineAnnotationFilterModel() override = default;

  void enableAnnotationFilter(const QString& annotationKey);
  void disableAnnotationFilter();

  /**
   * Set wether annotation filter should display matching or non matching sources.
   */
  void setAnnotationFilterMatching(bool matching);

  void enableSessionFilter(vtkSession* session);
  void disableSessionFilter();

protected:
  bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
  bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

private:
  bool FilterAnnotation;
  bool FilterSession;
};

#endif
