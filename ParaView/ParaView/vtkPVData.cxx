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
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWMenuButton.h"
#include "vtkKWNotebook.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWPushButton.h"
#include "vtkKWScale.h"
#include "vtkKWView.h"
#include "vtkKWWidget.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVRenderView.h"
#include "vtkPVSource.h"
#include "vtkPVWindow.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkScalarBarActor.h"
#include "vtkTexture.h"
#include "vtkTimerLog.h"
#include "vtkTreeComposite.h"
#include "vtkPVColorMap.h"

int vtkPVDataCommand(ClientData cd, Tcl_Interp *interp,
		     int argc, char *argv[]);

vtkCxxSetObjectMacro(vtkPVData,View, vtkKWView);


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
  
  this->View = NULL;
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

  // Create a unique id for creating tcl names.
  ++instanceCount;
  this->InstanceCount = instanceCount;
    
  this->Properties = vtkKWFrame::New();

  this->ScalarBarFrame = vtkKWLabeledFrame::New();
  this->ColorFrame = vtkKWLabeledFrame::New();
  this->DisplayStyleFrame = vtkKWLabeledFrame::New();
  this->StatsFrame = vtkKWWidget::New();
  this->ViewFrame = vtkKWLabeledFrame::New();
  
  this->NumCellsLabel = vtkKWLabel::New();
  this->NumPointsLabel = vtkKWLabel::New();
  
  this->BoundsDisplay = vtkKWBoundsDisplay::New();
  this->BoundsDisplay->ShowHideFrameOn();
  
  this->AmbientScale = vtkKWScale::New();

  this->ColorMenuLabel = vtkKWLabel::New();
  this->ColorMenu = vtkKWOptionMenu::New();

  this->ColorMapMenuLabel = vtkKWLabel::New();
  this->ColorMapMenu = vtkKWOptionMenu::New();
  
  this->ColorButton = vtkKWChangeColorButton::New();

   // Stuff for setting the range of the color map.
  this->ColorRangeFrame = vtkKWWidget::New();
  this->ColorRangeResetButton = vtkKWPushButton::New();
  this->ColorRangeMinEntry = vtkKWLabeledEntry::New();
  this->ColorRangeMaxEntry = vtkKWLabeledEntry::New();

  this->RepresentationMenuFrame = vtkKWWidget::New();
  this->RepresentationMenuLabel = vtkKWLabel::New();
  this->RepresentationMenu = vtkKWOptionMenu::New();
  
  this->InterpolationMenuFrame = vtkKWWidget::New();
  this->InterpolationMenuLabel = vtkKWLabel::New();
  this->InterpolationMenu = vtkKWOptionMenu::New();
  
  this->DisplayScalesFrame = vtkKWWidget::New();
  this->PointSizeLabel = vtkKWLabel::New();
  this->PointSizeScale = vtkKWScale::New();
  this->LineWidthLabel = vtkKWLabel::New();
  this->LineWidthScale = vtkKWScale::New();
  
  this->ScalarBarCheckFrame = vtkKWWidget::New();
  this->ScalarBarCheck = vtkKWCheckButton::New();
  this->ScalarBarOrientationCheck = vtkKWCheckButton::New();
  this->CubeAxesCheck = vtkKWCheckButton::New();

  this->VisibilityCheck = vtkKWCheckButton::New();

  this->ResetCameraButton = vtkKWPushButton::New();
  
  this->PreviousAmbient = 0.2;
  this->PreviousSpecular = 0.1;
  this->PreviousDiffuse = 0.8;
  this->PreviousWasSolid = 1;

  this->PVColorMap = NULL;
}

