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
#include "vtkKWCheckButton.h"
#include "vtkKWScale.h"
#include "vtkKWText.h"
#include "vtkKWTextLabeled.h"
#include "vtkPVTextProperty.h"
#include "vtkObjectFactory.h"
#include "vtkPVRenderView.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyAdaptor.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkPVTraceHelper.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVCornerAnnotation );
vtkCxxRevisionMacro(vtkPVCornerAnnotation, "1.5");

int vtkPVCornerAnnotationCommand(ClientData cd, Tcl_Interp *interp,
                                 int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVCornerAnnotation::vtkPVCornerAnnotation()
{
  this->TraceHelper = vtkPVTraceHelper::New();
  this->TraceHelper->SetObject(this);

  this->InternalCornerAnnotation = NULL;

  this->View= NULL;

  // Delete the vtkKWTextProperty, use the traced one, vtkPVTextProperty

  if (this->TextPropertyWidget)
    {
    this->TextPropertyWidget->Delete();
    }
  
  this->TextPropertyWidget = vtkPVTextProperty::New();
  vtkPVTextProperty *pvtpropw = 
    vtkPVTextProperty::SafeDownCast(this->TextPropertyWidget);
  pvtpropw->GetTraceHelper()->SetReferenceHelper(this->GetTraceHelper());
  pvtpropw->GetTraceHelper()->SetReferenceCommand(
    "GetTextPropertyWidget");
}

//----------------------------------------------------------------------------
vtkPVCornerAnnotation::~vtkPVCornerAnnotation()
{
  this->SetView(NULL);

  if (this->TraceHelper)
    {
    this->TraceHelper->Delete();
    this->TraceHelper = NULL;
    }

  if (this->InternalCornerAnnotation)
    {
    this->InternalCornerAnnotation->Delete();
    this->InternalCornerAnnotation = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkPVCornerAnnotation::SetView(vtkKWView *vw)
{ 
  vtkPVRenderView* rw = vtkPVRenderView::SafeDownCast(vw);

  if (this->View == rw) 
    {
    return;
    }

  if (this->View != NULL) 
    { 
    this->View->UnRegister(this); 
    }

  this->View = rw;

  // We are now in vtkKWView mode, create the corner prop and the composite

  if (this->View != NULL) 
    { 
    this->View->Register(this); 
    if (!this->InternalCornerAnnotation)
      {
      this->InternalCornerAnnotation = vtkCornerAnnotation::New();
      this->InternalCornerAnnotation->SetMaximumLineHeight(0.07);
      this->InternalCornerAnnotation->VisibilityOff();
      }
    this->CornerAnnotation = this->InternalCornerAnnotation;
    }
  else
    {
    this->CornerAnnotation = NULL;
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
          this->CornerAnnotation->GetVisibility()) ? 1 : 0;
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
      if (this->View)
        {
        this->View->AddAnnotationProp(this);
        }
      }
    else
      {
      this->CornerAnnotation->VisibilityOff();
      if (this->View)
        {
        this->View->RemoveAnnotationProp(this);
        }
      }
    }

  if (old_visibility != this->GetVisibility())
    {
    this->Update();
    this->Render();
    this->SendChangedEvent();
    this->GetTraceHelper()->AddEntry("$kw(%s) SetVisibility %d", this->GetTclName(), state);
    }
}

//----------------------------------------------------------------------------
void vtkPVCornerAnnotation::UpdateCornerText() 
{
  if (this->IsCreated())
    {
    for (int i = 0; i < 4; i++)
      {
      if (this->CornerText[i])
        {
        this->SetCornerTextInternal(
          this->CornerText[i]->GetWidget()->GetValue(), i);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVCornerAnnotation::SetCornerTextInternal(const char* text, int corner) 
{
  if (this->CornerAnnotation &&
      (!this->GetCornerText(corner) ||
       strcmp(this->GetCornerText(corner), text)))
    {
    this->CornerAnnotation->SetText(
      corner, this->Script("%s \"%s\"", "set pvCATemp", text));
    }
}

//----------------------------------------------------------------------------
void vtkPVCornerAnnotation::CornerTextCallback(int i) 
{
  if (this->IsCreated() && this->CornerText[i])
    {
    char* text = this->CornerText[i]->GetWidget()->GetValue();
    this->SetCornerTextInternal(text, i);

    this->Update();

    if (this->GetVisibility())
      {
      this->Render();
      }

    this->SendChangedEvent();

    this->GetTraceHelper()->AddEntry("$kw(%s) SetCornerText {%s} %d", 
                        this->GetTclName(), text, i);
    }
}

//----------------------------------------------------------------------------
void vtkPVCornerAnnotation::SetMaximumLineHeight(float v)
{
  this->Superclass::SetMaximumLineHeight(v);
  this->GetTraceHelper()->AddEntry(
    "$kw(%s) SetMaximumLineHeight %f", this->GetTclName(), v);
}

//----------------------------------------------------------------------------
void vtkPVCornerAnnotation::SetCornerText(const char *text, int corner) 
{
  char* oldValue = this->CornerText[corner]->GetWidget()->GetValue();
  if (this->CornerAnnotation && (strcmp(oldValue, text)))
    {
    this->CornerText[corner]->GetWidget()->SetValue(text);
    this->SetCornerTextInternal(text, corner);

    this->Update();

    if (this->GetVisibility())
      {
      this->Render();
      }

    this->SendChangedEvent();

    this->GetTraceHelper()->AddEntry("$kw(%s) SetCornerText {%s} %d", 
                        this->GetTclName(), text, corner);
    }
}


//----------------------------------------------------------------------------
void vtkPVCornerAnnotation::Update() 
{
  // Maximum line height

  if (this->MaximumLineHeightScale && this->CornerAnnotation)
    {
    this->MaximumLineHeightScale->SetValue(
      this->CornerAnnotation->GetMaximumLineHeight());
    }

  // Text property

  if (this->TextPropertyWidget)
    {
    this->TextPropertyWidget->SetTextProperty(
      this->CornerAnnotation ? this->CornerAnnotation->GetTextProperty():NULL);
    this->TextPropertyWidget->SetActor2D(this->CornerAnnotation);
    this->TextPropertyWidget->Update();
    }

  if (this->CheckButton)
    {
    this->CheckButton->SetState(this->CornerAnnotation->GetVisibility());
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
    if (this->CornerText[i]->GetWidget()->GetValue())
      {
      *file << this->CornerText[i]->GetWidget()->GetValue();
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
  os << indent << "TraceHelper: " << this->TraceHelper << endl;
}

