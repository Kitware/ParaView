/*=========================================================================

  Program:   ParaView
  Module:    vtkPVWidgetProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVWidgetProperty.h"

#include "vtkObjectFactory.h"
#include "vtkPVWidget.h"

vtkStandardNewMacro(vtkPVWidgetProperty);
vtkCxxRevisionMacro(vtkPVWidgetProperty, "1.6");

vtkPVWidgetProperty::vtkPVWidgetProperty()
{
  this->Widget = NULL;
  this->VTKSourceID.ID = 0;
}

vtkPVWidgetProperty::~vtkPVWidgetProperty()
{
  this->SetWidget(NULL);
  this->VTKSourceID.ID = 0;
}

void vtkPVWidgetProperty::Reset()
{
  if (!this->Widget || !this->Widget->GetApplication())
    {
    return;
    }
  
  this->Widget->Reset();
}

void vtkPVWidgetProperty::Accept()
{
  if (!this->Widget || !this->Widget->GetModifiedFlag())
    {
    return;
    }
  
  this->Widget->Accept();
}

void vtkPVWidgetProperty::SetWidget(vtkPVWidget *widget)
{
  if (this->Widget == widget)
    {
    return;
    }
  
  this->Widget = widget;
  if (this->Widget)
    {
    this->Widget->SetProperty(this);
    }
  this->Modified();
}

void vtkPVWidgetProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "Widget: ";
  if (this->Widget)
    {
    os << this->Widget << endl;
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "VTKSourceID: " << this->VTKSourceID
     << endl;
}
