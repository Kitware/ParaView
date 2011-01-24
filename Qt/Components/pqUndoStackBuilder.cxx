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
#include "vtkSMUndoStack.h"
#include "vtkUndoSet.h"
#include "vtkSMSession.h"
#include "vtkSMRemoteObject.h"

#include <vtksys/ios/sstream>
#include <vtksys/RegularExpression.hxx>

#ifdef FIXME_COLLABORATION
#include "pqProxyUnRegisterUndoElement.h"
#endif

vtkStandardNewMacro(pqUndoStackBuilder);
//-----------------------------------------------------------------------------
pqUndoStackBuilder::pqUndoStackBuilder()
{
  this->IgnoreIsolatedChanges = false;
  this->UndoRedoing = false;
}

//-----------------------------------------------------------------------------
pqUndoStackBuilder::~pqUndoStackBuilder()
{
}

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
void pqUndoStackBuilder::OnStateChange( vtkSMSession *session,
                                        vtkTypeUInt32 globalId,
                                        const vtkSMMessage *oldState,
                                        const vtkSMMessage *newState)
{
  vtkSMRemoteObject* proxy = session->GetRemoteObject(globalId);

  // FIXME COLLABORATION make sure we don't escape too much (Utkarsh)
  // We filter proxy type that must not be involved in undo/redo state.
  // The property themselves are already filtered based on a flag in the XML.
  // XML Flag: state_ignored="1"
  if( !proxy || (proxy && (
      proxy->IsA("vtkSMCameraProxy") ||
      proxy->IsA("vtkSMTimeKeeperProxy") ||
      proxy->IsA("vtkSMAnimationScene") ||
      proxy->IsA("vtkSMAnimationSceneProxy") ||
      proxy->IsA("vtkSMNewWidgetRepresentationProxy") ||
      proxy->IsA("vtkSMScalarBarWidgetRepresentationProxy"))))
    {
    return;
    }

  bool auto_element = !this->IgnoreAllChanges &&
    !this->IgnoreIsolatedChanges && !this->UndoRedoing;

  if (auto_element)
    {
    vtksys_ios::ostringstream stream;
    stream << "Changed '" << proxy->GetClassName() <<"'";
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
void pqUndoStackBuilder::OnUnRegisterProxy(const char* group,
  const char* name, vtkSMProxy* proxy)
{
  // proxies registered as prototypes don't participate in
  // undo/redo.
  vtksys::RegularExpression prototypesRe("_prototypes$");

  if (!proxy || (group && prototypesRe.find(group) != 0))
    {
    return;
    }

#ifdef FIXME_COLLABORATION
  vtkSMProxyUnRegisterUndoElement* elem =
    pqProxyUnRegisterUndoElement::New();
  elem->SetConnectionID(this->ConnectionID);
  elem->ProxyToUnRegister(group, name, proxy);
  this->UndoSet->AddElement(elem);
  elem->Delete();
#endif
}

//-----------------------------------------------------------------------------
void pqUndoStackBuilder::OnPropertyModified(vtkSMProxy* proxy, 
  const char* pname)
{
  if (proxy->IsA("vtkSMViewProxy"))
    {
    if (strcmp(pname, "GUISize" )== 0)
      {
      // GUISize is updated by the GUI.
      return;
      }

    if (strcmp(pname, "WindowPosition") == 0)
      {
      // WindowPosition is updated by the GUI.
      return;
      }

    if (strcmp(pname, "ViewTime") == 0)
      {
      // Render module's ViewTime is controlled by the GUI.
      return;
      }
    }

  if (proxy->IsA("vtkSMAnimationSceneProxy") && 
    strcmp(pname, "ViewModules") == 0)
    {
    // ViewModules are updated by the GUI automatically
    // as view modules are addeed, removed.
    return;
    }

  if (proxy->IsA("vtkSMScalarBarWidgetRepresentationProxy"))
    {
    // For scalar bar, we don't want the position changes to get recorded in the
    // undo-stack automatically.
    vtkSMProperty* prop = proxy->GetProperty(pname);
    if (prop && prop->GetInformationProperty())
      {
      return;
      }
    }
  else if (proxy->IsA("vtkSMNewWidgetRepresentationProxy"))
    {
    // We don't record 3D widget changes.
    return;
    }

  if (proxy->IsA("vtkSMTimeKeeperProxy") &&
    strcmp(pname, "Views") == 0)
    {
    // Views are updated by the GUI automatically.
    return;
    }

#ifdef FIXME_COLLABORATION
  bool auto_element = this->GetEnableMonitoring()==0 && 
    !this->IgnoreIsolatedChanges && !this->UndoRedoing;

  if (/*auto_element && */proxy->IsA("vtkSMViewProxy"))
    {
    // Ignore interaction changes.
    const char* names[] = {
      "CameraPosition", "CameraFocalPoint", 
      "CameraViewAngle",
      "CameraParallelScale",
      "CameraViewUp", "CameraClippingRange", "CenterOfRotation", 0};
    for (int cc=0; names[cc]; cc++)
      {
      if (strcmp(pname, names[cc]) == 0)
        {
        return;
        }
      }
    }

  if (auto_element)
    {
    vtksys_ios::ostringstream stream;
    vtkSMProperty* prop = proxy->GetProperty(pname);
    if (prop->GetInformationOnly() || prop->GetIsInternal())
      {
      return;
      }
    stream << "Changed '" << prop->GetXMLLabel() <<"'";
    this->Begin(stream.str().c_str());
    }

  this->Superclass::OnPropertyModified(proxy, pname);

 if (auto_element)
    {
    this->End();

    if (this->UndoSet->GetNumberOfElements() > 0)
      {
      this->PushToStack();
      }
    }
#endif
}

//-----------------------------------------------------------------------------
void pqUndoStackBuilder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "IgnoreIsolatedChanges: " 
    << this->IgnoreIsolatedChanges << endl;
}
