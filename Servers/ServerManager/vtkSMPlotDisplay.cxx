/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPlotDisplay.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPlotDisplay.h"

#include "vtkPVRenderModule.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkProp3D.h"
#include "vtkPVProcessModule.h"
#include "vtkSMPart.h"
#include "vtkSMSourceProxy.h"
#include "vtkPVProcessModule.h"
#include "vtkPVConfig.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkString.h"
#include "vtkRMScalarBarWidget.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"
#include "vtkFieldDataToAttributeDataFilter.h"
#include "vtkClientServerStream.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkMPIMoveData.h"
#include "vtkXYPlotWidget.h"
#include "vtkXYPlotActor.h"
#include "vtkPVOptions.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMPlotDisplay);
vtkCxxRevisionMacro(vtkSMPlotDisplay, "1.1");


//----------------------------------------------------------------------------
vtkSMPlotDisplay::vtkSMPlotDisplay()
{
  this->ProcessModule = NULL;

  this->Visibility = 1;
  this->GeometryIsValid = 0;

  this->DuplicateProxy = vtkSMProxy::New();
  this->DuplicateProxy->SetVTKClassName("vtkMPIMoveData");
  this->DuplicateProxy->SetServersSelf(vtkProcessModule::CLIENT_AND_SERVERS);

  this->UpdateSuppressorProxy = vtkSMProxy::New();
  this->UpdateSuppressorProxy->SetVTKClassName("vtkPVUpdateSuppressor");
  this->UpdateSuppressorProxy->SetServersSelf(vtkProcessModule::CLIENT_AND_SERVERS);

  this->XYPlotActorProxy = vtkSMProxy::New();
  this->XYPlotActorProxy->SetVTKClassName("vtkXYPlotActor");
  this->XYPlotActorProxy->SetServersSelf(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
}

//----------------------------------------------------------------------------
vtkSMPlotDisplay::~vtkSMPlotDisplay()
{
  this->DuplicateProxy->Delete();
  this->DuplicateProxy = 0;
  this->UpdateSuppressorProxy->Delete();
  this->UpdateSuppressorProxy = 0;
  this->XYPlotActorProxy->Delete();
  this->XYPlotActorProxy = 0;
  
  this->SetProcessModule(0);
}


//----------------------------------------------------------------------------
vtkPolyData* vtkSMPlotDisplay::GetCollectedData()
{
  vtkPVProcessModule *pm = this->GetProcessModule();
  if (pm == NULL)
    {
    return NULL;
    }
  vtkMPIMoveData* dp;
  dp = vtkMPIMoveData::SafeDownCast(
      pm->GetObjectFromID(this->DuplicateProxy->GetID(0)));
  if (dp == NULL)
    {
    return NULL;
    }

  return vtkPolyData::SafeDownCast(dp->GetOutput());
}


//----------------------------------------------------------------------------
void vtkSMPlotDisplay::CreateVTKObjects(int num)
{
  vtkPVProcessModule* pm;
  pm = vtkPVProcessModule::SafeDownCast(vtkProcessModule::GetProcessModule());
  vtkClientServerStream& stream = pm->GetStream();

  if (num != 1)
    {
    vtkErrorMacro("PlotFilter has multiple inputs, but only one output.");
    }

  // Plot does not handle multiblock yet..
  this->DuplicateProxy->CreateVTKObjects(num);
  this->UpdateSuppressorProxy->CreateVTKObjects(num);
  this->XYPlotActorProxy->CreateVTKObjects(num);

  // We always duplicate because all processes render the plot.
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->DuplicateProxy->GetID(0) << "SetMoveModeToClone"
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->DuplicateProxy->GetID(0) << "SetMPIMToNSocketConnection" 
    << pm->GetMPIMToNSocketConnectionID()
    << vtkClientServerStream::End;
  // create, SetPassThrough, and set the mToN connection
  // object on all servers and client
  pm->SendStream(vtkProcessModule::RENDER_SERVER|vtkProcessModule::DATA_SERVER);
  // always set client mode
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->DuplicateProxy->GetID(0) << "SetServerToClient"
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT);
  // if running in client mode
  // then set the server to be servermode
  if(pm->GetClientMode())
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->DuplicateProxy->GetID(0) << "SetServerToDataServer"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER);
    }
  // if running in render server mode
  if (pm->GetOptions()->GetRenderServerMode())
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->DuplicateProxy->GetID(0) << "SetServerToRenderServer"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::RENDER_SERVER);
    }  
    
  if(pm->GetClientMode())
    {
    // We need this because the socket controller has no way of distinguishing
    // between processes.
    //law int fixme;  // This is called twice!  Fix it.
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->DuplicateProxy->GetID(0) << "SetServerToClient"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT);
    }
  // Handle collection setup with client server.
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetSocketController"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->DuplicateProxy->GetID(0) << "SetSocketController"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER);

  // Now create the update supressors which keep the renderers/mappers
  // from updating the pipeline.  These are here to ensure that all
  // processes get updated at the same time.
  stream << vtkClientServerStream::Invoke 
         << this->DuplicateProxy->GetID(0) << "GetPolyDataOutput" 
         <<  vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke 
         << this->UpdateSuppressorProxy->GetID(0) << "SetInput" 
         << vtkClientServerStream::LastResult << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);

  // We cannot hook up the XY-plot until we know array names.
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->XYPlotActorProxy->GetID(0) 
                  << "GetPositionCoordinate" 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << vtkClientServerStream::LastResult 
                  << "SetValue" << 0.05 << 0.05 << 0
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->XYPlotActorProxy->GetID(0)
                  << "GetPosition2Coordinate" 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << vtkClientServerStream::LastResult 
                  << "SetValue" << 0.8 << 0.3 << 0
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->XYPlotActorProxy->GetID(0)
                  << "SetNumberOfLabels" << 5 
                  << vtkClientServerStream::End;
  // This is stupid and has to change! (Line division label is meaningless.
  //int fixme;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->XYPlotActorProxy->GetID(0) 
                  << "SetXTitle" << "Line Divisions" 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->XYPlotActorProxy->GetID(0) 
                  << "PlotPointsOn" 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->XYPlotActorProxy->GetID(0)
                  << "GetProperty" 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << vtkClientServerStream::LastResult 
                  << "SetColor" << 1 << 0.8 << 0.8
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->XYPlotActorProxy->GetID(0) 
                  << "GetProperty" 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << vtkClientServerStream::LastResult 
                  << "SetPointSize" << 2
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->XYPlotActorProxy->GetID(0) 
                  << "SetLegendPosition" << 0.4 << 0.6 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->XYPlotActorProxy->GetID(0)
                  << "SetLegendPosition2" << 0.5 << 0.25 
                  << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);

  // Tell the update suppressor to produce the correct partition.
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetNumberOfPartitions"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->UpdateSuppressorProxy->GetID(0) << "SetUpdateNumberOfPieces"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetPartitionId"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->UpdateSuppressorProxy->GetID(0) << "SetUpdatePiece"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
}

