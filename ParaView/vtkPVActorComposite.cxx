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
#include "vtkPVApplication.h"
#include "vtkPVAssignment.h"
#include "vtkPVData.h"
#include "vtkPVSource.h"
#include "vtkImageOutlineFilter.h"
#include "vtkGeometryFilter.h"
#include "vtkPVImageTextureFilter.h"
#include "vtkTexture.h"

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
  this->CommandFunction = vtkPVActorCompositeCommand;
  
  this->Properties = vtkKWWidget::New();
  this->Name = NULL;

  this->Mapper->ImmediateModeRenderingOn();
  
  this->NumCellsLabel = vtkKWLabel::New();
  this->BoundsLabel = vtkKWLabel::New();
  this->XRangeLabel = vtkKWLabel::New();
  this->YRangeLabel = vtkKWLabel::New();
  this->ZRangeLabel = vtkKWLabel::New();
  
  this->DataNotebookButton = vtkKWPushButton::New();
  this->AmbientScale = vtkKWScale::New();
  
  this->PVData = NULL;
  this->DataSetInput = NULL;
  this->Mode = VTK_PV_ACTOR_COMPOSITE_POLY_DATA_MODE;
}

//----------------------------------------------------------------------------
vtkPVActorComposite::~vtkPVActorComposite()
{
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
  
  this->DataNotebookButton->Delete();
  this->DataNotebookButton = NULL;
  
  this->AmbientScale->Delete();
  this->AmbientScale = NULL;
  
  this->SetPVData(NULL);
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::Clone(vtkPVApplication *pvApp)
{
  if (this->Application)
    {
    vtkErrorMacro("Application has already been set.");
    }
  this->SetApplication(pvApp);

  // Clone this object on every other process.
  pvApp->BroadcastScript("%s %s", this->GetClassName(), this->GetTclName());

  // The application is needed by the clones to send scalar ranges back.
  pvApp->BroadcastScript("%s SetApplication %s", this->GetTclName(),
			 pvApp->GetTclName());
  
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::CreateProperties()
{
  const char *actorPage;
  char *cellsLabel;
  char *xLabel;
  char *yLabel;
  char *zLabel;
  float bounds[6];
  
  cellsLabel = new char[350];
  xLabel = new char[350];
  yLabel = new char[350];
  zLabel = new char[350];
  
  this->GetPVData()->GetBounds(bounds);
  
  // invoke superclass always
  this->vtkKWActorComposite::CreateProperties();
  
  actorPage = this->GetClassName();
  this->Notebook->AddPage(actorPage);
  this->Properties->SetParent(this->Notebook->GetFrame(actorPage));
  this->Properties->Create(this->Application, "frame","");
  this->Script("pack %s -pady 2 -fill x -expand yes",
               this->Properties->GetWidgetName());
  
  this->NumCellsLabel->SetParent(this->Properties);
  this->NumCellsLabel->Create(this->Application, "");
  sprintf(cellsLabel, "number of cells: %d",
	  this->GetPVData()->GetNumberOfCells());
  this->NumCellsLabel->SetLabel(cellsLabel);
  this->BoundsLabel->SetParent(this->Properties);
  this->BoundsLabel->Create(this->Application, "");
  this->BoundsLabel->SetLabel("bounds:");
  this->XRangeLabel->SetParent(this->Properties);
  this->XRangeLabel->Create(this->Application, "");
  sprintf(xLabel, "x range: %f to %f", bounds[0], bounds[1]);
  this->XRangeLabel->SetLabel(xLabel);
  this->YRangeLabel->SetParent(this->Properties);
  this->YRangeLabel->Create(this->Application, "");
  sprintf(yLabel, "y range: %f to %f", bounds[2], bounds[3]);
  this->YRangeLabel->SetLabel(yLabel);
  this->ZRangeLabel->SetParent(this->Properties);
  this->ZRangeLabel->Create(this->Application, "");
  sprintf(zLabel, "z range: %f to %f", bounds[4], bounds[5]);
  this->ZRangeLabel->SetLabel(zLabel);
  this->DataNotebookButton->SetParent(this->Properties);
  this->DataNotebookButton->Create(this->Application, "");
  this->DataNotebookButton->SetLabel("Return to Data Notebook");
  this->DataNotebookButton->SetCommand(this, "ShowDataNotebook");
  this->AmbientScale->SetParent(this->Properties);
  this->AmbientScale->Create(this->Application, "-showvalue 1");
  this->AmbientScale->DisplayLabel("Ambient Light");
  this->AmbientScale->SetRange(0.0, 1.0);
  this->AmbientScale->SetResolution(0.1);
  this->AmbientScale->SetValue(this->GetActor()->GetProperty()->GetAmbient());
  this->AmbientScale->SetCommand(this, "AmbientChanged");
  this->Script("pack %s %s %s %s %s %s %s",
	       this->NumCellsLabel->GetWidgetName(),
	       this->BoundsLabel->GetWidgetName(),
	       this->XRangeLabel->GetWidgetName(),
	       this->YRangeLabel->GetWidgetName(),
	       this->ZRangeLabel->GetWidgetName(),
	       this->AmbientScale->GetWidgetName(),
	       this->DataNotebookButton->GetWidgetName());
  
  delete [] cellsLabel;
  delete [] xLabel;
  delete [] yLabel;
  delete [] zLabel;
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::AmbientChanged()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  float ambient = this->AmbientScale->GetValue();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetAmbient %f", this->GetTclName(), ambient);
    }
  
  this->SetAmbient(ambient);
  this->GetView()->Render();
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::SetAmbient(float ambient)
{
  this->GetActor()->GetProperty()->SetAmbient(ambient);
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::ShowDataNotebook()
{
  this->GetPVData()->GetPVSource()->ShowProperties();
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::SetPVData(vtkPVData *data)
{
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
    // Manage double pointer.
    tmp->SetActorComposite(NULL);
    tmp->UnRegister(this);
    }
  
  if (data)
    {
    this->PVData = data;
    data->Register(this);
    // Manage double pointer.
    data->SetActorComposite(this);
    }
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::Initialize()
{
  float range[2];

  this->GetInputScalarRange(range);
  this->SetScalarRange(range[0], range[1]);
}



//----------------------------------------------------------------------------
void vtkPVActorComposite::GetInputScalarRange(float range[2]) 
{ 
  float tmp[2];
  int idx;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkMultiProcessController *controller = pvApp->GetController();
  int numProcs = controller->GetNumberOfProcesses();
  
  this->Mapper->Update();
  this->Mapper->GetInput()->GetScalarRange(range);
  for (idx = 1; idx < numProcs; ++idx)
    {
    pvApp->RemoteScript(idx, "%s TransmitInputScalarRange",
			this->GetTclName());
    controller->Receive(tmp, 2, idx, 99399);
    if (range[0] > tmp[0])
      {
      range[0] = tmp[0];
      }
    if (range[1] < tmp[1])
      {
      range[1] = tmp[1];
      }    
    }
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::TransmitInputScalarRange() 
{ 
  float tmp[2];
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkMultiProcessController *controller = pvApp->GetController();
  
  this->Mapper->Update();
  this->Mapper->GetInput()->GetScalarRange(tmp);
  
  controller->Send(tmp, 2, 0, 99399);
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::SetScalarRange(float min, float max) 
{ 
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetScalarRange %f %f", this->GetTclName(), 
			   min, max);
    }
  
  this->Mapper->SetScalarRange(min, max);
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
  vtkKWMenu* MenuProperties = v->GetParentWindow()->GetMenuProperties();
  char* rbv = 
    MenuProperties->CreateRadioButtonVariable(MenuProperties,"Radio");

  // now add our own menu options
  if (MenuProperties->GetRadioButtonValue(MenuProperties,"Radio") >= 10)
    {
    if (this->LastSelectedProperty == 10)
      {
      this->View->ShowViewProperties();
      }
    if (this->LastSelectedProperty == 100 || this->LastSelectedProperty == -1)
      {
      this->ShowProperties();
      }
    }

  delete [] rbv;
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::Deselect(vtkKWView *v)
{
  // invoke super
  this->vtkKWComposite::Deselect(v);
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::ShowProperties()
{
  vtkKWWindow *pw = this->View->GetParentWindow();
  pw->ShowProperties();
  pw->GetMenuProperties()->CheckRadioButton(pw->GetMenuProperties(),
					    "Radio", 100);

  // unpack any current children
  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->View->GetPropertiesParent()->GetWidgetName());
  
  if (!this->PropertiesCreated)
    {
    this->InitializeProperties();
    }
  
  this->Script("pack %s -pady 2 -padx 2 -fill both -expand yes -anchor n",
               this->Notebook->GetWidgetName());
  this->View->PackProperties();
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::SetVisibility(int v)
{
  vtkProp * p = this->GetProp();
  vtkPVApplication *pvApp;
  
  if (p)
    {
    p->SetVisibility(v);
    }
  
  pvApp = (vtkPVApplication*)(this->Application);
  
  // Make the assignment in all of the processes.
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetVisibility %d", this->GetTclName(), v);
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
void vtkPVActorComposite::SetAssignment(vtkPVAssignment *a)
{
  this->Mapper->SetPiece(a->GetPiece());
  this->Mapper->SetNumberOfPieces(a->GetNumberOfPieces());
  //cerr << "ActorComp: " << this << ", mapper: " << this->Mapper 
  //     << ", assignment: " << a << ", piece: " << a->GetPiece()
  //     << ", numPieces: " << a->GetNumberOfPieces() << endl;
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
void vtkPVActorComposite::SetInput(vtkDataSet *input)
{
  int save = this->Mode;
  
  if (this->DataSetInput)
    {
    this->DataSetInput->UnRegister(this);
    this->DataSetInput = NULL;
    }
  if (input)
    {
    input->Register(this);
    this->DataSetInput = input;
    }

  // So the user can set mode and input in either order.
  // Force a mode change. This will set the super classes input.
  this->Mode = VTK_PV_ACTOR_COMPOSITE_NO_MODE;
  this->SetMode(save);
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::SetMode(int mode)
{
  if (this->Mode == mode)
    {
    return;
    }

  this->Mode = mode;

  if (this->DataSetInput == NULL)
    {
    return;
    }

  if (this->DataSetInput->IsA("vtkPolyData"))
    { // Only one mode for poly data.
    if (mode != VTK_PV_ACTOR_COMPOSITE_POLY_DATA_MODE)
      {
      vtkWarningMacro("Use poly data mode for poly data input.");
      this->Mode = VTK_PV_ACTOR_COMPOSITE_POLY_DATA_MODE;
      }
    this->vtkKWActorComposite::SetInput((vtkPolyData*)this->DataSetInput);
    return;
    }

  if (this->DataSetInput->IsA("vtkImageData"))
    { // Three possible modes for image data.
    if (mode == VTK_PV_ACTOR_COMPOSITE_POLY_DATA_MODE)
      {
      vtkWarningMacro("You cannot use poly data mode with an image input.");
      this->Mode = VTK_PV_ACTOR_COMPOSITE_DATA_SET_MODE;
      }
    if (this->Mode == VTK_PV_ACTOR_COMPOSITE_IMAGE_OUTLINE_MODE)
      {
      vtkImageOutlineFilter *outline = vtkImageOutlineFilter::New();
      outline->SetInput((vtkImageData *)(this->DataSetInput));
      this->vtkKWActorComposite::SetInput(outline->GetOutput());
      outline->Delete();
      return;
      }
    if (this->Mode == VTK_PV_ACTOR_COMPOSITE_IMAGE_TEXTURE_MODE)
      {
      vtkPVImageTextureFilter *tf = vtkPVImageTextureFilter::New();
      tf->SetInput((vtkImageData *)(this->DataSetInput));
      this->vtkKWActorComposite::SetInput(tf->GetGeometryOutput());

      vtkTexture *texture = vtkTexture::New();
      texture->SetInput(tf->GetTextureOutput());
      this->GetActor()->SetTexture(texture);
      texture->Delete();
      texture = NULL;
      return;
      }
    }
    
  // Default to data set mode.
  if (mode != VTK_PV_ACTOR_COMPOSITE_DATA_SET_MODE)
    {
    vtkWarningMacro("Use data set mode.");
    this->Mode = VTK_PV_ACTOR_COMPOSITE_DATA_SET_MODE;
    }
  vtkGeometryFilter *geom = vtkGeometryFilter::New();
  geom->SetInput(this->DataSetInput);
  this->vtkKWActorComposite::SetInput(geom->GetOutput());
  geom->Delete();
}

