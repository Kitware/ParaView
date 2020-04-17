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

namespace pqCompositeDataInformationTreeModelNS
{
class CNode;
}

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
 * @code
 *  pqCompositeDataInformationTreeModel* model = ...
 *  model->reset(sourceProxy->GetDataInformation(0));
 * @endcode
 *
 * pqCompositeDataInformationTreeModel does not save a reference to the
 * vtkPVDataInformation instance passed to `reset`. Hence it cannot update
 * itself when the data information changes. Updating the model to reflect
 * any potential changes in the hierarchy require another call to `reset`. As
 * name suggests, `reset` is complete reset on the model. Hence all data about
 * check states, or values for custom columns is discarded. If the should be
 * preserved, you will have to handle that externally (see
 * pqMultiBlockInspectorWidget).
 *
 * QTreeView typically collapses the tree when the model is reset, thus
 * discarded expand state for the nodes in the hierarchy. If the hierarchy
 * change was a minor update, then this can be quite jarring. You can use
 * pqTreeViewExpandState to attempt to preserve expand state on QTreeView nodes
 * across model resets.
 *
 * There are few properties on this model that should be set prior to calling
 * reset that determine how the model behaves. To allow the user to check/uncheck nodes
 * on the tree, set **userCheckable** to true (default: false). To expand datasets in a
 * multi-piece (`vtkMultiPieceDataSet`),
 * set **expandMultiPiece** to true (default: false). If
 * **userCheckable** is true, and you want to only allow the user to select one sub-tree
 * at a time, set **exclusivity** to true (default: false). Also, the default
 * checked state for the tree can be configured using **defaultCheckState**
 * (defaults to false i.e. unchecked).
 *
 * @section CustomColumns Custom Columns
 *
 * This model presents a single column tree. The check-state on this
 * 0th column is optionally settable. There may be need for
 * saving additional properties with the tree nodes, e.g. color or opacity
 * values. Such use-cases are supported via custom columns.
 *
 * One can add custom columns to the model using `addColumn`. If custom columns
 * are changed, you will need to call `reset` on the  model. The model
 * will behave unpredictably otherwise. Custom column values are stored as data
 * for `Qt::DisplayRole` for the corresponding column. Hence they can be set/get
 * using `setData` and `data` API on the QAbstractItemModel with an appropriate
 * QModelIndex.
 *
 * Setting column value on a non-leaf node in the tree will cause the subtree
 * anchored at that node to inherit the value, unless any of the nodes in the
 * subtree themselves have a value set. To clear the value set at any node,
 * simply call `setData` with an invalid QVariant.
 *
 * To determine if a custom column value is inherited or explicitly set, you can
 * use `pqCompositeDataInformationTreeModel::ValueInheritedRole)`. By making a
 * `data` call with this role, you can determine if a particular node's custom
 * column value is explicitly set or inherited.
 *
 * Since custom columns only support Qt::DisplayRole storage, one can use
 * a proxy model (e.g. `QIdentityProxyModel` subclass) to change how the column
 * data is communicated to the view. e.g. pqMultiBlockInspectorWidget renders
 * pixmaps for color and opacity columns.
 *
 * @section CheckStates Setting and querying check states
 *
 * pqCompositeDataInformationTreeModel provides multiple APIs to set and query
 * check states for nodes in the tree. The `set` methods clear current state
 * before setting, hence are not additive.
 *
 * `checkedNodes` returns a list of composite indexes (or flat indexes) for nodes
 * in the tree that are checked. If non-leaf node is checked, it is assumed
 * that all its children nodes are checked as well and hence node included in
 * the returned list.
 *
 * `checkedLeaves` returns a list of composite indexes for leaf nodes that are
 * checked. The list will never include a node with children.
 *
 * `setChecked` can be used as the set-counterpart for `checkedNodes` and
 * `checkedLeaves`. It argument can be a list of composite indexes for nodes
 * that are checked (either leaf or non-leaf). If a non-leaf node is included
 * in the list, then all its children are automatically checked.
 *
 * `checkedLevels` and `setCheckedLevels` is intended for AMR  datasets.
 * The list is simply the child index for a checked child
 * under the root. This corresponds to levels in an AMR dataset.
 *
 * `checkedLevelDatasets` and `setCheckedLevelDatasets` is also intended for
 * AMR datasets. The list is pair where first value is the level number and
 * second value is the dataset index in that level. This corresponds to level
 * index and dataset index at a level in an AMR dataset.
 *
 * `checkStates` and `setCheckStates` differ from other get/set API in that the
 * argument (or return value) list is not merely the collection of checked
 * nodes, but nodes and their states and hence can include unchecked nodes.
 *
 * @sa pqMultiBlockInspectorWidget
 */
