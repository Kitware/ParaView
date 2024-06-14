// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include <QSortFilterProxyModel>

#include "pqWidgetsModule.h"

/**
 * \brief: A custom QSortFilterProxyModel to do extended search.
 *
 * \details: Allow to do extended search in a model, using different ItemRole.
 *
 * Different kind of matches can be used to filter elements:
 *  - exact match: the item DefaultRole string starts with the "Request" string.
 *  - standard match: split the "Request" string around space character and try to match
 *  each of them in any order with the item DefaultRole string.
 *  - User match: match on UserRole instead of default one
 *
 * Items are sorted per match kind: exact match first, then standard match and finally user match.
 * Within a match kind, alphabetical sort is used.
 *
 * Items matching an Exlusion criteria are always hidden, regardless of the requests.
 */
class PQWIDGETS_EXPORT pqExtendedSortFilterProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT;
  typedef QSortFilterProxyModel Superclass;

public:
  pqExtendedSortFilterProxyModel(QObject* parent);

  ~pqExtendedSortFilterProxyModel() override = default;

  /**
   * Set the user requests.
   */
  ///@{
  /**
   * Set the user string request for default role.
   *
   * This is used for match operations.
   * Matching items are visible (if not excluded).
   *
   * filterRole is intended to be Qt::DisplayRole (default)
   * or at least a role for QString data.
   */
  void setRequest(const QString& request);

  /**
   * Set the user request for the given ItemRole.
   *
   * Items matching this value will be shown.
   * This acts as a "white list"
   *
   * see clearUserRequests(), setExclusion()
   */
  void setRequest(int role, const QVariant& value);

  /**
   * Clear user requests.
   * Remove custom role checks. Note that the main request is not touched.
   */
  void clearRequests();
  ///@}

  /**
   * Set the Exclusions list.
   */
  ///@{
  /**
   * Set an exclusion rule for ItemRole.
   *
   * Items matching this value will always be hidden.
   *
   * see clearExclusions(), setUserRequest()
   */
  void setExclusion(int role, const QVariant& value);

  /**
   * Clear the exclusion list.
   */
  void clearExclusions();
  ///@}

  /**
   * Triggers an update of the sort filter model.
   */
  void update();

protected:
  /**
   * Returns false if row is excluded.
   * Returns true if every sub part of the Request has a match.
   * Returns false otherwise.
   */
  bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

  /**
   * Sort elements depending on their priority:
   *  - default filterRole has a match
   *  - any userRole has a match
   *
   * At same priority, it uses alphanumeric comparison.
   */
  bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

private:
  /**
   * Returns true if an extended match is found.
   * For extended match, the input request is splitted around spaces characters.
   * Then each subrequest should have a match. If any subrequest is not found,
   * this will return false.
   */
  bool hasMatch(const QModelIndex& index) const;

  /**
   * Returns true if an exact match is found.
   * An item gives an exact match if it starts with the whole `Request` string.
   * An item with an exact match is order before classical match.
   * See setRequest
   */
  bool hasExactMatch(const QModelIndex& index) const;

  /**
   * Returns true if the index matches one of the user defined rules.
   */
  bool hasUserMatch(const QModelIndex& index) const;

  /**
   * Returns true if the index matches one of the exclusion rules.
   */
  bool isExcluded(const QModelIndex& index) const;

  QString Request;
  QList<QRegularExpression> DefaultRequests;

  QMap<int, QVariant> UserRequests;
  QMap<int, QVariant> ExclusionRules;
};
