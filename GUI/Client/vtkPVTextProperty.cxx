/*=========================================================================

  Module:    vtkPVTextProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTextProperty.h"

#include "vtkObjectFactory.h"
#include "vtkPVTraceHelper.h"

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVTextProperty);
vtkCxxRevisionMacro(vtkPVTextProperty, "1.1");

int vtkPVTextPropertyCommand(ClientData cd, Tcl_Interp *interp,
                      int argc, char *argv[]);

// ----------------------------------------------------------------------------
vtkPVTextProperty::vtkPVTextProperty()
{
  this->TraceHelper = vtkPVTraceHelper::New();
  this->TraceHelper->SetObject(this);
}

// ----------------------------------------------------------------------------
vtkPVTextProperty::~vtkPVTextProperty()
{
  if (this->TraceHelper)
    {
    this->TraceHelper->Delete();
    this->TraceHelper = NULL;
    }
}

// ----------------------------------------------------------------------------
void vtkPVTextProperty::SetColor(double r, double g, double b) 
{
  this->Superclass::SetColor(r, g, b);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetColor %lf %lf %lf", this->GetTclName(), r,g,b);
}

// ----------------------------------------------------------------------------
void vtkPVTextProperty::SetFontFamily(int v) 
{
  this->Superclass::SetFontFamily(v);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetFontFamily %d", this->GetTclName(), v);
}

// ----------------------------------------------------------------------------
void vtkPVTextProperty::SetBold(int v) 
{
  this->Superclass::SetBold(v);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetBold %d", this->GetTclName(), v);
}

// ----------------------------------------------------------------------------
void vtkPVTextProperty::SetItalic(int v) 
{
  this->Superclass::SetItalic(v);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetItalic %d", this->GetTclName(), v);
}

// ----------------------------------------------------------------------------
void vtkPVTextProperty::SetShadow(int v) 
{
  this->Superclass::SetShadow(v);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetShadow %d", this->GetTclName(), v);
}

// ----------------------------------------------------------------------------
void vtkPVTextProperty::SetOpacity(float v) 
{
  this->Superclass::SetOpacity(v);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetOpacity %f", this->GetTclName(), v);
}

//----------------------------------------------------------------------------
void vtkPVTextProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "TraceHelper: " << this->TraceHelper << endl;
}

