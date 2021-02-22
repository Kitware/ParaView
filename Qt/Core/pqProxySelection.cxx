/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
  foreach (pqServerManagerModelItem* item, source)
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
