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
#include "vtkPVProcessModule.h"
#include "vtkSMPart.h"
#include "vtkPVProcessModule.h"
#include "vtkPVConfig.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkString.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"
#include "vtkFieldDataToAttributeDataFilter.h"
#include "vtkClientServerStream.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkMPIMoveData.h"
#include "vtkMath.h"
#include "vtkPVOptions.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPlotDisplay);
vtkCxxRevisionMacro(vtkPVPlotDisplay, "1.4");


//----------------------------------------------------------------------------
vtkPVPlotDisplay::vtkPVPlotDisplay()
{
  this->ProcessModule = NULL;

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
  vtkPVProcessModule *pm = this->GetProcessModule();
  if(pm)
    {
    if (this->DuplicatePolyDataID.ID)
      {
      pm->DeleteStreamObject(this->DuplicatePolyDataID);
      pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
      }
    this->DuplicatePolyDataID.ID = 0;
    if (this->UpdateSuppressorID.ID)
      {
      pm->DeleteStreamObject(this->UpdateSuppressorID);
      pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
      }
    this->UpdateSuppressorID.ID = 0;
    if (this->XYPlotActorID.ID)
      {
      pm->DeleteStreamObject(this->XYPlotActorID);
      pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
      }
    this->XYPlotActorID.ID = 0;
    }

  this->SetPart(0);
  this->SetProcessModule(0);
}


//----------------------------------------------------------------------------
vtkPolyData* vtkPVPlotDisplay::GetCollectedData()
{
  vtkPVProcessModule *pm = this->GetProcessModule();
  if (pm == NULL)
    {
    return NULL;
    }
  vtkMPIMoveData* dp;
  dp = vtkMPIMoveData::SafeDownCast(
      pm->GetObjectFromID(this->DuplicatePolyDataID));
  if (dp == NULL)
    {
    return NULL;
    }

  return vtkPolyData::SafeDownCast(dp->GetOutput());
}


//----------------------------------------------------------------------------
void vtkPVPlotDisplay::CreateParallelTclObjects(vtkPVProcessModule *pm)
{
  vtkClientServerStream& stream = pm->GetStream();

  // Create the fliter wich duplicates the data on all processes.
  vtkClientServerID id = pm->NewStreamObject("vtkMPIMoveData");
  // Create a temporary input.
  // This is needed to get the output of the vtkDataSetToDataSetFitler
  vtkClientServerID id2;
  id2 = pm->NewStreamObject("vtkPolyData");
  pm->GetStream() << vtkClientServerStream::Invoke << id 
                  << "SetInput" << id2 
                  <<  vtkClientServerStream::End;
  pm->DeleteStreamObject(id2);
  // We always duplicate because all processes render the plot.
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << id << "SetMoveModeToClone"
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
    << id << "SetServerToClient"
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT);
  // if running in client mode
  // then set the server to be servermode
  if(pm->GetOptions()->GetClientMode())
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << id << "SetServerToDataServer"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER);
    }
  // if running in render server mode
  if(pm->GetOptions()->GetRenderServerMode())
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << id << "SetServerToRenderServer"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::RENDER_SERVER);
    }  
  this->DuplicatePolyDataID = id;
    
  if(pm->GetOptions()->GetClientMode())
    {
    // We need this because the socket controller has no way of distinguishing
    // between processes.
    // int fixme;  // No need for both client flag and client mode.

    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->DuplicatePolyDataID << "SetServerToClient"
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
void vtkPVPlotDisplay::SetInput(vtkSMPart* input)
{
  vtkPVProcessModule* pm;
  vtkPVDataInformation* dataInfo = input->GetDataInformation();
  vtkPVDataSetAttributesInformation* pdi = dataInfo->GetPointDataInformation();
  vtkPVArrayInformation* arrayInfo;
  const char* arrayName = 0;

  pm = this->GetProcessModule();  

  // Set vtkData as input to duplicate filter.
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->DuplicatePolyDataID 
                  << "SetInput" << input->GetID(0) 
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
      vtkMath::HSVToRGB(ccolor,1,1,&r,&g,&b);
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
void vtkPVPlotDisplay::InvalidateGeometry()
{
  this->GeometryIsValid = 0;
  // ....
}

//----------------------------------------------------------------------------
void vtkPVPlotDisplay::Update()
{
  // Current problem is that there is no input for the UpdateSuppressor object
  if ( ! this->GeometryIsValid && this->UpdateSuppressorID.ID != 0 )
    {
    vtkPVProcessModule *pm = this->GetProcessModule();
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
void vtkPVPlotDisplay::SetProcessModule(vtkPVProcessModule *pm)
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

  this->CreateParallelTclObjects(pm);
  this->ProcessModule = pm;
  this->ProcessModule->Register(this);
}

//----------------------------------------------------------------------------
void vtkPVPlotDisplay::RemoveAllCaches()
{
  vtkPVProcessModule *pm = this->GetProcessModule();
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
  vtkPVProcessModule *pm = this->GetProcessModule();
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
  os << indent << "PVProcessModule: " << this->ProcessModule << endl;
  os << indent << "UpdateSuppressorID: " << this->UpdateSuppressorID.ID << endl;
  os << indent << "XYPlotActorID: " << this->XYPlotActorID.ID << endl;
}

