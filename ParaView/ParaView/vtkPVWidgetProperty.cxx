/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVWidgetProperty.cxx
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
#include "vtkPVWidgetProperty.h"

#include "vtkObjectFactory.h"
#include "vtkPVWidget.h"

vtkStandardNewMacro(vtkPVWidgetProperty);
vtkCxxRevisionMacro(vtkPVWidgetProperty, "1.1.2.3");

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
  
  if (this->Widget)
    {
    this->Widget->UnRegister(this);
    }
  this->Widget = widget;
  if (this->Widget)
    {
    this->Widget->Register(this);
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
