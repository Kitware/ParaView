/*=========================================================================

   Program: ParaView
   Module:  pqDataAssemblyTreeModel.h

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
#ifndef pqDataAssemblyTreeModel_h
#define pqDataAssemblyTreeModel_h

#include "pqComponentsModule.h" // for exports
#include <QAbstractItemModel>
#include <QPair>          // for QPair
#include <QScopedPointer> // for QScopedPointer.

class vtkDataAssembly;

/**
 * @class pqDataAssemblyTreeModel
 * @brief QAbstractItemModel implementation for vtkDataAssembly
 *
 * pqDataAssemblyTreeModel builds a tree-model from a vtkDataAssembly.
 * The vtkDataAssembly to shown is set using `setDataAssembly`.
 * To allow user-settable check-states, use `setUserCheckable(true)`.
 *
 * One can store additional data associated with nodes using custom roles
 * (`Qt::UserRole`). For custom roles, one can define how a value for that
 * specific role get inherited by child nodes or overridden when set on a parent
 * node. This is done using `setRoleProperty`.
 *
 */
class PQCOMPONENTS_EXPORT pqDataAssemblyTreeModel : public QAbstractItemModel
{
  Q_OBJECT;
  typedef QAbstractItemModel Superclass;

public:
  pqDataAssemblyTreeModel(QObject* parent = nullptr);
  ~pqDataAssemblyTreeModel() override;

  /**
   * Get/Set the vtkDataAssembly to represent in this model.
   */
  void setDataAssembly(vtkDataAssembly* assembly);
  vtkDataAssembly* dataAssembly() const;

  //@{
  /**
   * Get whether the model is user-checkable.
   */
  void setUserCheckable(bool);
  bool userCheckable() const { return this->UserCheckable; }
  //@}

  //@{
  /**
   * Given a QModelIndex, returns the vtkDataAssembly node id, if any.
   * -1 is returned for invalid index.
   */
  int nodeId(const QModelIndex& idx) const;
  QList<int> nodeId(const QModelIndexList& idxes) const;
  //@}

  //@{
  /**
   * Get/Set the check state for Qt::Checked for nodes selected using the path
   * names specified.
   *
   * `setCheckedNodes` has effect as `setData(.., Qt::CheckStateRole)`, only differing
   * in how the values are specified. For `setCheckedNodes`, one only lists the
   * selectors for checked nodes, while `setData` accept pairs where the first
   * value is the selector while the second value is the check state.
   *
   * @note checking a node causes all it child nodes to be checked as well.
   * @note `checkedNodes()` may not returns exactly the same paths passed to
   *       `setCheckedNodes` even if the check-states were not changed in
   *       between the two calls.
   *
   * @sa vtkDataAssembly::SelectNodes
   */
  void setCheckedNodes(const QStringList& paths);
  QStringList checkedNodes() const;
  //@}

  /**
   * For custom roles, use this function to convert that role into a role that
   * can be used in `data()` to get a boolean indicating if the value is derived
   * or explicitly set.
   */
  static int GetIsDerivedRole(int role) { return -role; }

  /**
   * Supported role properties. `Standard` implies no special handling i.e. the
   * role does not affect the value of the value for child nodes in the
   * hierarchy. `Inherited` means for any node, if a value for this role is not
   * explicitly specified, then it is inherited from its parent node. Likewise,
   * if one sets a explicitly on a node (using `setData`), the it overrides
   * values for this role on all children recursively.
   * `InheritedUntilOverridden` is similar to `Inherited` except that the
   * recursion down the subtree stops when a child node with an explicitly set
   * value is encountered.
   */
  enum RoleProperties
  {
    Standard,
    Inherited,
    InheritedUntilOverridden,
  };

  //@{
  /**
   * Set properties for custom roles. If none specified, Standard is assumed.
   * For `Qt::CheckStateRole`, it is initialized to `Inherited`.
   */
  void setRoleProperty(int role, RoleProperties property);
  RoleProperties roleProperty(int role) const;
  //@}

  //@{
  /**
   * Get/Set the values for a specific role using selectors. The values are
   * specified as a list of pairs which pair comprising of a selector and the
   * associated value for that selector. This model is not capable of generating
   * advanced selectors, so `data()` will simply return paths as selectors.
   * `setData` can use any for for selector specification supported by the
   * underlying `vtkDataAssembly`.
   *
   * This model does not cache set values. When `setData` is called, nodes
   * matching the selectors are immediately located and updated. Hence, if this
   * method is called before any assembly is set, the values will be lost. In
   * this case, `setData` will return false.
   *
   * @sa vtkDataAssembly::SelectNodes
   */
  bool setData(const QList<QPair<QString, QVariant> >& values, int role);
  QList<QPair<QString, QVariant> > data(int role) const;
  //@}

  //@{
  /**
   * QAbstractItemModel interface implementation
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
  /**
   * This signal is fired in `setData` if the data is changed. `setData`
   * can result in multiple calls to `dataChanged` signal as the data is
   * updated on nodes in the hierarchy. This signal is fired only once for
   * `setData` call irrespective of how many internal nodes are updated.
   */
  void modelDataChanged(int role);

private:
  void fireDataChanged(const QModelIndex& root, const QVector<int>& roles);

private:
  Q_DISABLE_COPY(pqDataAssemblyTreeModel);

  class pqInternals;
  QScopedPointer<pqInternals> Internals;

  bool UserCheckable;
};

#endif
