/*=========================================================================

Program: ParaView
Module:    pqExpressionsTableModel.h

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

#ifndef pqExpressionsTableModel_h
#define pqExpressionsTableModel_h

#include "pqComponentsModule.h"

#include <QAbstractTableModel>

#include "pqExpressionsManager.h" // for pqExpression

class PQCOMPONENTS_EXPORT pqExpressionsTableModel : public QAbstractTableModel
{
  Q_OBJECT
  typedef QAbstractTableModel Superclass;

public:
  pqExpressionsTableModel(QObject* parent = nullptr);
  ~pqExpressionsTableModel() override = default;

  enum ColumnKey
  {
    GroupColumn = 0,
    NameColumn,
    ExpressionColumn,
    NumberOfColumns
  };

  /**
   * Add a new empty expression.
   * Return the index of the created item
   */
  QModelIndex addNewExpression();

  /**
   * Get all expressions.
   */
  QList<pqExpressionsManager::pqExpression> getExpressions();

  /**
   * Get expressions list for given group.
   */
  QList<pqExpressionsManager::pqExpression> getExpressions(const QString& group);

  /**
   * Get the expression at idx.
   */
  QString getExpressionAsString(const QModelIndex& idx);

  /**
   * Find the given expression and return the index of the first matching row, at ExpressionColumn.
   * Retrun invalid index if nothing found.
   */
  QModelIndex expressionIndex(const pqExpressionsManager::pqExpression& expression);

  /**
   * Get the group at idx.
   */
  QString getGroup(const QModelIndex& idx);

  /**
   * Set expressions. Erase any previous content.
   */
  void setExpressions(const QList<pqExpressionsManager::pqExpression>& expressions);

  /**
   * Add expressions to the current one.
   * This does not remove duplicates.
   */
  void addExpressions(const QList<pqExpressionsManager::pqExpression>& expressions);

  /**
   * Get the available groups.
   */
  QSet<QString> getGroups();

  /**
   * Set the expression name for the expression at idx.
   */
  bool setExpressionName(int idx, const QString& group);

  /**
   * Set the group name for the expression at idx.
   */
  bool setExpressionGroup(int idx, const QString& group);

  /**
   * Remove the expressions pointed by the list of indices.
   */
  void removeExpressions(const QModelIndexList& index);

  /**
   * Remove all expressions.
   */
  void removeAllExpressions();

  ///@{
  /**
   * Reimplements Superclass API to manage items.
   */
  int rowCount(const QModelIndex& parent) const override;
  int columnCount(const QModelIndex& parent) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
  Qt::ItemFlags flags(const QModelIndex& index) const override;
  bool setData(const QModelIndex& index, const QVariant& value, int role) override;
  bool insertRows(int position, int rows, const QModelIndex& parent) override;
  bool removeRows(int position, int rows, const QModelIndex& parent) override;
  ///@}

protected:
  /**
   * Set expression value at given idx.
   */
  bool setExpression(int idx, const QString& expression);
  /**
   * Remove expression at given idx.
   */
  bool removeExpression(int idx);

  QList<pqExpressionsManager::pqExpression> Expressions;
};

#endif
