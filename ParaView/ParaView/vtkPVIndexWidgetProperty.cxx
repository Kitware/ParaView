/*=========================================================================

  Program:   ParaView
  Module:    vtkPVIndexWidgetProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVIndexWidgetProperty.h"

#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVWidget.h"
#include "vtkClientServerStream.h"

vtkStandardNewMacro(vtkPVIndexWidgetProperty);
vtkCxxRevisionMacro(vtkPVIndexWidgetProperty, "1.5");

vtkPVIndexWidgetProperty::vtkPVIndexWidgetProperty()
{
  this->Index = 0;
  this->VTKCommand = NULL;
}

vtkPVIndexWidgetProperty::~vtkPVIndexWidgetProperty()
{
  this->SetVTKCommand(NULL);
}

void vtkPVIndexWidgetProperty::AcceptInternal()
{ 
  vtkPVProcessModule* pm = this->Widget->GetPVApplication()->GetProcessModule();
  pm->GetStream() << vtkClientServerStream::Invoke << this->VTKSourceID
                    << this->VTKCommand << this->Index << vtkClientServerStream::End;
  pm->SendStreamToServer();
}

void vtkPVIndexWidgetProperty::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "Index: " << this->Index << endl;
  os << indent << "VTKCommand: " << (this->VTKCommand ? this->VTKCommand :
                                     "(none)")
     << endl;
}

