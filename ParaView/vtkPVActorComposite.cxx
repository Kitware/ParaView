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
#include "vtkFastGeometryFilter.h"
//#include "vtkPVImageTextureFilter.h"
#include "vtkTexture.h"
#include "vtkScalarBarActor.h"
#include "vtkTimerLog.h"

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
  
  this->NumCellsLabel = vtkKWLabel::New();
  this->BoundsLabel = vtkKWLabel::New();
  this->XRangeLabel = vtkKWLabel::New();
  this->YRangeLabel = vtkKWLabel::New();
  this->ZRangeLabel = vtkKWLabel::New();
  
  this->AmbientScale = vtkKWScale::New();

  this->ColorMenuLabel = vtkKWLabel::New();
  this->ColorMenu = vtkKWOptionMenu::New();

  this->RepresentationMenuLabel = vtkKWLabel::New();
  this->RepresentationMenu = vtkKWOptionMenu::New();
  
  this->DecimateCheck = vtkKWCheckButton::New();
  this->ScalarBarCheck = vtkKWCheckButton::New();
  this->ReductionEntry = vtkKWEntry::New();

  this->VisibilityCheck = vtkKWCheckButton::New();
  
  this->PVData = NULL;
  this->DataSetInput = NULL;
  this->Mode = VTK_PV_ACTOR_COMPOSITE_POLY_DATA_MODE;
  
  this->Decimate = 0;

  //this->TextureFilter = NULL;
  
  this->ActorTclName = NULL;
  this->DeciTclName = NULL;
  this->MapperTclName = NULL;
  this->OutlineTclName = NULL;
  this->GeometryTclName = NULL;

  this->ScalarBar = vtkScalarBarActor::New();  
  this->ScalarBar->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  this->ScalarBar->GetPositionCoordinate()->SetValue(0.87, 0.25);
  this->ScalarBar->SetOrientationToVertical();
  this->ScalarBar->SetWidth(0.13);
  this->ScalarBar->SetHeight(0.5);
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
  
  this->ScalarBar->SetLookupTable(this->Mapper->GetLookupTable());
  
  // Get rid of previous object created by the superclass.
  if (this->Actor)
    {
    this->Actor->Delete();
    this->Actor = NULL;
    }
  // Make a new tcl object.
  sprintf(tclName, "Actor%d", this->InstanceCount);
  this->Actor = (vtkActor*)pvApp->MakeTclObject("vtkActor", tclName);
  this->ActorTclName = NULL;
  this->SetActorTclName(tclName);
  
  pvApp->BroadcastScript("%s SetMapper %s", this->ActorTclName, 
			this->MapperTclName);

  // Hard code assignment based on processes.
  numProcs = pvApp->GetController()->GetNumberOfProcesses() ;
  for (id = 0; id < numProcs; ++id)
    {
    pvApp->RemoteScript(id, "%s SetNumberOfPieces %d", this->MapperTclName, numProcs);
    pvApp->RemoteScript(id, "%s SetPiece %d", this->MapperTclName, id);
    //pvApp->RemoteScript(id, "%s SetNumberOfPieces 2", this->MapperTclName);
    //pvApp->RemoteScript(id, "%s SetPiece 1", this->MapperTclName);
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
  
  this->RepresentationMenuLabel->Delete();
  this->RepresentationMenuLabel = NULL;
  
  this->RepresentationMenu->Delete();
  this->RepresentationMenu = NULL;
  
  this->SetInput(NULL);
  
  //if (this->TextureFilter != NULL)
  //{
  // this->TextureFilter->Delete();
  //this->TextureFilter = NULL;
  //}
  
  this->ScalarBar->Delete();
  this->ScalarBar = NULL;
  
  pvApp->BroadcastScript("%s Delete", this->MapperTclName);
  this->SetMapperTclName(NULL);
  this->Mapper = NULL;

  pvApp->BroadcastScript("%s Delete", this->ActorTclName);
  this->SetActorTclName(NULL);
  this->Actor = NULL;

  this->DecimateCheck->Delete();
  this->DecimateCheck = NULL;
  
  this->ScalarBarCheck->Delete();
  this->ScalarBarCheck = NULL;
  
  this->ReductionEntry->Delete();
  this->ReductionEntry = NULL;
  
  this->VisibilityCheck->Delete();
  this->VisibilityCheck = NULL;
  
  if (this->DeciTclName)
    {
    pvApp->BroadcastScript("%s Delete", this->DeciTclName);
    this->SetDeciTclName(NULL);
    }
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
  
}


