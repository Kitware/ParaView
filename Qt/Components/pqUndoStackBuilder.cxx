/*=========================================================================

   Program: ParaView
   Module:    pqUndoStackBuilder.cxx

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
#include "pqUndoStackBuilder.h"

#include "vtkCommand.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkObjectFactory.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRemoteObject.h"
#include "vtkSMSession.h"
#include "vtkSMUndoStack.h"
#include "vtkUndoSet.h"

#include <sstream>
#include <vtksys/RegularExpression.hxx>

vtkStandardNewMacro(pqUndoStackBuilder);
//-----------------------------------------------------------------------------
pqUndoStackBuilder::pqUndoStackBuilder()
{
  this->IgnoreIsolatedChanges = false;
  this->UndoRedoing = false;
}

//-----------------------------------------------------------------------------
pqUndoStackBuilder::~pqUndoStackBuilder() = default;

//-----------------------------------------------------------------------------
void pqUndoStackBuilder::SetUndoStack(vtkSMUndoStack* stack)
{
  if (this->UndoStack == stack)
  {
    return;
  }

  this->Superclass::SetUndoStack(stack);
}

//-----------------------------------------------------------------------------
bool pqUndoStackBuilder::Filter(vtkSMSession* session, vtkTypeUInt32 globalId)
{
  vtkObject* remoteObj = session->GetRemoteObject(globalId);
  vtkSMProxy* proxy = vtkSMProxy::SafeDownCast(remoteObj);

  // We filter proxy type that must not be involved in undo/redo state.
  // The property themselves are already filtered based on a flag in the XML.
  // XML Flag: state_ignored="1"
  if (!remoteObj ||
    (proxy && (proxy->IsA("vtkSMCameraProxy") || proxy->IsA("vtkSMNewWidgetRepresentationProxy") ||
                proxy->IsA("vtkSMScalarBarWidgetRepresentationProxy") ||
                !strcmp(proxy->GetXMLName(), "FileInformationHelper"))))
  {
    return true;
  }

  // We do not keep track of ProxySelectionModel in Undo/Redo except if we're
  // already with in begin/end. In that case, it makes sense to track how the
  // active source/view is changed so it can restored after undo/redo.
  if (remoteObj->IsA("vtkSMProxySelectionModel") && !this->HandleChangeEvents())
  {
    return true;
  }

  // We keep the state, no filtering here
  return false;
}

//-----------------------------------------------------------------------------
void pqUndoStackBuilder::OnStateChange(vtkSMSession* session, vtkTypeUInt32 globalId,
  const vtkSMMessage* oldState, const vtkSMMessage* newState)
{
  if (this->Filter(session, globalId))
  {
    return;
  }

  bool auto_element = !this->IgnoreAllChanges && !this->IgnoreIsolatedChanges && !this->UndoRedoing;

  if (auto_element)
  {
    vtkSMRemoteObject* proxy = vtkSMRemoteObject::SafeDownCast(session->GetRemoteObject(globalId));
    std::ostringstream stream;
    stream << "Changed '" << proxy->GetClassName() << "'";
    this->Begin(stream.str().c_str());
  }

  this->Superclass::OnStateChange(session, globalId, oldState, newState);

  if (auto_element)
  {
    this->End();

    if (this->UndoSet->GetNumberOfElements() > 0)
    {
      this->PushToStack();
    }
  }
}

//-----------------------------------------------------------------------------
void pqUndoStackBuilder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "IgnoreIsolatedChanges: " << this->IgnoreIsolatedChanges << endl;
}