//----------------------------------------------------------------------------
void vtkSMPlotDisplay::SetInput(vtkSMSourceProxy* input)
{
  vtkPVProcessModule* pm;
  vtkPVDataInformation* dataInfo = input->GetDataInformation();
  vtkPVDataSetAttributesInformation* pdi = dataInfo->GetPointDataInformation();
  vtkPVArrayInformation* arrayInfo;
  const char* arrayName = 0;

  if (this->DuplicateProxy->GetNumberOfIDs() == 0)
    {
    this->CreateVTKObjects(1);
    }
  //law int fixme; // Do we need to remove the old consumer?
  input->AddConsumer(0, this);

  pm = this->GetProcessModule();  

  // Set vtkData as input to duplicate filter.
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->DuplicateProxy->GetID(0) 
                  << "SetInput" << input->GetPart(0)->GetID(0) 
                  << vtkClientServerStream::End;
  // Only the server has data.
  pm->SendStream(vtkProcessModule::DATA_SERVER);

  // Clear previous plots.
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->XYPlotActorProxy->GetID(0) 
                  << "RemoveAllInputs" 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->XYPlotActorProxy->GetID(0) 
                  << "SetYTitle" << ""
                  << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);

  int numArrays = dataInfo->GetPointDataInformation()->GetNumberOfArrays();
  float cstep = 1.0 / numArrays;
  float ccolor = 0;
  int arrayCount = 0;
  for ( int i = 0; i < numArrays; i++)
    {
    arrayInfo = pdi->GetArrayInformation(i);
    arrayName = arrayInfo->GetName();
    if (arrayInfo->GetNumberOfComponents() == 1)
      {
      pm->GetStream() << vtkClientServerStream::Invoke 
                      << this->UpdateSuppressorProxy->GetID(0) << "GetOutput"
                      << vtkClientServerStream::End;
      pm->GetStream() << vtkClientServerStream::Invoke 
                      << this->XYPlotActorProxy->GetID(0) 
                      << "AddInput" << vtkClientServerStream::LastResult 
                      << arrayName << 0
                      << vtkClientServerStream::End;
      pm->GetStream() << vtkClientServerStream::Invoke 
                      << this->XYPlotActorProxy->GetID(0) 
                      << "SetPlotLabel" << i << arrayName 
                      << vtkClientServerStream::End;
      float r, g, b;
      this->HSVtoRGB(ccolor, 1, 1, &r, &g, &b);
      pm->GetStream() << vtkClientServerStream::Invoke 
                      << this->XYPlotActorProxy->GetID(0) 
                      << "SetPlotColor" << i << r << g << b 
                      << vtkClientServerStream::End;
      ccolor += cstep;
      arrayCount ++;
      }      
    }  
  if ( arrayCount > 1 )
    {
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->XYPlotActorProxy->GetID(0) 
                    << "LegendOn"
                    << vtkClientServerStream::End;
    }
  else 
    {
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->XYPlotActorProxy->GetID(0) 
                    << "LegendOff"
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->XYPlotActorProxy->GetID(0) 
                    << "SetYTitle" << arrayName
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->XYPlotActorProxy->GetID(0) 
                    << "SetPlotColor" << 0 << 1 << 1 << 1
                    << vtkClientServerStream::End;
    }
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
}

