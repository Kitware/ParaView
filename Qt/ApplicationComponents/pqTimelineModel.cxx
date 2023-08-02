// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqTimelineModel.h"

struct pqTimelineModel::pqInternals
{
  pqTimelineModel* Self;
  QList<QList<QStandardItem*>> SourceRows;
  QList<QList<QStandardItem*>> AnimationRows;
  QList<QStandardItem*> AnimationHeaderRow;
  QList<QStandardItem*> TimeRow;

  pqInternals(pqTimelineModel* self)
    : Self(self)
  {
  }

  QList<QStandardItem*> getRowItems(QList<QList<QStandardItem*>> rows, const QString& name)
  {
    for (auto row : rows)
    {
      if (row[pqTimelineColumn::NAME]->data(pqTimelineItemRole::REGISTRATION_NAME).toString() ==
        name)
      {
        return row;
      }
    }

    return QList<QStandardItem*>();
  }

  void updateTimeCheckState()
  {
    auto flags = this->TimeRow[pqTimelineColumn::NAME]->flags();
    if (this->SourceRows.empty())
    {
      flags &= ~Qt::ItemIsEnabled;
      this->TimeRow[pqTimelineColumn::NAME]->setFlags(flags);
      this->TimeRow[pqTimelineColumn::NAME]->setCheckState(Qt::Unchecked);
      return;
    }
    this->TimeRow[pqTimelineColumn::NAME]->setFlags(flags | Qt::ItemIsEnabled);

    Qt::CheckState newState = this->SourceRows.first()[pqTimelineColumn::NAME]->checkState();
    for (auto source : this->SourceRows)
    {
      if (source[pqTimelineColumn::NAME]->checkState() != newState)
      {
        newState = Qt::PartiallyChecked;
        break;
      }
    }

    this->TimeRow[pqTimelineColumn::NAME]->setCheckState(newState);
  }
};

//-----------------------------------------------------------------------------
pqTimelineModel::pqTimelineModel(QObject* parent)
  : Superclass(parent)
  , Internals(new pqTimelineModel::pqInternals(this))
{
}

//-----------------------------------------------------------------------------
pqTimelineModel::~pqTimelineModel() = default;

//-----------------------------------------------------------------------------
QList<QStandardItem*> pqTimelineModel::createRow(
  pqTimelineTrack::TrackType type, const QString& name, QMap<int, QVariant> additionalData)
{
  QStandardItem* parent = this->invisibleRootItem();
  switch (type)
  {
    case pqTimelineTrack::SOURCE:
    {
      auto timeRow = this->rows(pqTimelineTrack::TIME).first();
      parent = timeRow[pqTimelineColumn::NAME];
    }
    break;
    case pqTimelineTrack::ANIMATION:
    {
      auto animationRow = this->rows(pqTimelineTrack::ANIMATION_HEADER).first();
      parent = animationRow[pqTimelineColumn::NAME];
    }
    break;
    default:
      break;
  }
  QList<QStandardItem*> row;
  for (int col = 0; col < this->columnCount(); col++)
  {
    row << new QStandardItem();
  }

  auto trackRemovalColumnItem = row[pqTimelineColumn::WIDGET];
  Qt::ItemFlags flags = Qt::NoItemFlags;
  trackRemovalColumnItem->setFlags(flags);

  auto nameColumnItem = row[pqTimelineColumn::NAME];
  nameColumnItem->setText(name);
  flags = Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
  if (type == pqTimelineTrack::TIME)
  {
    flags |= Qt::ItemIsAutoTristate;
  }
  nameColumnItem->setFlags(flags);
  nameColumnItem->setCheckState(Qt::Checked);

  auto trackColumnItem = row[pqTimelineColumn::TIMELINE];
  trackColumnItem->setFlags(Qt::NoItemFlags);

  for (auto item : row)
  {
    // by default, consider `name` as the registration name.
    // If not, caller can override this with additionalData.
    item->setData(name, pqTimelineItemRole::REGISTRATION_NAME);
    for (auto role : additionalData.keys())
    {
      item->setData(additionalData[role], role);
    }
    item->setData(type, pqTimelineItemRole::TYPE);
  }

  parent->appendRow(row);

  if (type == pqTimelineTrack::TIME)
  {
    this->Internals->TimeRow = row;
  }
  else if (type == pqTimelineTrack::ANIMATION_HEADER)
  {
    this->Internals->AnimationHeaderRow = row;
  }
  else if (type == pqTimelineTrack::SOURCE)
  {
    this->Internals->SourceRows << row;
    this->Internals->updateTimeCheckState();
  }
  else if (type == pqTimelineTrack::ANIMATION)
  {
    this->Internals->AnimationRows << row;
  }

  return row;
}

