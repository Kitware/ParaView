// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqExtendedSortFilterProxyModel.h"

//-----------------------------------------------------------------------------
pqExtendedSortFilterProxyModel::pqExtendedSortFilterProxyModel(QObject* parent)
  : Superclass(parent)
{
  this->setFilterKeyColumn(-1);
  this->setSortCaseSensitivity(Qt::CaseInsensitive);
}

//-----------------------------------------------------------------------------
void pqExtendedSortFilterProxyModel::setRequest(const QString& request)
{
  this->Request = request;
  this->DefaultRequests.clear();
  QStringList requests = request.split(" ");
  for (const QString& subRequest : requests)
  {
    QRegularExpression subExpression;
    subExpression.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    subExpression.setPattern(subRequest);
    this->DefaultRequests.push_back(subExpression);
  }
}

//-----------------------------------------------------------------------------
void pqExtendedSortFilterProxyModel::setRequest(int role, const QVariant& value)
{
  this->UserRequests[role] = value;
}

//-----------------------------------------------------------------------------
void pqExtendedSortFilterProxyModel::setExclusion(int role, const QVariant& value)
{
  this->ExclusionRules[role] = value;
}

//-----------------------------------------------------------------------------
void pqExtendedSortFilterProxyModel::clearExclusions()
{
  this->ExclusionRules.clear();
}

//-----------------------------------------------------------------------------
void pqExtendedSortFilterProxyModel::clearRequests()
{
  this->Request.clear();
  this->DefaultRequests.clear();
  this->UserRequests.clear();
}

//-----------------------------------------------------------------------------
bool pqExtendedSortFilterProxyModel::hasExactMatch(const QModelIndex& index) const
{
  auto proxyName = this->sourceModel()->data(index, this->filterRole()).toString();
  return proxyName.startsWith(this->Request, Qt::CaseInsensitive);
}

//-----------------------------------------------------------------------------
bool pqExtendedSortFilterProxyModel::hasMatch(const QModelIndex& index) const
{
  auto proxyName = this->sourceModel()->data(index, this->filterRole()).toString();

  for (const auto& subRequest : this->DefaultRequests)
  {
    if (!proxyName.contains(subRequest))
    {
      return false;
    }
  }

  return true;
}
//-----------------------------------------------------------------------------
bool pqExtendedSortFilterProxyModel::isExcluded(const QModelIndex& index) const
{
  for (auto exclusionRule = this->ExclusionRules.constBegin();
       exclusionRule != this->ExclusionRules.constEnd(); exclusionRule++)
  {
    QVariant indexData = this->sourceModel()->data(index, exclusionRule.key());
    if (!indexData.isValid())
    {
      continue;
    }

    if (indexData == exclusionRule.value())
    {
      return true;
    }
  }

  return false;
}

//-----------------------------------------------------------------------------
bool pqExtendedSortFilterProxyModel::hasUserMatch(const QModelIndex& index) const
{
  for (auto userRequest = this->UserRequests.constBegin();
       userRequest != this->UserRequests.constEnd(); userRequest++)
  {
    QVariant indexData = this->sourceModel()->data(index, userRequest.key());
    if (!indexData.isValid())
    {
      continue;
    }

    if (indexData.canConvert<QString>())
    {
      QRegularExpression userExpression;
      userExpression.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
      userExpression.setPattern(userRequest.value().toString());
      auto match = userExpression.match(indexData.toString());
      if (match.hasMatch())
      {
        return true;
      }
    }
    else if (indexData == userRequest.value())
    {
      return true;
    }
  }

  return false;
}

//-----------------------------------------------------------------------------
bool pqExtendedSortFilterProxyModel::filterAcceptsRow(
  int sourceRow, const QModelIndex& sourceParent) const
{
  auto proxyIndex = this->sourceModel()->index(sourceRow, 0, sourceParent);

  if (!proxyIndex.isValid())
  {
    return false;
  }

  if (this->isExcluded(proxyIndex))
  {
    return false;
  }

  if (this->hasExactMatch(proxyIndex) || this->hasMatch(proxyIndex) ||
    this->hasUserMatch(proxyIndex))
  {
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
bool pqExtendedSortFilterProxyModel::lessThan(
  const QModelIndex& left, const QModelIndex& right) const
{
  int leftPriority = this->hasExactMatch(left) ? 0
    : this->hasMatch(left)                     ? 1
    : this->hasUserMatch(left)                 ? 2
                                               : 3;
  int rightPriority = this->hasExactMatch(right) ? 0
    : this->hasMatch(right)                      ? 1
    : this->hasUserMatch(right)                  ? 2
                                                 : 3;

  if (leftPriority == rightPriority)
  {
    auto leftName = this->sourceModel()->data(left, Qt::DisplayRole).toString();
    auto rightName = this->sourceModel()->data(right, Qt::DisplayRole).toString();
    return leftName < rightName;
  }
  else
  {
    return leftPriority < rightPriority;
  }
}

//-----------------------------------------------------------------------------
void pqExtendedSortFilterProxyModel::update()
{
  this->invalidateFilter();
  this->sort(0);
}
