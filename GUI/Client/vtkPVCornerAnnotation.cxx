/*=========================================================================

  Module:    vtkPVCornerAnnotation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCornerAnnotation.h"

#include "vtkCornerAnnotation.h"
#include "vtkKWGenericComposite.h"
#include "vtkKWTextProperty.h"
#include "vtkKWView.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVCornerAnnotation );
vtkCxxRevisionMacro(vtkPVCornerAnnotation, "1.1");

int vtkPVCornerAnnotationCommand(ClientData cd, Tcl_Interp *interp,
                                 int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVCornerAnnotation::vtkPVCornerAnnotation()
{
  this->InternalCornerComposite  = NULL;
  this->View             = NULL;
}

//----------------------------------------------------------------------------
vtkPVCornerAnnotation::~vtkPVCornerAnnotation()
{
  if (this->InternalCornerComposite)
    {
    this->InternalCornerComposite->Delete();
    this->InternalCornerComposite = NULL;
    }

  this->SetView(NULL);
}

//----------------------------------------------------------------------------
void vtkPVCornerAnnotation::SetView(vtkKWView *_arg)
{ 
  if (this->View == _arg) 
    {
    return;
    }

  if (this->View != NULL) 
    { 
    this->View->UnRegister(this); 
    }

  this->View = _arg;

  // We are now in vtkKWView mode, create the corner prop and the composite

  if (this->View != NULL) 
    { 
    this->View->Register(this); 
    if (!this->InternalCornerAnnotation)
      {
      this->InternalCornerAnnotation = vtkCornerAnnotation::New();
      this->InternalCornerAnnotation->SetMaximumLineHeight(0.07);
      }
    if (!this->InternalCornerComposite)
      {
      this->InternalCornerComposite = vtkKWGenericComposite::New();
      }
    this->CornerAnnotation = this->InternalCornerAnnotation;
    }
  else
    {
    this->CornerAnnotation = NULL;
    }

  if (this->InternalCornerComposite)
    {
    this->InternalCornerComposite->SetProp(this->CornerAnnotation);
    }

  this->Modified();

  // Update the GUI. Test if it is alive because we might be in the middle
  // of destructing the whole GUI

  if (this->IsAlive())
    {
    this->Update();
    }
} 

//----------------------------------------------------------------------------
int vtkPVCornerAnnotation::GetVisibility() 
{
  // Note that the visibility here is based on the real visibility of the
  // annotation, not the state of the checkbutton

  return (this->CornerAnnotation &&
          this->CornerAnnotation->GetVisibility() &&
          this->View &&  
          this->View->HasComposite(this->InternalCornerComposite)) ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkPVCornerAnnotation::SetVisibility(int state)
{
  // In vtkKWView mode, add/remove the composite
  // In vtkKWRenderWidget mode, add/remove the prop

  int old_visibility = this->GetVisibility();

  if (this->CornerAnnotation)
    {
    if (state)
      {
      this->CornerAnnotation->VisibilityOn();
      if (this->View && 
          this->InternalCornerComposite &&
          !this->View->HasComposite(this->InternalCornerComposite))
        {
        this->View->Add2DComposite(this->InternalCornerComposite);
        }
      }
    else
      {
      this->CornerAnnotation->VisibilityOff();
      if (this->View && 
          this->InternalCornerComposite &&
          this->View->HasComposite(this->InternalCornerComposite))
        {
        this->View->Remove2DComposite(this->InternalCornerComposite);
        }
      }
    }

  if (old_visibility != this->GetVisibility())
    {
    this->Update();
    this->Render();
    this->SendChangedEvent();
    this->AddTraceEntry("$kw(%s) SetVisibility %d", this->GetTclName(), state);
    }
}

//----------------------------------------------------------------------------
void vtkPVCornerAnnotation::Render() 
{
  if (this->View)
    {
    this->View->Render();
    }
}

//----------------------------------------------------------------------------
void vtkPVCornerAnnotation::SaveState(ofstream *file)
{
  *file << "$kw(" << this->GetTclName() << ") SetVisibility "
        << this->GetVisibility() << endl;
  
  int i;
  for (i = 0; i < 4; i++)
    {
    *file << "$kw(" << this->GetTclName() << ") SetCornerText {";
    if (this->GetCornerText(i))
      {
      *file << this->GetCornerText(i);
      }
    *file << "} " << i << endl;
    }
  
  *file << "$kw(" << this->GetTclName() << ") SetMaximumLineHeight "
        << this->GetCornerAnnotation()->GetMaximumLineHeight() << endl;
  
  *file << "set kw(" << this->TextPropertyWidget->GetTclName()
        << ") [$kw(" << this->GetTclName() << ") GetTextPropertyWidget]"
        << endl;
  char *tclName =
    new char[10 + strlen(this->TextPropertyWidget->GetTclName())];
  sprintf(tclName, "$kw(%s)", this->TextPropertyWidget->GetTclName());
  this->TextPropertyWidget->SaveInTclScript(file, tclName, 0);
  delete [] tclName;
}

//----------------------------------------------------------------------------
void vtkPVCornerAnnotation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "View: " << this->GetView() << endl;
}