//----------------------------------------------------------------------------
void vtkPVActorComposite::CreateProperties()
{
  vtkPVApplication *pvApp = this->GetPVApplication();  
    
  this->Properties->SetParent(this->GetPVData()->GetPVSource()->GetNotebook()->GetFrame("Data"));
  this->Properties->Create(this->Application, "frame","");
  this->Script("pack %s -pady 2 -fill x -expand yes",
               this->Properties->GetWidgetName());
  
  this->NumCellsLabel->SetParent(this->Properties);
  this->NumCellsLabel->Create(this->Application, "");
  this->BoundsLabel->SetParent(this->Properties);
  this->BoundsLabel->Create(this->Application, "");
  this->BoundsLabel->SetLabel("bounds:");
  this->XRangeLabel->SetParent(this->Properties);
  this->XRangeLabel->Create(this->Application, "");
  this->YRangeLabel->SetParent(this->Properties);
  this->YRangeLabel->Create(this->Application, "");
  this->ZRangeLabel->SetParent(this->Properties);
  this->ZRangeLabel->Create(this->Application, "");
  
  //this->ColorByCellCheck->SetParent(this->Properties);
  //this->ColorByCellCheck->Create(this->Application, "-text {Color By Cell Data:}");
  //this->ColorByCellCheck->SetCommand(this, "ColorByCellCheckCallBack");
  
  this->AmbientScale->SetParent(this->Properties);
  this->AmbientScale->Create(this->Application, "-showvalue 1");
  this->AmbientScale->DisplayLabel("Ambient Light");
  this->AmbientScale->SetRange(0.0, 1.0);
  this->AmbientScale->SetResolution(0.1);
  this->AmbientScale->SetCommand(this, "AmbientChanged");
  
  this->ColorMenuLabel->SetParent(this->Properties);
  this->ColorMenuLabel->Create(this->Application, "");
  this->ColorMenuLabel->SetLabel("Color by variable:");
  
  this->ColorMenu->SetParent(this->Properties);
  this->ColorMenu->Create(this->Application, "");    

  this->RepresentationMenuLabel->SetParent(this->Properties);
  this->RepresentationMenuLabel->Create(this->Application, "");
  this->RepresentationMenuLabel->SetLabel("Data set representation:");
  
  this->RepresentationMenu->SetParent(this->Properties);
  this->RepresentationMenu->Create(this->Application, "");
  this->RepresentationMenu->AddEntryWithCommand("Wireframe", this,
                                                "DrawWireframe");
  this->RepresentationMenu->AddEntryWithCommand("Surface", this,
                                                "DrawSurface");
  this->RepresentationMenu->AddEntryWithCommand("Points", this,
                                                "DrawPoints");
  this->RepresentationMenu->SetValue("Surface");
  
  this->DecimateCheck->SetParent(this->Properties);
  this->DecimateCheck->Create(this->Application, "-text Decimate");
  this->Application->Script("%s configure -command {%s DecimateCheckCallback}",
                            this->DecimateCheck->GetWidgetName(),
                            this->GetTclName());

  this->ScalarBarCheck->SetParent(this->Properties);
  this->ScalarBarCheck->Create(this->Application, "-text ScalarBar");
  this->Application->Script("%s configure -command {%s ScalarBarCheckCallback}",
                            this->ScalarBarCheck->GetWidgetName(),
                            this->GetTclName());

  this->ReductionEntry->SetParent(this->Properties);
  this->ReductionEntry->Create(this->Application, "-text CompositeReduction");
  this->ReductionEntry->SetValue(1);
  this->Application->Script("bind %s <KeyPress-Return> {%s ReductionCallback}",
                            this->ReductionEntry->GetWidgetName(),
                            this->GetTclName());

  this->VisibilityCheck->SetParent(this->Properties);
  this->VisibilityCheck->Create(this->Application, "-text Visibility");
  this->Application->Script("%s configure -command {%s VisibilityCheckCallback}",
                            this->VisibilityCheck->GetWidgetName(),
                            this->GetTclName());
  this->VisibilityCheck->SetState(1);

  this->Script("pack %s",
	       this->NumCellsLabel->GetWidgetName());
  this->Script("pack %s",
	       this->BoundsLabel->GetWidgetName());
  this->Script("pack %s",
	       this->XRangeLabel->GetWidgetName());
  this->Script("pack %s",
	       this->YRangeLabel->GetWidgetName());
  this->Script("pack %s",
	       this->ZRangeLabel->GetWidgetName());
  this->Script("pack %s",
	       this->ColorMenuLabel->GetWidgetName());
  this->Script("pack %s",
               this->ColorMenu->GetWidgetName());
  this->Script("pack %s",
               this->RepresentationMenuLabel->GetWidgetName());
  this->Script("pack %s",
               this->RepresentationMenu->GetWidgetName());
  this->Script("pack %s",
               this->DecimateCheck->GetWidgetName());
  this->Script("pack %s",
               this->ScalarBarCheck->GetWidgetName());
  this->Script("pack %s",
               this->ReductionEntry->GetWidgetName());
  this->Script("pack %s",
               this->VisibilityCheck->GetWidgetName());
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

  
  if (this->UpdateTime > this->PVData->GetVTKData()->GetMTime())
    {
    return;
    }
  this->UpdateTime.Modified();

  vtkDebugMacro( << "Start timer");
  vtkTimerLog *timer = vtkTimerLog::New();
  timer->StartTimer();
  pvApp->BroadcastScript("%s Update", this->MapperTclName);
  this->GetPVData()->GetBounds(bounds);
  timer->StopTimer();
  vtkDebugMacro(<< "Stop timer : " << this->PVData->GetVTKDataTclName() << " : took " 
                  << timer->GetElapsedTime() << " seconds.");
  timer->Delete();
  
  
  sprintf(tmp, "number of cells: %d", 
	  this->GetPVData()->GetNumberOfCells());
  this->NumCellsLabel->SetLabel(tmp);

  sprintf(tmp, "x range: %f to %f", bounds[0], bounds[1]);
  this->XRangeLabel->SetLabel(tmp);
  sprintf(tmp, "y range: %f to %f", bounds[2], bounds[3]);
  this->YRangeLabel->SetLabel(tmp);
  sprintf(tmp, "z range: %f to %f", bounds[4], bounds[5]);
  this->ZRangeLabel->SetLabel(tmp);
  
    
  this->AmbientScale->SetValue(this->GetActor()->GetProperty()->GetAmbient());

  this->ColorMenu->ClearEntries();
  this->ColorMenu->AddEntryWithCommand("Property",
	                               this, "ColorByProperty");
  if (this->GetPVData()->GetVTKData()->GetPointData()->GetScalars())
    {
    this->ColorMenu->AddEntryWithCommand("Point Scalars",
					     this, "ColorByPointScalars");
    }
  if (this->GetPVData()->GetVTKData()->GetCellData()->GetScalars())
    {
    this->ColorMenu->AddEntryWithCommand("Cell Scalars",
					     this, "ColorByCellScalars");
    }
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
        } 
      }
    }

}

