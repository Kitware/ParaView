/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPlotDisplay.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPlotDisplay.h"

#include "vtkPVRenderModule.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkProp3D.h"
#include "vtkPVApplication.h"
#include "vtkPVPart.h"
#include "vtkPVProcessModule.h"
#include "vtkPVConfig.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkString.h"
#include "vtkPVColorMap.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"
#include "vtkFieldDataToAttributeDataFilter.h"
#include "vtkClientServerStream.h"
#include "vtkPVRenderView.h"
#include "vtkKWCheckButton.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkM2NDuplicate.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPlotDisplay);
vtkCxxRevisionMacro(vtkPVPlotDisplay, "1.8");


//----------------------------------------------------------------------------
vtkPVPlotDisplay::vtkPVPlotDisplay()
{
  this->PVApplication = NULL;

  this->Visibility = 1;
  this->Part = NULL;
  this->GeometryIsValid = 0;

  this->DuplicatePolyDataID.ID = 0;
  this->UpdateSuppressorID.ID = 0;
  this->XYPlotActorID.ID = 0;
}

//----------------------------------------------------------------------------
vtkPVPlotDisplay::~vtkPVPlotDisplay()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  if(pvApp)
    {
    vtkPVProcessModule* pm;
    pm = pvApp->GetProcessModule();  
    if (pm && this->DuplicatePolyDataID.ID)
      {
      pm->DeleteStreamObject(this->DuplicatePolyDataID);
      pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
      }
    this->DuplicatePolyDataID.ID = 0;
    if (pm && this->UpdateSuppressorID.ID)
      {
      pm->DeleteStreamObject(this->UpdateSuppressorID);
      pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
      }
    this->UpdateSuppressorID.ID = 0;
    if (pm && this->XYPlotActorID.ID)
      {
      pm->DeleteStreamObject(this->XYPlotActorID);
      pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
      }
    this->XYPlotActorID.ID = 0;
    }

  this->SetPart(NULL);
  this->SetPVApplication( NULL);
}


//----------------------------------------------------------------------------
vtkPolyData* vtkPVPlotDisplay::GetCollectedData()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  if (pvApp == NULL)
    {
    return NULL;
    }
  vtkPVProcessModule *pm = pvApp->GetProcessModule();
  if (pm == NULL)
    {
    return NULL;
    }
  vtkMPIDuplicatePolyData* dp;
  dp = vtkM2NDuplicate::SafeDownCast(
      pm->GetObjectFromID(this->DuplicatePolyDataID));
  if (dp == NULL)
    {
    return NULL;
    }

  return dp->GetOutput();
}


