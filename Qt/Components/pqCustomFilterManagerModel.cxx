// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

/// \file pqCustomFilterManagerModel.cxx
/// \date 6/23/2006

#include "pqCustomFilterManagerModel.h"

#include <QList>
#include <QPixmap>
#include <QString>
#include <QtDebug>

#include "pqApplicationCore.h"
#include "pqSettings.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include <sstream>

class pqCustomFilterManagerModelInternal : public QList<QString>
{
};

pqCustomFilterManagerModel::pqCustomFilterManagerModel(QObject* parentObject)
  : QAbstractListModel(parentObject)
{
  this->Internal = new pqCustomFilterManagerModelInternal();
}

pqCustomFilterManagerModel::~pqCustomFilterManagerModel()
{
  this->exportCustomFiltersToSettings();

  delete this->Internal;
}

int pqCustomFilterManagerModel::rowCount(const QModelIndex& parentIndex) const
{
  if (this->Internal && !parentIndex.isValid())
  {
    return this->Internal->size();
  }

  return 0;
}

QModelIndex pqCustomFilterManagerModel::index(
  int row, int column, const QModelIndex& parentIndex) const
{
  if (this->Internal && !parentIndex.isValid() && column == 0 && row >= 0 &&
    row < this->Internal->size())
  {
    return this->createIndex(row, column);
  }

  return QModelIndex();
}

QVariant pqCustomFilterManagerModel::data(const QModelIndex& idx, int role) const
{
  if (this->Internal && idx.isValid() && idx.model() == this)
  {
    switch (role)
    {
      case Qt::DisplayRole:
      case Qt::ToolTipRole:
      case Qt::EditRole:
      {
        return QVariant((*this->Internal)[idx.row()]);
      }
      case Qt::DecorationRole:
      {
        return QVariant(QPixmap(":/pqWidgets/Icons/pqBundle16.png"));
      }
    }
  }

  return QVariant();
}

Qt::ItemFlags pqCustomFilterManagerModel::flags(const QModelIndex&) const
{
  return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QString pqCustomFilterManagerModel::getCustomFilterName(const QModelIndex& idx) const
{
  if (this->Internal && idx.isValid() && idx.model() == this)
  {
    return (*this->Internal)[idx.row()];
  }

  return QString();
}

QModelIndex pqCustomFilterManagerModel::getIndexFor(const QString& filter) const
{
  if (this->Internal && !filter.isEmpty())
  {
    int row = this->Internal->indexOf(filter);
    if (row != -1)
    {
      return this->createIndex(row, 0);
    }
  }

  return QModelIndex();
}

void pqCustomFilterManagerModel::addCustomFilter(QString name)
{
  if (!this->Internal || name.isEmpty())
  {
    return;
  }

  // Make sure the name is new.
  if (this->Internal->contains(name))
  {
    // qDebug() << "Duplicate custom proxy definition added.";
    return;
  }

  // Insert the custom filter in alphabetical order.
  int row = 0;
  for (; row < this->Internal->size(); row++)
  {
    if (QString::compare(name, (*this->Internal)[row]) < 0)
    {
      break;
    }
  }

  this->beginInsertRows(QModelIndex(), row, row);
  this->Internal->insert(row, name);
  this->endInsertRows();

  Q_EMIT this->customFilterAdded(name);
}

void pqCustomFilterManagerModel::removeCustomFilter(QString name)
{
  if (!this->Internal || name.isEmpty())
  {
    return;
  }

  // Find the row for the custom filter.
  int row = this->Internal->indexOf(name);
  if (row == -1)
  {
    qDebug() << "Custom proxy definition not found in the model.";
    return;
  }

  // Notify the view that the index is going away.
  this->beginRemoveRows(QModelIndex(), row, row);
  this->Internal->removeAt(row);
  this->endRemoveRows();
}

void pqCustomFilterManagerModel::importCustomFiltersFromSettings()
{
  vtkSMSessionProxyManager* proxyManager =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  pqSettings* settings = pqApplicationCore::instance()->settings();

  QString key = "CustomFilters";
  if (!settings->contains(key))
  {
    return;
  }

  QString state = settings->value(key).toString();

  if (state.isNull())
  {
    return;
  }

  vtkPVXMLParser* parser = vtkPVXMLParser::New();
  parser->Parse(state.toUtf8().data());

  proxyManager->LoadCustomProxyDefinitions(parser->GetRootElement());

  parser->Delete();
}

void pqCustomFilterManagerModel::exportCustomFiltersToSettings()
{
  // Store the custom filters for a subsequent ParaView session
  vtkSMSessionProxyManager* proxyManager =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();

  if (!proxyManager)
  {
    // No active session
    return;
  }

  vtkPVXMLElement* root = vtkPVXMLElement::New();
  root->SetName("CustomFilterDefinitions");
  proxyManager->SaveCustomProxyDefinitions(root);

  std::ostringstream os;
  root->PrintXML(os, vtkIndent(0));
  QString state = os.str().c_str();
  root->Delete();

  pqApplicationCore::instance()->settings()->setValue("CustomFilters", state);
}