class PQCOMPONENTS_EXPORT pqCompositeDataInformationTreeModel : public QAbstractItemModel
{
  Q_OBJECT
  Q_PROPERTY(bool userCheckable READ userCheckable WRITE setUserCheckable);
  Q_PROPERTY(bool expandMultiPiece READ expandMultiPiece WRITE setExpandMultiPiece);
  Q_PROPERTY(bool exclusivity READ exclusivity WRITE setExclusivity);
  Q_PROPERTY(bool defaultCheckState READ defaultCheckState WRITE setDefaultCheckState);

  typedef QAbstractItemModel Superclass;

public:
  pqCompositeDataInformationTreeModel(QObject* parent = 0);
  ~pqCompositeDataInformationTreeModel() override;

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
   * When `UserCheckable` is true, this limits use-checkability to leaf nodes
   * only. Note that this has no effect if `userCheckable` is false.
   * Also please call reset() after changing this.
   */
  void setOnlyLeavesAreUserCheckable(bool val) { this->OnlyLeavesAreUserCheckable = val; }
  bool onlyLeavesAreUserCheckable() const { return this->OnlyLeavesAreUserCheckable; }
  //@}

  //@{
  /**
   * Get/Set the default check state for nodes. Default is unchecked (false).
   * Note: please call reset() after changing this.
   */
  void setDefaultCheckState(bool checked) { this->DefaultCheckState = checked; }
  bool defaultCheckState() const { return this->DefaultCheckState; }

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

  /**
   * Returns check state for nodes explicitly toggled.
   */
  QList<QPair<unsigned int, bool> > checkStates() const;

  /**
   * Set check states.
   */
  void setCheckStates(const QList<QPair<unsigned int, bool> >& states);

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
   * Return the QModelIndex for composite idx. May return an invalid QModelIndex
   * if none found.
   */
  QModelIndex find(unsigned int compositeIndex) const;

  /**
   * Returns the index for the root of the tree.
   */
  const QModelIndex rootIndex() const;

  /**
   * Add a custom column to model.
   * Must call `pqCompositeDataInformationTreeModel::reset` after adding/removing columns.
   */
  int addColumn(const QString& propertyName);

  /**
   * Returns the index for a custom column with the given name. -1 if no such
   * column exists.
   */
  int columnIndex(const QString& propertyName);

  /**
   * Remove all extra columns added via `addColumn` API. The default column 0 is
   * not removed.
   * Must call `pqCompositeDataInformationTreeModel::reset` after adding/removing columns.
   */
  void clearColumns();

  //@{
  /**
   * Methods to get/set custom column values. \c values is a list of pairs,
   * where first value is the composite index for the node, and second is the
   * column value. To clear a specific value, simply pass an invalid QVariant.
   * `setColumnStates` will clear current state of the column before setting the
   * new values specified.
   * `columnStates` returns column values for nodes that have been set. It does
   * not include any nodes that "inherited" the value from its parent.
   */
  void setColumnStates(
    const QString& propertyName, const QList<QPair<unsigned int, QVariant> >& values);
  QList<QPair<unsigned int, QVariant> > columnStates(const QString& propertyName) const;
  //@}

  /**
   * Custom roles available for `data`.
   */
  enum
  {
    /**
     * Used to get whether a custom column's value is inherited from a parent
     * node or explicitly specified. `data` will return true if inherited and
     * false otherwise.
     */
    ValueInheritedRole = Qt::UserRole,

    /**
     * For a leaf node (i.e node with any children) return it's position in a
     * ordered list of simply all leaf nodes of the tree. Returned value is a
     * unsigned int indicating the location or invalid QVariant for non-leaf
     * nodes.
     */
    LeafIndexRole,

    /**
     * For a node this return the flat index/composite index for that node.
     */
    CompositeIndexRole
  };

public Q_SLOTS:
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
  bool OnlyLeavesAreUserCheckable;
  bool ExpandMultiPiece;
  bool Exclusivity;
  bool DefaultCheckState;

  friend class pqCompositeDataInformationTreeModelNS::CNode;
};

#endif
