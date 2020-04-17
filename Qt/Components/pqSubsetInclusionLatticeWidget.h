/*=========================================================================

   Program: ParaView
   Module:  pqSubsetInclusionLatticeWidget.h

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
#ifndef pqSubsetInclusionLatticeWidget_h
#define pqSubsetInclusionLatticeWidget_h

#include <QWidget>

#include "pqComponentsModule.h" // for exports.
#include <QPointer>             // for QPointer.
#include <QScopedPointer>       // for QScopedPointer.

/**
 * @class pqSubsetInclusionLatticeWidget
 * @brief widget for showing a SIL (vtkSubsetInclusionLattice).
 *
 * pqSubsetInclusionLatticeWidget can be used to show a
 * vtkSubsetInclusionLattice in QWidget. Each top-level nodes in the SIL,
 * is shown as a Tab with the subtree under those shown in  separate pqTreeView
 * instances under each of the tabs.
 *
 * While it can work with any QAbstractItemModel, pqSubsetInclusionLatticeWidget
 * is intended to be used with pqSubsetInclusionLatticeTreeModel.
 *
 * @sa pqSILWidget
 */

class QAbstractItemModel;
class PQCOMPONENTS_EXPORT pqSubsetInclusionLatticeWidget : public QWidget
{
  Q_OBJECT;
  typedef QWidget Superclass;

public:
  pqSubsetInclusionLatticeWidget(QAbstractItemModel* amodel, QWidget* parent = 0);
  virtual ~pqSubsetInclusionLatticeWidget();

  /**
   * Returns the model being used.
   */
  QAbstractItemModel* model() const;

private Q_SLOTS:
  void modelReset();
  /*
  void rowsInserted(const QModelIndex &parent, int first, int last);
  void rowsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int
  row);
  void rowsRemoved(const QModelIndex &parent, int first, int last);
  */

private:
  Q_DISABLE_COPY(pqSubsetInclusionLatticeWidget);

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
