/*=========================================================================

  Program:   ParaView
  Module:    vtkPVData.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVData.h"

#include "vtkCubeAxesActor2D.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkKWBoundsDisplay.h"
#include "vtkKWChangeColorButton.h"
#include "vtkKWCheckButton.h"
#include "vtkKWCheckButton.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWMenuButton.h"
#include "vtkKWNotebook.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWPushButton.h"
#include "vtkKWScale.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWView.h"
#include "vtkKWWidget.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVColorMap.h"
#include "vtkPVConfig.h"
#include "vtkPVRenderView.h"
#include "vtkPVSource.h"
#include "vtkPVWindow.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkString.h"
#include "vtkTexture.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"
#include "vtkTreeComposite.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVData);
vtkCxxRevisionMacro(vtkPVData, "1.161.2.2");

int vtkPVDataCommand(ClientData cd, Tcl_Interp *interp,
                     int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVData::vtkPVData()
{
  this->CommandFunction = vtkPVDataCommand;

  this->VTKData = NULL;
  this->VTKDataTclName = NULL;
  this->PVSource = NULL;
  
  this->NumberOfPVConsumers = 0;
  this->PVConsumers = 0;

  this->PropertiesCreated = 0;


  // Used to be in vtkPVActorComposite
  static int instanceCount = 0;
  
  this->PVRenderView = NULL;
  this->PropertiesParent = NULL; 

  this->Mapper = NULL;

  this->Property = NULL;
  this->PropertyTclName = NULL;
  this->Prop = NULL;
  this->PropTclName = NULL;
  this->LODDeciTclName = NULL;
  this->MapperTclName = NULL;
  this->LODMapperTclName = NULL;
  this->GeometryTclName = NULL;
  this->CollectTclName = NULL;
  this->LODCollectTclName = NULL;
  this->CubeAxesTclName = NULL;
  this->UpdateSuppressorTclName = NULL;
  this->LODUpdateSuppressorTclName = NULL;

  // Create a unique id for creating tcl names.
  ++instanceCount;
  this->InstanceCount = instanceCount;
    
  this->Properties = vtkKWFrame::New();
  this->InformationFrame = vtkKWFrame::New();

  this->ColorFrame = vtkKWLabeledFrame::New();
  this->DisplayStyleFrame = vtkKWLabeledFrame::New();
  this->StatsFrame = vtkKWLabeledFrame::New();
  this->ViewFrame = vtkKWLabeledFrame::New();
  
  this->NumCellsLabel = vtkKWLabel::New();
  this->NumPointsLabel = vtkKWLabel::New();
  
  this->BoundsDisplay = vtkKWBoundsDisplay::New();
  this->BoundsDisplay->ShowHideFrameOn();
  
  this->AmbientScale = vtkKWScale::New();

  this->ColorMenuLabel = vtkKWLabel::New();
  this->ColorMenu = vtkKWOptionMenu::New();

  this->EditColorMapButton = vtkKWPushButton::New();
  
  this->ColorButton = vtkKWChangeColorButton::New();

  this->RepresentationMenuLabel = vtkKWLabel::New();
  this->RepresentationMenu = vtkKWOptionMenu::New();
  
  this->InterpolationMenuLabel = vtkKWLabel::New();
  this->InterpolationMenu = vtkKWOptionMenu::New();
  
  this->PointSizeLabel = vtkKWLabel::New();
  this->PointSizeScale = vtkKWScale::New();
  this->LineWidthLabel = vtkKWLabel::New();
  this->LineWidthScale = vtkKWScale::New();
  
  this->ScalarBarCheck = vtkKWCheckButton::New();
  this->CubeAxesCheck = vtkKWCheckButton::New();

  this->VisibilityCheck = vtkKWCheckButton::New();

  this->ResetCameraButton = vtkKWPushButton::New();

  this->ActorControlFrame = vtkKWLabeledFrame::New();
  this->TranslateLabel = vtkKWLabel::New();
  this->ScaleLabel = vtkKWLabel::New();
  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->TranslateEntry[cc] = vtkKWEntry::New();
    this->ScaleEntry[cc] = vtkKWEntry::New();
    }
  this->OpacityLabel = vtkKWLabel::New();
  this->Opacity = vtkKWScale::New();
  
  
  this->PreviousAmbient = 0.15;
  this->PreviousSpecular = 0.1;
  this->PreviousDiffuse = 0.8;
  this->PreviousWasSolid = 1;

  this->PVColorMap = NULL;

  this->LODResolution = 50;
  this->CollectThreshold = 2.0;

}

//----------------------------------------------------------------------------
vtkPVData::~vtkPVData()
{
  if (this->PVColorMap)
    {
    this->PVColorMap->UnRegister(this);
    this->PVColorMap = 0;
    }

  // Get rid of the circular reference created by the extent translator.
  if (this->VTKDataTclName)
    {
    this->GetPVApplication()->BroadcastScript("%s SetExtentTranslator {}",
                                              this->VTKDataTclName);
    }  
    
  this->SetVTKData(NULL, NULL);
  this->SetPVSource(NULL);
  
  delete [] this->PVConsumers;



  // Used to be in vtkPVActorComposite........
  vtkPVApplication *pvApp = this->GetPVApplication();

  this->SetPVRenderView(NULL);
  if (this->PropertiesParent)
    {
    this->PropertiesParent->UnRegister(this);
    this->PropertiesParent = NULL;
    }
    
  this->NumCellsLabel->Delete();
  this->NumCellsLabel = NULL;
  
  this->NumPointsLabel->Delete();
  this->NumPointsLabel = NULL;
  
  this->BoundsDisplay->Delete();
  this->BoundsDisplay = NULL;
  
  this->AmbientScale->Delete();
  this->AmbientScale = NULL;
  
  this->ColorMenuLabel->Delete();
  this->ColorMenuLabel = NULL;
  
  this->ColorMenu->Delete();
  this->ColorMenu = NULL;

  this->EditColorMapButton->Delete();
  this->EditColorMapButton = NULL;
    
  this->ColorButton->Delete();
  this->ColorButton = NULL;
  
  this->RepresentationMenuLabel->Delete();
  this->RepresentationMenuLabel = NULL;  
  this->RepresentationMenu->Delete();
  this->RepresentationMenu = NULL;
  
  this->InterpolationMenuLabel->Delete();
  this->InterpolationMenuLabel = NULL;
  this->InterpolationMenu->Delete();
  this->InterpolationMenu = NULL;
  
  this->PointSizeLabel->Delete();
  this->PointSizeLabel = NULL;
  this->PointSizeScale->Delete();
  this->PointSizeScale = NULL;
  this->LineWidthLabel->Delete();
  this->LineWidthLabel = NULL;
  this->LineWidthScale->Delete();
  this->LineWidthScale = NULL;

  this->ActorControlFrame->Delete();
  this->TranslateLabel->Delete();
  this->ScaleLabel->Delete();
  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->TranslateEntry[cc]->Delete();
    this->ScaleEntry[cc]->Delete();
    }
  this->OpacityLabel->Delete();
  this->Opacity->Delete();
 
  if (this->CubeAxesTclName)
    {
    if ( pvApp )
      {
      pvApp->Script("%s Delete", this->CubeAxesTclName);
      }
    this->SetCubeAxesTclName(NULL);
    }
  if ( pvApp )
    {
    pvApp->BroadcastScript("%s Delete", this->MapperTclName);
    }
  this->SetMapperTclName(NULL);
  this->Mapper = NULL;
  
  if ( pvApp )
    {
    pvApp->BroadcastScript("%s Delete", this->LODMapperTclName);
    }
  this->SetLODMapperTclName(NULL);
  
  if ( pvApp )
    {
    pvApp->BroadcastScript("%s Delete", this->PropTclName);
    }
  this->SetPropTclName(NULL);
  this->Prop = NULL;
  
  if ( pvApp )
    {
    pvApp->BroadcastScript("%s Delete", this->PropertyTclName);
    }
  this->SetPropertyTclName(NULL);
  this->Property = NULL;
  
  if (this->LODDeciTclName)
    {
    if ( pvApp )
      {
      pvApp->BroadcastScript("%s Delete", this->LODDeciTclName);
      }
    this->SetLODDeciTclName(NULL);
    }
  
  this->ScalarBarCheck->Delete();
  this->ScalarBarCheck = NULL;  
  
  this->CubeAxesCheck->Delete();
  this->CubeAxesCheck = NULL;
  
  this->VisibilityCheck->Delete();
  this->VisibilityCheck = NULL;
  
  if (this->GeometryTclName)
    {
    if ( pvApp )
      {
      pvApp->BroadcastScript("%s Delete", this->GeometryTclName);
      }
    this->SetGeometryTclName(NULL);
    }

  if (this->UpdateSuppressorTclName)
    {
    if ( pvApp )
      {
      pvApp->BroadcastScript("%s Delete", this->UpdateSuppressorTclName);
      }
    this->SetUpdateSuppressorTclName(NULL);
    }

  if (this->LODUpdateSuppressorTclName)
    {
    if ( pvApp )
      {
      pvApp->BroadcastScript("%s Delete", this->LODUpdateSuppressorTclName);
      }
    this->SetLODUpdateSuppressorTclName(NULL);
    }

  if (this->CollectTclName)
    {
    if ( pvApp )
      {
      pvApp->BroadcastScript("%s Delete", this->CollectTclName);
      }
    this->SetCollectTclName(NULL);
    }
  if (this->LODCollectTclName)
    {
    if ( pvApp )
      {
      pvApp->BroadcastScript("%s Delete", this->LODCollectTclName);
      }
    this->SetLODCollectTclName(NULL);
    }

  this->ColorFrame->Delete();
  this->ColorFrame = NULL;
  this->DisplayStyleFrame->Delete();
  this->DisplayStyleFrame = NULL;
  this->StatsFrame->Delete();
  this->StatsFrame = NULL;
  this->ViewFrame->Delete();
  this->ViewFrame = NULL;
  
  this->ResetCameraButton->Delete();
  this->ResetCameraButton = NULL;

  this->Properties->Delete();
  this->Properties = NULL;

  this->InformationFrame->Delete();
  this->InformationFrame = NULL;
}

//----------------------------------------------------------------------------
void vtkPVData::SetPVColorMap(vtkPVColorMap *colorMap)
{
  if (this->PVColorMap == colorMap)
    {
    return;
    }

  if (this->PVColorMap)
    {
    // If no one is using the color map any more, 
    // turn the scalar bar visibility off.
    // Only the Window will hold a reference.
    if (this->PVColorMap->GetReferenceCount() <= 2 )
      {
      this->PVColorMap->SetScalarBarVisibility(0);
      }
    this->PVColorMap->UnRegister(this);
    this->PVColorMap = NULL;
    }

  this->PVColorMap = colorMap;
  if (this->PVColorMap)
    {
    this->PVColorMap->Register(this);
    }
  this->UpdateProperties();
}

//----------------------------------------------------------------------------
void vtkPVData::CreateParallelTclObjects(vtkPVApplication *pvApp)
{
  int numProcs, id;
  char tclName[100];
  
  this->vtkKWObject::SetApplication(pvApp);
  
  // Create the one geometry filter,
  // which creates the poly data from all data sets.
  sprintf(tclName, "Geometry%d", this->InstanceCount);
  pvApp->BroadcastScript("vtkPVGeometryFilter %s", tclName);
  this->SetGeometryTclName(tclName);
  // Keep track of how long each geometry filter takes to execute.
  pvApp->BroadcastScript("%s SetStartMethod {$Application LogStartEvent "
                         "{Execute Geometry}}", this->GeometryTclName);
  pvApp->BroadcastScript("%s SetEndMethod {$Application LogEndEvent "
                         "{Execute Geometry}}", this->GeometryTclName);


  // Create the decimation filter which branches the LOD pipeline.
  sprintf(tclName, "LODDeci%d", this->InstanceCount);
  pvApp->BroadcastScript("vtkQuadricClustering %s", tclName);
  this->LODDeciTclName = NULL;
  this->SetLODDeciTclName(tclName);
  // Keep track of how long each decimation filter takes to execute.
  pvApp->BroadcastScript("%s SetStartMethod {$Application LogStartEvent {Execute Decimate}}", 
                         this->LODDeciTclName);
  pvApp->BroadcastScript("%s SetEndMethod {$Application LogEndEvent {Execute Decimate}}", 
                         this->LODDeciTclName);
  // The input of course is the geometry filter.
  pvApp->BroadcastScript("%s SetInput [%s GetOutput]", 
                         this->LODDeciTclName, this->GeometryTclName);
  pvApp->BroadcastScript("%s CopyCellDataOn", this->LODDeciTclName);
  pvApp->BroadcastScript("%s UseInputPointsOn", this->LODDeciTclName);
  pvApp->BroadcastScript("%s UseInternalTrianglesOff", this->LODDeciTclName);
  // These options reduce seams, but makes the decimation too slow.
  //pvApp->BroadcastScript("%s UseFeatureEdgesOn", this->LODDeciTclName);
  //pvApp->BroadcastScript("%s UseFeaturePointsOn", this->LODDeciTclName);
  // This should be changed to origin and spacing determined globally.
  pvApp->BroadcastScript("%s SetNumberOfDivisions %d %d %d", 
                         this->LODDeciTclName, this->LODResolution,
                         this->LODResolution, this->LODResolution); 

#ifdef VTK_USE_MPI
  // Create the collection filters which allow small models to render locally.  
  // They also redistributed data for SGI pipes option.
  // ===== Primary branch:
  sprintf(tclName, "Collect%d", this->InstanceCount);

  // Different filter for  pipe redistribution.
  if (pvApp->GetUseRenderingGroup())
    {
    pvApp->BroadcastScript("vtkAllToNRedistributePolyData %s", tclName);
    pvApp->BroadcastScript("%s SetNumberOfProcesses %d", tclName,
                           pvApp->GetNumberOfPipes());
    }
  else
    {
    pvApp->BroadcastScript("vtkCollectPolyData %s", tclName);
    }
  this->SetCollectTclName(tclName);
  pvApp->BroadcastScript("%s SetInput [%s GetOutput]", 
                         this->CollectTclName, this->GeometryTclName);
  pvApp->BroadcastScript("%s SetStartMethod {$Application LogStartEvent {Execute Collect}}", 
                         this->CollectTclName);
  pvApp->BroadcastScript("%s SetEndMethod {$Application LogEndEvent {Execute Collect}}", 
                         this->CollectTclName);
  //
  // ===== LOD branch:
  sprintf(tclName, "LODCollect%d", this->InstanceCount);

  // Different filter for pipe redistribution.
  if (pvApp->GetUseRenderingGroup())
    {
    pvApp->BroadcastScript("vtkAllToNRedistributePolyData %s", tclName);
    pvApp->BroadcastScript("%s SetNumberOfProcesses %d", tclName,
                           pvApp->GetNumberOfPipes());
    }
  else
    {
    pvApp->BroadcastScript("vtkCollectPolyData %s", tclName);
    }
  this->SetLODCollectTclName(tclName);
  pvApp->BroadcastScript("%s SetInput [%s GetOutput]", 
                         this->LODCollectTclName, this->LODDeciTclName);
  pvApp->BroadcastScript("%s SetStartMethod {$Application LogStartEvent {Execute LODCollect}}", 
                         this->LODCollectTclName);
  pvApp->BroadcastScript("%s SetEndMethod {$Application LogEndEvent {Execute LODCollect}}", 
                         this->LODCollectTclName);
#endif




  // Now create the update supressors which keep the renderers/mappers
  // from updating the pipeline.  These are here to ensure that all
  // processes get updated at the same time.
  // ===== Primary branch:
  sprintf(tclName, "UpdateSuppressor%d", this->InstanceCount);
  pvApp->BroadcastScript("vtkPVUpdateSuppressor %s", tclName);
  this->SetUpdateSuppressorTclName(tclName);
  if (this->CollectTclName)
    {
    pvApp->BroadcastScript("%s SetInput [%s GetOutput]", 
                           this->UpdateSuppressorTclName, 
                           this->CollectTclName);
    }
  else
    {
    pvApp->BroadcastScript("%s SetInput [%s GetOutput]", 
                           this->UpdateSuppressorTclName, 
                           this->GeometryTclName);
    }
  //
  // ===== LOD branch:
  sprintf(tclName, "LODUpdateSuppressor%d", this->InstanceCount);
  pvApp->BroadcastScript("vtkPVUpdateSuppressor %s", tclName);
  this->SetLODUpdateSuppressorTclName(tclName);
  if (this->LODCollectTclName)
    {
    pvApp->BroadcastScript("%s SetInput [%s GetOutput]", 
                           this->LODUpdateSuppressorTclName, 
                           this->LODCollectTclName);
    }
  else
    {
    pvApp->BroadcastScript("%s SetInput [%s GetOutput]", 
                           this->LODUpdateSuppressorTclName, 
                           this->LODDeciTclName);
    }


  // Now create the mappers for the two branches.
  // Make a new tcl object.
  // ===== Primary branch:
  sprintf(tclName, "Mapper%d", this->InstanceCount);
  this->Mapper = (vtkPolyDataMapper*)pvApp->MakeTclObject("vtkPolyDataMapper",
                                                          tclName);
  this->MapperTclName = NULL;
  this->SetMapperTclName(tclName);
  pvApp->BroadcastScript("%s UseLookupTableScalarRangeOn", this->MapperTclName);
  pvApp->BroadcastScript("%s SetColorModeToMapScalars", this->MapperTclName);
  pvApp->BroadcastScript("%s SetInput [%s GetOutput]", this->MapperTclName,
                         this->UpdateSuppressorTclName);
  //
  // ===== LOD branch:
  sprintf(tclName, "LODMapper%d", this->InstanceCount);
  this->Mapper = (vtkPolyDataMapper*)pvApp->MakeTclObject("vtkPolyDataMapper",
                                                          tclName);
  this->LODMapperTclName = NULL;
  this->SetLODMapperTclName(tclName);
  pvApp->BroadcastScript("%s UseLookupTableScalarRangeOn", this->LODMapperTclName);
  pvApp->BroadcastScript("%s SetColorModeToMapScalars", this->LODMapperTclName);
  pvApp->BroadcastScript("%s SetInput [%s GetOutput]", this->LODMapperTclName,
                         this->LODUpdateSuppressorTclName);
  
  
  // Now the two branches merge at the LOD actor.
  sprintf(tclName, "Actor%d", this->InstanceCount);
  this->Prop = (vtkProp*)pvApp->MakeTclObject("vtkPVLODActor", tclName);
  this->SetPropTclName(tclName);

  // Make a new tcl object.
  sprintf(tclName, "Property%d", this->InstanceCount);
  this->Property = (vtkProperty*)pvApp->MakeTclObject("vtkProperty", tclName);
  this->SetPropertyTclName(tclName);
  pvApp->BroadcastScript("%s SetAmbient 0.15", this->PropertyTclName);
  pvApp->BroadcastScript("%s SetDiffuse 0.85", this->PropertyTclName);
  pvApp->BroadcastScript("%s SetProperty %s", this->PropTclName, 
                         this->PropertyTclName);
  pvApp->BroadcastScript("%s SetMapper %s", this->PropTclName, 
                         this->MapperTclName);
  pvApp->BroadcastScript("%s SetLODMapper %s", this->PropTclName,
                         this->LODMapperTclName);
  


  // Hard code assignment based on processes.
  numProcs = pvApp->GetController()->GetNumberOfProcesses();

  // Special debug situation. Only generate half the data.
  // This allows us to debug the parallel features of the
  // application and VTK on only one process.
  int debugNum = numProcs;
  if (getenv("PV_DEBUG_ZERO") != NULL)
    {
    this->Script("%s SetNumberOfPieces 0",this->MapperTclName);
    this->Script("%s SetPiece 0", this->MapperTclName);
    this->Script("%s SetUpdateNumberOfPieces 0",this->UpdateSuppressorTclName);
    this->Script("%s SetUpdatePiece 0", this->UpdateSuppressorTclName);
    this->Script("%s SetNumberOfPieces 0", this->LODMapperTclName);
    this->Script("%s SetPiece 0", this->LODMapperTclName);
    for (id = 1; id < numProcs; ++id)
      {
      pvApp->RemoteScript(id, "%s SetNumberOfPieces %d",
                          this->MapperTclName, debugNum-1);
      pvApp->RemoteScript(id, "%s SetPiece %d", this->MapperTclName, id-1);
      pvApp->RemoteScript(id, "%s SetUpdateNumberOfPieces %d",
                          this->UpdateSuppressorTclName, debugNum-1);
      pvApp->RemoteScript(id, "%s SetUpdatePiece %d", 
                          this->UpdateSuppressorTclName, id-1);
      pvApp->RemoteScript(id, "%s SetNumberOfPieces %d",
                          this->LODMapperTclName, debugNum-1);
      pvApp->RemoteScript(id, "%s SetPiece %d", this->LODMapperTclName, id-1);
      pvApp->RemoteScript(id, "%s SetUpdateNumberOfPieces %d",
                          this->LODUpdateSuppressorTclName, debugNum-1);
      pvApp->RemoteScript(id, "%s SetUpdatePiece %d", 
                          this->LODUpdateSuppressorTclName, id-1);
      }
    }
  else 
    {
    if (getenv("PV_DEBUG_HALF") != NULL)
      {
      debugNum *= 2;
      }
    for (id = 0; id < numProcs; ++id)
      {
      pvApp->RemoteScript(id, "%s SetNumberOfPieces %d",
                          this->MapperTclName, debugNum);
      pvApp->RemoteScript(id, "%s SetPiece %d", this->MapperTclName, id);
      pvApp->RemoteScript(id, "%s SetUpdateNumberOfPieces %d",
                          this->UpdateSuppressorTclName, debugNum);
      pvApp->RemoteScript(id, "%s SetUpdatePiece %d", 
                          this->UpdateSuppressorTclName, id);
      pvApp->RemoteScript(id, "%s SetNumberOfPieces %d",
                          this->LODMapperTclName, debugNum);
      pvApp->RemoteScript(id, "%s SetPiece %d", this->LODMapperTclName, id);
      pvApp->RemoteScript(id, "%s SetUpdateNumberOfPieces %d",
                          this->LODUpdateSuppressorTclName, debugNum);
      pvApp->RemoteScript(id, "%s SetUpdatePiece %d", 
                          this->LODUpdateSuppressorTclName, id);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVData::ForceUpdate(vtkPVApplication* pvApp)
{
  if ( this->UpdateSuppressorTclName )
    {
    pvApp->BroadcastScript("%s ForceUpdate", this->UpdateSuppressorTclName);
    pvApp->BroadcastScript("%s ForceUpdate", this->LODUpdateSuppressorTclName);
    }
}

//----------------------------------------------------------------------------
void vtkPVData::DeleteCallback()
{
  this->SetCubeAxesVisibility(0);
  this->SetScalarBarVisibility(0);
}

//----------------------------------------------------------------------------
void vtkPVData::SetPVApplication(vtkPVApplication *pvApp)
{
  this->CreateParallelTclObjects(pvApp);
  this->vtkKWObject::SetApplication(pvApp);
}


//----------------------------------------------------------------------------
// Tcl does the reference counting, so we are not going to put an 
// additional reference of the data.
void vtkPVData::SetVTKData(vtkDataSet *data, const char *tclName)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp == NULL)
    {
    vtkErrorMacro("Set the application before you set the VTKDataTclName.");
    return;
    }
  
  if (this->VTKDataTclName)
    {
    pvApp->BroadcastScript("%s Delete", this->VTKDataTclName);
    delete [] this->VTKDataTclName;
    this->VTKDataTclName = NULL;
    this->VTKData = NULL;
    }
  if (tclName)
    {
    this->VTKDataTclName = new char[strlen(tclName) + 1];
    strcpy(this->VTKDataTclName, tclName);
    this->VTKData = data;
    
    }
}

void vtkPVData::AddPVConsumer(vtkPVSource *c)
{
  // make sure it isn't already there
  if (this->IsPVConsumer(c))
    {
    return;
    }
  // add it to the list, reallocate memory
  vtkPVSource **tmp = this->PVConsumers;
  this->NumberOfPVConsumers++;
  this->PVConsumers = new vtkPVSource* [this->NumberOfPVConsumers];
  for (int i = 0; i < (this->NumberOfPVConsumers-1); i++)
    {
    this->PVConsumers[i] = tmp[i];
    }
  this->PVConsumers[this->NumberOfPVConsumers-1] = c;
  // free old memory
  delete [] tmp;
}

void vtkPVData::RemovePVConsumer(vtkPVSource *c)
{
  // make sure it is already there
  if (!this->IsPVConsumer(c))
    {
    return;
    }
  // remove it from the list, reallocate memory
  vtkPVSource **tmp = this->PVConsumers;
  this->NumberOfPVConsumers--;
  this->PVConsumers = new vtkPVSource* [this->NumberOfPVConsumers];
  int cnt = 0;
  int i;
  for (i = 0; i <= this->NumberOfPVConsumers; i++)
    {
    if (tmp[i] != c)
      {
      this->PVConsumers[cnt] = tmp[i];
      cnt++;
      }
    }
  // free old memory
  delete [] tmp;
}

int vtkPVData::IsPVConsumer(vtkPVSource *c)
{
  int i;
  for (i = 0; i < this->NumberOfPVConsumers; i++)
    {
    if (this->PVConsumers[i] == c)
      {
      return 1;
      }
    }
  return 0;
}

vtkPVSource *vtkPVData::GetPVConsumer(int i)
{
  if (i >= this->NumberOfPVConsumers)
    {
    return 0;
    }
  return this->PVConsumers[i];
}

//----------------------------------------------------------------------------
// Data is expected to be updated.
void vtkPVData::GetBounds(float bounds[6])
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkMultiProcessController *controller = pvApp->GetController();
  float tmp[6];
  int id, num;
  
  if (this->VTKData == NULL)
    {
    bounds[0] = bounds[1] = bounds[2] = VTK_LARGE_FLOAT;
    bounds[3] = bounds[4] = bounds[5] = -VTK_LARGE_FLOAT;
    return;
    }

  this->VTKData->GetBounds(bounds);

  pvApp->BroadcastScript("$Application SendDataBounds %s", 
                         this->VTKDataTclName);
  
  num = controller->GetNumberOfProcesses();
  for (id = 1; id < num; ++id)
    {
    controller->Receive(tmp, 6, id, 1967);
    if (tmp[0] < bounds[0])
      {
      bounds[0] = tmp[0];
      }
    if (tmp[1] > bounds[1])
      {
      bounds[1] = tmp[1];
      }
    if (tmp[2] < bounds[2])
      {
      bounds[2] = tmp[2];
      }
    if (tmp[3] > bounds[3])
      {
      bounds[3] = tmp[3];
      }
    if (tmp[4] < bounds[4])
      {
      bounds[4] = tmp[4];
      }
    if (tmp[5] > bounds[5])
      {
      bounds[5] = tmp[5];
      }
    }
}

//----------------------------------------------------------------------------
// Data is expected to be updated.
int vtkPVData::GetNumberOfCells()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkMultiProcessController *controller = pvApp->GetController();
  int tmp = 0;
  int numCells, id, numProcs;
  
  if (this->VTKData == NULL)
    {
    return 0;
    }

  numCells = this->VTKData->GetNumberOfCells();

  pvApp->BroadcastScript("$Application SendDataNumberOfCells %s", 
                         this->VTKDataTclName);
  
  numProcs = controller->GetNumberOfProcesses();
  for (id = 1; id < numProcs; ++id)
    {
    controller->Receive(&tmp, 1, id, 1968);
    numCells += tmp;
    }
  return numCells;
}

//----------------------------------------------------------------------------
// Data is expected to be updated.
int vtkPVData::GetNumberOfPoints()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkMultiProcessController *controller = pvApp->GetController();
  int tmp = 0;
  int numPoints, id, numProcs;
  
  if (this->VTKData == NULL)
    {
    return 0;
    }

  numPoints = this->VTKData->GetNumberOfPoints();
  
  pvApp->BroadcastScript("$Application SendDataNumberOfPoints %s", 
                         this->VTKDataTclName);
  
  numProcs = controller->GetNumberOfProcesses();
  for (id = 1; id < numProcs; ++id)
    {
    controller->Receive(&tmp, 1, id, 1969);
    numPoints += tmp;
    }
  return numPoints;
}

//----------------------------------------------------------------------------
// WE DO NOT REFERENCE COUNT HERE BECAUSE OF CIRCULAR REFERENCES.
// THE SOURCE OWNS THE DATA.
void vtkPVData::SetPVSource(vtkPVSource *source)
{
  if (this->PVSource == source)
    {
    return;
    }
  this->Modified();

  this->PVSource = source;

  this->SetTraceReferenceObject(source);
  this->SetTraceReferenceCommand("GetPVOutput");
}

//----------------------------------------------------------------------------
void vtkPVData::Update()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  // The mapper has the assignment for this processor.
  pvApp->BroadcastScript("%s SetUpdateExtent [%s GetPiece] [%s GetNumberOfPieces]", 
                         this->VTKDataTclName, 
                         this->MapperTclName, this->MapperTclName);
  pvApp->BroadcastScript("%s Update", this->VTKDataTclName);
}


//----------------------------------------------------------------------------
void vtkPVData::InsertExtractPiecesIfNecessary()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if ( this->VTKData == NULL)
    {
    return;
    }
  this->VTKData->UpdateInformation();
  if (this->VTKData->GetMaximumNumberOfPieces() != 1)
    { // The source can already produce pieces.
    return;
    }
  
  // We are going to create the piece filter with a dummy tcl name,
  // setup the pipeline, and remove tcl's reference to the objects.
  // The vtkData object will be moved to the output of the piece filter.
  if (this->VTKData->IsA("vtkPolyData"))
    {
    // Transmit is more efficient, but has the possiblity of hanging.
    // It will hang if all procs do not  call execute.
    if (getenv("PV_LOCK_SAFE") != NULL)
      {
      pvApp->BroadcastSimpleScript("vtkExtractPolyDataPiece pvTemp");
      }
    else
      {
      pvApp->BroadcastSimpleScript("vtkTransmitPolyDataPiece pvTemp");
      }
    }
  else if (this->VTKData->IsA("vtkUnstructuredGrid"))
    {
    // Transmit is more efficient, but has the possiblity of hanging.
    // It will hang if all procs do not  call execute.
    if (getenv("PV_LOCK_SAFE") != NULL)
      {
      pvApp->BroadcastSimpleScript("vtkExtractUnstructuredGridPiece pvTemp");
      }
    else
      {
      pvApp->BroadcastSimpleScript("vtkTransmitUnstructuredGridPiece pvTemp");
      }
    }
  else
    {
    return;
    }

  if (this->PVSource->GetVTKSourceTclName())
    {
    pvApp->BroadcastSimpleScript("pvTemp SetInput [pvTemp GetOutput]");
    pvApp->BroadcastScript("pvTemp SetOutput %s", this->VTKDataTclName);
    pvApp->BroadcastScript("%s SetOutput [pvTemp GetInput]",
                           this->PVSource->GetVTKSourceTclName());
    pvApp->BroadcastSimpleScript("[pvTemp GetInput] ReleaseDataFlagOn");
    // Now delete Tcl's reference to the piece filter.
    pvApp->BroadcastSimpleScript("pvTemp Delete");
    return;
    }

  // No source, just create new static data.
  // This happens with data set reader.
  if (this->VTKData->IsA("vtkPolyData"))
    {
    pvApp->BroadcastSimpleScript("vtkPolyData pvTempOut");
    }
  else if (this->VTKData->IsA("vtkUnstructuredGrid"))
    {
    pvApp->BroadcastSimpleScript("vtkUnstructuredGrid pvTempOut");
    }

  pvApp->BroadcastScript("pvTempOut ShallowCopy %s", this->VTKDataTclName);
  pvApp->BroadcastSimpleScript("pvTemp SetInput pvTempOut");
  pvApp->BroadcastScript("pvTemp SetOutput %s", this->VTKDataTclName);


  // Now delete Tcl's reference to the piece filter.
  pvApp->BroadcastSimpleScript("pvTemp Delete");
  pvApp->BroadcastSimpleScript("pvTempOut Delete");
}





// ============= Use to be in vtkPVActorComposite ===================

//----------------------------------------------------------------------------
void vtkPVData::CreateProperties()
{
  if (this->PropertiesCreated)
    {
    return;
    }

  // Properties (aka Display page)

  if (this->GetPVSource()->GetHideDisplayPage())
    {
    // We use the parameters frame as a bogus parent.
    // This is not a problem since we never pack the
    // properties frame in this case.
    this->Properties->SetParent(
      this->GetPVSource()->GetParametersParent());
    }
  else
    {
    this->Properties->SetParent(
      this->GetPVSource()->GetNotebook()->GetFrame("Display"));
    }
  this->Properties->Create(this->Application, 1);

  // We are going to 'grid' most of it, so let's define some const

  int col_1_padx = 2;
  int button_pady = 1;
  int col_0_weight = 0;
  int col_1_weight = 1;
  float col_0_factor = 1.5;
  float col_1_factor = 1.0;

  // View frame


  this->ViewFrame->SetParent(this->Properties->GetFrame());
  this->ViewFrame->ShowHideFrameOn();
  this->ViewFrame->Create(this->Application);
  this->ViewFrame->SetLabel("View");
 
  this->VisibilityCheck->SetParent(this->ViewFrame->GetFrame());
  this->VisibilityCheck->Create(this->Application, "-text Data");
  this->Application->Script(
    "%s configure -command {%s VisibilityCheckCallback}",
    this->VisibilityCheck->GetWidgetName(),
    this->GetTclName());
  this->VisibilityCheck->SetState(1);

  this->ResetCameraButton->SetParent(this->ViewFrame->GetFrame());
  this->ResetCameraButton->Create(this->Application, "");
  this->ResetCameraButton->SetLabel("Set View to Data");
  this->ResetCameraButton->SetCommand(this, "CenterCamera");

  this->ScalarBarCheck->SetParent(this->ViewFrame->GetFrame());
  this->ScalarBarCheck->Create(this->Application, "-text {Scalar bar}");
  this->Application->Script(
    "%s configure -command {%s ScalarBarCheckCallback}",
    this->ScalarBarCheck->GetWidgetName(),
    this->GetTclName());

  this->EditColorMapButton->SetParent(this->ViewFrame->GetFrame());
  this->EditColorMapButton->Create(this->Application, "");
  this->EditColorMapButton->SetLabel("Edit Color Map...");
  this->EditColorMapButton->SetCommand(this,"EditColorMapCallback");
  
  this->CubeAxesCheck->SetParent(this->ViewFrame->GetFrame());
  this->CubeAxesCheck->Create(this->Application, "-text CubeAxes");
  this->CubeAxesCheck->SetCommand(this, "CubeAxesCheckCallback");

  this->Script("pack %s -fill x -expand t", this->ViewFrame->GetWidgetName());

  this->Script("grid %s %s -sticky wns",
               this->VisibilityCheck->GetWidgetName(),
               this->ResetCameraButton->GetWidgetName());

  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->ResetCameraButton->GetWidgetName(),
               col_1_padx, button_pady);

  this->Script("grid %s %s -sticky wns",
               this->ScalarBarCheck->GetWidgetName(),
               this->EditColorMapButton->GetWidgetName());

  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->EditColorMapButton->GetWidgetName(),
               col_1_padx, button_pady);

  this->Script("grid %s -sticky wns",
               this->CubeAxesCheck->GetWidgetName());
  // Color

  this->ColorFrame->SetParent(this->Properties->GetFrame());
  this->ColorFrame->ShowHideFrameOn();
  this->ColorFrame->Create(this->Application);
  this->ColorFrame->SetLabel("Color");

  this->ColorMenuLabel->SetParent(this->ColorFrame->GetFrame());
  this->ColorMenuLabel->Create(this->Application, "");
  this->ColorMenuLabel->SetLabel("Color by:");
  
  this->ColorMenu->SetParent(this->ColorFrame->GetFrame());
  this->ColorMenu->Create(this->Application, "");   

  this->ColorButton->SetParent(this->ColorFrame->GetFrame());
  this->ColorButton->Create(this->Application, "");
  this->ColorButton->SetText("Actor Color");
  this->ColorButton->SetCommand(this, "ChangeActorColor");
  
  this->Script("pack %s -fill x -expand t", this->ColorFrame->GetWidgetName());

  this->Script("grid %s %s -sticky wns",
               this->ColorMenuLabel->GetWidgetName(),
               this->ColorMenu->GetWidgetName());

  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->ColorMenu->GetWidgetName(),
               col_1_padx, button_pady);

  this->Script("grid %s -column 1 -sticky news -padx %d -pady %d",
               this->ColorButton->GetWidgetName(),
               col_1_padx, button_pady);

  this->Script("grid remove %s", this->ColorButton->GetWidgetName());

  // Display style

  this->DisplayStyleFrame->SetParent(this->Properties->GetFrame());
  this->DisplayStyleFrame->ShowHideFrameOn();
  this->DisplayStyleFrame->Create(this->Application);
  this->DisplayStyleFrame->SetLabel("Display Style");

  this->AmbientScale->SetParent(this->Properties->GetFrame());
  this->AmbientScale->Create(this->Application, "-showvalue 1");
  this->AmbientScale->DisplayLabel("Ambient Light");
  this->AmbientScale->SetRange(0.0, 1.0);
  this->AmbientScale->SetResolution(0.1);
  this->AmbientScale->SetCommand(this, "AmbientChanged");
  
  this->RepresentationMenuLabel->SetParent(
    this->DisplayStyleFrame->GetFrame());
  this->RepresentationMenuLabel->Create(this->Application, "");
  this->RepresentationMenuLabel->SetLabel("Representation:");

  this->RepresentationMenu->SetParent(this->DisplayStyleFrame->GetFrame());
  this->RepresentationMenu->Create(this->Application, "");
  this->RepresentationMenu->AddEntryWithCommand("Wireframe", this,
                                                "DrawWireframe");
  this->RepresentationMenu->AddEntryWithCommand("Surface", this,
                                                "DrawSurface");
  this->RepresentationMenu->AddEntryWithCommand("Points", this,
                                                "DrawPoints");
  this->RepresentationMenu->SetValue("Surface");

  this->InterpolationMenuLabel->SetParent(this->DisplayStyleFrame->GetFrame());
  this->InterpolationMenuLabel->Create(this->Application, "");
  this->InterpolationMenuLabel->SetLabel("Interpolation:");

  this->InterpolationMenu->SetParent(this->DisplayStyleFrame->GetFrame());
  this->InterpolationMenu->Create(this->Application, "");
  this->InterpolationMenu->AddEntryWithCommand("Flat", this,
                                               "SetInterpolationToFlat");
  this->InterpolationMenu->AddEntryWithCommand("Gouraud", this,
                                               "SetInterpolationToGouraud");
  this->InterpolationMenu->SetValue("Gouraud");

  this->PointSizeLabel->SetParent(this->DisplayStyleFrame->GetFrame());
  this->PointSizeLabel->Create(this->Application, "");
  this->PointSizeLabel->SetLabel("Point size:");
  
  this->PointSizeScale->SetParent(this->DisplayStyleFrame->GetFrame());
  this->PointSizeScale->Create(this->Application, "");
  this->PointSizeScale->SetRange(1, 5);
  this->PointSizeScale->SetResolution(1);
  this->PointSizeScale->SetValue(1);
  this->PointSizeScale->DisplayEntry();
  this->PointSizeScale->DisplayEntryAndLabelOnTopOff();
  this->Script("%s configure -width 5", 
               this->PointSizeScale->GetEntry()->GetWidgetName());
  this->PointSizeScale->SetCommand(this, "ChangePointSize");

  this->LineWidthLabel->SetParent(this->DisplayStyleFrame->GetFrame());
  this->LineWidthLabel->Create(this->Application, "");
  this->LineWidthLabel->SetLabel("Line width:");
  
  this->LineWidthScale->SetParent(this->DisplayStyleFrame->GetFrame());
  this->LineWidthScale->Create(this->Application, "");
  this->LineWidthScale->SetRange(1, 5);
  this->LineWidthScale->SetResolution(1);
  this->LineWidthScale->SetValue(1);
  this->LineWidthScale->DisplayEntry();
  this->LineWidthScale->DisplayEntryAndLabelOnTopOff();
  this->Script("%s configure -width 5", 
               this->LineWidthScale->GetEntry()->GetWidgetName());
  this->LineWidthScale->SetCommand(this, "ChangeLineWidth");

  this->Script("pack %s -fill x", this->DisplayStyleFrame->GetWidgetName());
  
  this->Script("grid %s %s -sticky wns",
               this->RepresentationMenuLabel->GetWidgetName(),
               this->RepresentationMenu->GetWidgetName());

  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->RepresentationMenu->GetWidgetName(), 
               col_1_padx, button_pady);

  this->Script("grid %s %s -sticky wns",
               this->InterpolationMenuLabel->GetWidgetName(),
               this->InterpolationMenu->GetWidgetName());

  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->InterpolationMenu->GetWidgetName(),
               col_1_padx, button_pady);
  
  this->Script("grid %s %s -sticky wns",
               this->PointSizeLabel->GetWidgetName(),
               this->PointSizeScale->GetWidgetName());

  this->Script("grid %s -sticky news -padx %d",
               this->PointSizeScale->GetWidgetName(), col_1_padx);

  this->Script("grid %s %s -sticky wns",
               this->LineWidthLabel->GetWidgetName(),
               this->LineWidthScale->GetWidgetName());

  this->Script("grid %s -sticky news -padx %d",
               this->LineWidthScale->GetWidgetName(), col_1_padx);

  // Now synchronize all those grids to have them aligned

  const char *widgets[3];
  widgets[0] = this->ViewFrame->GetFrame()->GetWidgetName();
  widgets[1] = this->ColorFrame->GetFrame()->GetWidgetName();
  widgets[2] = this->DisplayStyleFrame->GetFrame()->GetWidgetName();

  int weights[2];
  weights[0] = col_0_weight;
  weights[1] = col_1_weight;

  float factors[2];
  factors[0] = col_0_factor;
  factors[1] = col_1_factor;

  vtkKWTkUtilities::SynchroniseGridsColumnMinimumSize(
    this->GetPVApplication()->GetMainInterp(), 3, widgets, factors, weights);
  
  // Actor Control

  this->ActorControlFrame->SetParent(this->Properties->GetFrame());
  this->ActorControlFrame->ShowHideFrameOn();
  this->ActorControlFrame->Create(this->Application);
  this->ActorControlFrame->SetLabel("Actor Control");

  this->TranslateLabel->SetParent(this->ActorControlFrame->GetFrame());
  this->TranslateLabel->Create(this->Application, 0);
  this->TranslateLabel->SetLabel("Translate:");

  this->ScaleLabel->SetParent(this->ActorControlFrame->GetFrame());
  this->ScaleLabel->Create(this->Application, 0);
  this->ScaleLabel->SetLabel("Scale:");

  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->TranslateEntry[cc]->SetParent(this->ActorControlFrame->GetFrame());
    this->TranslateEntry[cc]->Create(this->Application, 0);
    this->TranslateEntry[cc]->SetValue(0, 4);
    this->Script("bind %s <Key-Return> { %s SetActorTranslate }",
                 this->TranslateEntry[cc]->GetWidgetName(),
                 this->GetTclName());

    this->ScaleEntry[cc]->SetParent(this->ActorControlFrame->GetFrame());
    this->ScaleEntry[cc]->Create(this->Application, 0);
    this->ScaleEntry[cc]->SetValue(1, 4);
    this->Script("bind %s <Key-Return> { %s SetActorScale }",
                 this->ScaleEntry[cc]->GetWidgetName(),
                 this->GetTclName());
    }

  this->OpacityLabel->SetParent(this->ActorControlFrame->GetFrame());
  this->OpacityLabel->Create(this->Application, 0);
  this->OpacityLabel->SetLabel("Opacity:");

  this->Opacity->SetParent(this->ActorControlFrame->GetFrame());
  this->Opacity->Create(this->Application, 0);
  this->Opacity->SetRange(0, 1);
  this->Opacity->SetResolution(0.1);
  this->Opacity->SetValue(1);
  this->Opacity->DisplayEntry();
  this->Opacity->DisplayEntryAndLabelOnTopOff();
  this->Script("%s configure -width 5", 
               this->Opacity->GetEntry()->GetWidgetName());
  this->Opacity->SetCommand(this, "OpacityChangedCallback");

  this->Script("grid %s %s %s %s -sticky news",
               this->TranslateLabel->GetWidgetName(),
               this->TranslateEntry[0]->GetWidgetName(),
               this->TranslateEntry[1]->GetWidgetName(),
               this->TranslateEntry[2]->GetWidgetName());

  this->Script("grid %s -sticky nws",
               this->TranslateLabel->GetWidgetName());

  this->Script("grid %s %s %s %s -sticky news",
               this->ScaleLabel->GetWidgetName(),
               this->ScaleEntry[0]->GetWidgetName(),
               this->ScaleEntry[1]->GetWidgetName(),
               this->ScaleEntry[2]->GetWidgetName());

  this->Script("grid %s -sticky nws",
               this->ScaleLabel->GetWidgetName());

  this->Script("grid %s %s - -  -sticky news",
               this->OpacityLabel->GetWidgetName(),
               this->Opacity->GetWidgetName());

  this->Script("grid %s -sticky nws",
               this->OpacityLabel->GetWidgetName());

  this->Script("grid columnconfigure %s 0 -weight 0", 
               this->ActorControlFrame->GetFrame()->GetWidgetName());

  this->Script("grid columnconfigure %s 1 -weight 2", 
               this->ActorControlFrame->GetFrame()->GetWidgetName());

  this->Script("grid columnconfigure %s 2 -weight 2", 
               this->ActorControlFrame->GetFrame()->GetWidgetName());

  this->Script("grid columnconfigure %s 3 -weight 2", 
               this->ActorControlFrame->GetFrame()->GetWidgetName());

  this->Script("pack %s -fill x -expand yes -side top", 
               this->ActorControlFrame->GetWidgetName());

  if (!this->GetPVSource()->GetHideDisplayPage())
    {
    this->Script("pack %s -fill both -expand yes -side top",
                 this->Properties->GetWidgetName());
    }

  // Information page

  if (this->GetPVSource()->GetHideInformationPage())
    {
    // We use the parameters frame as a bogus parent.
    // This is not a problem since we never pack the
    // properties frame in this case.
    this->InformationFrame->SetParent(
      this->GetPVSource()->GetParametersParent());
    }
  else
    {
    this->InformationFrame->SetParent(
      this->GetPVSource()->GetNotebook()->GetFrame("Information"));
    }
  this->InformationFrame->Create(this->Application, 1);

  this->StatsFrame->SetParent(this->InformationFrame->GetFrame());
  this->StatsFrame->ShowHideFrameOn();
  this->StatsFrame->Create(this->Application);
  this->StatsFrame->SetLabel("Statistics");

  this->NumCellsLabel->SetParent(this->StatsFrame->GetFrame());
  this->NumCellsLabel->Create(this->Application, "");

  this->NumPointsLabel->SetParent(this->StatsFrame->GetFrame());
  this->NumPointsLabel->Create(this->Application, "");
  
  this->BoundsDisplay->SetParent(this->InformationFrame->GetFrame());
  this->BoundsDisplay->Create(this->Application);
  
  this->Script("pack %s -fill x", this->StatsFrame->GetWidgetName());

  this->Script("pack %s %s -side top -anchor nw",
               this->NumCellsLabel->GetWidgetName(),
               this->NumPointsLabel->GetWidgetName());

  this->Script("pack %s -fill x -expand t", 
               this->BoundsDisplay->GetWidgetName());

  if (!this->GetPVSource()->GetHideInformationPage())
    {
    this->Script("pack %s -fill both -expand yes -side top",
                 this->InformationFrame->GetWidgetName());
    }

  // OK, all done

  this->PropertiesCreated = 1;
}

//----------------------------------------------------------------------------
void vtkPVData::EditColorMapCallback()
{
  if (this->PVColorMap == NULL)
    {
    // We could get the color map from the window,
    // but it must already be set for this button to be visible.
    vtkErrorMacro("Expecting a color map.");
    return;
    }
  this->Script("pack forget [pack slaves %s]",
          this->GetPVRenderView()->GetPropertiesParent()->GetWidgetName());
  this->Script("pack %s -side top -fill both -expand t",
          this->PVColorMap->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVData::UpdateProperties()
{
  vtkPVSource* source = this->GetPVSource();

  if (!source)
    {
    return;
    }

  char tmp[350], cmd[1024];
  float bounds[6];
  int i, j, numArrays, numComps;
  vtkFieldData *fieldData;
  vtkPVApplication *pvApp = this->GetPVApplication();  
  vtkDataArray *array;
  char *currentColorBy;
  int currentColorByFound = 0;
  vtkPVWindow *window;

  // Set LOD based on point threshold of render view.
  // This is imperfect because the critera is based on
  // the data, not the rendered geometry.
  // It the solution communication in the PVLodActor?
  int numberOfPoints = this->GetNumberOfPoints();
  if (numberOfPoints > this->GetPVRenderView()->GetLODThreshold())
    {
    pvApp->BroadcastScript("%s EnableLODOn", this->PropTclName);
    }
  else
    {
    pvApp->BroadcastScript("%s EnableLODOff", this->PropTclName);
    }

  if (this->PVColorMap)
    {
    if (this->PVColorMap->GetScalarBarVisibility())
      {
      this->ScalarBarCheck->SetState(1);
      }
    else
      {
      this->ScalarBarCheck->SetState(0);
      }
    }

  if (this->UpdateTime > this->GetVTKData()->GetMTime())
    {
    return;
    }
  this->UpdateTime.Modified();

  if (source->GetHideDisplayPage())
    {
    return;
    }

  window = this->GetPVApplication()->GetMainWindow();

  // Update and time the filter.
  char *str = new char[strlen(this->GetVTKDataTclName()) + 80];
  sprintf(str, "Accept: %s", this->GetVTKDataTclName());
  vtkTimerLog::MarkStartEvent(str);
  pvApp->BroadcastScript("%s ForceUpdate", this->UpdateSuppressorTclName);
  pvApp->BroadcastScript("%s Update", this->MapperTclName);
  // Get bounds to time completion (not just triggering) of update.
  this->GetBounds(bounds);
  vtkTimerLog::MarkEndEvent(str);
  delete [] str;

  // Time creation of the LOD
  vtkTimerLog::MarkStartEvent("Create LOD");
  pvApp->BroadcastScript("%s ForceUpdate", this->LODUpdateSuppressorTclName);
  pvApp->BroadcastScript("%s Update", this->LODMapperTclName);
  // Get bounds to time completion (not just triggering) of update.
  this->GetBounds(bounds);
  vtkTimerLog::MarkEndEvent("Create LOD");

  sprintf(tmp, "Number of cells: %d", 
          this->GetNumberOfCells());
  this->NumCellsLabel->SetLabel(tmp);
  sprintf(tmp, "Number of points: %d", numberOfPoints);
  this->NumPointsLabel->SetLabel(tmp);
  
  this->BoundsDisplay->SetBounds(bounds);
  if (this->CubeAxesTclName)
    {  
    this->Script("%s SetBounds %f %f %f %f %f %f",
                 this->CubeAxesTclName, bounds[0], bounds[1], bounds[2],
                 bounds[3], bounds[4], bounds[5]);
    }
  // This doesn't need to be set currently because we're not packing
  // the AmbientScale.
  //  this->AmbientScale->SetValue(this->Property->GetAmbient());


  // Temporary fix because empty VTK objects do not have arrays.
  // This will create arrays if they exist on other processes.
  pvApp->CompleteArrays(this->Mapper, this->MapperTclName);

  currentColorBy = this->ColorMenu->GetValue();
  this->ColorMenu->ClearEntries();
  this->ColorMenu->AddEntryWithCommand("Property",
                                       this, "ColorByProperty");
  fieldData = this->Mapper->GetInput()->GetPointData();
  if (fieldData)
    {
    numArrays = fieldData->GetNumberOfArrays();
    for (i = 0; i < numArrays; i++)
      {
      if (fieldData->GetArrayName(i))
        {
        array = fieldData->GetArray(i);
        numComps = array->GetNumberOfComponents();
        for (j = 0; j < numComps; ++j)
          {
          sprintf(cmd, "ColorByPointFieldComponent {%s} %d",
                  fieldData->GetArrayName(i), j);
          if (numComps == 1)
            {
            sprintf(tmp, "Point %s", fieldData->GetArrayName(i));
            }
          else
            {
            sprintf(tmp, "Point %s %d", fieldData->GetArrayName(i), j);
            }
          this->ColorMenu->AddEntryWithCommand(tmp, this, cmd);
          if (strcmp(tmp, currentColorBy) == 0)
            {
            currentColorByFound = 1;
            }
          }
        }
      }
    }
  
  fieldData = this->Mapper->GetInput()->GetCellData();
  if (fieldData)
    {
    numArrays = fieldData->GetNumberOfArrays();
    for (i = 0; i < numArrays; i++)
      {
      if (fieldData->GetArrayName(i))
        {
        array = fieldData->GetArray(i);
        numComps = array->GetNumberOfComponents();
        for (j = 0; j < numComps; ++j)
          {
          sprintf(cmd, "ColorByCellFieldComponent {%s} %d",
                  fieldData->GetArrayName(i), j);
          if (numComps == 1)
            {
            sprintf(tmp, "Cell %s", fieldData->GetArrayName(i));
            } 
          else
            {
            sprintf(tmp, "Cell %s %d", fieldData->GetArrayName(i), j);
            }
          this->ColorMenu->AddEntryWithCommand(tmp, this, cmd);
          if (strcmp(tmp, currentColorBy) == 0)
            {
            currentColorByFound = 1;
            }
          }
        }
      }
    }
  // If the current array we are coloring by has disappeared,
  // then default back to the property.
  if ( ! currentColorByFound)
    {
    this->ColorMenu->SetValue("Property");
    this->ColorByPropertyInternal();
    }

}

//----------------------------------------------------------------------------
void vtkPVData::SetActorColor(float r, float g, float b)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  pvApp->BroadcastScript("%s SetColor %f %f %f", 
                         this->PropertyTclName, r, g, b);

  // Add a bit of specular when just coloring by property.
  pvApp->BroadcastScript("%s SetSpecular 0.1", 
                         this->PropertyTclName);

  pvApp->BroadcastScript("%s SetSpecularPower 100.0", 
                         this->PropertyTclName);

  pvApp->BroadcastScript("%s SetSpecularColor 1.0 1.0 1.0", 
                         this->PropertyTclName);
}  

//----------------------------------------------------------------------------
void vtkPVData::ChangeActorColor(float r, float g, float b)
{
  if (this->Mapper->GetScalarVisibility())
    {
    return;
    }

  this->AddTraceEntry("$kw(%s) ChangeActorColor %f %f %f",
                      this->GetTclName(), r, g, b);

  this->SetActorColor(r, g, b);
  this->ColorButton->SetColor(r, g, b);

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}



//----------------------------------------------------------------------------
void vtkPVData::SetColorRange(float min, float max)
{
  if (this->PVColorMap == NULL)
    {
    vtkErrorMacro("Color map is missing.");
    }

  this->PVColorMap->SetScalarRange(min, max);
}

//----------------------------------------------------------------------------
// Hack for now.
void vtkPVData::SetColorRangeInternal(float min, float max)
{
  if (this->PVColorMap == NULL)
    {
    vtkErrorMacro("Color map is missing.");
    }

  this->PVColorMap->SetScalarRangeInternal(min, max);
}

//----------------------------------------------------------------------------
void vtkPVData::ResetColorRange()
{
  if (this->PVColorMap == NULL)
    {
    vtkErrorMacro("Color map is missing.");
    return;
    }

  this->PVColorMap->ResetScalarRange();
  this->UpdateProperties();
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::ColorByProperty()
{
  this->AddTraceEntry("$kw(%s) ColorByProperty", this->GetTclName());
  this->ColorMenu->SetValue("Property");
  this->ColorByPropertyInternal();
}

//----------------------------------------------------------------------------
void vtkPVData::ColorByPropertyInternal()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  pvApp->BroadcastScript("%s ScalarVisibilityOff", this->MapperTclName);
  pvApp->BroadcastScript("%s ScalarVisibilityOff", this->LODMapperTclName);

  float *color = this->ColorButton->GetColor();
  this->SetActorColor(color[0], color[1], color[2]);

  this->SetPVColorMap(NULL);

  this->Script("grid remove %s %s",
               this->ScalarBarCheck->GetWidgetName(),
               this->EditColorMapButton->GetWidgetName());

  this->Script("grid %s", this->ColorButton->GetWidgetName());

  if (this->GetPVRenderView())
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}


//----------------------------------------------------------------------------
void vtkPVData::ColorByPointFieldComponent(const char *name, int comp)
{
  this->AddTraceEntry("$kw(%s) ColorByPointFieldComponent {%s} %d", 
                      this->GetTclName(), name, comp);
  this->ColorByPointFieldComponentInternal(name, comp);
}

//----------------------------------------------------------------------------
void vtkPVData::ColorByPointFieldComponentInternal(const char *name, 
                                                             int comp)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  // I would like to make this an argument, but not right now.
  int numComps;
  vtkDataArray *a = this->VTKData->GetPointData()->GetArray(name);
  if (a == NULL)
    {
    vtkErrorMacro("Could not find array.");
    return;
    }
  numComps = a->GetNumberOfComponents();


  this->SetPVColorMap(pvApp->GetMainWindow()->GetPVColorMap(name));
  if (this->PVColorMap == NULL)
    {
    vtkErrorMacro("Could not get the color map.");
    return;
    }

  // Turn off the specualr so it does not interfere with data.
  pvApp->BroadcastScript("%s SetSpecular 0.0", this->PropertyTclName);

  pvApp->BroadcastScript("%s SetLookupTable %s", this->MapperTclName,
                         this->PVColorMap->GetLookupTableTclName());
  pvApp->BroadcastScript("%s ScalarVisibilityOn", this->MapperTclName);
  pvApp->BroadcastScript("%s SetScalarModeToUsePointFieldData",
                         this->MapperTclName);
  pvApp->BroadcastScript("%s SelectColorArray {%s}",
                         this->MapperTclName, name);
  this->PVColorMap->SetVectorComponent(comp, numComps);

  pvApp->BroadcastScript("%s SetLookupTable %s", this->LODMapperTclName,
                         this->PVColorMap->GetLookupTableTclName());
  pvApp->BroadcastScript("%s ScalarVisibilityOn", this->LODMapperTclName);
  pvApp->BroadcastScript("%s SetScalarModeToUsePointFieldData",
                         this->LODMapperTclName);
  pvApp->BroadcastScript("%s SelectColorArray {%s}",
                         this->LODMapperTclName, name);
  
  this->Script("grid remove %s", this->ColorButton->GetWidgetName());

  this->Script("grid %s %s",
               this->ScalarBarCheck->GetWidgetName(),
               this->EditColorMapButton->GetWidgetName());

  // Synchronize the UI with the new color map.
  this->UpdateProperties();
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::ColorByCellFieldComponent(const char *name, int comp)
{
  this->AddTraceEntry("$kw(%s) ColorByCellFieldComponent {%s} %d", 
                      this->GetTclName(), name, comp);
  this->ColorByCellFieldComponentInternal(name, comp);
}

//----------------------------------------------------------------------------
void vtkPVData::ColorByCellFieldComponentInternal(const char *name, 
                                                            int comp)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  // I would like to make this an argument, but not right now.
  int numComps;
  vtkDataArray *a = this->VTKData->GetCellData()->GetArray(name);
  if (a == NULL)
    {
    //vtkErrorMacro("Could not find array.");
    return;
    }
  numComps = a->GetNumberOfComponents();


  this->SetPVColorMap(pvApp->GetMainWindow()->GetPVColorMap(name));
  if (this->PVColorMap == NULL)
    {
    vtkErrorMacro("Could not get the color map.");
    return;
    }

  // Turn off the specualr so it does not interfere with data.
  pvApp->BroadcastScript("%s SetSpecular 0.0", this->PropertyTclName);

  pvApp->BroadcastScript("%s SetLookupTable %s", this->MapperTclName,
                         this->PVColorMap->GetLookupTableTclName());
  pvApp->BroadcastScript("%s ScalarVisibilityOn", this->MapperTclName);
  pvApp->BroadcastScript("%s SetScalarModeToUseCellFieldData",
                         this->MapperTclName);
  pvApp->BroadcastScript("%s SelectColorArray {%s}",
                         this->MapperTclName, name);
  this->PVColorMap->SetVectorComponent(comp, numComps);

  pvApp->BroadcastScript("%s SetLookupTable %s", this->LODMapperTclName,
                         this->PVColorMap->GetLookupTableTclName());
  pvApp->BroadcastScript("%s ScalarVisibilityOn", this->LODMapperTclName);
  pvApp->BroadcastScript("%s SetScalarModeToUseCellFieldData",
                         this->LODMapperTclName);
  pvApp->BroadcastScript("%s SelectColorArray {%s}",
                         this->LODMapperTclName, name);
  
  this->Script("grid remove %s", this->ColorButton->GetWidgetName());

  this->Script("grid %s %s",
               this->ScalarBarCheck->GetWidgetName(),
               this->EditColorMapButton->GetWidgetName());

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::SetRepresentation(const char* repr)
{
  if ( vtkString::Equals(repr, "Wireframe") )
    {
    this->DrawWireframe();
    }
  else if ( vtkString::Equals(repr, "Surface") )
    {
    this->DrawSurface();
    }
  else if ( vtkString::Equals(repr, "Points") )
    {
    this->DrawPoints();
    }
  else
    {
    vtkErrorMacro("Don't know the representation: " << repr);
    this->DrawSurface();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::DrawWireframe()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  this->AddTraceEntry("$kw(%s) DrawWireframe", this->GetTclName());
  this->RepresentationMenu->SetValue("Wireframe");

  if (this->PropertyTclName)
    {
    if (this->PreviousWasSolid)
      {
      this->PreviousAmbient = this->Property->GetAmbient();
      this->PreviousDiffuse = this->Property->GetDiffuse();
      this->PreviousSpecular = this->Property->GetSpecular();
      }
    this->PreviousWasSolid = 0;
    pvApp->BroadcastScript("%s SetAmbient 1", this->PropertyTclName);
    pvApp->BroadcastScript("%s SetDiffuse 0", this->PropertyTclName);
    pvApp->BroadcastScript("%s SetSpecular 0", this->PropertyTclName);
    pvApp->BroadcastScript("%s SetRepresentationToWireframe",
                           this->PropertyTclName);
    }
  
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::DrawPoints()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  this->AddTraceEntry("$kw(%s) DrawPoints", this->GetTclName());
  this->RepresentationMenu->SetValue("Points");

  if (this->PropertyTclName)
    {
    if (this->PreviousWasSolid)
      {
      this->PreviousAmbient = this->Property->GetAmbient();
      this->PreviousDiffuse = this->Property->GetDiffuse();
      this->PreviousSpecular = this->Property->GetSpecular();
      }
    this->PreviousWasSolid = 0;
    pvApp->BroadcastScript("%s SetAmbient 1", this->PropertyTclName);
    pvApp->BroadcastScript("%s SetDiffuse 0", this->PropertyTclName);
    pvApp->BroadcastScript("%s SetSpecular 0", this->PropertyTclName);
    pvApp->BroadcastScript("%s SetRepresentationToPoints",
                           this->PropertyTclName);
    }
  
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::DrawSurface()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  this->AddTraceEntry("$kw(%s) DrawSurface", this->GetTclName());
  this->RepresentationMenu->SetValue("Surface");

  if (this->PropertyTclName)
    {
    if (!this->PreviousWasSolid)
      {
      pvApp->BroadcastScript("%s SetAmbient %f",
                             this->PropertyTclName, this->PreviousAmbient);
      pvApp->BroadcastScript("%s SetDiffuse %f",
                             this->PropertyTclName, this->PreviousDiffuse);
      pvApp->BroadcastScript("%s SetSpecular %f",
                             this->PropertyTclName, this->PreviousSpecular);
      pvApp->BroadcastScript("%s SetRepresentationToSurface",
                             this->PropertyTclName);
      }
    this->PreviousWasSolid = 1;
    }
  
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::SetInterpolation(const char* repr)
{
  if ( vtkString::Equals(repr, "Flat") )
    {
    this->SetInterpolationToFlat();
    }
  else if ( vtkString::Equals(repr, "Gouraud") )
    {
    this->SetInterpolationToGouraud();
    }
  else
    {
    vtkErrorMacro("Don't know the interpolation: " << repr);
    this->DrawSurface();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::SetInterpolationToFlat()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  this->AddTraceEntry("$kw(%s) SetInterpolationToFlat", 
                      this->GetTclName());
  this->InterpolationMenu->SetValue("Flat");

  if (this->PropertyTclName)
    {
    pvApp->BroadcastScript("%s SetInterpolationToFlat",
                           this->PropertyTclName);
    }
  
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}


//----------------------------------------------------------------------------
void vtkPVData::SetInterpolationToGouraud()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  this->AddTraceEntry("$kw(%s) SetInterpolationToGouraud", 
                      this->GetTclName());
  this->InterpolationMenu->SetValue("Gouraud");

  if (this->PropertyTclName)
    {
    pvApp->BroadcastScript("%s SetInterpolationToGouraud",
                           this->PropertyTclName);
    }
  
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}



//----------------------------------------------------------------------------
void vtkPVData::AmbientChanged()
{
  // This doesn't currently need to do anything since we aren't actually
  //packing the ambient scale.
/*  vtkPVApplication *pvApp = this->GetPVApplication();
  float ambient = this->AmbientScale->GetValue();
  
  //pvApp->BroadcastScript("%s SetAmbient %f", this->GetTclName(), ambient);

  
  this->SetAmbient(ambient);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
*/
}

