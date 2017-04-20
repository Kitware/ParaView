/*=========================================================================

   Program: ParaView
   Module:  pqCompositeDataInformationTreeModel.h

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
#ifndef pqCompositeDataInformationTreeModel_h
#define pqCompositeDataInformationTreeModel_h

#include "pqComponentsModule.h" // for exports.
#include <QAbstractItemModel>
#include <QPair>          // for QPair.
#include <QScopedPointer> // for ivar.

class vtkPVDataInformation;

/**
 * @class pqCompositeDataInformationTreeModel
 * @brief Tree model that use vtkPVDataInformation to model composite data tree.
 *
 * pqCompositeDataInformationTreeModel is designed to map a data set hierarchy
 * represented by vtkCompositeDataSet (and subclasses) to a Qt tree model.
 *
 * To use this model, one calls `pqCompositeDataInformationTreeModel::reset()` with
 * the data information object to use to build the structure from.
 *
 * There are few properties on this model that should be set prior to calling
 * reset that determine how the model behaves. To allow the user to check/uncheck nodes
 * on the tree, set **userCheckable** to true (default: false). To expand datasets in a
 * multipiece, set **expandMultiPiece** to true (default: false). Finally, if
 * **userCheckable** is true, and you want to only allow the user to select one sub-tree
 * at a time, set **exclusivity** to true (default: false).
 */
class PQCOMPONENTS_EXPORT pqCompositeDataInformationTreeModel : public QAbstractItemModel
{
  Q_OBJECT
  Q_PROPERTY(bool userCheckable READ userCheckable WRITE setUserCheckable);
  Q_PROPERTY(bool expandMultiPiece READ expandMultiPiece WRITE setExpandMultiPiece);
  Q_PROPERTY(bool exclusivity READ exclusivity WRITE setExclusivity);

  typedef QAbstractItemModel Superclass;

public:
  pqCompositeDataInformationTreeModel(QObject* parent = 0);
  virtual ~pqCompositeDataInformationTreeModel();

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
  QVariant headerData(
    int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  bool setHeaderData(int section, Qt::Orientation orientation, const QVariant& value,
    int role = Qt::DisplayRole) override;
  //@}

  //@{
  /**
   * Toggle whether the model allows user to change check state.
   * Note: please call reset() after changing this.
   */
  void setUserCheckable(bool val) { this->UserCheckable = val; }
  bool userCheckable() const { return this->UserCheckable; }
  //@}

  //@{
  /**
   * Toggle whether multipiece datasets are expanded (default false).
   * Note: please call reset() after changing this.
   */
  void setExpandMultiPiece(bool val) { this->ExpandMultiPiece = val; }
  bool expandMultiPiece() const { return this->ExpandMultiPiece; }
  //@}

  //@{
  /**
   * When set to true for a userCheckable model, if the user checks a node,
   * the all other nodes that are not children of the checked node are unchecked.
   */
  void setExclusivity(bool val) { this->Exclusivity = val; }
  bool exclusivity() const { return this->Exclusivity; }
  //@}

  /**
   * API to get flat-indices for checked nodes. `checkedNodes` may return a
   * combination of leaf and non-leaf nodes i.e. if all child nodes of a node
   * are checked, then it will prefer the parent node's index rather than indices of
   * each of the children.
   */
  QList<unsigned int> checkedNodes() const;

  /**
   * API to get flat-indices for checked nodes. `checkedLeaves` returns indices for
   * the leaf nodes. The flat-index for a non-leaf node is never returned by this method.
   */
  QList<unsigned int> checkedLeaves() const;

  /**
   * API to set the flat-indices for checked nodes. This methods accepts indices for
   * both non-leaf and leaf nodes. Note that if a non-leaf node is checked,
   * then all its children are also considered as checked.
   */
  void setChecked(const QList<unsigned int>& indices);

  //@{
  /**
   * This is useful when dealing with AMR datasets. It sets/returns the level numbers for selected
   * levels
   * in the AMR dataset.
   */
  QList<unsigned int> checkedLevels() const;
  void setCheckedLevels(const QList<unsigned int>& indices);
  //@}

  //@{
  /**
   * This is useful when dealing with AMR datasets. It sets/returns the level and dataset indices
   * for AMR datasets.
   */
  QList<QPair<unsigned int, unsigned int> > checkedLevelDatasets() const;
  void setCheckedLevelDatasets(const QList<QPair<unsigned int, unsigned int> >& indices);
  //@}

  /**
   * Returns the flat or composite index associated with the node. If idx is not valid,
   * then this will simply return 0.
   */
  unsigned int compositeIndex(const QModelIndex& idx) const;

  /**
   * Returns the index for the root of the tree.
   */
  const QModelIndex rootIndex() const;
public slots:
  /**
   * Reset and rebuild the model using the data information object provided.
   * The model does not maintain a reference to the vtkPVDataInformation
   * instance. Hence it will not automatically update when \c info changes.
   *
   * @returns true is the data information refers to a composite dataset.
   *          Otherwise, returns false.
   */
  bool reset(vtkPVDataInformation* info = nullptr);

private:
  Q_DISABLE_COPY(pqCompositeDataInformationTreeModel);

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
  QString HeaderLabel;
  bool UserCheckable;
  bool ExpandMultiPiece;
  bool Exclusivity;
};

#endif
