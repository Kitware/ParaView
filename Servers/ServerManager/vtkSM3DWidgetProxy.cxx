/*=========================================================================

  Program:   ParaView
  Module:    vtkSM3DWidgetProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSM3DWidgetProxy.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkPVRenderModule.h"
#include "vtkClientServerStream.h"
#include "vtkRenderer.h"
#include "vtkInteractorObserver.h"

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkSM3DWidgetProxy, "1.6");

//----------------------------------------------------------------------------
vtkSM3DWidgetProxy::vtkSM3DWidgetProxy()
{
  this->Placed = 1;
  this->IgnorePlaceWidgetChanges = 0;
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = 0.0;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = 1.0;
}

//----------------------------------------------------------------------------
vtkSM3DWidgetProxy::~vtkSM3DWidgetProxy()
{
}

//----------------------------------------------------------------------------
void vtkSM3DWidgetProxy::InitializeObservers(vtkInteractorObserver* widget3D) 
{
  this->Superclass::InitializeObservers(widget3D);
  if (widget3D)
    {
    widget3D->AddObserver(vtkCommand::PlaceWidgetEvent, 
      reinterpret_cast<vtkCommand*>(this->Observer));
    }
}

//----------------------------------------------------------------------------
void vtkSM3DWidgetProxy::UpdateVTKObjects()
{
  this->Placed = 1;
  this->Superclass::UpdateVTKObjects();
  if (!this->Placed)
    {
    // We send a PlaceWidget message only when the bounds have been 
    // changed (achieved by the this->Placed flag).
    unsigned int cc;
    vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
    for(cc=0; cc < this->GetNumberOfIDs(); cc++)
      {
      pm->GetStream() << vtkClientServerStream::Invoke << this->GetID(cc)
        << "PlaceWidget" 
        << this->Bounds[0] << this->Bounds[1] << this->Bounds[2] 
        << this->Bounds[3] 
        << this->Bounds[4] << this->Bounds[5] << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
      } 
    this->Placed = 1;
    }
}

//----------------------------------------------------------------------------
void vtkSM3DWidgetProxy::PlaceWidget(double bds[6])
{
  if (this->Bounds[0] == bds[0] &&
    this->Bounds[1] == bds[1] &&
    this->Bounds[2] == bds[2] &&
    this->Bounds[3] == bds[3] &&
    this->Bounds[4] == bds[4] &&
    this->Bounds[5] == bds[5])
    {
    return;
    }

  this->Bounds[0] = bds[0];
  this->Bounds[1] = bds[1];
  this->Bounds[2] = bds[2];
  this->Bounds[3] = bds[3];
  this->Bounds[4] = bds[4];
  this->Bounds[5] = bds[5];

  this->Placed = 0;
  
}

//----------------------------------------------------------------------------
void vtkSM3DWidgetProxy::CreateVTKObjects(int numObjects)
{
  if(this->ObjectsCreated)
    {
    return;
    }
  this->Superclass::CreateVTKObjects(numObjects);
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  if(pm->GetRenderModule())
    {
    vtkClientServerID rendererID = pm->GetRenderModule()->GetRendererID();
    vtkClientServerID interactorID = pm->GetRenderModule()->GetInteractorID();
    this->SetCurrentRenderer(rendererID);
    this->SetInteractor(interactorID);
    }
  for (unsigned int cc=0; cc <this->GetNumberOfIDs(); cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    pm->GetStream() << vtkClientServerStream::Invoke << id
      << "SetPlaceFactor" << 1.0
      << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke << id 
      << "PlaceWidget"
      << 0 << 1 << 0 << 1 << 0 << 1 
      << vtkClientServerStream::End;
    // this->Bounds have already been initialized to 0,1,0,1,0,1
    pm->SendStream(this->GetServers());
    }
}

//----------------------------------------------------------------------------
void vtkSM3DWidgetProxy::SaveInBatchScript(ofstream *file)
{
  for (unsigned int cc=0;cc < this->GetNumberOfIDs(); cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    *file << endl;
    *file << "set pvTemp" << id.ID
      <<" [$proxyManager NewProxy 3d_widgets " 
      << this->GetXMLName()
      << "]"
      <<endl;
    *file << "  $proxyManager RegisterProxy 3d_widgets pvTemp"
      << id.ID << " $pvTemp" << id.ID
      << endl;
    *file << "  $pvTemp" << id.ID << " UnRegister {}" << endl;
    *file << "  [$Ren1 GetProperty Displayers] AddProxy $pvTemp"
      << id.ID << endl;

    *file << "  [$pvTemp" << id.ID << " GetProperty IgnorePlaceWidgetChanges]"
      << " SetElements1 1" << endl;
    for(int i=0;i < 6; i++)
      {
      *file << "  [$pvTemp" << id.ID << " GetProperty PlaceWidget] "
        << "SetElement " << i << " "
        << this->Bounds[i] 
        << endl;
      }

    *file << "  [$pvTemp" << id.ID << " GetProperty Visibility] "
      << "SetElements1 " << this->Enabled << endl;

    *file << "  $pvTemp" << id.ID << " UpdateVTKObjects" << endl;
    *file << endl;
    }

}

//----------------------------------------------------------------------------
void vtkSM3DWidgetProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "IgnorePlaceWidgetChanges: " 
    << this->IgnorePlaceWidgetChanges << endl;
}
