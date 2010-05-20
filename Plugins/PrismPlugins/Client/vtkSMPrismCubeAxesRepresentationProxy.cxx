/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPrismCubeAxesRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPrismCubeAxesRepresentationProxy.h"

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
#include "vtkSMIntVectorProperty.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkStringList.h"
#include "pqSMAdaptor.h"

vtkStandardNewMacro(vtkSMPrismCubeAxesRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMPrismCubeAxesRepresentationProxy::vtkSMPrismCubeAxesRepresentationProxy()
{
  this->OutlineFilter = 0;
  this->CubeAxesActor = 0;
  this->Position[0] = this->Position[1] = this->Position[2] = 0.0;
  this->Orientation[0] = this->Orientation[1] = this->Orientation[2] = 0.0;
  this->Scale[0] = this->Scale[1] = this->Scale[2] = 1.0;
}

//----------------------------------------------------------------------------
vtkSMPrismCubeAxesRepresentationProxy::~vtkSMPrismCubeAxesRepresentationProxy()
{
}

void vtkSMPrismCubeAxesRepresentationProxy::SetCubeAxesVisibility(int visible)
{
    this->CubeAxesVisibility = visible;
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
        this->CubeAxesActor->GetProperty("Visibility"));
    if (ivp)
    {
        ivp->SetElement(0, visible);
        this->CubeAxesActor->UpdateProperty("Visibility");
    }

  this->CubeAxesActor->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
bool vtkSMPrismCubeAxesRepresentationProxy::BeginCreateVTKObjects()
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
bool vtkSMPrismCubeAxesRepresentationProxy::EndCreateVTKObjects()
{
  vtkSMSourceProxy* input = this->GetInputProxy();
  this->Connect(input, this->OutlineFilter);
  this->Connect(this->Property, this->CubeAxesActor, "Property");
  return this->Superclass::EndCreateVTKObjects();
}

//----------------------------------------------------------------------------
bool vtkSMPrismCubeAxesRepresentationProxy::InitializeStrategy(
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
bool vtkSMPrismCubeAxesRepresentationProxy::AddToView(vtkSMViewProxy* view)
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
bool vtkSMPrismCubeAxesRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
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
void vtkSMPrismCubeAxesRepresentationProxy::Update(vtkSMViewProxy* view)
{
    this->Superclass::Update(view);
    if (this->GetVisibility() && this->Strategy)
    {
        // Get bounds and set on the actor.
        //  vtkSMSourceProxy* output = this->Strategy->GetOutput();
        // this->Strategy->UpdateVTKObjects();
        vtkSMSourceProxy* output = this->GetInputProxy();

        vtkPVDataInformation* info = output->GetDataInformation(0);
        if (info)
        {
            vtkPVDataSetAttributesInformation* fieldInfo=info->GetFieldDataInformation();
            if(fieldInfo)
            {
                double labelRanges[6];
                vtkPVArrayInformation* xRangeArrayInfo=fieldInfo->GetArrayInformation("XRange");
                if(xRangeArrayInfo)
                {
                    double* range=xRangeArrayInfo->GetComponentRange(0);
                    labelRanges[0]=range[0];
                    labelRanges[1]=range[1];
                }
                vtkPVArrayInformation* yRangeArrayInfo=fieldInfo->GetArrayInformation("YRange");
                if(yRangeArrayInfo)
                {
                    double* range=yRangeArrayInfo->GetComponentRange(0);
                    labelRanges[2]=range[0];
                    labelRanges[3]=range[1];
                }
                vtkPVArrayInformation* zRangeArrayInfo=fieldInfo->GetArrayInformation("ZRange");
                if(zRangeArrayInfo)
                {
                    double* range=zRangeArrayInfo->GetComponentRange(0);
                    labelRanges[4]=range[0];
                    labelRanges[5]=range[1];
                }


                vtkstd::string name=output->GetXMLName();
                if(name=="PrismSurfaceReader")
                {
                    vtkSMProperty* xVariableProperty = output->GetProperty("XAxisVariableName");
                    QVariant str = pqSMAdaptor::getEnumerationProperty(xVariableProperty);

                    pqSMAdaptor::setElementProperty(
                        this->CubeAxesActor->GetProperty("XTitle"),
                        str);

                    vtkSMProperty* yVariableProperty = output->GetProperty("YAxisVariableName");
                    str = pqSMAdaptor::getEnumerationProperty(yVariableProperty);

                    pqSMAdaptor::setElementProperty(
                        this->CubeAxesActor->GetProperty("YTitle"),
                        str);

                    vtkSMProperty* zVariableProperty = output->GetProperty("ZAxisVariableName");
                    str = pqSMAdaptor::getEnumerationProperty(zVariableProperty);

                    pqSMAdaptor::setElementProperty(
                        this->CubeAxesActor->GetProperty("ZTitle"),
                        str);
                }
                else if(name=="PrismFilter")
                {
                    vtkSMProperty* xVariableProperty = output->GetProperty("SESAMEXAxisVariableName");
                    QVariant str = pqSMAdaptor::getEnumerationProperty(xVariableProperty);

                    pqSMAdaptor::setElementProperty(
                        this->CubeAxesActor->GetProperty("XTitle"),
                        str);

                    vtkSMProperty* yVariableProperty = output->GetProperty("SESAMEYAxisVariableName");
                    str = pqSMAdaptor::getEnumerationProperty(yVariableProperty);

                    pqSMAdaptor::setElementProperty(
                        this->CubeAxesActor->GetProperty("YTitle"),
                        str);

                    vtkSMProperty* zVariableProperty = output->GetProperty("SESAMEZAxisVariableName");
                    str = pqSMAdaptor::getEnumerationProperty(zVariableProperty);

                    pqSMAdaptor::setElementProperty(
                        this->CubeAxesActor->GetProperty("ZTitle"),
                        str);
                }


                vtkSMDoubleVectorProperty* rvp = vtkSMDoubleVectorProperty::SafeDownCast(
                    this->CubeAxesActor->GetProperty("LabelRanges"));
                rvp->SetElements(labelRanges);


            }



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
            dvp->SetElements(bds);


            this->CubeAxesActor->UpdateVTKObjects();
        }
    }
}

//----------------------------------------------------------------------------
void vtkSMPrismCubeAxesRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
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