//----------------------------------------------------------------------------
void vtkSMPlotDisplay::AddToRenderer(vtkClientServerID rendererID)
{
  //law int fixme; // This used to be renderer2D.  Resolve API to handle 2D actors.
  vtkPVProcessModule *pm = this->GetProcessModule();
  if (pm == 0 || pm->GetRenderModule() == 0)
    { // I had a crash on exit because render module was NULL.
    return;
    }  

  // There will be only one, but this is more general and protects
  // against the user calling this method before "MakeVTKObjects".
  int i, num;
  num = this->XYPlotActorProxy->GetNumberOfIDs();
  for (i = 0; i < num; ++i)
    {
    // Enable XYPlotActor on server for tiled display.
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << rendererID
                    << "AddActor"
                    << this->XYPlotActorProxy->GetID(i) 
                    << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::RENDER_SERVER);
    }
}

//----------------------------------------------------------------------------
void vtkSMPlotDisplay::RemoveFromRenderer(vtkClientServerID rendererID)
{
  vtkPVProcessModule *pm = this->GetProcessModule();
  if (pm == 0 || pm->GetRenderModule() == 0)
    { // I had a crash on exit because render module was NULL.
    return;
    }  

  // There will be only one, but this is more general and protects
  // against the user calling this method before "MakeVTKObjects".
  int i, num;
  num = this->XYPlotActorProxy->GetNumberOfIDs();
  for (i = 0; i < num; ++i)
    {
    // Enable XYPlotActor on server for tiled display.
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << rendererID
                    << "RemoveActor"
                    << this->XYPlotActorProxy->GetID(i) 
                    << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::RENDER_SERVER);
    }
}

//----------------------------------------------------------------------------
void vtkSMPlotDisplay::ConnectWidgetAndActor(vtkXYPlotWidget* widget)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  widget->SetXYPlotActor(vtkXYPlotActor::SafeDownCast(
     pm->GetObjectFromID(this->XYPlotActorProxy->GetID(0))));
}

