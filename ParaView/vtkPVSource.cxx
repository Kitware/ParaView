/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVSource.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-2000 Kitware Inc. 469 Clifton Corporate Parkway,
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

#include "vtkPVSource.h"
#include "vtkPVApplication.h"
#include "vtkKWView.h"
#include "vtkKWRenderView.h"
#include "vtkPVWindow.h"


int vtkPVSourceCommand(ClientData cd, Tcl_Interp *interp,
			   int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVSource::vtkPVSource()
{
  this->CommandFunction = vtkPVSourceCommand;
  this->Name = NULL;
  
  this->Input = NULL;
  this->Output = NULL;
  this->Properties = vtkKWWidget::New();
  
  this->DataCreated = 0;
}

//----------------------------------------------------------------------------
vtkPVSource::~vtkPVSource()
{
  if (this->Output)
    {
    this->Output->UnRegister(this);
    this->Output = NULL;
    }
  
  if (this->Input)
    {
    this->Input->UnRegister(this);
    this->Input = NULL;
    }
  this->SetName(NULL);
  this->Properties->Delete();
}

//----------------------------------------------------------------------------
vtkPVSource* vtkPVSource::New()
{
  return new vtkPVSource();
}

//----------------------------------------------------------------------------
void vtkPVSource::Clone(vtkPVApplication *pvApp)
{
  if (this->Application)
    {
    vtkErrorMacro("Application has already been set.");
    }
  this->SetApplication(pvApp);

  // Clone this object on every other process.
  pvApp->BroadcastScript("%s %s", this->GetClassName(), this->GetTclName());

  // The clones might as well have an application.
  pvApp->BroadcastScript("%s SetApplication %s", this->GetTclName(),
			 pvApp->GetTclName());  
}


//----------------------------------------------------------------------------
void vtkPVSource::SetPVData(vtkPVData *data)
{
  if (this->Output == data)
    {
    return;
    }
  this->Modified();

  if (this->Output)
    {
    // extra careful for circular references
    vtkPVData *tmp = this->Output;
    this->Output = NULL;
    // Manage double pointer.
    tmp->SetPVSource(NULL);
    tmp->UnRegister(this);
    }
  if (data)
    {
    this->Output = data;
    data->Register(this);
    // Manage double pointer.
    data->SetPVSource(this);
    }
}
  

//----------------------------------------------------------------------------
vtkPVWindow* vtkPVSource::GetWindow()
{
  if (this->View == NULL || this->View->GetParentWindow() == NULL)
    {
    return NULL;
    }
  
  return vtkPVWindow::SafeDownCast(this->View->GetParentWindow());
}

//----------------------------------------------------------------------------
vtkPVApplication* vtkPVSource::GetPVApplication()
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
vtkProp* vtkPVSource::GetProp()
{
  vtkPVData *data = this->GetPVData();

  if (data == NULL)
    {
    return NULL;
    }
  return data->GetProp();
}

//----------------------------------------------------------------------------
void vtkPVSource::CreateProperties()
{ 
  const char *sourcePage;
  vtkPVApplication *app = this->GetPVApplication();

  // invoke super
  this->vtkKWComposite::CreateProperties();  

  sourcePage = this->GetClassName();
  this->Notebook->AddPage(sourcePage);
  this->Properties->SetParent(this->Notebook->GetFrame(sourcePage));
  this->Properties->Create(this->Application,"frame","");
  this->Script("pack %s -pady 2 -fill x -expand yes",
               this->Properties->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVSource::CreateDataPage()
{
  if (!this->DataCreated)
    {
    const char *dataPage;
    vtkPVData *data = this->GetPVData();
    
    if (data == NULL)
      {
      vtkErrorMacro("must have data before creating data page");
      return;
      }
    
    dataPage = data->GetClassName();
    this->Notebook->AddPage(dataPage);
    
    data->SetParent(this->Notebook->GetFrame(dataPage));
    data->Create("");
    this->Script("pack %s", data->GetWidgetName());
    
    this->DataCreated = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPVSource::Select(vtkKWView *v)
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

  MenuProperties->AddRadioButton(100, " Data", rbv, this, "ShowProperties");
  delete [] rbv;
}

//----------------------------------------------------------------------------
void vtkPVSource::Deselect(vtkKWView *v)
{
  // invoke super
  this->vtkKWComposite::Deselect(v);
  v->GetParentWindow()->GetMenuProperties()->DeleteMenuItem(" Data");
}

//----------------------------------------------------------------------------
void vtkPVSource::ShowProperties()
{
  vtkKWWindow *pw = this->View->GetParentWindow();
  pw->ShowProperties();
  pw->GetMenuProperties()->CheckRadioButton(
    pw->GetMenuProperties(),"Radio",100);
  
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
char* vtkPVSource::GetName()
{
  return this->Name;
}

//----------------------------------------------------------------------------
void vtkPVSource::SetName (const char* arg) 
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
void vtkPVSource::SetVisibility(int v)
{
  vtkProp * p = this->GetProp();
  vtkPVApplication *pvApp;
  
  if (p)
    {
    p->SetVisibility(v);
    }
  
  pvApp = (vtkPVApplication*)(this->Application);
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetVisibility %d", this->GetTclName(), v);
    }
}

  
//----------------------------------------------------------------------------
int vtkPVSource::GetVisibility()
{
  vtkProp *p = this->GetProp();
  
  if (p == NULL)
    {
    return 0;
    }
  
  return p->GetVisibility();
}