//----------------------------------------------------------------------------
void vtkPVPlotDisplay::CreateParallelTclObjects(vtkPVApplication *pvApp)
{
  vtkPVProcessModule *pm = pvApp->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();

  // Create the fliter wich duplicates the data on all processes.
  vtkClientServerID id = pm->NewStreamObject("vtkM2NDuplicate");
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << id << "SetPassThrough" << 0
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << id << "SetMPIMToNSocketConnection" 
    << pm->GetMPIMToNSocketConnectionID()
    << vtkClientServerStream::End;
  // create, SetPassThrough, and set the mToN connection
  // object on all servers and client
  pm->SendStream(vtkProcessModule::RENDER_SERVER|vtkProcessModule::DATA_SERVER);
  // always set client mode
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << id << "SetClientMode" << 1
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT);
  // if running in client mode
  // then set the server to be servermode
  if(pvApp->GetClientMode())
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << id << "SetServerMode" << 1
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER);
    }
  // if running in render server mode
  if(pvApp->GetRenderServerMode())
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << id << "SetRenderServerMode" << 1
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::RENDER_SERVER);
    }  
  this->DuplicatePolyDataID = id;
    
  if(pvApp->GetClientMode())
    {
    // We need this because the socket controller has no way of distinguishing
    // between processes.
    // int fixme;  // No need for both client flag and client mode.

    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->DuplicatePolyDataID << "SetClientFlag" << 1
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT);
    }
  // Handle collection setup with client server.
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetSocketController"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->DuplicatePolyDataID << "SetSocketController"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER);

  // Now create the update supressors which keep the renderers/mappers
  // from updating the pipeline.  These are here to ensure that all
  // processes get updated at the same time.
  this->UpdateSuppressorID = pm->NewStreamObject("vtkPVUpdateSuppressor");
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
  stream << vtkClientServerStream::Invoke << this->DuplicatePolyDataID << "GetOutput" 
         <<  vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << this->UpdateSuppressorID << "SetInput" 
         << vtkClientServerStream::LastResult << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);

  // We cannot hook up the XY-plot until we know array names.
  this->XYPlotActorID = pm->NewStreamObject("vtkXYPlotActor");
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->XYPlotActorID 
                  << "GetPositionCoordinate" 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << vtkClientServerStream::LastResult 
                  << "SetValue" << 0.05 << 0.05 << 0
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->XYPlotActorID 
                  << "GetPosition2Coordinate" 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << vtkClientServerStream::LastResult 
                  << "SetValue" << 0.8 << 0.3 << 0
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->XYPlotActorID 
                  << "SetNumberOfLabels" << 5 
                  << vtkClientServerStream::End;
  // This is stupid and has to change! (Line division is meaningless.
  //int fixme;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->XYPlotActorID 
                  << "SetXTitle" << "Line Divisions" 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->XYPlotActorID 
                  << "PlotPointsOn" 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->XYPlotActorID 
                  << "GetProperty" 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << vtkClientServerStream::LastResult 
                  << "SetColor" << 1 << 0.8 << 0.8
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->XYPlotActorID 
                  << "GetProperty" 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << vtkClientServerStream::LastResult 
                  << "SetPointSize" << 2
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->XYPlotActorID 
                  << "SetLegendPosition" << 0.4 << 0.6 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->XYPlotActorID 
                  << "SetLegendPosition2" << 0.5 << 0.25 
                  << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);

  // Tell the update suppressor to produce the correct partition.
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetNumberOfPartitions"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->UpdateSuppressorID << "SetUpdateNumberOfPieces"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetPartitionId"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->UpdateSuppressorID << "SetUpdatePiece"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
}

//----------------------------------------------------------------------------
void vtkPVPlotDisplay::SetInput(vtkPVPart* input)
{
  vtkPVProcessModule* pm;
  vtkPVDataInformation* dataInfo = input->GetDataInformation();
  vtkPVDataSetAttributesInformation* pdi = dataInfo->GetPointDataInformation();
  vtkPVArrayInformation* arrayInfo;
  const char* arrayName = 0;

  vtkPVApplication *pvApp = this->GetPVApplication();
  if( ! pvApp)
    {
    vtkErrorMacro("Missing Application.");
    return;
    }
  pm = pvApp->GetProcessModule();  

  // Set vtkData as input to duplicate filter.
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->DuplicatePolyDataID 
                  << "SetInput" << input->GetVTKDataID() 
                  << vtkClientServerStream::End;
  // Only the server has data.
  pm->SendStream(vtkProcessModule::DATA_SERVER);

  // Clear previous plots.
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->XYPlotActorID 
                  << "RemoveAllInputs" 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->XYPlotActorID 
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
                      << this->UpdateSuppressorID << "GetOutput"
                      << vtkClientServerStream::End;
      pm->GetStream() << vtkClientServerStream::Invoke 
                      << this->XYPlotActorID 
                      << "AddInput" << vtkClientServerStream::LastResult 
                      << arrayName << 0
                      << vtkClientServerStream::End;
      pm->GetStream() << vtkClientServerStream::Invoke 
                      << this->XYPlotActorID 
                      << "SetPlotLabel" << i << arrayName 
                      << vtkClientServerStream::End;
      float r, g, b;
      this->HSVtoRGB(ccolor, 1, 1, &r, &g, &b);
      pm->GetStream() << vtkClientServerStream::Invoke 
                      << this->XYPlotActorID 
                      << "SetPlotColor" << i << r << g << b 
                      << vtkClientServerStream::End;
      ccolor += cstep;
      arrayCount ++;
      }      
    }  
  if ( arrayCount > 1 )
    {
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->XYPlotActorID 
                    << "LegendOn"
                    << vtkClientServerStream::End;
    }
  else 
    {
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->XYPlotActorID 
                    << "LegendOff"
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->XYPlotActorID 
                    << "SetYTitle" << arrayName
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->XYPlotActorID 
                    << "SetPlotColor" << 0 << 1 << 1 << 1
                    << vtkClientServerStream::End;
    }
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
}