//----------------------------------------------------------------------------
void vtkPVData::SetAmbient(float ambient)
{
  this->Property->SetAmbient(ambient);
}


//----------------------------------------------------------------------------
void vtkPVData::Initialize()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  float bounds[6];
  vtkDataArray *array;
  char *tclName;
  char newTclName[100];
  
  pvApp->BroadcastScript("%s SetInput %s",
                               this->GeometryTclName,
                               this->GetVTKDataTclName());
  

  vtkDebugMacro( << "Initialize --------")
  this->UpdateProperties();
  
  this->GetBounds(bounds);
  if (bounds[0] < 0)
    {
    bounds[0] += 0.05 * (bounds[1] - bounds[0]);
    if (bounds[1] > 0)
      {
      bounds[1] += 0.05 * (bounds[1] - bounds[0]);
      }
    else
      {
      bounds[1] -= 0.05 * (bounds[1] - bounds[0]);
      }
    }
  else
    {
    bounds[0] -= 0.05 * (bounds[1] - bounds[0]);
    bounds[1] += 0.05 * (bounds[1] - bounds[0]);
    }
  if (bounds[2] < 0)
    {
    bounds[2] += 0.05 * (bounds[3] - bounds[2]);
    if (bounds[3] > 0)
      {
      bounds[3] += 0.05 * (bounds[3] - bounds[2]);
      }
    else
      {
      bounds[3] -= 0.05 * (bounds[3] - bounds[2]);
      }
    }
  else
    {
    bounds[2] -= 0.05 * (bounds[3] - bounds[2]);
    bounds[3] += 0.05 * (bounds[3] - bounds[2]);
    }
  if (bounds[4] < 0)
    {
    bounds[4] += 0.05 * (bounds[5] - bounds[4]);
    if (bounds[5] > 0)
      {
      bounds[5] += 0.05 * (bounds[5] - bounds[4]);
      }
    else
      {
      bounds[5] -= 0.05 * (bounds[5] - bounds[4]);
      }
    }
  else
    {
    bounds[4] -= 0.05 * (bounds[5] - bounds[4]);
    bounds[5] += 0.05 * (bounds[5] - bounds[4]);
    }
  
  tclName = this->GetPVRenderView()->GetRendererTclName();
  
  sprintf(newTclName, "CubeAxes%d", this->InstanceCount);
  this->SetCubeAxesTclName(newTclName);
  this->Script("vtkCubeAxesActor2D %s", this->GetCubeAxesTclName());
  this->Script("%s SetFlyModeToOuterEdges", this->GetCubeAxesTclName());
  this->Script("[%s GetProperty] SetColor 1 1 1",
               this->GetCubeAxesTclName());
  
  this->Script("%s SetBounds %f %f %f %f %f %f",
               this->GetCubeAxesTclName(), bounds[0], bounds[1], bounds[2],
               bounds[3], bounds[4], bounds[5]);
  this->Script("%s SetCamera [%s GetActiveCamera]",
               this->GetCubeAxesTclName(), tclName);
  this->Script("%s SetInertia 20", this->GetCubeAxesTclName());
  
  if ((array =
       this->Mapper->GetInput()->GetPointData()->GetScalars()) &&
      (array->GetName()))
    {
    char *arrayName = (char*)array->GetName();
    char tmp[350];
    sprintf(tmp, "Point %s", arrayName);
    // Order is important here because Internal method check for consistent
    // menu value and arrays.
    this->ColorMenu->SetValue(tmp);
    this->ColorByPointFieldComponentInternal(arrayName, 0);
    }
  else if ((array =
            this->Mapper->GetInput()->GetCellData()->GetScalars()) &&
            (array->GetName()))
    {
    char *arrayName = (char*)array->GetName();
    char tmp[350];
    sprintf(tmp, "Cell %s", arrayName);
    // Order is important here because Internal method check for consistent
    // menu value and arrays.
    this->ColorMenu->SetValue(tmp);
    this->ColorByCellFieldComponentInternal(arrayName, 0);
    }
  else
    {
    this->ColorByPropertyInternal();
    this->ColorMenu->SetValue("Property");
    }
  
  if (this->GetVTKData()->IsA("vtkPolyData") ||
      this->GetVTKData()->IsA("vtkUnstructuredGrid"))
    {
    pvApp->BroadcastScript("%s SetUseStrips %d", this->GeometryTclName,
                           this->GetPVRenderView()->GetTriangleStripsCheck()->GetState());
    }
  pvApp->BroadcastScript("%s SetImmediateModeRendering %d",
                         this->MapperTclName,
                         this->GetPVRenderView()->GetImmediateModeCheck()->GetState());
  pvApp->BroadcastScript("%s SetImmediateModeRendering %d", 
                         this->LODMapperTclName,
                         this->GetPVRenderView()->GetImmediateModeCheck()->GetState());
}


