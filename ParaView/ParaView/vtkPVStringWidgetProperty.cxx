/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVStringWidgetProperty.cxx
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
#include "vtkPVStringWidgetProperty.h"

#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVWidget.h"

vtkStandardNewMacro(vtkPVStringWidgetProperty);
vtkCxxRevisionMacro(vtkPVStringWidgetProperty, "1.1.2.6");

vtkPVStringWidgetProperty::vtkPVStringWidgetProperty()
{
  this->String = NULL;
  this->VTKCommand = NULL;
  this->ElementType = vtkPVSelectWidget::STRING;
}

vtkPVStringWidgetProperty::~vtkPVStringWidgetProperty()
{
  this->SetString(NULL);
  this->SetVTKCommand(NULL);
}

void vtkPVStringWidgetProperty::SetStringType(vtkPVSelectWidget::ElementTypes t)
{
  this->ElementType = t;
}


void vtkPVStringWidgetProperty::AcceptInternal()
{
  vtkPVApplication *pvApp = this->GetWidget()->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  
  if (this->String[0] == '[')
    {
    vtkErrorMacro("nasty [ used in string for vtkPVStringWidgetProperty");
    return;
    }
  
  pm->GetStream() << vtkClientServerStream::Invoke
                  << this->VTKSourceID 
                  << this->VTKCommand;
  switch(this->ElementType)
    {
    case vtkPVSelectWidget::INT:
      pm->GetStream() << atoi(this->String);
      break;
    case vtkPVSelectWidget::FLOAT:
      pm->GetStream() << atof(this->String);
      break;
    case vtkPVSelectWidget::STRING:
      pm->GetStream() << this->String;
      break;
    case vtkPVSelectWidget::OBJECT:
      {
      pm->GetStream() << this->ObjectID;
      }
      break;
    }
  
  pm->GetStream() << vtkClientServerStream::End;
  pm->SendStreamToServer();
}

void vtkPVStringWidgetProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "String: " << (this->String ? this->String : "(none)")
     << endl;
  os << indent << "VTKCommand: " << (this->VTKCommand ? this->VTKCommand :
                                     "(none")
     << endl;
}
