/*=========================================================================

  Program:   ParaView
  Module:    vtkPVStringWidgetProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVStringWidgetProperty.h"

#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVWidget.h"
#include "vtkClientServerStream.h"

vtkStandardNewMacro(vtkPVStringWidgetProperty);
vtkCxxRevisionMacro(vtkPVStringWidgetProperty, "1.5");

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
  os << indent << "ObjectID: " << this->ObjectID << endl;
  os << indent << "VTKCommand: " << (this->VTKCommand ? this->VTKCommand :
                                     "(none")
     << endl;
}
