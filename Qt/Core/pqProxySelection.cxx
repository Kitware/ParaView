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
#include "pqProxy.h"
#include "pqServerManagerModel.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProxySelectionModel.h"

//-----------------------------------------------------------------------------
bool pqProxySelection::copyFrom(vtkSMProxySelectionModel* other)
{
  Q_ASSERT(other != NULL);

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();

  pqProxySelection new_selection;
  vtkSMProxySelectionModel::SelectionType::const_iterator iter;
  const vtkSMProxySelectionModel::SelectionType& selection = other->GetSelection();
  for (iter = selection.begin(); iter != selection.end(); ++iter)
  {
    vtkSMProxy* proxy = iter->GetPointer();
    pqServerManagerModelItem* item = smmodel->findItem<pqServerManagerModelItem*>(proxy);
    if (item)
    {
      new_selection.insert(item);
    }
  }

  if (*this != new_selection)
  {
    *this = new_selection;
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
bool pqProxySelection::copyTo(vtkSMProxySelectionModel* other) const
{
  Q_ASSERT(other != NULL);

  vtkSMProxySelectionModel::SelectionType selection;
  foreach (pqServerManagerModelItem* item, *this)
  {
    pqProxy* proxy = qobject_cast<pqProxy*>(item);
    pqOutputPort* port = qobject_cast<pqOutputPort*>(item);
    if (port)
    {
      selection.insert(port->getOutputPortProxy());
    }
    else if (proxy)
    {
      selection.insert(proxy->getProxy());
    }
  }
  if (other->GetSelection() != selection)
  {
    other->Select(selection, vtkSMProxySelectionModel::CLEAR_AND_SELECT);
    return true;
  }
  return false;
}