//----------------------------------------------------------------------------
vtkPVData::~vtkPVData()
{
  if (this->PVColorMap)
    {
    this->PVColorMap->Delete();
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

  if (this->View)
    {
    this->View->UnRegister(this);
    this->View = NULL;
    }
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

  this->ColorMapMenuLabel->Delete();
  this->ColorMapMenuLabel = NULL;
  this->ColorMapMenu->Delete();
  this->ColorMapMenu = NULL;
  
  this->ColorButton->Delete();
  this->ColorButton = NULL;
  
   // Stuff for setting the range of the color map.
  this->ColorRangeFrame->Delete();
  this->ColorRangeFrame = NULL;
  this->ColorRangeResetButton->Delete();
  this->ColorRangeResetButton = NULL;
  this->ColorRangeMinEntry->Delete();
  this->ColorRangeMinEntry = NULL;
  this->ColorRangeMaxEntry->Delete();
  this->ColorRangeMaxEntry = NULL;

  this->RepresentationMenuLabel->Delete();
  this->RepresentationMenuLabel = NULL;  
  this->RepresentationMenu->Delete();
  this->RepresentationMenu = NULL;
  
  this->InterpolationMenuLabel->Delete();
  this->InterpolationMenuLabel = NULL;
  this->InterpolationMenu->Delete();
  this->InterpolationMenu = NULL;
  
  this->RepresentationMenuFrame->Delete();
  this->RepresentationMenuFrame = NULL;
  this->InterpolationMenuFrame->Delete();
  this->InterpolationMenuFrame = NULL;
  
  this->PointSizeLabel->Delete();
  this->PointSizeLabel = NULL;
  this->PointSizeScale->Delete();
  this->PointSizeScale = NULL;
  this->LineWidthLabel->Delete();
  this->LineWidthLabel = NULL;
  this->LineWidthScale->Delete();
  this->LineWidthScale = NULL;
  this->DisplayScalesFrame->Delete();
  this->DisplayScalesFrame = NULL;

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
  
  this->ScalarBarCheckFrame->Delete();
  this->ScalarBarCheckFrame = NULL;
  this->ScalarBarCheck->Delete();
  this->ScalarBarCheck = NULL;  
  this->ScalarBarOrientationCheck->Delete();
  this->ScalarBarOrientationCheck = NULL;
  
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

  this->ScalarBarFrame->Delete();
  this->ScalarBarFrame = NULL;
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
  
  sprintf(tclName, "Geometry%d", this->InstanceCount);
  pvApp->BroadcastScript("vtkPVGeometryFilter %s", tclName);
  this->SetGeometryTclName(tclName);
  pvApp->BroadcastScript("%s SetStartMethod {Application LogStartEvent "
			 "{Execute Geometry}}", this->GeometryTclName);
  pvApp->BroadcastScript("%s SetEndMethod {Application LogEndEvent "
			 "{Execute Geometry}}", this->GeometryTclName);


#ifdef VTK_USE_MPI
  //sprintf(tclName, "Collect%d", this->InstanceCount);
  //pvApp->BroadcastScript("vtkCollectPolyData %s", tclName);
  //this->SetCollectTclName(tclName);
  //pvApp->BroadcastScript("%s SetInput [%s GetOutput]", 
  //                       this->CollectTclName, this->GeometryTclName);

  //pvApp->BroadcastScript("%s SetStartMethod {Application LogStartEvent "
  //			 "{Execute Collect}}", this->CollectTclName);
  //pvApp->BroadcastScript("%s SetEndMethod {Application LogEndEvent "
  //			 "{Execute Collect}}", this->CollectTclName);
#endif

  // Get rid of previous object created by the superclass.
  if (this->Mapper)
    {
    this->Mapper->Delete();
    this->Mapper = NULL;
    }
  // Make a new tcl object.
  sprintf(tclName, "Mapper%d", this->InstanceCount);
  this->Mapper = (vtkPolyDataMapper*)pvApp->MakeTclObject("vtkPolyDataMapper",
							  tclName);
  this->MapperTclName = NULL;
  this->SetMapperTclName(tclName);

  pvApp->BroadcastScript("%s UseLookupTableScalarRangeOn", this->MapperTclName);
  pvApp->BroadcastScript("%s SetColorModeToMapScalars", this->MapperTclName);
  if (this->CollectTclName)
    {
    pvApp->BroadcastScript("%s SetInput [%s GetOutput]", this->MapperTclName,
			   this->CollectTclName);
    }
  else
    {
    pvApp->BroadcastScript("%s SetInput [%s GetOutput]", this->MapperTclName,
			   this->GeometryTclName);
    }
  
  
  sprintf(tclName, "LODDeci%d", this->InstanceCount);
  pvApp->BroadcastScript("vtkQuadricClustering %s", tclName);
  this->LODDeciTclName = NULL;
  this->SetLODDeciTclName(tclName);
  pvApp->BroadcastScript("%s SetStartMethod {Application LogStartEvent {Execute Decimate}}", 
                         this->LODDeciTclName);
  pvApp->BroadcastScript("%s SetEndMethod {Application LogEndEvent {Execute Decimate}}", 
                         this->LODDeciTclName);
  pvApp->BroadcastScript("%s SetInput [%s GetOutput]", 
                         this->LODDeciTclName, this->GeometryTclName);
  pvApp->BroadcastScript("%s CopyCellDataOn", this->LODDeciTclName);
  pvApp->BroadcastScript("%s UseInputPointsOn", this->LODDeciTclName);
  pvApp->BroadcastScript("%s UseInternalTrianglesOff", this->LODDeciTclName);
  //pvApp->BroadcastScript("%s UseFeatureEdgesOn", this->LODDeciTclName);
  //pvApp->BroadcastScript("%s UseFeaturePointsOn", this->LODDeciTclName);
  pvApp->BroadcastScript("%s SetNumberOfDivisions 50 50 50", 
                         this->LODDeciTclName);

#ifdef VTK_USE_MPI
  //sprintf(tclName, "LODCollect%d", this->InstanceCount);
  //pvApp->BroadcastScript("vtkCollectPolyData %s", tclName);
  //this->SetLODCollectTclName(tclName);
  //pvApp->BroadcastScript("%s SetInput [%s GetOutput]", 
  //                       this->LODCollectTclName, this->LODDeciTclName);
  //pvApp->BroadcastScript("%s SetStartMethod {Application LogStartEvent "
  //                       "{Execute LODCollect}}", this->LODCollectTclName);
  //pvApp->BroadcastScript("%s SetEndMethod {Application LogEndEvent "
  //                       "{Execute LODCollect}}", this->LODCollectTclName);
#endif

  sprintf(tclName, "LODMapper%d", this->InstanceCount);
  pvApp->BroadcastScript("vtkPolyDataMapper %s", tclName);
  this->LODMapperTclName = NULL;
  this->SetLODMapperTclName(tclName);

  pvApp->BroadcastScript("%s UseLookupTableScalarRangeOn", this->LODMapperTclName);
  pvApp->BroadcastScript("%s SetColorModeToMapScalars", this->LODMapperTclName);
 
  // Make a new tcl object.
  sprintf(tclName, "Actor%d", this->InstanceCount);
  this->Prop = (vtkProp*)pvApp->MakeTclObject("vtkPVLODActor", tclName);
  this->SetPropTclName(tclName);

  // Make a new tcl object.
  sprintf(tclName, "Property%d", this->InstanceCount);
  this->Property = (vtkProperty*)pvApp->MakeTclObject("vtkProperty", tclName);
  this->SetPropertyTclName(tclName);
  pvApp->BroadcastScript("%s SetAmbient 0.2", this->PropertyTclName);
  pvApp->BroadcastScript("%s SetDiffuse 0.8", this->PropertyTclName);

  
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
    this->Script("%s SetNumberOfPieces 0", this->LODMapperTclName);
    this->Script("%s SetPiece 0", this->LODMapperTclName);
    for (id = 1; id < numProcs; ++id)
      {
      pvApp->RemoteScript(id, "%s SetNumberOfPieces %d",
                          this->MapperTclName, debugNum-1);
      pvApp->RemoteScript(id, "%s SetPiece %d", this->MapperTclName, id-1);
      pvApp->RemoteScript(id, "%s SetNumberOfPieces %d",
                          this->LODMapperTclName, debugNum-1);
      pvApp->RemoteScript(id, "%s SetPiece %d", this->LODMapperTclName, id-1);
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
      pvApp->RemoteScript(id, "%s SetNumberOfPieces %d",
		  	this->LODMapperTclName, debugNum);
      pvApp->RemoteScript(id, "%s SetPiece %d", this->LODMapperTclName, id);
      }
    }
}