//----------------------------------------------------------------------------
void vtkPVData::CenterCamera()
{
  float bounds[6];
  vtkPVApplication *pvApp = this->GetPVApplication();
  char* tclName;
  
  tclName = this->GetPVRenderView()->GetRendererTclName();
  this->GetBounds(bounds);
  pvApp->BroadcastScript("%s ResetCamera %f %f %f %f %f %f",
                         tclName, bounds[0], bounds[1], bounds[2],
                         bounds[3], bounds[4], bounds[5]);


  pvApp->BroadcastScript("%s ResetCameraClippingRange", tclName);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::VisibilityCheckCallback()
{
  this->SetVisibility(this->VisibilityCheck->GetState());
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::SetVisibility(int v)
{
  this->AddTraceEntry("$kw(%s) SetVisibility %d", this->GetTclName(), v);
  this->SetVisibilityInternal(v);
}
  
//----------------------------------------------------------------------------
void vtkPVData::SetVisibilityInternal(int v)
{
  vtkPVApplication *pvApp;
  pvApp = (vtkPVApplication*)(this->Application);


  if (this->VisibilityCheck->GetApplication())
    {
    if (this->VisibilityCheck->GetState() != v)
      {
      this->VisibilityCheck->SetState(v);
      }
    }

  if (this->PropTclName)
    {
    pvApp->BroadcastScript("%s SetVisibility %d", this->PropTclName, v);
    }
  if (v == 0 && this->GeometryTclName)
    {
    pvApp->BroadcastScript("[%s GetInput] ReleaseData", this->MapperTclName);
    }
}
  
//----------------------------------------------------------------------------
int vtkPVData::GetVisibility()
{
  return this->VisibilityCheck->GetState();
}

//----------------------------------------------------------------------------
vtkPVRenderView* vtkPVData::GetPVRenderView()
{
  return this->PVRenderView;
}

//----------------------------------------------------------------------------
void vtkPVData::SetPVRenderView(vtkPVRenderView* view)
{
  if (this->PVRenderView == view)
    {
    return;
    }
  if (view)
    {
    view->Register(this);
    this->SetLODResolution(view->GetLODResolution());
    this->SetCollectThreshold(view->GetCollectThreshold());
    }
  if (this->PVRenderView)
    {
    this->PVRenderView->UnRegister(this);
    }
  this->PVRenderView = view;
}


//----------------------------------------------------------------------------
vtkPVApplication* vtkPVData::GetPVApplication()
{
  if (this->Application == NULL)
    {
    return NULL;
    }
  
  if (this->Application->IsA("vtkPVApplication"))
    {  
    return (vtkPVApplication*)(this->Application);
    }
  else
    {
    vtkErrorMacro("Bad typecast");
    return NULL;
    } 
}

//----------------------------------------------------------------------------
/*
void vtkPVData::SetMode(int mode)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp == NULL)
    {
    vtkErrorMacro("I cannot set the mode with no application.");
    }
  
  this->Mode = mode;

  if (this->PVData == NULL)
    {
    return;
    }

  pvApp->BroadcastScript("%s SetInput %s", this->GeometryTclName, 
                                           this->PVData->GetVTKDataTclName());
  if (mode == VTK_PV_ACTOR_COMPOSITE_POLY_DATA_MODE)
    {
    pvApp->BroadcastScript("%s SetModeToSurface", this->GeometryTclName);
    }
  else if (mode == VTK_PV_ACTOR_COMPOSITE_IMAGE_OUTLINE_MODE)
    {
    pvApp->BroadcastScript("%s SetModeToImageOutline", this->GeometryTclName); 
    }
  else if (mode == VTK_PV_ACTOR_COMPOSITE_DATA_SET_MODE)
    {
    pvApp->BroadcastScript("%s SetModeToSurface", this->GeometryTclName);
    }  
}
*/

//----------------------------------------------------------------------------
void vtkPVData::SetScalarBarVisibility(int val)
{
  if (this->PVColorMap)
    {
    this->PVColorMap->SetScalarBarVisibility(val);
    }
  
  if (this->ScalarBarCheck->GetState() != val)
    {
    this->ScalarBarCheck->SetState(val);
    }
}

//----------------------------------------------------------------------------
void vtkPVData::SetCubeAxesVisibility(int val)
{
  vtkRenderer *ren;
  char *tclName;
  
  if (!this->GetPVRenderView())
    {
    return;
    }
  
  ren = this->GetPVRenderView()->GetRenderer();
  tclName = this->GetPVRenderView()->GetRendererTclName();
  
  if (ren == NULL)
    {
    return;
    }
  
  if (this->CubeAxesCheck->GetState() != val)
    {
    this->AddTraceEntry("$kw(%s) SetCubeAxesVisibility %d", this->GetTclName(), val);
    this->CubeAxesCheck->SetState(val);
    }

  // I am going to add and remove it from the renderer instead of using visibility.
  // Composites should really have multiple props.
  
  if (ren)
    {
    if (val)
      {
      this->Script("%s AddProp %s", tclName, this->GetCubeAxesTclName());
      }
    else
      {
      this->Script("%s RemoveProp %s", tclName, this->GetCubeAxesTclName());
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVData::ScalarBarCheckCallback()
{
  this->SetScalarBarVisibility(this->ScalarBarCheck->GetState());
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::CubeAxesCheckCallback()
{
  this->AddTraceEntry("$kw(%s) SetCubeAxesVisibility %d", this->GetTclName(),
                      this->CubeAxesCheck->GetState());
  this->SetCubeAxesVisibility(this->CubeAxesCheck->GetState());
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}


//----------------------------------------------------------------------------
void vtkPVData::SetPointSize(int size)
{
  if ( this->PointSizeScale->GetValue() == size )
    {
    return;
    }
  this->PointSizeScale->SetValue(size);
}

//----------------------------------------------------------------------------
void vtkPVData::SetLineWidth(int width)
{
  if ( this->LineWidthScale->GetValue() == width )
    {
    return;
    }
  this->LineWidthScale->SetValue(width);
}

//----------------------------------------------------------------------------
void vtkPVData::ChangePointSize()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (this->PropertyTclName)
    {
    pvApp->BroadcastScript("%s SetPointSize %f",
                           this->PropertyTclName,
                           this->PointSizeScale->GetValue());
    }
  
  this->AddTraceEntry("$kw(%s) SetPointSize %d", this->GetTclName(),
                      (int)(this->PointSizeScale->GetValue()));

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
} 

//----------------------------------------------------------------------------
void vtkPVData::ChangeLineWidth()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (this->PropertyTclName)
    {
    pvApp->BroadcastScript("%s SetLineWidth %f",
                           this->PropertyTclName,
                           this->LineWidthScale->GetValue());
    }

  this->AddTraceEntry("$kw(%s) SetLineWidth %d", this->GetTclName(),
                      (int)(this->LineWidthScale->GetValue()));

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}




//----------------------------------------------------------------------------
void vtkPVData::SetPropertiesParent(vtkKWWidget *parent)
{
  if (this->PropertiesParent == parent)
    {
    return;
    }
  if (this->PropertiesParent)
    {
    vtkErrorMacro("Cannot reparent properties.");
    return;
    }
  this->PropertiesParent = parent;
  parent->Register(this);
}





//----------------------------------------------------------------------------
void vtkPVData::SaveInTclScript(ofstream *file, int interactiveFlag, 
                                int vtkFlag)
{
  float range[2];
  const char* scalarMode;
  char* result;
  char* renTclName;

  renTclName = this->GetPVRenderView()->GetRendererTclName();

  if (this->GetVisibility())
    {
    *file << "vtkPVGeometryFilter " << this->GeometryTclName << "\n\t"
          << this->GeometryTclName << " SetInput [" 
          << this->GetPVSource()->GetVTKSourceTclName() << " GetOutput]\n";

    if (this->PVColorMap)
      {
      this->PVColorMap->SaveInTclScript(file, interactiveFlag, vtkFlag);
      }

    *file << "vtkPolyDataMapper " << this->MapperTclName << "\n\t"
          << this->MapperTclName << " SetInput ["
          << this->GeometryTclName << " GetOutput]\n\t";  
    *file << this->MapperTclName << " SetImmediateModeRendering "
          << this->Mapper->GetImmediateModeRendering() << "\n\t";
    this->Mapper->GetScalarRange(range);
    *file << this->MapperTclName << " UseLookupTableScalarRangeOn\n\t";
    *file << this->MapperTclName << " SetScalarVisibility "
          << this->Mapper->GetScalarVisibility() << "\n\t"
          << this->MapperTclName << " SetScalarModeTo";
    scalarMode = this->Mapper->GetScalarModeAsString();
    *file << scalarMode << "\n";
    if (strcmp(scalarMode, "UsePointFieldData") == 0 ||
        strcmp(scalarMode, "UseCellFieldData") == 0)
      {
      *file << "\t" << this->MapperTclName << " SelectColorArray {"
            << this->Mapper->GetArrayName() << "}\n";
      }
    if (this->PVColorMap)
      {
      *file << this->MapperTclName << " SetLookupTable " 
            << this->PVColorMap->GetLookupTableTclName() << endl;
      }
  
    *file << "vtkActor " << this->PropTclName << "\n\t"
          << this->PropTclName << " SetMapper " << this->MapperTclName << "\n\t"
          << "[" << this->PropTclName << " GetProperty] SetRepresentationTo"
          << this->Property->GetRepresentationAsString() << "\n\t"
          << "[" << this->PropTclName << " GetProperty] SetInterpolationTo"
          << this->Property->GetInterpolationAsString() << "\n";

    *file << "\t[" << this->PropTclName << " GetProperty] SetAmbient "
          << this->Property->GetAmbient() << "\n";
    *file << "\t[" << this->PropTclName << " GetProperty] SetDiffuse "
          << this->Property->GetDiffuse() << "\n";
    *file << "\t[" << this->PropTclName << " GetProperty] SetSpecular "
          << this->Property->GetSpecular() << "\n";
    *file << "\t[" << this->PropTclName << " GetProperty] SetSpecularPower "
          << this->Property->GetSpecularPower() << "\n";
    float *color = this->Property->GetSpecularColor();
    *file << "\t[" << this->PropTclName << " GetProperty] SetSpecularColor "
          << color[0] << " " << color[1] << " " << color[2] << "\n";
    if (this->Property->GetLineWidth() > 1)
      {
      *file << "\t[" << this->PropTclName << " GetProperty] SetLineWidth "
          << this->Property->GetLineWidth() << "\n";
      }
    if (this->Property->GetPointSize() > 1)
      {
      *file << "\t[" << this->PropTclName << " GetProperty] SetPointSize "
          << this->Property->GetPointSize() << "\n";
      }

    if (!this->Mapper->GetScalarVisibility())
      {
      float propColor[3];
      this->Property->GetColor(propColor);
      *file << "[" << this->PropTclName << " GetProperty] SetColor "
            << propColor[0] << " " << propColor[1] << " " << propColor[2]
            << "\n";
      }

    *file << renTclName << " AddActor " << this->PropTclName << "\n";
    }  

  if (this->CubeAxesCheck->GetState())
    {
    *file << "vtkCubeAxesActor2D " << this->CubeAxesTclName << "\n\t"
          << this->CubeAxesTclName << " SetFlyModeToOuterEdges\n\t"
          << "[" << this->CubeAxesTclName << " GetProperty] SetColor 1 1 1\n\t"
          << this->CubeAxesTclName << " SetBounds ";
    this->Script("set tempResult [%s GetBounds]", this->CubeAxesTclName);
    result = this->GetPVApplication()->GetMainInterp()->result;
    *file << result << "\n\t"
          << this->CubeAxesTclName << " SetCamera [";
    *file << renTclName << " GetActiveCamera]\n\t"
          << this->CubeAxesTclName << " SetInertia 20\n";
    *file << renTclName << " AddProp " << this->CubeAxesTclName << "\n";
    }
}

//----------------------------------------------------------------------------
void vtkPVData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ColorMap: " << this->PVColorMap << endl;
  os << indent << "ColorMenu: " << this->ColorMenu << endl;
  os << indent << "EditColorMapButton: " << this->EditColorMapButton << endl;
  os << indent << "CubeAxesTclName: " << (this->CubeAxesTclName?this->CubeAxesTclName:"none") << endl;
  os << indent << "GeometryTclName: " << (this->GeometryTclName?this->GeometryTclName:"none") << endl;
  os << indent << "LODMapperTclName: " << (this->LODMapperTclName?this->LODMapperTclName:"none") << endl;
  os << indent << "Mapper: " << this->GetMapper() << endl;
  os << indent << "MapperTclName: " << (this->MapperTclName?this->MapperTclName:"none") << endl;
  os << indent << "NumberOfPVConsumers: " << this->GetNumberOfPVConsumers() << endl;
  os << indent << "PVSource: " << this->GetPVSource() << endl;
  os << indent << "PropTclName: " << (this->PropTclName?this->PropTclName:"none") << endl;
  os << indent << "PropertiesParent: " << this->GetPropertiesParent() << endl;
  if (this->PVColorMap)
    {
    os << indent << "PVColorMap: " << this->PVColorMap->GetScalarBarTitle() << endl;
    }
  else
    {
    os << indent << "PVColorMap: NULL\n";
    }
  os << indent << "VTKData: " << this->GetVTKData() << endl;
  os << indent << "VTKDataTclName: " << (this->VTKDataTclName?this->VTKDataTclName:"none") << endl;
  os << indent << "PVRenderView: " << this->PVRenderView << endl;
  os << indent << "PropertiesCreated: " << this->PropertiesCreated << endl;
  os << indent << "CubeAxesCheck: " << this->CubeAxesCheck << endl;
  os << indent << "ScalarBarCheck: " << this->ScalarBarCheck << endl;
  os << indent << "RepresentationMenu: " << this->RepresentationMenu << endl;
  os << indent << "InterpolationMenu: " << this->InterpolationMenu << endl;
  os << indent << "LODResolution: " << this->LODResolution << endl;
  os << indent << "CollectThreshold: " << this->CollectThreshold << endl;
}

//-------}---------------------------------------------------------------------
void vtkPVData::GetColorRange(float *range)
{
  float *tmp;
  range[0] = 0.0;
  range[1] = 1.0;
  
  if (this->PVColorMap == NULL)
    {
    vtkErrorMacro("Color map missing.");
    return;
    }
  tmp = this->PVColorMap->GetScalarRange();
  range[0] = tmp[0];
  range[1] = tmp[1];
}

//----------------------------------------------------------------------------
void vtkPVData::OpacityChangedCallback()
{
  float val = this->Opacity->GetValue();

  this->GetPVApplication()->BroadcastScript("[ %s GetProperty ] SetOpacity %f",
                                            this->PropTclName, val);
  this->AddTraceEntry("$kw(%s) SetOpacity %f", this->GetTclName(),
                      val);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::SetOpacity(float val)
{ 
  this->Opacity->SetValue(val);
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorTranslate()
{
  float point[3];
  this->GetActorTranslate(point);
  this->SetActorTranslate(point);
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorTranslate(float* point)
{
  this->SetActorTranslate(point[0], point[1], point[2]);
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorTranslate(float x, float y, float z)
{
  this->TranslateEntry[0]->SetValue(x, 4);
  this->TranslateEntry[1]->SetValue(y, 4);
  this->TranslateEntry[2]->SetValue(z, 4);
  this->GetPVApplication()->BroadcastScript("%s SetPosition %f %f %f",
                                            this->PropTclName, x, y, z);
  this->AddTraceEntry("$kw(%s) SetActorTranslate %f %f %f",
                      this->GetTclName(), x, y, z);  
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::GetActorTranslate(float* point)
{
  point[0] = this->TranslateEntry[0]->GetValueAsFloat();
  point[1] = this->TranslateEntry[1]->GetValueAsFloat();
  point[2] = this->TranslateEntry[2]->GetValueAsFloat();
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorScale()
{
  float point[3];
  this->GetActorScale(point);
  this->SetActorScale(point);
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorScale(float* point)
{
  this->SetActorScale(point[0], point[1], point[2]);
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorScale(float x, float y, float z)
{
  this->ScaleEntry[0]->SetValue(x, 4);
  this->ScaleEntry[1]->SetValue(y, 4);
  this->ScaleEntry[2]->SetValue(z, 4);
  this->GetPVApplication()->BroadcastScript("%s SetScale %f %f %f",
                                            this->PropTclName, x, y, z);
  this->AddTraceEntry("$kw(%s) SetActorScale %f %f %f",
                      this->GetTclName(), x, y, z);  
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVData::GetActorScale(float* point)
{
  point[0] = this->ScaleEntry[0]->GetValueAsFloat();
  point[1] = this->ScaleEntry[1]->GetValueAsFloat();
  point[2] = this->ScaleEntry[2]->GetValueAsFloat();
}

//----------------------------------------------------------------------------
void vtkPVData::SetLODResolution(int dim)
{
  if (this->LODResolution == dim)
    {
    return;
    }
  this->LODResolution = dim;
  
  vtkPVApplication *pvApp = this->GetPVApplication();
    
  pvApp->BroadcastScript("%s SetNumberOfDivisions %d %d %d", 
                         this->LODDeciTclName, this->LODResolution,
                         this->LODResolution, this->LODResolution); 
}

//----------------------------------------------------------------------------
void vtkPVData::SetCollectThreshold(float threshold)
{
  if (this->CollectThreshold == threshold)
    {
    return;
    }
  this->CollectThreshold = threshold;
  
  vtkPVApplication *pvApp = this->GetPVApplication();
    
  if (!pvApp->GetUseRenderingGroup())
    {
    pvApp->BroadcastScript("%s SetThreshold %d", this->CollectTclName,
                           static_cast<unsigned long>(threshold*1000.0));
    pvApp->BroadcastScript("%s SetThreshold %d", this->LODCollectTclName,
                           static_cast<unsigned long>(threshold*1000.0));
    }
}


//----------------------------------------------------------------------------
void vtkPVData::SerializeRevision(ostream& os, vtkIndent indent)
{
  this->Superclass::SerializeRevision(os,indent);
  os << indent << "vtkPVData ";
  this->ExtractRevision(os,"$Revision: 1.161.2.2 $");
}

//----------------------------------------------------------------------------
void vtkPVData::SerializeSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::SerializeSelf(os, indent);
  os << indent << "Visibility " << this->VisibilityCheck->GetState() << endl;
  // Color by is not serialized because of gui does not follow tracing
  os << indent << "ColorBy " << this->ColorMenu->GetValue() << endl;
  if ( !vtkString::Equals(this->ColorMenu->GetValue(), "Property") )
    {
    float *tmp = this->PVColorMap->GetScalarRange();
    os << indent << "ColorRange " << tmp[0] << " " << tmp[1] << endl;
    }

  os << indent << "Representation " << this->RepresentationMenu->GetValue() << endl;
  os << indent << "Interpolation " << this->InterpolationMenu->GetValue() << endl;
  os << indent << "PointSize " << this->PointSizeScale->GetValue() << endl;
  os << indent << "LineWidth " << this->LineWidthScale->GetValue() << endl;
  os << indent << "CubeAxes " << this->CubeAxesCheck->GetState() << endl;
  os << indent << "ActorTranslate " 
     << this->TranslateEntry[0]->GetValue() << " "
     << this->TranslateEntry[1]->GetValue() << " " 
     << this->TranslateEntry[1]->GetValue() << endl;
  os << indent << "Opacity " << this->Opacity->GetValue() << endl;
  os << indent << "ActorScale " 
     << this->ScaleEntry[0]->GetValue() << " "
     << this->ScaleEntry[1]->GetValue() << " " 
     << this->ScaleEntry[1]->GetValue() << endl;
}

//------------------------------------------------------------------------------
void vtkPVData::SerializeToken(istream& is, const char token[1024])
{
  if ( vtkString::Equals(token, "Visibility") )
    {
    int cor = 0;
    if (! (is >> cor) )
      {
      vtkErrorMacro("Problem Parsing session file");
      }
    this->SetVisibility(cor);
    }
  else if ( vtkString::Equals(token, "PointSize") )
    {
    int cor = 0;
    if (! (is >> cor) )
      {
      vtkErrorMacro("Problem Parsing session file");
      }
    this->SetPointSize(cor);
    }
  else if ( vtkString::Equals(token, "LineWidth") )
    {
    int cor = 0;
    if (! (is >> cor) )
      {
      vtkErrorMacro("Problem Parsing session file");
      }
    this->SetLineWidth(cor);
    }
  else if ( vtkString::Equals(token, "CubeAxes") )
    {
    int cor = 0;
    if (! (is >> cor) )
      {
      vtkErrorMacro("Problem Parsing session file");
      }
    this->SetCubeAxesVisibility(cor);
    }
  else if ( vtkString::Equals(token, "Opacity") )
    {
    float cor = 0;
    if (! (is >> cor) )
      {
      vtkErrorMacro("Problem Parsing session file");
      }
    this->SetOpacity(cor);
    }
  else if ( vtkString::Equals(token, "Representation") )
    {
    char repr[1024];
    repr[0] = 0;
    if (! (is >> repr) )
      {
      vtkErrorMacro("Problem Parsing session file");
      }
    this->SetRepresentation(repr);
    }
  else if ( vtkString::Equals(token, "Interpolation") )
    {
    char repr[1024];
    repr[0] = 0;
    if (! (is >> repr) )
      {
      vtkErrorMacro("Problem Parsing session file");
      }
    this->SetInterpolation(repr);
    }
  else if ( vtkString::Equals(token, "ActorTranslate") )
    {
    float cor[3];
    int cc;
    for ( cc = 0; cc < 3; cc ++ )
      {
      cor[cc] = 0.0;
      if (! (is >> cor[cc]) )
        {
        vtkErrorMacro("Problem Parsing session file");
        return;
        }
      }
    this->SetActorTranslate(cor);
    }
  else if ( vtkString::Equals(token, "ActorScale") )
    {
    float cor[3];
    int cc;
    for ( cc = 0; cc < 3; cc ++ )
      {
      cor[cc] = 0.0;
      if (! (is >> cor[cc]) )
        {
        vtkErrorMacro("Problem Parsing session file");
        return;
        }
      }
    this->SetActorScale(cor);
    }
  else if ( vtkString::Equals(token, "ColorRange") )
    {
    float cor[2];
    int cc;
    for ( cc = 0; cc < 2; cc ++ )
      {
      cor[cc] = 0.0;
      if (! (is >> cor[cc]) )
        {
        vtkErrorMacro("Problem Parsing session file");
        return;
        }
      }
    this->SetColorRange(cor[0], cor[1]);
    }
  else
    {
    this->Superclass::SerializeToken(is,token);  
    }
}
  
//----------------------------------------------------------------------------
void vtkPVData::GetArrayComponentRange(float *range, int pointDataFlag,
                                       const char *arrayName, int component)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkMultiProcessController *controller = pvApp->GetController();
  int id, num;
  vtkDataArray *array;
  float temp[2];

  range[0] = VTK_LARGE_FLOAT;
  range[1] = -VTK_LARGE_FLOAT;

  if (pointDataFlag)
    {
    array = this->VTKData->GetPointData()->GetArray(arrayName);
    }
  else
    {
    array = this->VTKData->GetCellData()->GetArray(arrayName);
    }

  if (array == NULL || array->GetName() == NULL)
    {
    return;
    }

  array->GetRange(range, 0);  

  pvApp->BroadcastScript("$Application SendDataArrayRange %s %d {%s} %d",
                         this->GetVTKDataTclName(),
                         pointDataFlag, array->GetName(), component);
  
  num = controller->GetNumberOfProcesses();
  for (id = 1; id < num; id++)
    {
    controller->Receive(temp, 2, id, 1976);
    // try to protect against invalid ranges.
    if (range[0] > range[1])
      {
      range[0] = temp[0];
      range[1] = temp[1];
      }
    else if (temp[0] <= temp[1])
      {
      if (temp[0] < range[0])
        {
        range[0] = temp[0];
        }
      if (temp[1] > range[1])
        {
        range[1] = temp[1];
        }
      }
    }
}



