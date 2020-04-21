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
#include <QScopedPointer> // for QScopedPointer.

class vtkDataAssembly;

/**
 * @class pqDataAssemblyTreeModel
 * @brief QAbstractItemModel implementation for vtkDataAssembly
 *
 * pqDataAssemblyTreeModel builds a tree-model from a vtkDataAssembly.
 * The vtkDataAssembly to shown is set using `setDataAssembly`.
 * To allow user-settable check-states, use `setUserCheckable(true)`.
 * The model doesn't support any additional custom properties. One is expected
 * to use a proxy model to store additional meta-data associated with nodes.
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
   * Get/Set the check state for Qt::Checked for nodes selected using the path
   * names specified.
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