//----------------------------------------------------------------------------
vtkPVData* vtkPVData::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVData");
  if(ret)
    {
    return (vtkPVData*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVData;
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

  pvApp->BroadcastScript("Application SendDataBounds %s", 
			this->VTKDataTclName);
  
  this->VTKData->GetBounds(bounds);
  
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

  pvApp->BroadcastScript("Application SendDataNumberOfCells %s", 
			this->VTKDataTclName);
  
  numCells = this->VTKData->GetNumberOfCells();
  
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

  pvApp->BroadcastScript("Application SendDataNumberOfPoints %s", 
			this->VTKDataTclName);
  
  numPoints = this->VTKData->GetNumberOfPoints();
  
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
    if (getenv("PV_USE_TRANSMIT") != NULL)
      {
      pvApp->BroadcastSimpleScript("vtkTransmitPolyDataPiece pvTemp");
      }
    else
      {
      pvApp->BroadcastSimpleScript("vtkExtractPolyDataPiece pvTemp");
      }
    }
  else if (this->VTKData->IsA("vtkUnstructuredGrid"))
    {
    // Transmit is more efficient, but has the possiblity of hanging.
    // It will hang if all procs do not  call execute.
    if (getenv("PV_USE_TRANSMIT") != NULL)
      {
      pvApp->BroadcastSimpleScript("vtkTransmitUnstructuredGridPiece pvTemp");
      }
    else
      {
      pvApp->BroadcastSimpleScript("vtkExtractUnstructuredGridPiece pvTemp");
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

  this->ScalarBarFrame->SetParent(this->Properties->GetFrame());
  this->ScalarBarFrame->ShowHideFrameOn();
  this->ScalarBarFrame->Create(this->Application);
  this->ScalarBarFrame->SetLabel("Scalar Bar");
  this->ColorFrame->SetParent(this->Properties->GetFrame());
  this->ColorFrame->ShowHideFrameOn();
  this->ColorFrame->Create(this->Application);
  this->ColorFrame->SetLabel("Color");
  this->DisplayStyleFrame->SetParent(this->Properties->GetFrame());
  this->DisplayStyleFrame->ShowHideFrameOn();
  this->DisplayStyleFrame->Create(this->Application);
  this->DisplayStyleFrame->SetLabel("Display Style");
  this->StatsFrame->SetParent(this->Properties->GetFrame());
  this->StatsFrame->Create(this->Application, "frame", "");
  this->ViewFrame->SetParent(this->Properties->GetFrame());
  this->ViewFrame->ShowHideFrameOn();
  this->ViewFrame->Create(this->Application);
  this->ViewFrame->SetLabel("View");
 
  this->NumCellsLabel->SetParent(this->StatsFrame);
  this->NumCellsLabel->Create(this->Application, "");
  this->NumPointsLabel->SetParent(this->StatsFrame);
  this->NumPointsLabel->Create(this->Application, "");
  
  this->BoundsDisplay->SetParent(this->Properties->GetFrame());
  this->BoundsDisplay->Create(this->Application);
  
  this->AmbientScale->SetParent(this->Properties->GetFrame());
  this->AmbientScale->Create(this->Application, "-showvalue 1");
  this->AmbientScale->DisplayLabel("Ambient Light");
  this->AmbientScale->SetRange(0.0, 1.0);
  this->AmbientScale->SetResolution(0.1);
  this->AmbientScale->SetCommand(this, "AmbientChanged");
  
  this->ColorMenuLabel->SetParent(this->ColorFrame->GetFrame());
  this->ColorMenuLabel->Create(this->Application, "");
  this->ColorMenuLabel->SetLabel("Color by:");
  
  this->ColorMenu->SetParent(this->ColorFrame->GetFrame());
  this->ColorMenu->Create(this->Application, "");    
  this->ColorButton->SetParent(this->ColorFrame->GetFrame());
  this->ColorButton->Create(this->Application, "");
  this->ColorButton->SetText("Actor Color");
  this->ColorButton->SetCommand(this, "ChangeActorColor");
  
  this->ScalarBarCheckFrame->SetParent(this->ScalarBarFrame->GetFrame());
  this->ScalarBarCheckFrame->Create(this->Application, "frame", "");

  this->ColorMapMenuLabel->SetParent(this->ScalarBarCheckFrame);
  this->ColorMapMenuLabel->Create(this->Application, "");
  this->ColorMapMenuLabel->SetLabel("Color map:");
  
  this->ColorMapMenu->SetParent(this->ScalarBarCheckFrame);
  this->ColorMapMenu->Create(this->Application, "");
  this->ColorMapMenu->AddEntryWithCommand("Red to Blue", this,
                                          "ChangeColorMapToRedBlue");
  this->ColorMapMenu->AddEntryWithCommand("Blue to Red", this,
                                          "ChangeColorMapToBlueRed");
  this->ColorMapMenu->AddEntryWithCommand("Grayscale", this,
                                          "ChangeColorMapToGrayscale");
  this->ColorMapMenu->SetValue("Red to Blue");
  
  this->ColorRangeFrame->SetParent(this->ScalarBarFrame->GetFrame());
  this->ColorRangeFrame->Create(this->Application, "frame", "");
  this->ColorRangeResetButton->SetParent(this->ColorRangeFrame);
  this->ColorRangeResetButton->Create(this->Application, 
				      "-text {Reset Range}");
  this->ColorRangeResetButton->SetCommand(this, "ResetColorRange");
  this->ColorRangeMinEntry->SetParent(this->ColorRangeFrame);
  this->ColorRangeMinEntry->Create(this->Application);
  this->ColorRangeMinEntry->SetLabel("Min:");
  this->ColorRangeMinEntry->GetEntry()->SetWidth(7);
  this->Script("bind %s <KeyPress-Return> {%s ColorRangeEntryCallback}",
               this->ColorRangeMinEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());
  this->Script("bind %s <FocusOut> {%s ColorRangeEntryCallback}",
               this->ColorRangeMinEntry->GetEntry()->GetWidgetName(),
               this->GetTclName()); 
  this->ColorRangeMaxEntry->SetParent(this->ColorRangeFrame);
  this->ColorRangeMaxEntry->Create(this->Application);
  this->ColorRangeMaxEntry->SetLabel("Max:");
  this->ColorRangeMaxEntry->GetEntry()->SetWidth(7);
  this->Script("bind %s <KeyPress-Return> {%s ColorRangeEntryCallback}",
               this->ColorRangeMaxEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());
  this->Script("bind %s <FocusOut> {%s ColorRangeEntryCallback}",
               this->ColorRangeMaxEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());

  this->RepresentationMenuFrame->SetParent(
    this->DisplayStyleFrame->GetFrame());
  this->RepresentationMenuFrame->Create(this->Application, "frame", "");
  
  this->RepresentationMenuLabel->SetParent(this->RepresentationMenuFrame);
  this->RepresentationMenuLabel->Create(this->Application, "");
  this->RepresentationMenuLabel->SetLabel("Representation:");
  this->RepresentationMenu->SetParent(this->RepresentationMenuFrame);
  this->RepresentationMenu->Create(this->Application, "");
  this->RepresentationMenu->AddEntryWithCommand("Wireframe", this,
                                                "DrawWireframe");
  this->RepresentationMenu->AddEntryWithCommand("Surface", this,
                                                "DrawSurface");
  this->RepresentationMenu->AddEntryWithCommand("Points", this,
                                                "DrawPoints");
  this->RepresentationMenu->SetValue("Surface");
  
  this->InterpolationMenuFrame->SetParent(this->DisplayStyleFrame->GetFrame());
  this->InterpolationMenuFrame->Create(this->Application, "frame", "");
  this->InterpolationMenuLabel->SetParent(this->InterpolationMenuFrame);
  this->InterpolationMenuLabel->Create(this->Application, "");
  this->InterpolationMenuLabel->SetLabel("Interpolation:");
  this->InterpolationMenu->SetParent(this->InterpolationMenuFrame);
  this->InterpolationMenu->Create(this->Application, "");
  this->InterpolationMenu->AddEntryWithCommand("Flat", this,
					       "SetInterpolationToFlat");
  this->InterpolationMenu->AddEntryWithCommand("Gouraud", this,
					       "SetInterpolationToGouraud");
  this->InterpolationMenu->SetValue("Gouraud");

  this->DisplayScalesFrame->SetParent(this->DisplayStyleFrame->GetFrame());
  this->DisplayScalesFrame->Create(this->Application, "frame", "");
  
  this->PointSizeLabel->SetParent(this->DisplayScalesFrame);
  this->PointSizeLabel->Create(this->Application, "");
  this->PointSizeLabel->SetLabel("Point Size");
  
  this->PointSizeScale->SetParent(this->DisplayScalesFrame);
  this->PointSizeScale->Create(this->Application, "-showvalue 1");
  this->PointSizeScale->SetRange(1, 5);
  this->PointSizeScale->SetResolution(1);
  this->PointSizeScale->SetValue(1);
  this->PointSizeScale->SetCommand(this, "ChangePointSize");

  this->LineWidthLabel->SetParent(this->DisplayScalesFrame);
  this->LineWidthLabel->Create(this->Application, "");
  this->LineWidthLabel->SetLabel("Line Width");
  
  this->LineWidthScale->SetParent(this->DisplayScalesFrame);
  this->LineWidthScale->Create(this->Application, "-showvalue 1");
  this->LineWidthScale->SetRange(1, 5);
  this->LineWidthScale->SetResolution(1);
  this->LineWidthScale->SetValue(1);
  this->LineWidthScale->SetCommand(this, "ChangeLineWidth");
  
  
  this->ScalarBarCheck->SetParent(this->ScalarBarCheckFrame);
  this->ScalarBarCheck->Create(this->Application, "-text Visibility");
  this->Application->Script(
    "%s configure -command {%s ScalarBarCheckCallback}",
    this->ScalarBarCheck->GetWidgetName(),
    this->GetTclName());

  this->ScalarBarOrientationCheck->SetParent(this->ScalarBarCheckFrame);
  this->ScalarBarOrientationCheck->Create(this->Application, "-text Vertical");
  this->ScalarBarOrientationCheck->SetState(1);
  this->ScalarBarOrientationCheck->SetCommand(this, 
					      "ScalarBarOrientationCallback");
  
  this->CubeAxesCheck->SetParent(this->Properties->GetFrame());
  this->CubeAxesCheck->Create(this->Application, "-text CubeAxes");
  this->CubeAxesCheck->SetCommand(this, "CubeAxesCheckCallback");
  
  this->VisibilityCheck->SetParent(this->ViewFrame->GetFrame());
  this->VisibilityCheck->Create(this->Application, "-text Visibility");
  this->VisibilityCheck->SetState(1);
  this->Application->Script(
    "%s configure -command {%s VisibilityCheckCallback}",
    this->VisibilityCheck->GetWidgetName(),
    this->GetTclName());
  this->VisibilityCheck->SetState(1);

  this->ResetCameraButton->SetParent(this->ViewFrame->GetFrame());
  this->ResetCameraButton->Create(this->Application, "");
  this->ResetCameraButton->SetLabel("Set View to Data");
  this->ResetCameraButton->SetCommand(this, "CenterCamera");
  
  this->Script("pack %s %s -side left",
               this->VisibilityCheck->GetWidgetName(),
               this->ResetCameraButton->GetWidgetName());
  this->Script("pack %s", this->StatsFrame->GetWidgetName());
  this->Script("pack %s %s -side left",
               this->NumCellsLabel->GetWidgetName(),
               this->NumPointsLabel->GetWidgetName());
  this->Script("pack %s -fill x -expand t", 
	       this->BoundsDisplay->GetWidgetName());
  this->Script("pack %s -fill x -expand t", this->ViewFrame->GetWidgetName());
  this->Script("pack %s -fill x -expand t", this->ColorFrame->GetWidgetName());
  this->Script("pack %s %s -side left",
               this->ColorMenuLabel->GetWidgetName(),
               this->ColorMenu->GetWidgetName());
  this->Script("pack %s %s -side top -expand t -fill x",
	       this->ScalarBarCheckFrame->GetWidgetName(),
	       this->ColorRangeFrame->GetWidgetName());
  this->Script("pack %s %s %s %s -side left",
               this->ScalarBarCheck->GetWidgetName(),
               this->ScalarBarOrientationCheck->GetWidgetName(),
               this->ColorMapMenuLabel->GetWidgetName(),
               this->ColorMapMenu->GetWidgetName());
  this->Script("pack %s -side left -expand f",
	       this->ColorRangeResetButton->GetWidgetName());
  this->Script("pack %s %s -side left -expand t -fill x",
	       this->ColorRangeMinEntry->GetWidgetName(),
	       this->ColorRangeMaxEntry->GetWidgetName());

  this->Script("pack %s %s %s -side top -fill x",
               this->RepresentationMenuFrame->GetWidgetName(),
               this->InterpolationMenuFrame->GetWidgetName(),
               this->DisplayScalesFrame->GetWidgetName());
  this->Script("pack %s %s -side left",
               this->RepresentationMenuLabel->GetWidgetName(),
               this->RepresentationMenu->GetWidgetName());
  this->Script("pack %s %s -side left",
               this->InterpolationMenuLabel->GetWidgetName(),
               this->InterpolationMenu->GetWidgetName());
  this->Script("pack %s %s %s %s -side left",
               this->PointSizeLabel->GetWidgetName(),
               this->PointSizeScale->GetWidgetName(),
               this->LineWidthLabel->GetWidgetName(),
               this->LineWidthScale->GetWidgetName());
  this->Script("pack %s -fill x", this->DisplayStyleFrame->GetWidgetName());
  this->Script("pack %s",
               this->CubeAxesCheck->GetWidgetName());

  if (!this->GetPVSource()->GetHideDisplayPage())
    {
    this->Script("pack %s -fill both -expand yes -side top",
		 this->Properties->GetWidgetName());
    }

  this->PropertiesCreated = 1;

}

//----------------------------------------------------------------------------
void vtkPVData::UpdateProperties()
{
  
  char tmp[350], cmd[1024];
  float bounds[6];
  int i, j, numArrays, numComps;
  vtkFieldData *fieldData;
  vtkPVApplication *pvApp = this->GetPVApplication();  
  vtkDataArray *array;
  char *currentColorBy;
  int currentColorByFound = 0;
  vtkPVWindow *window;

  if (this->PVColorMap)
    {
    float *range = this->PVColorMap->GetScalarRange();
    this->ColorRangeMinEntry->SetValue(range[0], 5);
    this->ColorRangeMaxEntry->SetValue(range[1], 5);
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

  if (this->GetPVSource()->GetHideDisplayPage())
    {
    return;
    }

  window = this->GetPVApplication()->GetMainWindow();

  // Update and time the filter.
  char *str = new char[strlen(this->GetVTKDataTclName()) + 80];
  sprintf(str, "Accept: %s", this->GetVTKDataTclName());
  vtkTimerLog::MarkStartEvent(str);
  pvApp->BroadcastScript("%s Update", this->MapperTclName);
  // Get bounds to time completion (not just triggering) of update.
  this->GetBounds(bounds);
  vtkTimerLog::MarkEndEvent(str);
  delete [] str;

  // Time creation of the LOD
  vtkTimerLog::MarkStartEvent("Create LOD");
  pvApp->BroadcastScript("%s Update", this->LODMapperTclName);
  // Get bounds to time completion (not just triggering) of update.
  this->GetBounds(bounds);
  vtkTimerLog::MarkEndEvent("Create LOD");


  sprintf(tmp, "number of cells: %d", 
	  this->GetNumberOfCells());
  this->NumCellsLabel->SetLabel(tmp);
  sprintf(tmp, "number of points: %d",
          this->GetNumberOfPoints());
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
void vtkPVData::ChangeActorColor(float r, float g, float b)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (this->Mapper->GetScalarVisibility())
    {
    return;
    }

  this->AddTraceEntry("$kw(%s) ChangeActorColor %f %f %f",
                      this->GetTclName(), r, g, b);

  pvApp->BroadcastScript("%s SetColor %f %f %f",
                         this->PropertyTclName, r, g, b);
  this->GetPVRenderView()->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVData::ChangeColorMapToRedBlue()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  this->AddTraceEntry("$kw(%s) ChanceColorMapToRedBlue",
                      this->GetTclName());
  
  pvApp->BroadcastScript("[%s GetLookupTable] SetHueRange 0 0.666667",
                         this->MapperTclName);
  pvApp->BroadcastScript("[%s GetLookupTable] SetSaturationRange 1 1",
                         this->MapperTclName);
  pvApp->BroadcastScript("[%s GetLookupTable] SetValueRange 1 1",
                         this->MapperTclName);
  pvApp->BroadcastScript("[%s GetLookupTable] Build",
                         this->MapperTclName);

  this->GetPVRenderView()->EventuallyRender();
}


//----------------------------------------------------------------------------
void vtkPVData::ChangeColorMapToBlueRed()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  this->AddTraceEntry("$kw(%s) ChanceColorMapToBlueRed",
                      this->GetTclName());

  pvApp->BroadcastScript("[%s GetLookupTable] SetHueRange 0.666667 0",
                         this->MapperTclName);
  pvApp->BroadcastScript("[%s GetLookupTable] SetSaturationRange 1 1",
                         this->MapperTclName);
  pvApp->BroadcastScript("[%s GetLookupTable] SetValueRange 1 1",
                         this->MapperTclName);
  pvApp->BroadcastScript("[%s GetLookupTable] Build",
                         this->MapperTclName);

  this->GetPVRenderView()->EventuallyRender();
}


//----------------------------------------------------------------------------
void vtkPVData::ChangeColorMapToGrayscale()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  this->AddTraceEntry("$kw(%s) ChanceColorMapToGrayscale",
                      this->GetTclName());

  pvApp->BroadcastScript("[%s GetLookupTable] SetHueRange 0 0",
                         this->MapperTclName);
  pvApp->BroadcastScript("[%s GetLookupTable] SetSaturationRange 0 0",
                         this->MapperTclName);
  pvApp->BroadcastScript("[%s GetLookupTable] SetValueRange 0 1",
                         this->MapperTclName);
  pvApp->BroadcastScript("[%s GetLookupTable] Build",
                         this->MapperTclName);

  this->GetPVRenderView()->EventuallyRender();
}


//----------------------------------------------------------------------------
void vtkPVData::SetColorRange(float min, float max)
{
  this->AddTraceEntry("$kw(%s) SetColorRange %f %f", 
                      this->GetTclName(), min, max);

  this->SetColorRangeInternal(min, max);
}

//----------------------------------------------------------------------------
void vtkPVData::SetColorRangeInternal(float min, float max)
{
  if (this->PVColorMap == NULL)
    {
    vtkErrorMacro("Color map is missing.");
    }

  this->PVColorMap->SetScalarRange(min, max);

  this->ColorRangeMinEntry->SetValue(min, 5);
  this->ColorRangeMaxEntry->SetValue(max, 5);
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
  this->GetPVRenderView()->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVData::ColorRangeEntryCallback()
{
  float min, max;

  min = this->ColorRangeMinEntry->GetValueAsFloat();
  max = this->ColorRangeMaxEntry->GetValueAsFloat();
  
  // Avoid the bad range error
  if (max <= min)
    {
    max = min + 0.00001;
    }

  this->SetColorRange(min, max);
  this->GetPVRenderView()->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVData::ColorByProperty()
{
  this->AddTraceEntry("$kw(%s) ColorByProperty", this->GetTclName());
  this->ColorByPropertyInternal();
}

//----------------------------------------------------------------------------
void vtkPVData::ColorByPropertyInternal()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  pvApp->BroadcastScript("%s ScalarVisibilityOff", this->MapperTclName);
  pvApp->BroadcastScript("%s ScalarVisibilityOff", this->LODMapperTclName);
  float *color;
  
  color = this->ColorButton->GetColor();
  pvApp->BroadcastScript("%s SetColor %f %f %f", 
			 this->PropertyTclName, color[0], color[1], color[2]);
  // Add a bit of specular when just coloring by property.
  pvApp->BroadcastScript("%s SetSpecular 0.1", this->PropertyTclName);
  pvApp->BroadcastScript("%s SetSpecularPower 100.0", this->PropertyTclName);
  pvApp->BroadcastScript("%s SetSpecularColor 1.0 1.0 1.0", this->PropertyTclName);
  
  this->SetPVColorMap(NULL);

  this->Script("pack forget %s", this->ScalarBarFrame->GetWidgetName());
  this->Script("pack %s -side left",
               this->ColorButton->GetWidgetName());

  this->GetPVRenderView()->EventuallyRender();
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
  
  this->Script("pack forget %s",
               this->ColorButton->GetWidgetName());
  this->Script("pack %s -after %s -fill x", this->ScalarBarFrame->GetWidgetName(),
               this->ColorFrame->GetWidgetName());

  // Synchronize the UI with the new color map.
  this->UpdateProperties();
  this->GetPVRenderView()->EventuallyRender();
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
  
  this->Script("pack forget %s",
               this->ColorButton->GetWidgetName());
  this->Script("pack %s -after %s -fill x", this->ScalarBarFrame->GetWidgetName(),
               this->ColorFrame->GetWidgetName());

  this->GetPVRenderView()->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVData::DrawWireframe()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  this->AddTraceEntry("$kw(%s) DrawWireframe", this->GetTclName());

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
  
  this->GetPVRenderView()->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVData::DrawPoints()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  this->AddTraceEntry("$kw(%s) DrawPoints", this->GetTclName());

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
  
  this->GetPVRenderView()->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVData::DrawSurface()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  this->AddTraceEntry("$kw(%s) DrawSurface", this->GetTclName());

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
  
  this->GetPVRenderView()->EventuallyRender();
}



//----------------------------------------------------------------------------
void vtkPVData::SetInterpolationToFlat()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  this->AddTraceEntry("$kw(%s) SetInterpolationToFlat", this->GetTclName());

  if (this->PropertyTclName)
    {
    pvApp->BroadcastScript("%s SetInterpolationToFlat",
                           this->PropertyTclName);
    }
  
  this->GetPVRenderView()->EventuallyRender();
}


//----------------------------------------------------------------------------
void vtkPVData::SetInterpolationToGouraud()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  this->AddTraceEntry("$kw(%s) SetInterpolationToGouraud", this->GetTclName());

  if (this->PropertyTclName)
    {
    pvApp->BroadcastScript("%s SetInterpolationToGouraud",
                           this->PropertyTclName);
    }
  
  this->GetPVRenderView()->EventuallyRender();
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
  this->GetPVRenderView()->EventuallyRender(); */
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
  
  if (this->GetVTKData()->IsA("vtkPolyData"))
    {
    pvApp->BroadcastScript("%s SetInput %s",
			       this->GeometryTclName,
			       this->GetVTKDataTclName());
    }
  else
    {
    // Keep the conditional becuase I want to try eliminating the geometry
    // filter with poly data.
    pvApp->BroadcastScript("%s SetInput %s",
			       this->GeometryTclName,
			       this->GetVTKDataTclName());
    }
  
  if (this->LODCollectTclName)
    {
    pvApp->BroadcastScript("%s SetInput [%s GetOutput]",
			   this->LODMapperTclName,
			   this->LODCollectTclName);
    }
  else
    {
    pvApp->BroadcastScript("%s SetInput [%s GetOutput]",
			   this->LODMapperTclName,
			   this->LODDeciTclName);
    }

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
                           static_cast<vtkPVRenderView*>(
			     this->GetView())->GetTriangleStripsCheck()->GetState());
    }
  pvApp->BroadcastScript("%s SetImmediateModeRendering %d",
                         this->MapperTclName,
                         static_cast<vtkPVRenderView*>(
			   this->GetView())->GetImmediateModeCheck()->GetState());
  pvApp->BroadcastScript("%s SetImmediateModeRendering %d", 
			 this->LODMapperTclName,
			 static_cast<vtkPVRenderView*>(
			   this->GetView())->GetImmediateModeCheck()->GetState());

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
  this->GetPVRenderView()->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVData::VisibilityCheckCallback()
{
  this->SetVisibility(this->VisibilityCheck->GetState());
  this->GetPVRenderView()->EventuallyRender();
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
  return vtkPVRenderView::SafeDownCast(this->GetView());
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
    // Here incase this is called from a script.
    this->AddTraceEntry("$kw(%s) SetScalarBarVisibility %d", this->GetTclName(), val);
    }

}