//----------------------------------------------------------------------------
void vtkPVPlotDisplay::HSVtoRGB(float h, float s, float v, float *r, float *g, float *b)
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
void vtkPVPlotDisplay::SetVisibility(int v)
{
  this->Visibility = v;
  // ....
}

//----------------------------------------------------------------------------
void vtkPVPlotDisplay::InvalidateGeometry()
{
  this->GeometryIsValid = 0;
  // ....
}

//----------------------------------------------------------------------------
void vtkPVPlotDisplay::Update()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  // Current problem is that there is no input for the UpdateSuppressor object
  if ( ! this->GeometryIsValid && this->UpdateSuppressorID.ID != 0 )
    {
    vtkPVProcessModule *pm = pvApp->GetProcessModule();
    vtkClientServerStream& stream = pm->GetStream();
    stream << vtkClientServerStream::Invoke << this->UpdateSuppressorID 
           << "ForceUpdate" << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
    // We need to tell the plot to regenerate.
    stream << vtkClientServerStream::Invoke 
           << this->XYPlotActorID
           << "Modified" << vtkClientServerStream::End;
    this->GeometryIsValid = 1;
    pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }
}

//----------------------------------------------------------------------------
void vtkPVPlotDisplay::SetPVApplication(vtkPVApplication *pvApp)
{
  if (pvApp == NULL)
    {
    if (this->PVApplication)
      {
      this->PVApplication->Delete();
      this->PVApplication = NULL;
      }
    return;
    }

  if (this->PVApplication)
    {
    vtkErrorMacro("PVApplication already set and part has been initialized.");
    return;
    }

  this->CreateParallelTclObjects(pvApp);
  this->PVApplication = pvApp;
  this->PVApplication->Register(this);
}

//----------------------------------------------------------------------------
void vtkPVPlotDisplay::RemoveAllCaches()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkPVProcessModule *pm = pvApp->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  stream << vtkClientServerStream::Invoke << this->UpdateSuppressorID 
         << "RemoveAllCaches" << vtkClientServerStream::End; 
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
}


//----------------------------------------------------------------------------
// Assume that this method is only called when the part is visible.
// This is like the ForceUpdate method, but uses cached values if possible.
void vtkPVPlotDisplay::CacheUpdate(int idx, int total)
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkPVProcessModule *pm = pvApp->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  stream << vtkClientServerStream::Invoke << this->UpdateSuppressorID 
         << "CacheUpdate" << idx << total << vtkClientServerStream::End;
  // I don't like calling Modified directly, but I need the scalars to be
  // remapped through the lookup table, and this causes that to happen.
  //stream << vtkClientServerStream::Invoke << this->MapperID << "Modified"
  //       << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
}


//----------------------------------------------------------------------------
void vtkPVPlotDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Visibility: " << this->Visibility << endl;
  os << indent << "Part: " << this->Part << endl;
  os << indent << "PVApplication: " << this->PVApplication << endl;
  os << indent << "UpdateSuppressorID: " << this->UpdateSuppressorID.ID << endl;
  os << indent << "XYPlotActorID: " << this->XYPlotActorID.ID << endl;
}