//-----------------------------------------------------------------------------
void pqTimelineModel::deleteRow(pqTimelineTrack::TrackType type, const QString& registrationName)
{
  QModelIndex parentIndex;
  int row;
  if (type == pqTimelineTrack::ANIMATION)
  {
    parentIndex = this->indexFromItem(this->Internals->AnimationHeaderRow.first());
    auto rowItems = this->Internals->getRowItems(this->Internals->AnimationRows, registrationName);
    row = this->indexFromItem(rowItems.first()).row();
    this->Internals->AnimationRows.removeOne(rowItems);
    this->removeRow(row, parentIndex);
  }
}

//-----------------------------------------------------------------------------
void pqTimelineModel::clearRows(pqTimelineTrack::TrackType type)
{
  QModelIndex parentIndex;
  if (type == pqTimelineTrack::SOURCE)
  {
    this->Internals->SourceRows.clear();
    parentIndex = this->indexFromItem(this->Internals->TimeRow.first());
  }
  else if (type == pqTimelineTrack::ANIMATION)
  {
    this->Internals->AnimationRows.clear();
    parentIndex = this->indexFromItem(this->Internals->AnimationHeaderRow.first());
  }

  if (parentIndex.isValid())
  {
    this->removeRows(0, this->rowCount(parentIndex), parentIndex);
    if (type == pqTimelineTrack::SOURCE)
    {
      this->Internals->updateTimeCheckState();
    }
  }
}

//-----------------------------------------------------------------------------
bool pqTimelineModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  auto ret = Superclass::setData(index, value, role);
  if (role == Qt::CheckStateRole)
  {
    bool handled = false;
    switch (index.data(pqTimelineItemRole::TYPE).toInt())
    {
      // Handle time tracks
      case pqTimelineTrack::SOURCE:
      {
        this->Internals->updateTimeCheckState();

        Q_EMIT this->sourceStateChanged();
        handled = true;
        break;
      }

      case pqTimelineTrack::TIME:
      {
        auto newState = value.value<Qt::CheckState>();
        if (newState != Qt::PartiallyChecked)
        {
          for (auto source : this->Internals->SourceRows)
          {
            source[pqTimelineColumn::NAME]->setCheckState(newState);
          }
          Q_EMIT this->sourceStateChanged();
        }

        handled = true;
        break;
      }

        // Handle animations
      case pqTimelineTrack::ANIMATION:
      {
        auto newState = value.value<Qt::CheckState>();
        for (auto track : this->Internals->AnimationRows)
        {
          if (track[pqTimelineColumn::NAME]->checkState() != newState)
          {
            newState = Qt::PartiallyChecked;
            break;
          }
        }
        this->Internals->AnimationHeaderRow[pqTimelineColumn::NAME]->setCheckState(newState);

        Q_EMIT this->animationStateChanged();

        handled = true;
        break;
      }

      case pqTimelineTrack::ANIMATION_HEADER:
      {
        auto newState = value.value<Qt::CheckState>();
        if (newState != Qt::PartiallyChecked)
        {
          for (auto track : this->Internals->AnimationRows)
          {
            track[pqTimelineColumn::NAME]->setCheckState(newState);
          }
        }

        Q_EMIT this->animationStateChanged();

        handled = true;
        break;
      }
      default:
        break;
    }

    if (handled)
    {
      Q_EMIT this->dataChanged(index, index, { Qt::CheckStateRole });
      return true;
    }
  }

  return ret;
}

