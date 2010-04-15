/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCubeAxesRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCubeAxesRepresentationProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkSmartPointer.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSourceProxy.h"
#include "vtkBoundingBox.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkSMCubeAxesRepresentationProxy);
vtkCxxRevisionMacro(vtkSMCubeAxesRepresentationProxy, "1.6");
//----------------------------------------------------------------------------
vtkSMCubeAxesRepresentationProxy::vtkSMCubeAxesRepresentationProxy()
{
  this->OutlineFilter = 0;
  this->CubeAxesActor = 0;
  this->Position[0] = this->Position[1] = this->Position[2] = 0.0;
  this->Orientation[0] = this->Orientation[1] = this->Orientation[2] = 0.0;
  this->Scale[0] = this->Scale[1] = this->Scale[2] = 1.0;
  this->CustomBounds[0] = this->CustomBounds[2] = this->CustomBounds[4] = 0.0;
  this->CustomBounds[1] = this->CustomBounds[3] = this->CustomBounds[5] = 1.0;
  this->CustomBoundsActive[0] = 0;
  this->CustomBoundsActive[1] = 0;
  this->CustomBoundsActive[2] = 0;
}

//----------------------------------------------------------------------------
vtkSMCubeAxesRepresentationProxy::~vtkSMCubeAxesRepresentationProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSMCubeAxesRepresentationProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }

  this->OutlineFilter = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("OutlineFilter"));
  this->CubeAxesActor = this->GetSubProxy("Prop2D");
  this->Property = this->GetSubProxy("Property");

  if (!this->OutlineFilter || !this->CubeAxesActor || !this->Property)
    {
    vtkErrorMacro("Missing required subproxies");
    return false;
    }

  this->OutlineFilter->SetServers(vtkProcessModule::DATA_SERVER);
  this->CubeAxesActor->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->Property->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMCubeAxesRepresentationProxy::EndCreateVTKObjects()
{
  vtkSMSourceProxy* input = this->GetInputProxy();
  this->Connect(input, this->OutlineFilter);
  this->Connect(this->Property, this->CubeAxesActor, "Property");
  return this->Superclass::EndCreateVTKObjects();
}

//----------------------------------------------------------------------------
bool vtkSMCubeAxesRepresentationProxy::InitializeStrategy(
  vtkSMViewProxy* vtkNotUsed(view))
{
  // Since we use an outline filter, the data type fed into the strategy is
  // always polydata.
  // Also, we don't need to deliver the data anywhere, since we will obtain
  // bounds using vtkPVDataInformation. Hence we directly create the
  // BlockDeliveryStrategy. We use BlockDeliveryStrategy since it does not
  // create any update suppressor on render server or client -- only the data
  // server where the data is.
  vtkSmartPointer<vtkSMRepresentationStrategy> strategy;

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  // Initialize strategy for the selection pipeline.
  strategy.TakeReference(vtkSMRepresentationStrategy::SafeDownCast(
      pxm->NewProxy("strategies", "BlockDeliveryStrategy")));
  if (!strategy.GetPointer())
    {
    vtkErrorMacro("Could not create strategy for selection pipeline.");
    return false;
    }

  strategy->SetConnectionID(this->ConnectionID);

  // LOD pipeline not needed.
  strategy->SetEnableLOD(false);
  // Caching may be employed when requested.
  strategy->SetEnableCaching(true);

  this->Connect(this->OutlineFilter, strategy);

  strategy->UpdateVTKObjects();

  this->AddStrategy(strategy);
  this->Strategy = strategy; // we keep a reference to make it easy to access the output.
  return true;
}
//----------------------------------------------------------------------------
bool vtkSMCubeAxesRepresentationProxy::AddToView(vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* renderView = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!renderView)
    {
    vtkErrorMacro("View must be a vtkSMRenderViewProxy.");
    return false;
    }

  if (!this->Superclass::AddToView(view))
    {
    return false;
    }

  renderView->AddPropToRenderer(this->CubeAxesActor);

  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << renderView->GetRendererProxy()->GetID()
          << "GetActiveCamera"
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << this->CubeAxesActor->GetID()
          << "SetCamera"
          << vtkClientServerStream::LastResult
          << vtkClientServerStream::End;

  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID, vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER,
    stream);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMCubeAxesRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* renderView = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!renderView)
    {
    vtkErrorMacro("View must be a vtkSMRenderViewProxy.");
    return false;
    }

  renderView->RemovePropFromRenderer(this->CubeAxesActor);

  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << this->CubeAxesActor->GetID()
          << "SetCamera" << 0
          << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID, vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER,
    stream);
  this->Strategy = 0;
  return this->Superclass::RemoveFromView(view);
}


//----------------------------------------------------------------------------
void vtkSMCubeAxesRepresentationProxy::Update(vtkSMViewProxy* view)
{
  this->Superclass::Update(view);
  if (this->GetVisibility() && this->Strategy)
    {
    // Get bounds and set on the actor.
    vtkSMSourceProxy* output = this->Strategy->GetOutput();
    vtkPVDataInformation* info = output->GetDataInformation();
    if (info)
      {
      double *scale = this->Scale;
      double *position = this->Position;
      double *rotation = this->Orientation;
      double bds[6];
      if (scale[0] != 1.0 || scale[1] != 1.0 || scale[2] != 1.0 ||
          position[0] != 0.0 || position[1] != 0.0 || position[2] != 0.0 ||
          rotation[0] != 0.0 || rotation[1] != 0.0 || rotation[2] != 0.0)
        {
        const double *bounds = info->GetBounds();
        vtkSmartPointer<vtkTransform> transform = 
          vtkSmartPointer<vtkTransform>::New();
        transform->Translate(position);
        transform->RotateZ(rotation[2]);
        transform->RotateX(rotation[0]);
        transform->RotateY(rotation[1]);
        transform->Scale(scale);
        vtkBoundingBox bbox;
        int i, j, k;
        double origX[3], x[3];

        for (i = 0; i < 2; i++)
          {
          origX[0] = bounds[i];
          for (j = 0; j < 2; j++)
            {
            origX[1] = bounds[2 + j];
            for (k = 0; k < 2; k++)
              {
              origX[2] = bounds[4 + k];
              transform->TransformPoint(origX, x);
              bbox.AddPoint(x);
              }
            }
          }
        bbox.GetBounds(bds);
        }
      else 
        {
        info->GetBounds(bds);
        }
      vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
        this->CubeAxesActor->GetProperty("Bounds"));
      
      //overload bounds with the active custom bounds
      int pos=0;
      for ( int i=0; i < 3; ++i)
        {
        pos = i * 2;
        if ( this->CustomBoundsActive[i] )
          {
          bds[pos]=this->CustomBounds[pos];
          bds[pos+1]=this->CustomBounds[pos+1];
          }
        }
      dvp->SetElements(bds);
      this->CubeAxesActor->UpdateVTKObjects();
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Scale: " << 
    this->Scale[0] << ", " << this->Scale[1] << ", " << this->Scale[2] << endl;
  os << indent << "Position: " 
    << this->Position[0] << ", " 
    << this->Position[1] << ", " 
    << this->Position[2] << endl;
  os << indent << "Orientation: " 
    << this->Orientation[0] << ", " 
    << this->Orientation[1] << ", " 
    << this->Orientation[2] << endl;
}


