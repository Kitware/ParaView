/*=========================================================================

  Module:    vtkPVTextPropertyEditor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTextPropertyEditor.h"

#include "vtkObjectFactory.h"
#include "vtkPVTraceHelper.h"

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVTextPropertyEditor);
vtkCxxRevisionMacro(vtkPVTextPropertyEditor, "1.1");

int vtkPVTextPropertyEditorCommand(ClientData cd, Tcl_Interp *interp,
                      int argc, char *argv[]);

// ----------------------------------------------------------------------------
vtkPVTextPropertyEditor::vtkPVTextPropertyEditor()
{
  this->TraceHelper = vtkPVTraceHelper::New();
  this->TraceHelper->SetObject(this);
}

// ----------------------------------------------------------------------------
vtkPVTextPropertyEditor::~vtkPVTextPropertyEditor()
{
  if (this->TraceHelper)
    {
    this->TraceHelper->Delete();
    this->TraceHelper = NULL;
    }
}

// ----------------------------------------------------------------------------
void vtkPVTextPropertyEditor::SetColor(double r, double g, double b) 
{
  this->Superclass::SetColor(r, g, b);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetColor %lf %lf %lf", this->GetTclName(), r,g,b);
}

// ----------------------------------------------------------------------------
void vtkPVTextPropertyEditor::SetFontFamily(int v) 
{
  this->Superclass::SetFontFamily(v);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetFontFamily %d", this->GetTclName(), v);
}

// ----------------------------------------------------------------------------
void vtkPVTextPropertyEditor::SetBold(int v) 
{
  this->Superclass::SetBold(v);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetBold %d", this->GetTclName(), v);
}

// ----------------------------------------------------------------------------
void vtkPVTextPropertyEditor::SetItalic(int v) 
{
  this->Superclass::SetItalic(v);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetItalic %d", this->GetTclName(), v);
}

// ----------------------------------------------------------------------------
void vtkPVTextPropertyEditor::SetShadow(int v) 
{
  this->Superclass::SetShadow(v);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetShadow %d", this->GetTclName(), v);
}

// ----------------------------------------------------------------------------
void vtkPVTextPropertyEditor::SetOpacity(float v) 
{
  this->Superclass::SetOpacity(v);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetOpacity %f", this->GetTclName(), v);
}

//----------------------------------------------------------------------------
void vtkPVTextPropertyEditor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "TraceHelper: " << this->TraceHelper << endl;
}