//-----------------------------------------------------------------------------
void pqTimelineModel::setRowEnabled(
  pqTimelineTrack::TrackType type, bool state, const QString& name)
{
  QList<QStandardItem*> row;
  if (type == pqTimelineTrack::SOURCE)
  {
    row = this->Internals->getRowItems(this->Internals->SourceRows, name);
  }
  else if (type == pqTimelineTrack::ANIMATION)
  {
    row = this->Internals->getRowItems(this->Internals->AnimationRows, name);
  }
  else if (type == pqTimelineTrack::TIME)
  {
    row = this->Internals->TimeRow;
  }

  if (row.size() == pqTimelineColumn::COUNT)
  {
    row[pqTimelineColumn::NAME]->setCheckState(state ? Qt::Checked : Qt::Unchecked);
    if (type == pqTimelineTrack::SOURCE || type == pqTimelineTrack::TIME)
    {
      this->Internals->updateTimeCheckState();
    }
  }
}

//-----------------------------------------------------------------------------
bool pqTimelineModel::isRowEnabled(pqTimelineTrack::TrackType type, const QString& name)
{
  QList<QStandardItem*> row;
  if (type == pqTimelineTrack::SOURCE)
  {
    row = this->Internals->getRowItems(this->Internals->SourceRows, name);
  }
  else if (type == pqTimelineTrack::ANIMATION)
  {
    row = this->Internals->getRowItems(this->Internals->AnimationRows, name);
  }
  else if (type == pqTimelineTrack::TIME)
  {
    row = this->Internals->TimeRow;
  }

  if (row.size() == pqTimelineColumn::COUNT)
  {
    return row[pqTimelineColumn::NAME]->checkState() != Qt::Unchecked;
  }

  return false;
}

//-----------------------------------------------------------------------------
void pqTimelineModel::toggleRow(pqTimelineTrack::TrackType type, const QString& name)
{
  this->setRowEnabled(type, !this->isRowEnabled(type, name), name);
}

//-----------------------------------------------------------------------------
void pqTimelineModel::setRowsEnabled(pqTimelineTrack::TrackType type, bool enabled)
{
  if (type == pqTimelineTrack::SOURCE)
  {
    for (auto row : this->Internals->SourceRows)
    {
      row[pqTimelineColumn::NAME]->setCheckState(enabled ? Qt::Checked : Qt::Unchecked);
    }
    this->Internals->TimeRow[pqTimelineColumn::NAME]->setCheckState(
      enabled ? Qt::Checked : Qt::Unchecked);
  }
  else if (type == pqTimelineTrack::ANIMATION)
  {
    for (auto row : this->Internals->AnimationRows)
    {
      row[pqTimelineColumn::NAME]->setCheckState(enabled ? Qt::Checked : Qt::Unchecked);
    }
  }
}

//-----------------------------------------------------------------------------
QList<QList<QStandardItem*>> pqTimelineModel::rows(pqTimelineTrack::TrackType type)
{
  auto rows = QList<QList<QStandardItem*>>();
  if (type == pqTimelineTrack::SOURCE)
  {
    rows = this->Internals->SourceRows;
  }
  else if (type == pqTimelineTrack::ANIMATION)
  {
    rows = this->Internals->AnimationRows;
  }
  else if (type == pqTimelineTrack::TIME)
  {
    rows << this->Internals->TimeRow;
  }
  else if (type == pqTimelineTrack::ANIMATION_HEADER)
  {
    rows << this->Internals->AnimationHeaderRow;
  }

  return rows;
}

//-----------------------------------------------------------------------------
QList<QList<QStandardItem*>> pqTimelineModel::uncheckedRows(pqTimelineTrack::TrackType type)
{
  QList<QList<QStandardItem*>> uncheckedRows;
  for (const auto& row : this->rows(type))
  {
    if (row[pqTimelineColumn::NAME]->checkState() != Qt::Checked)
    {
      uncheckedRows << row;
    }
  }

  return uncheckedRows;
}
