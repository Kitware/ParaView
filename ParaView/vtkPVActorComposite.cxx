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
  this->Label = vtkKWLabel::New();
  this->Name = NULL;
}

//----------------------------------------------------------------------------
vtkPVActorComposite::~vtkPVActorComposite()
{
  this->Label->Delete();
  this->Label = NULL;
  
  this->Properties->Delete();
  this->Properties = NULL;
  
  this->SetName(NULL);
}

//----------------------------------------------------------------------------
void vtkPVActorComposite::CreateProperties()
{
  const char *actorPage;
  
  // invoke superclass always
  this->vtkKWActorComposite::CreateProperties();
  
  actorPage = this->GetClassName();
  this->Notebook->AddPage(actorPage);
  this->Properties->SetParent(this->Notebook->GetFrame(actorPage));
  this->Properties->Create(this->Application, "frame","");
  this->Script("pack %s -pady 2 -fill x -expand yes",
               this->Properties->GetWidgetName());
  
  this->Label->SetParent(this->Properties);
  this->Label->Create(this->Application, "");
  this->Label->SetLabel("vtkPVActorComposite label");
  this->Script("pack %s", this->Label->GetWidgetName());
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
void vtkPVActorComposite::SetApplication(vtkKWApplication *app)
{
  this->Application = app;  
}

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
  cerr << "ActorComp: " << this << ", mapper: " << this->Mapper 
       << ", assignment: " << a << ", piece: " << a->GetPiece()
       << ", numPieces: " << a->GetNumberOfPieces() << endl;
  

}