//----------------------------------------------------------------------------
void vtkSMPlotDisplay::HSVtoRGB(float h, float s, float v, float *r, float *g, float *b)
{
  float R, G, B;
  float max = 1.0;
  float third = max / 3.0;
  float temp;

  // compute rgb assuming S = 1.0;
  if (h >= 0.0 && h <= third) // red -> green
    {
    G = h/third;
    R = 1.0 - G;
    B = 0.0;
    }
  else if (h >= third && h <= 2.0*third) // green -> blue
    {
    B = (h - third)/third;
    G = 1.0 - B;
    R = 0.0;
    }
  else // blue -> red
    {
    R = (h - 2.0 * third)/third;
    B = 1.0 - R;
    G = 0.0;
    }
        
  // add Saturation to the equation.
  s = s / max;
  //R = S + (1.0 - S)*R;
  //G = S + (1.0 - S)*G;
  //B = S + (1.0 - S)*B;
  // what happend to this?
  R = s*R + (1.0 - s);
  G = s*G + (1.0 - s);
  B = s*B + (1.0 - s);
      
  // Use value to get actual RGB 
  // normalize RGB first then apply value
  temp = R + G + B; 
  //V = 3 * V / (temp * max);
  // and what happend to this?
  v = 3 * v / (temp);
  R = R * v;
  G = G * v;
  B = B * v;
      
  // clip below 255
  //if (R > 255.0) R = max;
  //if (G > 255.0) G = max;
  //if (B > 255.0) B = max;
  // mixed constant 255 and max ?????
  if (R > max)
    {
    R = max;
    }
  if (G > max)
    {
    G = max;
    }
  if (B > max)
    {
    B = max;
    }
  *r = R;
  *g = G;
  *b = B;
}

//----------------------------------------------------------------------------
void vtkSMPlotDisplay::SetVisibility(int v)
{
  if (v)
    {
    v = 1;
    }
  if (v == this->Visibility)
    {
    return;
    }    
  
  this->Visibility = v;
    
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (this->XYPlotActorProxy->GetNumberOfIDs() > 0)
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->XYPlotActorProxy->GetID(0) 
      << "SetVisibility" << v << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }}

//----------------------------------------------------------------------------
void vtkSMPlotDisplay::InvalidateGeometry()
{
  this->GeometryIsValid = 0;
}

//----------------------------------------------------------------------------
void vtkSMPlotDisplay::Update()
{
  // Current problem is that there is no input for the UpdateSuppressor object
  if ( ! this->GeometryIsValid && this->UpdateSuppressorProxy != 0 )
    {
    vtkPVProcessModule *pm = this->GetProcessModule();
    vtkClientServerStream& stream = pm->GetStream();
    stream << vtkClientServerStream::Invoke << this->UpdateSuppressorProxy->GetID(0) 
           << "ForceUpdate" << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
    // We need to tell the plot to regenerate.
    stream << vtkClientServerStream::Invoke 
           << this->XYPlotActorProxy->GetID(0)
           << "Modified" << vtkClientServerStream::End;
    this->GeometryIsValid = 1;
    pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }
}

//----------------------------------------------------------------------------
void vtkSMPlotDisplay::SetProcessModule(vtkPVProcessModule *pm)
{
  if (pm == 0)
    {
    if (this->ProcessModule)
      {
      this->ProcessModule->Delete();
      this->ProcessModule = 0;
      }
    return;
    }

  if (this->ProcessModule)
    {
    vtkErrorMacro("ProcessModule already set and part has been initialized.");
    return;
    }

  this->ProcessModule = pm;
  this->ProcessModule->Register(this);
}

//----------------------------------------------------------------------------
void vtkSMPlotDisplay::RemoveAllCaches()
{
  vtkPVProcessModule *pm = this->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  stream << vtkClientServerStream::Invoke 
         << this->UpdateSuppressorProxy->GetID(0) 
         << "RemoveAllCaches" << vtkClientServerStream::End; 
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
}


//----------------------------------------------------------------------------
// Assume that this method is only called when the part is visible.
// This is like the ForceUpdate method, but uses cached values if possible.
void vtkSMPlotDisplay::CacheUpdate(int idx, int total)
{
  vtkPVProcessModule *pm = this->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  stream << vtkClientServerStream::Invoke 
         << this->UpdateSuppressorProxy->GetID(0) 
         << "CacheUpdate" << idx << total << vtkClientServerStream::End;
  // I don't like calling Modified directly, but I need the scalars to be
  // remapped through the lookup table, and this causes that to happen.
  //stream << vtkClientServerStream::Invoke << this->MapperID << "Modified"
  //       << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
}


//----------------------------------------------------------------------------
void vtkSMPlotDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Visibility: " << this->Visibility << endl;
  os << indent << "PVProcessModule: " << this->ProcessModule << endl;
  os << indent << "UpdateSuppressorProxy: " << this->UpdateSuppressorProxy << endl;
  os << indent << "XYPlotActorProxy: " << this->XYPlotActorProxy << endl;
}