void vtkPVActorComposite::ResetColorRange()
{
  float range[2];
  this->GetColorRange(range);
  
  this->GetPVApplication()->BroadcastScript("%s SetScalarRange %f %f",
			this->MapperTclName, range[0], range[1]);
  this->GetView()->Render();
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

  // No scalars visible.  Turn off scalar bar.
  this->SetScalarBarVisibility(0);

  this->GetView()->Render();
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::ColorByPointScalars()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  pvApp->BroadcastScript("%s ScalarVisibilityOn", this->MapperTclName);
  pvApp->BroadcastScript("%s SetScalarModeToUsePointData",
                         this->MapperTclName);

  this->ScalarBar->SetTitle("Point Scalars");  
    
  this->ResetColorRange();
  this->GetView()->Render();
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::ColorByCellScalars()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  pvApp->BroadcastScript("%s ScalarVisibilityOn", this->MapperTclName);
  pvApp->BroadcastScript("%s SetScalarModeToUseCellData",
                         this->MapperTclName);

  this->ScalarBar->SetTitle("Cell Scalars");  
  
  this->ResetColorRange();
  this->GetView()->Render();
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
  
  this->ScalarBar->SetTitle(name);
  
  this->ResetColorRange();
  this->GetView()->Render();
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

  this->ScalarBar->SetTitle(name);  
  
  this->ResetColorRange();
  this->GetView()->Render();
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
  
  this->GetActor()->GetProperty()->SetRepresentationToWireframe();
  this->GetView()->Render();
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
  
  this->GetActor()->GetProperty()->SetRepresentationToSurface();
  this->GetView()->Render();
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
  
  this->GetActor()->GetProperty()->SetRepresentationToPoints();
  this->GetView()->Render();
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::AmbientChanged()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  float ambient = this->AmbientScale->GetValue();
  
  //pvApp->BroadcastScript("%s SetAmbient %f", this->GetTclName(), ambient);

  
  this->SetAmbient(ambient);
  this->GetView()->Render();
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::SetAmbient(float ambient)
{
  this->GetActor()->GetProperty()->SetAmbient(ambient);
}


//----------------------------------------------------------------------------
void vtkPVActorComposite::Initialize()
{
  if (this->PVData->GetVTKData()->IsA("vtkPolyData"))
    {
    this->SetModeToPolyData();
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

  vtkDebugMacro( << "Initialize --------")
  this->UpdateProperties();
  
  // Mapper needs an input, so the mode needs to be set first.
  //this->ResetColorRange();
  this->ColorByProperty();
  this->ColorMenu->SetValue("Property");
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::SetInput(vtkPVData *data)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  char *vtkDataTclName = NULL;
  
  if (this->PVData == data)
    {
    return;
    }
  this->Modified();
  
  if (this->PVData)
    {
    // extra careful for circular references
    vtkPVData *tmp = this->PVData;
    this->PVData = NULL;
    tmp->UnRegister(this);
    }
  
  if (data)
    {
    this->PVData = data;
    data->Register(this);
    vtkDataTclName = data->GetVTKDataTclName();
    }
    
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::SetScalarRange(float min, float max) 
{ 
  vtkPVApplication *pvApp = this->GetPVApplication();

  pvApp->BroadcastScript("%s SetScalarRange %f %f", this->MapperTclName, min, max);
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
  
//  this->Script("pack %s -pady 2 -padx 2 -fill both -expand yes -anchor n",
//               this->Notebook->GetWidgetName());
  
  this->UpdateProperties();
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::Deselect(vtkKWView *v)
{
  this->SetScalarBarVisibility(0);

  // invoke super
  this->vtkKWComposite::Deselect(v);

  this->Script("pack forget %s", this->Notebook->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::VisibilityCheckCallback()
{
  this->SetVisibility(this->VisibilityCheck->GetState());
  this->GetView()->Render();
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
    }
  else if (mode == VTK_PV_ACTOR_COMPOSITE_DATA_SET_MODE)
    {
    if (this->GeometryTclName == NULL)
      {
      char tclName[150];
      sprintf(tclName, "ActorCompositeGeometry%d", this->InstanceCount);
      //pvApp->MakeTclObject("vtkGeometryFilter", tclName);
      pvApp->MakeTclObject("vtkFastGeometryFilter", tclName);
      this->SetGeometryTclName(tclName);
      }
    pvApp->BroadcastScript("%s SetInput %s", this->GeometryTclName,
			   this->PVData->GetVTKDataTclName());
    pvApp->BroadcastScript("%s SetInput [%s GetOutput]", 
			   this->MapperTclName, this->GeometryTclName);    
    }
  
}


//----------------------------------------------------------------------------
void vtkPVActorComposite::DecimateCheckCallback()
{
  this->SetDecimate(this->DecimateCheck->GetState());
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::SetDecimate(int val)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (val > 1)
    {
    val = 1;
    }
  if (val < 0)
    {
    val = 0;
    }
  if (val == this->Decimate)
    {
    return;
    }
  
  if (pvApp == NULL)
    {
    vtkErrorMacro("I cannot decimate without an application.");
    }

  if (val)
    {
    if (this->DeciTclName == NULL)
      {
      char tclName[150];
      float bds[6];
      float spacing;

      sprintf(tclName, "ActorCompositeDeci%d", this->InstanceCount);
      pvApp->MakeTclObject("vtkQuadricClustering", tclName);
      this->SetDeciTclName(tclName);

      // Determine bounds and spacing.
      this->GetPVData()->GetBounds(bds);
	  // I would consider the number of cells too, 
      // but I cannot get the number of cells of geometry output.
      // numCells = this->GetPVData()->GetNumberOfCells());
      spacing = ((bds[1]-bds[0])+(bds[3]-bds[2])+(bds[5]-bds[4]))/150.0;
      pvApp->BroadcastScript("%s SetDivisionSpacing %f %f %f", this->DeciTclName,
                             spacing, spacing, spacing);
      pvApp->BroadcastScript("%s UseFeatureEdgesOn", this->DeciTclName);
      pvApp->BroadcastScript("%s UseInputPointsOn", this->DeciTclName);
      }

    // Place in front of the mapper.
    pvApp->BroadcastScript("%s SetInput [%s GetInput]", 
                           this->DeciTclName, this->MapperTclName);
    pvApp->BroadcastScript("%s SetInput [%s GetOutput]", 
                           this->MapperTclName, this->DeciTclName);
    }
  else
    {
    // Take decimate out of pipeline.
    pvApp->BroadcastScript("%s SetInput [%s GetInput]", 
                           this->MapperTclName, this->DeciTclName);
    } 
  this->Decimate = val;
}

#include "vtkPVWindow.h"
//----------------------------------------------------------------------------
void vtkPVActorComposite::ReductionCallback()
{
  int factor;
  
  
  factor = this->ReductionEntry->GetValueAsInt();

  vtkDebugMacro( << "Setting reduction factor to " << factor);
  this->PVData->GetPVSource()->GetWindow()->GetMainView()->GetComposite()->SetReductionFactor(factor);
}

  
//----------------------------------------------------------------------------
void vtkPVActorComposite::SetScalarBarVisibility(int val)
{
  vtkRenderer *ren;
  
  ren = this->GetView()->GetRenderer();
  
  if (this->ScalarBarCheck->GetState() != val)
    {
    this->ScalarBarCheck->SetState(0);
    }

  // I am going to add and remove it from the renderer instead of using visibility.
  // Composites should really have multiple props.
  
  if (val)
    {
    ren->AddActor(this->ScalarBar);
    }
  else
    {
    ren->RemoveActor(this->ScalarBar);
    }
}


//----------------------------------------------------------------------------
void vtkPVActorComposite::ScalarBarCheckCallback()
{
  this->SetScalarBarVisibility(this->ScalarBarCheck->GetState());
  this->GetView()->Render();  
}

