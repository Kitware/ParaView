/*=========================================================================

   Program: ParaView
   Module:  pqSubsetInclusionLatticeTreeModel.h

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
#ifndef pqSubsetInclusionLatticeTreeModel_h
#define pqSubsetInclusionLatticeTreeModel_h

#include <QAbstractItemModel>

#include "pqComponentsModule.h" // for exports
#include <QScopedPointer>       // for QScopedPointer.

/**
 * @class pqSubsetInclusionLatticeTreeModel
 * @brief Tree model using a vtkSubsetInclusionLattice.
 *
 * pqSubsetInclusionLatticeTreeModel is QAbstractItemModel implementation for
 * vtkSubsetInclusionLattice. It exposes a tree-model using parent-child
 * relationships defined among vtkSubsetInclusionLattice nodes. Cross links are
 * ignored.
 *
 * pqSubsetInclusionLatticeTreeModel is given a vtkSubsetInclusionLattice to
 * model itself after. Since vtkSubsetInclusionLattice provides API to track and
 * update selection states, this class simply uses vtkSubsetInclusionLattice to
 * store it. However, to avoid modifying the instance passed in to
 * `setSubsetInclusionLattice`, this class creates an internal deep copy and
 * uses that to modify and track selection states.
 *
 * @note pqSubsetInclusionLatticeTreeModel will eventually replace pqSILModel
 * after all readers have been updated to use vtkSubsetInclusionLattice rather
 * that the vtkGraph-based SIL.
 */

class vtkSubsetInclusionLattice;
class vtkObject;
class PQCOMPONENTS_EXPORT pqSubsetInclusionLatticeTreeModel : public QAbstractItemModel
{
  Q_OBJECT
  typedef QAbstractItemModel Superclass;

  Q_PROPERTY(QList<QVariant> selection READ selection WRITE setSelection NOTIFY selectionModified);

public:
  pqSubsetInclusionLatticeTreeModel(QObject* parent = nullptr);
  virtual ~pqSubsetInclusionLatticeTreeModel();

  //@{
  /**
   * Get/Set the vtkSubsetInclusionLattice. This class observes the instance
   * passed, thus the model will continue to reflect the state for the SIL.
   * `sil` is treated as constant i.e. no methods that modify its structure or
   * selection state are called.
   */
  void setSubsetInclusionLattice(vtkSubsetInclusionLattice* sil);
  vtkSubsetInclusionLattice* subsetInclusionLattice() const;
  //@}

  /**
   * Returns the list of nodes that were selected.
   */
  QList<QVariant> selection() const;
  void setSelection(const QList<QVariant>& sel);

  //@{
  /**
   * QAbstractItemModel interface implementation.
   */
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex& index = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  Qt::ItemFlags flags(const QModelIndex& index) const override;
  bool setData(const QModelIndex& index, const QVariant& value, int role) override;
  //@}

Q_SIGNALS:
  void selectionModified();

protected:
  void silSelectionModified(vtkObject*, unsigned long, void*);
  void silStructureModified();

  QModelIndex indexForNode(int node) const;

private:
  Q_DISABLE_COPY(pqSubsetInclusionLatticeTreeModel);

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
