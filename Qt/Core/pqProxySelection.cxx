// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqProxySelection.h"

#include "pqApplicationCore.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqProxy.h"
#include "pqServerManagerModel.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProxySelectionModel.h"

#include <QSet>
#include <cassert>

//-----------------------------------------------------------------------------
bool pqProxySelectionUtilities::copy(vtkSMProxySelectionModel* source, pqProxySelection& dest)
{
  assert(source != nullptr);

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();

  pqProxySelection new_selection;
  vtkSMProxySelectionModel::SelectionType::const_iterator iter;
  const vtkSMProxySelectionModel::SelectionType& selection = source->GetSelection();
  for (iter = selection.begin(); iter != selection.end(); ++iter)
  {
    vtkSMProxy* proxy = iter->GetPointer();
    pqServerManagerModelItem* item = smmodel->findItem<pqServerManagerModelItem*>(proxy);
    if (item)
    {
      new_selection.push_back(item);
    }
  }

  if (dest != new_selection)
  {
    dest = new_selection;
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
bool pqProxySelectionUtilities::copy(const pqProxySelection& source, vtkSMProxySelectionModel* dest)
{
  assert(dest != nullptr);

  vtkSMProxySelectionModel::SelectionType selection;
  Q_FOREACH (pqServerManagerModelItem* item, source)
  {
    pqProxy* proxy = qobject_cast<pqProxy*>(item);
    pqOutputPort* port = qobject_cast<pqOutputPort*>(item);
    if (port)
    {
      selection.push_back(port->getOutputPortProxy());
    }
    else if (proxy)
    {
      selection.push_back(proxy->getProxy());
    }
  }
  if (dest->GetSelection() != selection)
  {
    dest->Select(selection, vtkSMProxySelectionModel::CLEAR_AND_SELECT);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
pqProxySelection pqProxySelectionUtilities::getPipelineProxies(const pqProxySelection& sel)
{
  QSet<pqServerManagerModelItem*> proxies;
  for (auto& item : sel)
  {
    if (auto port = qobject_cast<pqOutputPort*>(item))
    {
      proxies.insert(port->getSource());
    }
    else if (auto proxy = qobject_cast<pqProxy*>(item))
    {
      proxies.insert(proxy);
    }
  }

  return proxies.values();
}
