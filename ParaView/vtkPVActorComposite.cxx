/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVActorComposite.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include "vtkPVActorComposite.h"
#include "vtkKWWidget.h"
#include "vtkKWView.h"
#include "vtkObjectFactory.h"
#include "vtkKWWindow.h"
#include "vtkKWCheckButton.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVSource.h"
#include "vtkImageOutlineFilter.h"
#include "vtkGeometryFilter.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkTexture.h"
#include "vtkScalarBarActor.h"
#include "vtkCubeAxesActor2D.h"
#include "vtkTimerLog.h"
#include "vtkPVRenderView.h"
#include "vtkTreeComposite.h"
#include "vtkPVSourceInterface.h"

//----------------------------------------------------------------------------
vtkPVActorComposite* vtkPVActorComposite::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVActorComposite");
  if(ret)
    {
    return (vtkPVActorComposite*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVActorComposite;
}

int vtkPVActorCompositeCommand(ClientData cd, Tcl_Interp *interp,
			       int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVActorComposite::vtkPVActorComposite()
{
  static int instanceCount = 0;
  
  // Create a unique id for creating tcl names.
  ++instanceCount;
  this->InstanceCount = instanceCount;
  
  this->CommandFunction = vtkPVActorCompositeCommand;
  
  this->Properties = vtkKWWidget::New();
  this->Name = NULL;

  this->ScalarBarFrame = vtkKWLabeledFrame::New();
  this->ColorFrame = vtkKWLabeledFrame::New();
  this->DisplayStyleFrame = vtkKWLabeledFrame::New();
  this->StatsFrame = vtkKWWidget::New();
  
  this->NumCellsLabel = vtkKWLabel::New();
  this->NumPointsLabel = vtkKWLabel::New();
  
  this->BoundsLabel = vtkKWLabel::New();
  this->XRangeLabel = vtkKWLabel::New();
  this->YRangeLabel = vtkKWLabel::New();
  this->ZRangeLabel = vtkKWLabel::New();
  
  this->AmbientScale = vtkKWScale::New();

  this->ColorMenuLabel = vtkKWLabel::New();
  this->ColorMenu = vtkKWOptionMenu::New();

  this->ColorMapMenuLabel = vtkKWLabel::New();
  this->ColorMapMenu = vtkKWOptionMenu::New();
  
  this->ColorButton = vtkKWChangeColorButton::New();
  
  this->RepresentationMenuLabel = vtkKWLabel::New();
  this->RepresentationMenu = vtkKWOptionMenu::New();
  
  this->InterpolationMenuLabel = vtkKWLabel::New();
  this->InterpolationMenu = vtkKWOptionMenu::New();
  
  this->ScalarBarCheck = vtkKWCheckButton::New();
  this->ScalarBarOrientationCheck = vtkKWCheckButton::New();
  this->CubeAxesCheck = vtkKWCheckButton::New();

  this->VisibilityCheck = vtkKWCheckButton::New();

  this->ResetCameraButton = vtkKWPushButton::New();
  
  this->PVData = NULL;
  this->DataSetInput = NULL;
  this->Mode = VTK_PV_ACTOR_COMPOSITE_POLY_DATA_MODE;
  
  //this->TextureFilter = NULL;
  
  this->ActorTclName = NULL;
  this->LODDeciTclName = NULL;
  this->MapperTclName = NULL;
  this->LODMapperTclName = NULL;
  this->OutlineTclName = NULL;
  this->GeometryTclName = NULL;
  this->OutputPortTclName = NULL;
  this->AppendPolyDataTclName = NULL;
  
  this->ScalarBarTclName = NULL;

  this->CubeAxesTclName = NULL;
}


//----------------------------------------------------------------------------
void vtkPVActorComposite::CreateParallelTclObjects(vtkPVApplication *pvApp)
{
  int numProcs, id;
  char tclName[100];
  
  this->SetApplication(pvApp);
  
  // Get rid of previous object created by the superclass.
  if (this->Mapper)
    {
    this->Mapper->Delete();
    this->Mapper = NULL;
    }
  // Make a new tcl object.
  sprintf(tclName, "Mapper%d", this->InstanceCount);
  this->Mapper = (vtkPolyDataMapper*)pvApp->MakeTclObject("vtkPolyDataMapper", tclName);
  this->MapperTclName = NULL;
  this->SetMapperTclName(tclName);
  pvApp->BroadcastScript("%s ImmediateModeRenderingOn", this->MapperTclName);
  
  sprintf(tclName, "ScalarBar%d", this->InstanceCount);
  this->SetScalarBarTclName(tclName);
  this->Script("vtkScalarBarActor %s", this->GetScalarBarTclName());
  this->Script("[%s GetPositionCoordinate] SetCoordinateSystemToNormalizedViewport",
               this->GetScalarBarTclName());
  this->Script("[%s GetPositionCoordinate] SetValue 0.87 0.25",
               this->GetScalarBarTclName());
  this->Script("%s SetOrientationToVertical",
               this->GetScalarBarTclName());
  this->Script("%s SetWidth 0.13", this->GetScalarBarTclName());
  this->Script("%s SetHeight 0.5", this->GetScalarBarTclName());
  
  this->Script("%s SetLookupTable [%s GetLookupTable]",
               this->GetScalarBarTclName(), this->MapperTclName);
  
  sprintf(tclName, "LODDeci%d", this->InstanceCount);
  pvApp->MakeTclObject("vtkQuadricClustering", tclName);
  this->LODDeciTclName = NULL;
  this->SetLODDeciTclName(tclName);
  pvApp->BroadcastScript("%s UseInputPointsOn", this->LODDeciTclName);
  pvApp->BroadcastScript("%s UseFeatureEdgesOn", this->LODDeciTclName);
  pvApp->BroadcastScript("%s UseFeaturePointsOn", this->LODDeciTclName);
  sprintf(tclName, "LODMapper%d", this->InstanceCount);
  pvApp->MakeTclObject("vtkPolyDataMapper", tclName);
  this->LODMapperTclName = NULL;
  this->SetLODMapperTclName(tclName);

  this->Script("%s SetLookupTable [%s GetLookupTable]",
               this->GetScalarBarTclName(), this->MapperTclName);
  pvApp->BroadcastScript("%s SetLookupTable [%s GetLookupTable]", 
                         this->LODMapperTclName, this->MapperTclName);
 
  // Get rid of previous object created by the superclass.
  if (this->Actor)
    {
    this->Actor->Delete();
    this->Actor = NULL;
    }
  // Make a new tcl object.
  sprintf(tclName, "Actor%d", this->InstanceCount);
//  this->Actor = (vtkActor*)pvApp->MakeTclObject("vtkActor", tclName);
  this->Actor = (vtkActor*)pvApp->MakeTclObject("vtkLODActor", tclName);
  this->ActorTclName = NULL;
  this->SetActorTclName(tclName);
  
  pvApp->BroadcastScript("%s SetMapper %s", this->ActorTclName, 
			this->MapperTclName);
  pvApp->BroadcastScript("%s AddLODMapper %s", this->ActorTclName,
			 this->LODMapperTclName);
  
  // Hard code assignment based on processes.
  numProcs = pvApp->GetController()->GetNumberOfProcesses() ;
  for (id = 0; id < numProcs; ++id)
    {
    pvApp->RemoteScript(id, "%s SetNumberOfPieces %d",
			this->MapperTclName, numProcs);
    pvApp->RemoteScript(id, "%s SetPiece %d", this->MapperTclName, id);
    pvApp->RemoteScript(id, "%s SetNumberOfPieces %d",
			this->LODMapperTclName, numProcs);
    pvApp->RemoteScript(id, "%s SetPiece %d", this->LODMapperTclName, id);
    }
}

//----------------------------------------------------------------------------
vtkPVActorComposite::~vtkPVActorComposite()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  this->Properties->Delete();
  this->Properties = NULL;
  
  this->SetName(NULL);
  
  this->NumCellsLabel->Delete();
  this->NumCellsLabel = NULL;
  
  this->NumPointsLabel->Delete();
  this->NumPointsLabel = NULL;
  
  this->BoundsLabel->Delete();
  this->BoundsLabel = NULL;
  this->XRangeLabel->Delete();
  this->XRangeLabel = NULL;
  this->YRangeLabel->Delete();
  this->YRangeLabel = NULL;
  this->ZRangeLabel->Delete();
  this->ZRangeLabel = NULL;
  
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
  
  this->RepresentationMenuLabel->Delete();
  this->RepresentationMenuLabel = NULL;  
  this->RepresentationMenu->Delete();
  this->RepresentationMenu = NULL;
  
  this->InterpolationMenuLabel->Delete();
  this->InterpolationMenuLabel = NULL;
  this->InterpolationMenu->Delete();
  this->InterpolationMenu = NULL;
  
  this->SetInput(NULL);
    
  if (this->ScalarBarTclName)
    {
    pvApp->Script("%s Delete", this->ScalarBarTclName);
    this->SetScalarBarTclName(NULL);
    }
  
  if (this->CubeAxesTclName)
    {
    pvApp->Script("%s Delete", this->CubeAxesTclName);
    this->SetCubeAxesTclName(NULL);
    }
  
  pvApp->BroadcastScript("%s Delete", this->MapperTclName);
  this->SetMapperTclName(NULL);
  this->Mapper = NULL;
  
  pvApp->BroadcastScript("%s Delete", this->LODMapperTclName);
  this->SetLODMapperTclName(NULL);

  pvApp->BroadcastScript("%s Delete", this->ActorTclName);
  this->SetActorTclName(NULL);
  this->Actor = NULL;

  pvApp->BroadcastScript("%s Delete", this->LODDeciTclName);
  this->SetLODDeciTclName(NULL);
  
  if (this->OutputPortTclName)
    {
    pvApp->BroadcastScript("%s Delete", this->OutputPortTclName);
    this->SetOutputPortTclName(NULL);
    }

  if (this->AppendPolyDataTclName)
    {
    pvApp->BroadcastScript("%s Delete", this->AppendPolyDataTclName);
    this->SetAppendPolyDataTclName(NULL);
    }
  
  if (this->LODDeciTclName)
    {
    pvApp->BroadcastScript("%s Delete", this->LODDeciTclName);
    this->SetLODDeciTclName(NULL);
    }
  
  this->ScalarBarCheck->Delete();
  this->ScalarBarCheck = NULL;
  
  this->ScalarBarOrientationCheck->Delete();
  this->ScalarBarOrientationCheck = NULL;
  
  this->CubeAxesCheck->Delete();
  this->CubeAxesCheck = NULL;
  
  this->VisibilityCheck->Delete();
  this->VisibilityCheck = NULL;
  
  if (this->OutlineTclName)
    {
    pvApp->BroadcastScript("%s Delete", this->OutlineTclName);
    this->SetOutlineTclName(NULL);
    }  
  if (this->GeometryTclName)
    {
    pvApp->BroadcastScript("%s Delete", this->GeometryTclName);
    this->SetGeometryTclName(NULL);
    }

  this->ScalarBarFrame->Delete();
  this->ScalarBarFrame = NULL;
  this->ColorFrame->Delete();
  this->ColorFrame = NULL;
  this->DisplayStyleFrame->Delete();
  this->DisplayStyleFrame = NULL;
  this->StatsFrame->Delete();
  this->StatsFrame = NULL;
  
  this->ResetCameraButton->Delete();
  this->ResetCameraButton = NULL;
}


//----------------------------------------------------------------------------
void vtkPVActorComposite::CreateProperties()
{
  vtkPVApplication *pvApp = this->GetPVApplication();  
    
  this->Properties->SetParent(this->GetPVData()->GetPVSource()->GetNotebook()->GetFrame("Display"));
  this->Properties->Create(this->Application, "frame","");
  this->Script("pack %s -pady 2 -fill x -expand yes",
               this->Properties->GetWidgetName());
 
  this->ScalarBarFrame->SetParent(this->Properties);
  this->ScalarBarFrame->Create(this->Application);
  this->ScalarBarFrame->SetLabel("Scalar Bar");
  this->ColorFrame->SetParent(this->Properties);
  this->ColorFrame->Create(this->Application);
  this->ColorFrame->SetLabel("Color");
  this->DisplayStyleFrame->SetParent(this->Properties);
  this->DisplayStyleFrame->Create(this->Application);
  this->DisplayStyleFrame->SetLabel("Display Style");
  this->StatsFrame->SetParent(this->Properties);
  this->StatsFrame->Create(this->Application, "frame", "");
 
  this->NumCellsLabel->SetParent(this->StatsFrame);
  this->NumCellsLabel->Create(this->Application, "");
  this->NumPointsLabel->SetParent(this->StatsFrame);
  this->NumPointsLabel->Create(this->Application, "");
  
  this->BoundsLabel->SetParent(this->Properties);
  this->BoundsLabel->Create(this->Application, "");
  this->BoundsLabel->SetLabel("bounds:");
  this->XRangeLabel->SetParent(this->Properties);
  this->XRangeLabel->Create(this->Application, "");
  this->YRangeLabel->SetParent(this->Properties);
  this->YRangeLabel->Create(this->Application, "");
  this->ZRangeLabel->SetParent(this->Properties);
  this->ZRangeLabel->Create(this->Application, "");
  
  this->AmbientScale->SetParent(this->Properties);
  this->AmbientScale->Create(this->Application, "-showvalue 1");
  this->AmbientScale->DisplayLabel("Ambient Light");
  this->AmbientScale->SetRange(0.0, 1.0);
  this->AmbientScale->SetResolution(0.1);
  this->AmbientScale->SetCommand(this, "AmbientChanged");
  
  this->ColorMenuLabel->SetParent(this->ColorFrame->GetFrame());
  this->ColorMenuLabel->Create(this->Application, "");
  this->ColorMenuLabel->SetLabel("Color by variable:");
  
  this->ColorMenu->SetParent(this->ColorFrame->GetFrame());
  this->ColorMenu->Create(this->Application, "");    

  this->ColorButton->SetParent(this->ColorFrame->GetFrame());
  this->ColorButton->Create(this->Application, "");
  this->ColorButton->SetText("Actor Color");
  this->ColorButton->SetCommand(this, "ChangeActorColor");
  
  this->ColorMapMenuLabel->SetParent(this->ScalarBarFrame->GetFrame());
  this->ColorMapMenuLabel->Create(this->Application, "");
  this->ColorMapMenuLabel->SetLabel("Color map:");
  
  this->ColorMapMenu->SetParent(this->ScalarBarFrame->GetFrame());
  this->ColorMapMenu->Create(this->Application, "");
  this->ColorMapMenu->AddEntryWithCommand("Red to Blue", this,
                                          "ChangeColorMap");
  this->ColorMapMenu->AddEntryWithCommand("Blue to Red", this,
                                          "ChangeColorMap");
  this->ColorMapMenu->AddEntryWithCommand("Grayscale", this,
                                          "ChangeColorMap");
  this->ColorMapMenu->SetValue("Red to Blue");
  
  this->RepresentationMenuLabel->SetParent(this->DisplayStyleFrame->GetFrame());
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
  
  this->ScalarBarCheck->SetParent(this->ScalarBarFrame->GetFrame());
  this->ScalarBarCheck->Create(this->Application, "-text Visibility");
  this->Application->Script("%s configure -command {%s ScalarBarCheckCallback}",
                            this->ScalarBarCheck->GetWidgetName(),
                            this->GetTclName());

  this->ScalarBarOrientationCheck->SetParent(this->ScalarBarFrame->GetFrame());
  this->ScalarBarOrientationCheck->Create(this->Application, "-text Vertical");
  this->ScalarBarOrientationCheck->SetState(1);
  this->ScalarBarOrientationCheck->SetCommand(this, "ScalarBarOrientationCallback");
  
  this->CubeAxesCheck->SetParent(this->Properties);
  this->CubeAxesCheck->Create(this->Application, "-text CubeAxes");
  this->CubeAxesCheck->SetCommand(this, "CubeAxesCheckCallback");
  
  this->VisibilityCheck->SetParent(this->Properties);
  this->VisibilityCheck->Create(this->Application, "-text Visibility");
  this->Application->Script("%s configure -command {%s VisibilityCheckCallback}",
                            this->VisibilityCheck->GetWidgetName(),
                            this->GetTclName());
  this->VisibilityCheck->SetState(1);

  this->ResetCameraButton->SetParent(this->Properties);
  this->ResetCameraButton->Create(this->Application, "");
  this->ResetCameraButton->SetLabel("Reset Camera");
  this->ResetCameraButton->SetCommand(this, "CenterCamera");
  
  this->Script("pack %s", this->StatsFrame->GetWidgetName());
  this->Script("pack %s %s -side left",
	       this->NumCellsLabel->GetWidgetName(),
               this->NumPointsLabel->GetWidgetName());
  this->Script("pack %s",
	       this->BoundsLabel->GetWidgetName());
  this->Script("pack %s",
	       this->XRangeLabel->GetWidgetName());
  this->Script("pack %s",
	       this->YRangeLabel->GetWidgetName());
  this->Script("pack %s",
	       this->ZRangeLabel->GetWidgetName());
  this->Script("pack %s -fill x", this->ColorFrame->GetWidgetName());
  this->Script("pack %s %s %s -side left",
	       this->ColorMenuLabel->GetWidgetName(),
               this->ColorMenu->GetWidgetName(),
               this->ColorButton->GetWidgetName());
  this->Script("pack %s -fill x", this->ScalarBarFrame->GetWidgetName());
  this->Script("pack %s %s %s %s -side left",
               this->ScalarBarCheck->GetWidgetName(),
               this->ScalarBarOrientationCheck->GetWidgetName(),
               this->ColorMapMenuLabel->GetWidgetName(),
               this->ColorMapMenu->GetWidgetName());
  this->Script("pack %s -fill x", this->DisplayStyleFrame->GetWidgetName());
  this->Script("pack %s %s %s %s -side left",
               this->RepresentationMenuLabel->GetWidgetName(),
               this->RepresentationMenu->GetWidgetName(),
               this->InterpolationMenuLabel->GetWidgetName(),
               this->InterpolationMenu->GetWidgetName());
  this->Script("pack %s",
               this->CubeAxesCheck->GetWidgetName());
  this->Script("pack %s",
               this->VisibilityCheck->GetWidgetName());
  this->Script("pack %s",
               this->ResetCameraButton->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::UpdateProperties()
{
  char tmp[350], cmd[1024];
  float bounds[6];
  int i, j, numArrays, numComps;
  vtkFieldData *fieldData;
  vtkPVApplication *pvApp = this->GetPVApplication();  
  vtkDataArray *array;
  char *currentColorBy;
  int currentColorByFound = 0;

  
  if (this->UpdateTime > this->PVData->GetVTKData()->GetMTime())
    {
    return;
    }
  this->UpdateTime.Modified();

  vtkDebugMacro( << "Start timer");
  vtkTimerLog *timer = vtkTimerLog::New();
  timer->StartTimer();
  pvApp->BroadcastScript("%s Update", this->MapperTclName);
  pvApp->BroadcastScript("%s Update", this->LODMapperTclName);
  this->GetPVData()->GetBounds(bounds);
  timer->StopTimer();
  vtkDebugMacro(<< "Stop timer : " << this->PVData->GetVTKDataTclName() << " : took " 
                  << timer->GetElapsedTime() << " seconds.");
  timer->Delete();
  
  
  sprintf(tmp, "number of cells: %d", 
	  this->GetPVData()->GetNumberOfCells());
  this->NumCellsLabel->SetLabel(tmp);
  sprintf(tmp, "number of points: %d",
          this->GetPVData()->GetNumberOfPoints());
  this->NumPointsLabel->SetLabel(tmp);
  
  sprintf(tmp, "x range: %f to %f", bounds[0], bounds[1]);
  this->XRangeLabel->SetLabel(tmp);
  sprintf(tmp, "y range: %f to %f", bounds[2], bounds[3]);
  this->YRangeLabel->SetLabel(tmp);
  sprintf(tmp, "z range: %f to %f", bounds[4], bounds[5]);
  this->ZRangeLabel->SetLabel(tmp);
  
    
  this->AmbientScale->SetValue(this->GetActor()->GetProperty()->GetAmbient());


  currentColorBy = this->ColorMenu->GetValue();
  this->ColorMenu->ClearEntries();
  this->ColorMenu->AddEntryWithCommand("Property",
	                               this, "ColorByProperty");
  fieldData = this->GetPVData()->GetVTKData()->GetPointData()->GetFieldData();
  if (fieldData)
    {
    numArrays = fieldData->GetNumberOfArrays();
    for (i = 0; i < numArrays; i++)
      {
      array = fieldData->GetArray(i);
      numComps = array->GetNumberOfComponents();
      for (j = 0; j < numComps; ++j)
        {
        sprintf(cmd, "ColorByPointFieldComponent %s %d",
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
  fieldData = this->GetPVData()->GetVTKData()->GetCellData()->GetFieldData();
  if (fieldData)
    {
    numArrays = fieldData->GetNumberOfArrays();
    for (i = 0; i < numArrays; i++)
      {
      array = fieldData->GetArray(i);
      numComps = array->GetNumberOfComponents();
      for (j = 0; j < numComps; ++j)
        {
        sprintf(cmd, "ColorByCellFieldComponent %s %d",
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
  // If the current array we are colloring by has disappeared,
  // then default back to the property.
  if ( ! currentColorByFound)
    {
    this->ColorMenu->SetValue("Property");
    this->ColorByProperty();
    }
}

void vtkPVActorComposite::ChangeActorColor(float r, float g, float b)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (this->Mapper->GetScalarVisibility())
    {
    return;
    }
  
  pvApp->BroadcastScript("[%s GetProperty] SetColor %f %f %f",
                         this->ActorTclName, r, g, b);
  this->GetPVRenderView()->EventuallyRender();
}

void vtkPVActorComposite::ChangeColorMap()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (strcmp(this->ColorMapMenu->GetValue(), "Red to Blue") == 0)
    {
    pvApp->BroadcastScript("[%s GetLookupTable] SetHueRange 0 0.666667",
                           this->MapperTclName);
    pvApp->BroadcastScript("[%s GetLookupTable] SetSaturationRange 1 1",
                           this->MapperTclName);
    pvApp->BroadcastScript("[%s GetLookupTable] SetValueRange 1 1",
                           this->MapperTclName);
    }
  else if (strcmp(this->ColorMapMenu->GetValue(), "Blue to Red") == 0)
    {
    pvApp->BroadcastScript("[%s GetLookupTable] SetHueRange 0.666667 0",
                           this->MapperTclName);
    pvApp->BroadcastScript("[%s GetLookupTable] SetSaturationRange 1 1",
                           this->MapperTclName);
    pvApp->BroadcastScript("[%s GetLookupTable] SetValueRange 1 1",
                           this->MapperTclName);
    }
  else
    {
    pvApp->BroadcastScript("[%s GetLookupTable] SetHueRange 0 0",
                           this->MapperTclName);
    pvApp->BroadcastScript("[%s GetLookupTable] SetSaturationRange 0 0",
                           this->MapperTclName);
    pvApp->BroadcastScript("[%s GetLookupTable] SetValueRange 0 1",
                           this->MapperTclName);
    }
  
  this->GetPVRenderView()->EventuallyRender();
}

void vtkPVActorComposite::ResetColorRange()
{
  float range[2];
  this->GetColorRange(range);
  
  // Avoid the bad range error
  if (range[1] <= range[0])
    {
    range[1] = range[0] + 0.000001;
    }

  this->GetPVApplication()->BroadcastScript("%s SetScalarRange %f %f",
					    this->MapperTclName,
					    range[0], range[1]);
  this->GetPVApplication()->BroadcastScript("%s SetScalarRange %f %f",
					    this->LODMapperTclName,
					    range[0], range[1]);
  this->GetPVRenderView()->EventuallyRender();
}

void vtkPVActorComposite::GetColorRange(float range[2])
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkMultiProcessController *controller = pvApp->GetController();
  float tmp[2];
  int id, num;
  
  if (this->Mapper->GetColors() == NULL)
    {
    range[0] = 0.0;
    range[1] = 1.0;
    return;
    }

  pvApp->BroadcastScript("Application SendMapperColorRange %s", 
			 this->MapperTclName);
  
  this->Mapper->GetColors()->GetRange(range);
  
  num = controller->GetNumberOfProcesses();
  for (id = 1; id < num; ++id)
    {
    controller->Receive(tmp, 2, id, 1969);
    if (tmp[0] < range[0])
      {
      range[0] = tmp[0];
      }
    if (tmp[1] > range[1])
      {
      range[1] = tmp[1];
      }
    }
  
  if (range[0] > range[1])
    {
    range[0] = 0.0;
    range[1] = 1.0;
    }
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::ColorByProperty()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  pvApp->BroadcastScript("%s ScalarVisibilityOff", this->MapperTclName);
  float *color;
  
  color = this->ColorButton->GetColor();
  pvApp->BroadcastScript("[%s GetProperty] SetColor %f %f %f", 
			 this->ActorTclName, color[0], color[1], color[2]);
  
  // No scalars visible.  Turn off scalar bar.
  this->SetScalarBarVisibility(0);

  this->GetPVRenderView()->EventuallyRender();
}


//----------------------------------------------------------------------------
void vtkPVActorComposite::ColorByPointFieldComponent(char *name, int comp)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  pvApp->BroadcastScript("%s ScalarVisibilityOn", this->MapperTclName);
  pvApp->BroadcastScript("%s SetScalarModeToUsePointFieldData",
                         this->MapperTclName);
  pvApp->BroadcastScript("%s ColorByArrayComponent %s %d",
                         this->MapperTclName, name, comp);

  pvApp->BroadcastScript("%s ScalarVisibilityOn", this->LODMapperTclName);
  pvApp->BroadcastScript("%s SetScalarModeToUsePointFieldData",
                         this->LODMapperTclName);
  pvApp->BroadcastScript("%s ColorByArrayComponent %s %d",
                         this->LODMapperTclName, name, comp);

  this->Script("%s SetTitle %s", this->GetScalarBarTclName(), name);
  
  this->ResetColorRange();
  this->GetPVRenderView()->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::ColorByCellFieldComponent(char *name, int comp)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  pvApp->BroadcastScript("%s ScalarVisibilityOn", this->MapperTclName);
  pvApp->BroadcastScript("%s SetScalarModeToUseCellFieldData",
                         this->MapperTclName);
  pvApp->BroadcastScript("%s ColorByArrayComponent %s %d",
                         this->MapperTclName, name, comp);

  pvApp->BroadcastScript("%s ScalarVisibilityOn", this->LODMapperTclName);
  pvApp->BroadcastScript("%s SetScalarModeToUseCellFieldData",
                         this->LODMapperTclName);
  pvApp->BroadcastScript("%s ColorByArrayComponent %s %d",
                         this->LODMapperTclName, name, comp);

  this->Script("%s SetTitle %s", this->GetScalarBarTclName(), name);
  
  this->ResetColorRange();
  this->GetPVRenderView()->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::DrawWireframe()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (this->ActorTclName)
    {
    pvApp->BroadcastScript("[%s GetProperty] SetRepresentationToWireframe",
			   this->ActorTclName);
    }
  
  //this->GetActor()->GetProperty()->SetRepresentationToWireframe();
  this->GetPVRenderView()->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::DrawSurface()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (this->ActorTclName)
    {
    pvApp->BroadcastScript("[%s GetProperty] SetRepresentationToSurface",
			   this->ActorTclName);
    }
  
  //this->GetActor()->GetProperty()->SetRepresentationToSurface();
  this->GetPVRenderView()->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::DrawPoints()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (this->ActorTclName)
    {
    pvApp->BroadcastScript("[%s GetProperty] SetRepresentationToPoints",
			   this->ActorTclName);
    }
  
  //this->GetActor()->GetProperty()->SetRepresentationToPoints();
  this->GetPVRenderView()->EventuallyRender();
}


//----------------------------------------------------------------------------
void vtkPVActorComposite::SetInterpolationToFlat()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (this->ActorTclName)
    {
    pvApp->BroadcastScript("[%s GetProperty] SetInterpolationToFlat",
			   this->ActorTclName);
    }
  
  //this->GetActor()->GetProperty()->SetRepresentationToWireframe();
  this->GetPVRenderView()->EventuallyRender();
}


//----------------------------------------------------------------------------
void vtkPVActorComposite::SetInterpolationToGouraud()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (this->ActorTclName)
    {
    pvApp->BroadcastScript("[%s GetProperty] SetInterpolationToGouraud",
			   this->ActorTclName);
    }
  
  //this->GetActor()->GetProperty()->SetRepresentationToWireframe();
  this->GetPVRenderView()->EventuallyRender();
}



//----------------------------------------------------------------------------
void vtkPVActorComposite::AmbientChanged()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  float ambient = this->AmbientScale->GetValue();
  
  //pvApp->BroadcastScript("%s SetAmbient %f", this->GetTclName(), ambient);

  
  this->SetAmbient(ambient);
  this->GetPVRenderView()->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::SetAmbient(float ambient)
{
  this->GetActor()->GetProperty()->SetAmbient(ambient);
}


//----------------------------------------------------------------------------
void vtkPVActorComposite::Initialize()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  float bounds[6];
  vtkDataArray *array;
  char *tclName;
  char newTclName[100];
  
  if (this->PVData->GetVTKData()->IsA("vtkPolyData"))
    {
    this->SetModeToPolyData();
    pvApp->BroadcastScript("%s SetInput %s",
			   this->LODDeciTclName,
			   this->PVData->GetVTKDataTclName());
    }
  else if (this->PVData->GetVTKData()->IsA("vtkImageData"))
    {
    int *ext;
    this->PVData->GetVTKData()->UpdateInformation();
    ext = this->PVData->GetVTKData()->GetWholeExtent();
    if (ext[1] > ext[0] && ext[3] > ext[2] && ext[5] > ext[4])
      {
      this->SetModeToImageOutline();
      }
    else
      {
      this->SetModeToDataSet();
      }      
    }
  else 
    {
    this->SetModeToDataSet();
    }
  
  pvApp->BroadcastScript("%s SetInput [%s GetOutput]",
			 this->LODMapperTclName,
			 this->LODDeciTclName);

  vtkDebugMacro( << "Initialize --------")
  this->UpdateProperties();
  
  this->PVData->GetBounds(bounds);
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
  
  sprintf(newTclName, "");
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
  
  if (array = this->PVData->GetVTKData()->GetPointData()->GetActiveScalars())
    {
    char *arrayName = (char*)array->GetName();
    char tmp[350];
    sprintf(tmp, "Point %s", arrayName);
    this->ColorByPointFieldComponent(arrayName, 0);
    this->ColorMenu->SetValue(tmp);
    }
  else
    {
    this->ColorByProperty();
    this->ColorMenu->SetValue("Property");
    }
}

//----------------------------------------------------------------------------
// No reference counting because the PVData owns this actor composite.
// In fact is it the same object.
void vtkPVActorComposite::SetInput(vtkPVData *data)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  char *vtkDataTclName = NULL;
  
  if (this->PVData == data)
    {
    return;
    }
  this->Modified();
  
  this->PVData = data;
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::SetScalarRange(float min, float max) 
{ 
  vtkPVApplication *pvApp = this->GetPVApplication();

  // Avoid the bad range error
  if (max <= min)
    {
    max = min + 0.000001;
    }


  pvApp->BroadcastScript("%s SetScalarRange %f %f", this->MapperTclName,
			 min, max);
  pvApp->BroadcastScript("%s SetScalarRange %f %f", this->LODMapperTclName,
			 min, max);
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::SetName (const char* arg) 
{ 
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " 
                << this->Name << " to " << arg ); 
  if ( this->Name && arg && (!strcmp(this->Name,arg))) 
    { 
    return;
    } 
  if (this->Name) 
    { 
    delete [] this->Name; 
    } 
  if (arg) 
    { 
    this->Name = new char[strlen(arg)+1]; 
    strcpy(this->Name,arg); 
    } 
  else 
    { 
    this->Name = NULL;
    }
  this->Modified(); 
} 

//----------------------------------------------------------------------------
char* vtkPVActorComposite::GetName()
{
  return this->Name;
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::Select(vtkKWView *v)
{
  // invoke super
  this->vtkKWComposite::Select(v); 
  
  this->UpdateProperties();
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::Deselect(vtkKWView *v)
{
  this->SetScalarBarVisibility(0);
  this->SetCubeAxesVisibility(0);

  // invoke super
  this->vtkKWComposite::Deselect(v);

  this->Script("pack forget %s", this->Notebook->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::CenterCamera()
{
  float bounds[6];
  vtkPVApplication *pvApp = this->GetPVApplication();
  char* tclName;
  
  tclName = this->GetPVRenderView()->GetRendererTclName();
  this->GetPVData()->GetBounds(bounds);
  pvApp->BroadcastScript("%s ResetCamera %f %f %f %f %f %f",
                         tclName, bounds[0], bounds[1], bounds[2],
                         bounds[3], bounds[4], bounds[5]);
  this->GetPVRenderView()->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::VisibilityCheckCallback()
{
  this->SetVisibility(this->VisibilityCheck->GetState());
  this->GetPVRenderView()->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::SetVisibility(int v)
{
  vtkPVApplication *pvApp;
  pvApp = (vtkPVApplication*)(this->Application);
  if (this->ActorTclName)
    {
    pvApp->BroadcastScript("%s SetVisibility %d", this->ActorTclName, v);
    }
  if (v == 0 && this->GeometryTclName)
    {
    pvApp->BroadcastScript("[%s GetInput] ReleaseData", this->MapperTclName);
    }
}
  
//----------------------------------------------------------------------------
int vtkPVActorComposite::GetVisibility()
{
  vtkProp *p = this->GetProp();
  
  if (p == NULL)
    {
    return 0;
    }
  
  return p->GetVisibility();
}

//----------------------------------------------------------------------------
vtkPVRenderView* vtkPVActorComposite::GetPVRenderView()
{
  return vtkPVRenderView::SafeDownCast(this->GetView());
}


//----------------------------------------------------------------------------
vtkPVApplication* vtkPVActorComposite::GetPVApplication()
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
void vtkPVActorComposite::SetMode(int mode)
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

  if (mode == VTK_PV_ACTOR_COMPOSITE_POLY_DATA_MODE)
    {
    pvApp->BroadcastScript("%s SetInput %s", this->MapperTclName, 
			   this->PVData->GetVTKDataTclName());
    }
  else if (mode == VTK_PV_ACTOR_COMPOSITE_IMAGE_OUTLINE_MODE)
    {
    if (this->OutlineTclName == NULL)
      {
      char tclName[150];
      sprintf(tclName, "ActorCompositeOutline%d", this->InstanceCount);
      pvApp->MakeTclObject("vtkImageOutlineFilter", tclName);
      this->SetOutlineTclName(tclName);
      }
    pvApp->BroadcastScript("%s SetInput %s", this->OutlineTclName, 
			   this->PVData->GetVTKDataTclName());
    pvApp->BroadcastScript("%s SetInput [%s GetOutput]", 
			   this->MapperTclName, this->OutlineTclName);
    pvApp->BroadcastScript("%s SetInput [%s GetOutput]",
			   this->LODDeciTclName,
			   this->OutlineTclName);
    }
  else if (mode == VTK_PV_ACTOR_COMPOSITE_DATA_SET_MODE)
    {
    if (this->GeometryTclName == NULL)
      {
      char tclName[150];
      sprintf(tclName, "ActorCompositeGeometry%d", this->InstanceCount);
      pvApp->MakeTclObject("vtkDataSetSurfaceFilter", tclName);
      this->SetGeometryTclName(tclName);
      }
    pvApp->BroadcastScript("%s SetInput %s", this->GeometryTclName,
			   this->PVData->GetVTKDataTclName());
    pvApp->BroadcastScript("%s SetInput [%s GetOutput]", 
			   this->MapperTclName, this->GeometryTclName);    
    pvApp->BroadcastScript("%s SetInput [%s GetOutput]",
			   this->LODDeciTclName,
			   this->GeometryTclName);
    }
  
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::SetScalarBarVisibility(int val)
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
  
  if (this->ScalarBarCheck->GetState() != val)
    {
    this->ScalarBarCheck->SetState(0);
    }

  // I am going to add and remove it from the renderer instead of using visibility.
  // Composites should really have multiple props.
  
  if (ren)
    {
    if (val)
      {
      this->Script("%s AddActor %s", tclName, this->GetScalarBarTclName());
      }
    else
      {
      this->Script("%s RemoveActor %s", tclName, this->GetScalarBarTclName());
      }
    }
}

void vtkPVActorComposite::SetCubeAxesVisibility(int val)
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
    this->CubeAxesCheck->SetState(0);
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
void vtkPVActorComposite::ScalarBarCheckCallback()
{
  this->SetScalarBarVisibility(this->ScalarBarCheck->GetState());
  this->GetPVRenderView()->EventuallyRender();  
}

void vtkPVActorComposite::CubeAxesCheckCallback()
{
  this->SetCubeAxesVisibility(this->CubeAxesCheck->GetState());
  this->GetPVRenderView()->EventuallyRender();  
}

void vtkPVActorComposite::ScalarBarOrientationCallback()
{
  int state = this->ScalarBarOrientationCheck->GetState();
  
  if (state)
    {
    this->Script("[%s GetPositionCoordinate] SetValue 0.87 0.25",
                 this->GetScalarBarTclName());
    this->Script("%s SetOrietationToVertical", this->GetScalarBarTclName());
    this->Script("%s SetHeight 0.5", this->GetScalarBarTclName());
    this->Script("%s SetWidth 0.13", this->GetScalarBarTclName());
    }
  else
    {
    this->Script("[%s GetPositionCoordinate] SetValue 0.25 0.13",
                 this->GetScalarBarTclName());
    this->Script("%s SetOrientationToHorizontal", this->GetScalarBarTclName());
    this->Script("%s SetHeight 0.13", this->GetScalarBarTclName());
    this->Script("%s SetWidth 0.5", this->GetScalarBarTclName());
    }
  this->GetPVRenderView()->EventuallyRender();
}

void vtkPVActorComposite::Save(ofstream *file, const char *sourceName)
{
  char* charFound;
  int pos;
  float range[2], position[2];
  const char* scalarMode;
  static int readerNum = -1;
  static int outputNum;
  int newReaderNum;
  char* result;
  char* tclName;
  char* dataTclName;
  
  if (this->Mode == VTK_PV_ACTOR_COMPOSITE_IMAGE_OUTLINE_MODE)
    {
    *file << "vtkImageOutlineFilter " << this->OutlineTclName << "\n\t"
          << this->OutlineTclName << " SetInput [" << sourceName
          << " GetOutput";
    if (strcmp(this->GetPVData()->GetPVSource()->GetInterface()->
               GetSourceClassName(), "vtkGenericEnSightReader") == 0)
      {
      dataTclName = this->GetPVData()->GetVTKDataTclName();
      charFound = strrchr(dataTclName, 'O');
      pos = charFound - dataTclName - 1;
      newReaderNum = atoi(dataTclName + pos);
      if (newReaderNum != readerNum)
        {
        readerNum = newReaderNum;
        outputNum = 0;
        }
      else
        {
        outputNum++;
        }
      *file << " " << outputNum << "]\n\n";
      }
    else
      {
      *file << "]\n\n";
      }
    
    *file << "vtkPolyDataMapper " << this->MapperTclName << "\n\t"
          << this->MapperTclName << " SetInput ["
          << this->OutlineTclName << " GetOutput]\n\t";
    }
  else if (this->Mode == VTK_PV_ACTOR_COMPOSITE_DATA_SET_MODE)
    {
    *file << "vtkDataSetSurfaceFilter " << this->GeometryTclName << "\n\t"
          << this->GeometryTclName << " SetInput [" << sourceName
          << " GetOutput";
    if (strcmp(this->GetPVData()->GetPVSource()->GetInterface()->
               GetSourceClassName(), "vtkGenericEnSightReader") == 0)
      {
      dataTclName = this->GetPVData()->GetVTKDataTclName();
      charFound = strrchr(dataTclName, 'O');
      pos = charFound - dataTclName - 1;
      newReaderNum = atoi(dataTclName + pos);
      if (newReaderNum != readerNum)
        {
        readerNum = newReaderNum;
        outputNum = 0;
        }
      else
        {
        outputNum++;
        }
      *file << " " << outputNum << "]\n\n";
      }
    else
      {
      *file << "]\n\n";
      }
    
    *file << "vtkPolyDataMapper " << this->MapperTclName << "\n\t"
          << this->MapperTclName << " SetInput ["
          << this->GeometryTclName << " GetOutput]\n\t";
    }
  else
    {
    *file << "vtkPolyDataMapper " << this->MapperTclName << "\n\t"
          << this->MapperTclName << " SetInput [" << sourceName
          << " GetOutput";
    if (strcmp(this->GetPVData()->GetPVSource()->GetInterface()->
               GetSourceClassName(), "vtkGenericEnsightReader") == 0)
      {
      dataTclName = this->GetPVData()->GetVTKDataTclName();
      charFound = strrchr(dataTclName, 'O');
      pos = charFound - dataTclName - 1;
      newReaderNum = atoi(dataTclName + pos);
      if (newReaderNum != readerNum)
        {
        readerNum = newReaderNum;
        outputNum = 0;
        }
      else
        {
        outputNum++;
        }
      *file << " " << outputNum << "]\n\t";
      }
    else
      {
      *file << "]\n\t";
      }
    }
  
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
    *file << "\t" << this->MapperTclName << " ColorByArrayComponent "
          << this->Mapper->GetArrayName() << " "
          << this->Mapper->GetArrayComponent() << "\n";
    }
  *file << "\n";
  
  *file << "vtkActor " << this->ActorTclName << "\n\t"
        << this->ActorTclName << " SetMapper " << this->MapperTclName << "\n\t"
        << "[" << this->ActorTclName << " GetProperty] SetRepresentationTo"
        << this->Actor->GetProperty()->GetRepresentationAsString() << "\n\t"
        << "[" << this->ActorTclName << " GetProperty] SetInterpolationTo"
        << this->Actor->GetProperty()->GetInterpolationAsString() << "\n\t"
        << this->ActorTclName << " SetVisibility "
        << this->Actor->GetVisibility() << "\n\n";
  
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
          << this->ScalarBarTclName << " SetTitle ";
    this->Script("set tempResult [%s GetTitle]",
                 this->ScalarBarTclName);
    result = this->GetPVApplication()->GetMainInterp()->result;
    *file << result << "\n\n";
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

    tclName = this->GetPVRenderView()->GetRendererTclName();
    
    *file << tclName << " GetActiveCamera]\n\t"
          << this->CubeAxesTclName << " SetInertia 20\n\n";
    }
}
