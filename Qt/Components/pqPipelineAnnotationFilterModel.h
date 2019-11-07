/*=========================================================================

   Program: ParaView
   Module:    pqPipelineAnnotationFilterModel.h

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

/**
* \file pqPipelineAnnotationFilterModel.h
* \date 9/22/2011
*/

#ifndef _pqPipelineAnnotationFilterModel_h
#define _pqPipelineAnnotationFilterModel_h

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
  pqPipelineAnnotationFilterModel(QObject* parent = 0);
  ~pqPipelineAnnotationFilterModel() override{};

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