//----------------------------------------------------------------------------
void vtkPVData::SetCubeAxesVisibility(int val)
{
  vtkRenderer *ren;
  char *tclName;
  
  if (!this->GetView())
    {
    return;
    }
  
  ren = this->GetView()->GetRenderer();
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
  this->AddTraceEntry("$kw(%s) SetScalarBarVisibility %d", this->GetTclName(),
                      this->ScalarBarCheck->GetState());
  this->SetScalarBarVisibility(this->ScalarBarCheck->GetState());
  this->GetPVRenderView()->EventuallyRender();  
}

//----------------------------------------------------------------------------
void vtkPVData::CubeAxesCheckCallback()
{
  this->AddTraceEntry("$kw(%s) SetCubeAxesVisibility %d", this->GetTclName(),
                      this->CubeAxesCheck->GetState());
  this->SetCubeAxesVisibility(this->CubeAxesCheck->GetState());
  this->GetPVRenderView()->EventuallyRender();  
}


//----------------------------------------------------------------------------
void vtkPVData::SetPointSize(int size)
{
  this->PointSizeScale->SetValue(size);
  this->ChangePointSize();
}

//----------------------------------------------------------------------------
void vtkPVData::SetLineWidth(int width)
{
  this->LineWidthScale->SetValue(width);
  this->ChangeLineWidth();
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

  this->GetPVRenderView()->EventuallyRender();
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

  this->GetPVRenderView()->EventuallyRender();
}




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
void vtkPVData::SaveInTclScript(ofstream *file)
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
    *file << "vtkPolyDataMapper " << this->MapperTclName << "\n\t"
          << this->MapperTclName << " SetInput ["
          << this->GeometryTclName << " GetOutput]\n\t";  
    *file << this->MapperTclName << " SetImmediateModeRendering "
          << this->Mapper->GetImmediateModeRendering() << "\n\t";
    this->Mapper->GetScalarRange(range);
    *file << this->MapperTclName << " SetScalarRange " << range[0] << " "
          << range[1] << "\n\t";
    *file << this->MapperTclName << " SetScalarVisibility "
          << this->Mapper->GetScalarVisibility() << "\n\t"
          << this->MapperTclName << " SetScalarModeTo";

    scalarMode = this->Mapper->GetScalarModeAsString();
    *file << scalarMode << "\n";
    if (strcmp(scalarMode, "UsePointFieldData") == 0 ||
        strcmp(scalarMode, "UseCellFieldData") == 0)
      {
      *file << "\t" << this->MapperTclName << " ColorByArrayComponent {"
            << this->Mapper->GetArrayName() << "} "
            << this->Mapper->GetArrayComponent() << "\n";
      }
  
    *file << "vtkActor " << this->PropTclName << "\n\t"
          << this->PropTclName << " SetMapper " << this->MapperTclName << "\n\t"
          << "[" << this->PropTclName << " GetProperty] SetRepresentationTo"
          << this->Property->GetRepresentationAsString() << "\n\t"
          << "[" << this->PropTclName << " GetProperty] SetInterpolationTo"
          << this->Property->GetInterpolationAsString() << "\n";

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
  /*
  if (this->ScalarBarCheck->GetState())
    {
    *file << "vtkScalarBarActor " << this->ScalarBarTclName << "\n\t"
          << "[" << this->ScalarBarTclName
          << " GetPositionCoordinate] SetCoordinateSystemToNormalizedViewport\n\t"
          << "[" << this->ScalarBarTclName
          << " GetPositionCoordinate] SetValue ";
    this->Script("set tempResult [[%s GetPositionCoordinate] GetValue]",
                 this->ScalarBarTclName);
    result = this->GetPVApplication()->GetMainInterp()->result;
    sscanf(result, "%f %f", &position[0], &position[1]);
    *file << position[0] << " " << position[1] << "\n\t"
          << this->ScalarBarTclName << " SetOrientationTo";
    this->Script("set tempResult [%s GetOrientation]",
                 this->ScalarBarTclName);
    result = this->GetPVApplication()->GetMainInterp()->result;
    if (strncmp(result, "0", 1) == 0)
      {
      *file << "Horizontal\n\t";
      }
    else
      {
      *file << "Vertical\n\t";
      }
    *file << this->ScalarBarTclName << " SetWidth ";
    this->Script("set tempResult [%s GetWidth]",
                 this->ScalarBarTclName);
    result = this->GetPVApplication()->GetMainInterp()->result;
    *file << result << "\n\t";
    *file << this->ScalarBarTclName << " SetHeight ";
    this->Script("set tempResult [%s GetHeight]",
                 this->ScalarBarTclName);
    result = this->GetPVApplication()->GetMainInterp()->result;
    *file << result << "\n\t"
          << this->ScalarBarTclName << " SetLookupTable ["
          << this->MapperTclName << " GetLookupTable]\n\t"
          << this->ScalarBarTclName << " SetTitle {";
    this->Script("set tempResult [%s GetTitle]",
                 this->ScalarBarTclName);
    result = this->GetPVApplication()->GetMainInterp()->result;
    *file << result << "}\n";
    *file << renTclName << " AddProp " << this->ScalarBarTclName << "\n";
    }
  */

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
  os << indent << "ColorMapMenu: " << this->GetColorMapMenu() << endl;
  os << indent << "ColorMenu: " << this->GetColorMenu() << endl;
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
    os << indent << "PVColorMap: " << this->PVColorMap->GetName() << endl;
    }
  else
    {
    os << indent << "PVColorMap: NULL\n";
    }

  os << indent << "VTKData: " << this->GetVTKData() << endl;
  os << indent << "VTKDataTclName: " << (this->VTKDataTclName?this->VTKDataTclName:"none") << endl;
  os << indent << "View: " << this->GetView() << endl;
  os << indent << "PropertiesCreated: " << this->PropertiesCreated << endl;
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
void vtkPVData::ScalarBarOrientationCallback()
{
  int state = this->ScalarBarOrientationCheck->GetState();
  
  if (this->PVColorMap == NULL)
    {
    vtkErrorMacro("Color map missing.");
    return;
    }

  if (state)
    {
    this->PVColorMap->SetScalarBarOrientationToVertical();
    this->AddTraceEntry("$kw(%s) SetScalarBarOrientationToVertical", this->GetTclName());
    }
  else
    {
    this->PVColorMap->SetScalarBarOrientationToHorizontal();
    this->AddTraceEntry("$kw(%s) SetScalarBarOrientationToHorizontal", this->GetTclName());
    }
  this->GetPVRenderView()->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVData::SetScalarBarOrientationToVertical()
{
  this->ScalarBarOrientationCheck->SetState(1);
  this->ScalarBarOrientationCallback();
}

//----------------------------------------------------------------------------
void vtkPVData::SetScalarBarOrientationToHorizontal()
{
  this->ScalarBarOrientationCheck->SetState(0);
  this->ScalarBarOrientationCallback();
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

  pvApp->BroadcastScript("Application SendDataArrayRange %s %d {%s} %d",
                         this->GetVTKDataTclName(),
                         pointDataFlag, array->GetName(), component);
  
  array->GetRange(range, 0);  

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



