/*=========================================================================

  Program:   ParaView
  Module:    vtkPVActorComposite.cxx
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
#include "vtkObjectFactory.h"
#include "vtkPVActorComposite.h"
#include "vtkKWWidget.h"
#include "vtkKWView.h"
#include "vtkKWBoundsDisplay.h"
#include "vtkKWWindow.h"
#include "vtkKWCheckButton.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVSource.h"
#include "vtkImageOutlineFilter.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkTexture.h"
#include "vtkScalarBarActor.h"
#include "vtkCubeAxesActor2D.h"
#include "vtkTimerLog.h"
#include "vtkPVRenderView.h"
#include "vtkTreeComposite.h"
#include "vtkPVSourceInterface.h"
#include "vtkKWCheckButton.h"

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
  
  this->Property = NULL;
  this->PropertyTclName = NULL;
  this->Prop = NULL;
  this->PropTclName = NULL;
  this->LODDeciTclName = NULL;
  this->MapperTclName = NULL;
  this->LODMapperTclName = NULL;
  this->GeometryTclName = NULL;
  this->OutputPortTclName = NULL;
  this->AppendPolyDataTclName = NULL;
  this->ScalarBarTclName = NULL;
  this->CubeAxesTclName = NULL;

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
  this->ViewFrame = vtkKWLabeledFrame::New();
  
  this->NumCellsLabel = vtkKWLabel::New();
  this->NumPointsLabel = vtkKWLabel::New();
  
  this->BoundsDisplay = vtkKWBoundsDisplay::New();
  
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
  
  this->PVData = NULL;
  this->DataSetInput = NULL;
//  this->Mode = VTK_PV_ACTOR_COMPOSITE_POLY_DATA_MODE;
  
  //this->TextureFilter = NULL;  
}


//----------------------------------------------------------------------------
void vtkPVActorComposite::CreateParallelTclObjects(vtkPVApplication *pvApp)
{
  int numProcs, id;
  char tclName[100];
  
  this->SetApplication(pvApp);
  
  sprintf(tclName, "Geometry%d", this->InstanceCount);
  pvApp->MakeTclObject("vtkPVGeometryFilter", tclName);
  this->SetGeometryTclName(tclName);

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
  pvApp->BroadcastScript("%s SetInput [%s GetOutput]", this->MapperTclName,
                         this->GeometryTclName);
  
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
  pvApp->BroadcastScript("%s SetInput [%s GetOutput]", 
                         this->LODDeciTclName, this->GeometryTclName);
  pvApp->BroadcastScript("%s CopyCellDataOn", this->LODDeciTclName);
  pvApp->BroadcastScript("%s UseInputPointsOn", this->LODDeciTclName);
  pvApp->BroadcastScript("%s UseInternalTrianglesOff", this->LODDeciTclName);
  //pvApp->BroadcastScript("%s UseFeatureEdgesOn", this->LODDeciTclName);
  //pvApp->BroadcastScript("%s UseFeaturePointsOn", this->LODDeciTclName);

  sprintf(tclName, "LODMapper%d", this->InstanceCount);
  pvApp->MakeTclObject("vtkPolyDataMapper", tclName);
  this->LODMapperTclName = NULL;
  this->SetLODMapperTclName(tclName);
  pvApp->BroadcastScript("%s ImmediateModeRenderingOn", this->LODMapperTclName);

  this->Script("%s SetLookupTable [%s GetLookupTable]",
               this->GetScalarBarTclName(), this->MapperTclName);
  pvApp->BroadcastScript("%s SetLookupTable [%s GetLookupTable]", 
                         this->LODMapperTclName, this->MapperTclName);
 
  // Get rid of previous object created by the superclass.
  // Maybe we should not be a subclass of vtkActorComposite!
  if (this->Actor)
    {
    this->Actor->Delete();
    this->Actor = NULL;
    }

  // Make a new tcl object.
  sprintf(tclName, "Actor%d", this->InstanceCount);
  this->Prop = (vtkProp*)pvApp->MakeTclObject("vtkPVLODActor", tclName);
  //this->Actor = (vtkLODProp3D*)pvApp->MakeTclObject("vtkLODProp3D", tclName);
  this->SetPropTclName(tclName);

  // Make a new tcl object.
  sprintf(tclName, "Property%d", this->InstanceCount);
  this->Property = (vtkProperty*)pvApp->MakeTclObject("vtkProperty", tclName);
  this->SetPropertyTclName(tclName);
  
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

//----------------------------------------------------------------------------
vtkPVActorComposite::~vtkPVActorComposite()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  this->SetName(NULL);
  
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

  pvApp->BroadcastScript("%s Delete", this->PropTclName);
  this->SetPropTclName(NULL);
  this->Prop = NULL;

  pvApp->BroadcastScript("%s Delete", this->PropertyTclName);
  this->SetPropertyTclName(NULL);
  this->Property = NULL;

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
  this->ViewFrame->Delete();
  this->ViewFrame = NULL;
  
  this->ResetCameraButton->Delete();
  this->ResetCameraButton = NULL;

  this->Properties->Delete();
  this->Properties = NULL;
}


//----------------------------------------------------------------------------
void vtkPVActorComposite::CreateProperties()
{
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
  this->ViewFrame->SetParent(this->Properties);
  this->ViewFrame->Create(this->Application);
  this->ViewFrame->SetLabel("View");
 
  this->NumCellsLabel->SetParent(this->StatsFrame);
  this->NumCellsLabel->Create(this->Application, "");
  this->NumPointsLabel->SetParent(this->StatsFrame);
  this->NumPointsLabel->Create(this->Application, "");
  
  this->BoundsDisplay->SetParent(this->Properties);
  this->BoundsDisplay->Create(this->Application);
  
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
  
  this->ScalarBarCheckFrame->SetParent(this->ScalarBarFrame->GetFrame());
  this->ScalarBarCheckFrame->Create(this->Application, "frame", "");

  this->ColorMapMenuLabel->SetParent(this->ScalarBarCheckFrame);
  this->ColorMapMenuLabel->Create(this->Application, "");
  this->ColorMapMenuLabel->SetLabel("Color map:");
  
  this->ColorMapMenu->SetParent(this->ScalarBarCheckFrame);
  this->ColorMapMenu->Create(this->Application, "");
  this->ColorMapMenu->AddEntryWithCommand("Red to Blue", this,
                                          "ChangeColorMap");
  this->ColorMapMenu->AddEntryWithCommand("Blue to Red", this,
                                          "ChangeColorMap");
  this->ColorMapMenu->AddEntryWithCommand("Grayscale", this,
                                          "ChangeColorMap");
  this->ColorMapMenu->SetValue("Red to Blue");
  
  this->ColorRangeFrame->SetParent(this->ScalarBarFrame->GetFrame());
  this->ColorRangeFrame->Create(this->Application, "frame", "");
  this->ColorRangeResetButton->SetParent(this->ColorRangeFrame);
  this->ColorRangeResetButton->Create(this->Application, "-text {Reset Range}");
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

  this->RepresentationMenuFrame->SetParent(this->DisplayStyleFrame->GetFrame());
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
  this->PointSizeScale->SetCommand(this, "ChangePointSize");
  this->PointSizeScale->SetValue(1);

  this->LineWidthLabel->SetParent(this->DisplayScalesFrame);
  this->LineWidthLabel->Create(this->Application, "");
  this->LineWidthLabel->SetLabel("Line Width");
  
  this->LineWidthScale->SetParent(this->DisplayScalesFrame);
  this->LineWidthScale->Create(this->Application, "-showvalue 1");
  this->LineWidthScale->SetRange(1, 5);
  this->LineWidthScale->SetResolution(1);
  this->LineWidthScale->SetCommand(this, "ChangeLineWidth");
  this->LineWidthScale->SetValue(1);
  
  
  this->ScalarBarCheck->SetParent(this->ScalarBarCheckFrame);
  this->ScalarBarCheck->Create(this->Application, "-text Visibility");
  this->Application->Script("%s configure -command {%s ScalarBarCheckCallback}",
                            this->ScalarBarCheck->GetWidgetName(),
                            this->GetTclName());

  this->ScalarBarOrientationCheck->SetParent(this->ScalarBarCheckFrame);
  this->ScalarBarOrientationCheck->Create(this->Application, "-text Vertical");
  this->ScalarBarOrientationCheck->SetState(1);
  this->ScalarBarOrientationCheck->SetCommand(this, "ScalarBarOrientationCallback");
  
  this->CubeAxesCheck->SetParent(this->Properties);
  this->CubeAxesCheck->Create(this->Application, "-text CubeAxes");
  this->CubeAxesCheck->SetCommand(this, "CubeAxesCheckCallback");
  
  this->VisibilityCheck->SetParent(this->ViewFrame->GetFrame());
  this->VisibilityCheck->Create(this->Application, "-text Visibility");
  this->VisibilityCheck->SetState(1);
  this->Application->Script("%s configure -command {%s VisibilityCheckCallback}",
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
  this->Script("pack %s -fill x -expand t", this->BoundsDisplay->GetWidgetName());
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
  float time;
  vtkKWWindow *window;
  
  if (this->UpdateTime > this->PVData->GetVTKData()->GetMTime())
    {
    return;
    }
  this->UpdateTime.Modified();

  window = this->GetView()->GetParentWindow();

  vtkDebugMacro( << "Start timer");
  vtkTimerLog *timer = vtkTimerLog::New();

  // Update and time the filter.
  timer->StartTimer();
  pvApp->BroadcastScript("%s Update", this->MapperTclName);
  // Get bounds to time completion (not just triggering) of update.
  this->GetPVData()->GetBounds(bounds);
  timer->StopTimer();
  time = timer->GetElapsedTime();
  if (time > 0.05)
    {
    sprintf(tmp, "%s : took %f seconds", 
            this->PVData->GetVTKDataTclName(), time); 
    window->SetStatusText(tmp);
    pvApp->AddLogEntry(this->PVData->GetVTKDataTclName(), time);
    pvApp->AddLogEntry("NumCells", this->PVData->GetVTKData()->GetNumberOfCells());
    }

  vtkDebugMacro(<< "Stop timer : " << this->PVData->GetVTKDataTclName() << " : took " 
                  << time << " seconds.");


  // Time creation of the LOD
  timer->StartTimer();
  pvApp->BroadcastScript("%s Update", this->LODMapperTclName);
  // Get bounds to time completion (not just triggering) of update.
  this->GetPVData()->GetBounds(bounds);
  timer->StopTimer();
  time = timer->GetElapsedTime();
  if (time > 0.05)
    {
    pvApp->AddLogEntry("DeciTime", time);
    }

  timer->Delete(); 

  sprintf(tmp, "number of cells: %d", 
	  this->GetPVData()->GetNumberOfCells());
  this->NumCellsLabel->SetLabel(tmp);
  sprintf(tmp, "number of points: %d",
          this->GetPVData()->GetNumberOfPoints());
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
    }
  // If the current array we are coloring by has disappeared,
  // then default back to the property.
  if ( ! currentColorByFound)
    {
    this->ColorMenu->SetValue("Property");
    this->ColorByProperty();
    }
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::ChangeActorColor(float r, float g, float b)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (this->Mapper->GetScalarVisibility())
    {
    return;
    }
  
  pvApp->BroadcastScript("%s SetColor %f %f %f",
                         this->PropertyTclName, r, g, b);
  this->GetPVRenderView()->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::ChangeColorMap()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  // LODMapper shares a lookup table with Mapper.
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

//----------------------------------------------------------------------------
void vtkPVActorComposite::SetColorRange(float min, float max)
{
  this->GetPVApplication()->BroadcastScript("%s SetScalarRange %f %f",
					    this->MapperTclName,
					    min, max);
  this->GetPVApplication()->BroadcastScript("%s SetScalarRange %f %f",
					    this->LODMapperTclName,
					    min, max);

  this->ColorRangeMinEntry->SetValue(min, 5);
  this->ColorRangeMaxEntry->SetValue(max, 5);
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::ResetColorRange()
{
  float range[2];
  this->GetColorRange(range);
  
  // Avoid the bad range error
  if (range[1] < range[0])
    {
    range[1] = range[0];
    }

  this->SetColorRange(range[0], range[1]);
  this->GetPVRenderView()->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::ColorRangeEntryCallback()
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
void vtkPVActorComposite::GetColorRange(float range[2])
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkMultiProcessController *controller = pvApp->GetController();
  float tmp[2];
  int id, num;

  pvApp->BroadcastScript("Application SendMapperColorRange %s", 
                         this->MapperTclName);
  pvApp->GetMapperColorRange(range, this->Mapper);
    
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
  pvApp->BroadcastScript("%s ScalarVisibilityOff", this->LODMapperTclName);
  float *color;
  
  color = this->ColorButton->GetColor();
  pvApp->BroadcastScript("%s SetColor %f %f %f", 
			 this->PropertyTclName, color[0], color[1], color[2]);
  
  // No scalars visible.  Turn off scalar bar.
  this->SetScalarBarVisibility(0);

  this->Script("pack forget %s", this->ScalarBarFrame->GetWidgetName());
  this->Script("pack %s -side left",
               this->ColorButton->GetWidgetName());

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

  this->Script("pack forget %s",
               this->ColorButton->GetWidgetName());
  this->Script("pack %s -after %s -fill x", this->ScalarBarFrame->GetWidgetName(),
               this->ColorFrame->GetWidgetName());

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

  this->Script("pack forget %s",
               this->ColorButton->GetWidgetName());
  this->Script("pack %s -after %s -fill x", this->ScalarBarFrame->GetWidgetName(),
               this->ColorFrame->GetWidgetName());

  this->GetPVRenderView()->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::DrawWireframe()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (this->PropertyTclName)
    {
    pvApp->BroadcastScript("%s SetRepresentationToWireframe",
                           this->PropertyTclName);
    }
  
  this->GetPVRenderView()->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::DrawSurface()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (this->PropertyTclName)
    {
    pvApp->BroadcastScript("%s SetRepresentationToSurface",
                           this->PropertyTclName);
    }
  
  this->GetPVRenderView()->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::DrawPoints()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (this->PropertyTclName)
    {
    pvApp->BroadcastScript("%s SetRepresentationToPoints",
			   this->PropertyTclName);
    }
  
  this->GetPVRenderView()->EventuallyRender();
}


//----------------------------------------------------------------------------
void vtkPVActorComposite::SetInterpolationToFlat()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (this->PropertyTclName)
    {
    pvApp->BroadcastScript("%s SetInterpolationToFlat",
                           this->PropertyTclName);
    }
  
  this->GetPVRenderView()->EventuallyRender();
}


//----------------------------------------------------------------------------
void vtkPVActorComposite::SetInterpolationToGouraud()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (this->PropertyTclName)
    {
    pvApp->BroadcastScript("%s SetInterpolationToGouraud",
                           this->PropertyTclName);
    }
  
  this->GetPVRenderView()->EventuallyRender();
}



//----------------------------------------------------------------------------
void vtkPVActorComposite::AmbientChanged()
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
void vtkPVActorComposite::SetAmbient(float ambient)
{
  this->Property->SetAmbient(ambient);
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
    pvApp->BroadcastScript("%s SetInput %s",
			       this->GeometryTclName,
			       this->PVData->GetVTKDataTclName());
    }
  else
    {
    // Keep the conditional becuase I want to try eliminating the geometry
    // filter with poly data.
    pvApp->BroadcastScript("%s SetInput %s",
			       this->GeometryTclName,
			       this->PVData->GetVTKDataTclName());
    }
  
  // Do we really need to do this here ???????????????  I don't think so.
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
    this->ColorByPointFieldComponent(arrayName, 0);
    this->ColorMenu->SetValue(tmp);
    }
  else if ((array =
            this->Mapper->GetInput()->GetCellData()->GetScalars()) &&
            (array->GetName()))
    {
    char *arrayName = (char*)array->GetName();
    char tmp[350];
    sprintf(tmp, "Cell %s", arrayName);
    this->ColorByCellFieldComponent(arrayName, 0);
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
  if (max < min)
    {
    max = min;
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
  pvApp->BroadcastScript("%s ResetCameraClippingRange", tclName);
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
  if (this->PropTclName)
    {
    pvApp->BroadcastScript("%s SetVisibility %d", this->PropTclName, v);
    }
  if (v == 0 && this->GeometryTclName)
    {
    pvApp->BroadcastScript("[%s GetInput] ReleaseData", this->MapperTclName);
    }

  if (this->VisibilityCheck->GetState() != v)
    {
    this->VisibilityCheck->SetState(v);
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
/*
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
    this->ScalarBarCheck->SetState(val);
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
    this->Script("%s SetOrientationToVertical", this->GetScalarBarTclName());
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

void vtkPVActorComposite::SetScalarBarOrientationToVertical()
{
  this->ScalarBarOrientationCheck->SetState(1);
  this->ScalarBarOrientationCallback();
}

void vtkPVActorComposite::SetScalarBarOrientationToHorizontal()
{
  this->ScalarBarOrientationCheck->SetState(0);
  this->ScalarBarOrientationCallback();
}

void vtkPVActorComposite::SetPointSize(int size)
{
  this->PointSizeScale->SetValue(size);
  this->ChangePointSize();
}


void vtkPVActorComposite::ChangePointSize()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (this->PropertyTclName)
    {
    pvApp->BroadcastScript("%s SetPointSize %f",
                           this->PropertyTclName,
                           this->PointSizeScale->GetValue());
    }
  
  this->GetPVRenderView()->EventuallyRender();
}

void vtkPVActorComposite::ChangeLineWidth()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (this->PropertyTclName)
    {
    pvApp->BroadcastScript("%s SetLineWidth %f",
                           this->PropertyTclName,
                           this->LineWidthScale->GetValue());
    }

  this->GetPVRenderView()->EventuallyRender();
}

void vtkPVActorComposite::SaveInTclScript(ofstream *file, const char *sourceName)
{
  char* charFound;
  int pos;
  float range[2], position[2];
  const char* scalarMode;
  static int readerNum = -1;
  static int outputNum;
  int newReaderNum;
  char* result;
  char* dataTclName;
  char* renTclName;
  vtkPVSourceInterface *pvsInterface =
    this->GetPVData()->GetPVSource()->GetInterface();

  renTclName = this->GetPVRenderView()->GetRendererTclName();

  *file << "vtkPVGeometryFilter " << this->GeometryTclName << "\n\t"
        << this->GeometryTclName << " SetInput [" << sourceName
        << " GetOutput";
  if (pvsInterface && strcmp(pvsInterface->GetSourceClassName(), 
                             "vtkGenericEnSightReader") == 0)
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
  
  *file << "vtkActor " << this->PropTclName << "\n\t"
        << this->PropTclName << " SetMapper " << this->MapperTclName << "\n\t"
        << "[" << this->PropTclName << " GetProperty] SetRepresentationTo"
        << this->Property->GetRepresentationAsString() << "\n\t"
        << "[" << this->PropTclName << " GetProperty] SetInterpolationTo"
        << this->Property->GetInterpolationAsString() << "\n\t"
        << this->PropTclName << " SetVisibility "
        << this->Prop->GetVisibility() << "\n\n";

  if (!this->Mapper->GetScalarVisibility())
    {
    float propColor[3];
    this->Property->GetColor(propColor);
    *file << "[" << this->PropTclName << " GetProperty] SetColor "
          << propColor[0] << " " << propColor[1] << " " << propColor[2]
          << "\n\n";
    }
  
  //*file << "\n";
  //*file << renTclName << " AddActor " << this->PropTclName << "\n"
  
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

    
    *file << renTclName << " GetActiveCamera]\n\t"
          << this->CubeAxesTclName << " SetInertia 20\n\n";
    }
}
