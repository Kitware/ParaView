/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPlaneWidgetWriter.cxx
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
#include "vtkXMLPlaneWidgetWriter.h"

#include "vtkPlaneWidget.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLPropertyWriter.h"

vtkStandardNewMacro(vtkXMLPlaneWidgetWriter);
vtkCxxRevisionMacro(vtkXMLPlaneWidgetWriter, "1.1");

//----------------------------------------------------------------------------
char* vtkXMLPlaneWidgetWriter::GetRootElementName()
{
  return "PlaneWidget";
}

//----------------------------------------------------------------------------
char* vtkXMLPlaneWidgetWriter::GetHandlePropertyElementName()
{
  return "HandleProperty";
}

//----------------------------------------------------------------------------
char* vtkXMLPlaneWidgetWriter::GetSelectedHandlePropertyElementName()
{
  return "SelectedHandleProperty";
}

//----------------------------------------------------------------------------
char* vtkXMLPlaneWidgetWriter::GetPlanePropertyElementName()
{
  return "PlaneProperty";
}

//----------------------------------------------------------------------------
char* vtkXMLPlaneWidgetWriter::GetSelectedPlanePropertyElementName()
{
  return "SelectedPlaneProperty";
}

//----------------------------------------------------------------------------
int vtkXMLPlaneWidgetWriter::AddAttributes(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddAttributes(elem))
    {
    return 0;
    }

  vtkPlaneWidget *obj = vtkPlaneWidget::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The PlaneWidget is not set!");
    return 0;
    }

  elem->SetIntAttribute("Resolution", obj->GetResolution());

  elem->SetVectorAttribute("Origin", 3, obj->GetOrigin());

  elem->SetVectorAttribute("Point1", 3, obj->GetPoint1());

  elem->SetVectorAttribute("Point2", 3, obj->GetPoint2());

  elem->SetVectorAttribute("Center", 3, obj->GetCenter());

  elem->SetVectorAttribute("Normal", 3, obj->GetNormal());

  elem->SetIntAttribute("Representation", obj->GetRepresentation());

  elem->SetIntAttribute("NormalToXAxis", obj->GetNormalToXAxis());

  elem->SetIntAttribute("NormalToYAxis", obj->GetNormalToYAxis());

  elem->SetIntAttribute("NormalToZAxis", obj->GetNormalToZAxis());

  return 1;
}
//----------------------------------------------------------------------------
int vtkXMLPlaneWidgetWriter::AddNestedElements(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddNestedElements(elem))
    {
    return 0;
    }

  vtkPlaneWidget *obj = vtkPlaneWidget::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The PlaneWidget is not set!");
    return 0;
    }

  // Handle and Plane Property

  vtkXMLPropertyWriter *xmlw = vtkXMLPropertyWriter::New();
  vtkProperty *prop;

  prop = obj->GetHandleProperty();
  if (prop)
    {
    xmlw->SetObject(prop);
    xmlw->CreateInNestedElement(elem, this->GetHandlePropertyElementName());
    }
 
  prop = obj->GetSelectedHandleProperty();
  if (prop)
    {
    xmlw->SetObject(prop);
    xmlw->CreateInNestedElement(
      elem, this->GetSelectedHandlePropertyElementName());
    }
 
  prop = obj->GetPlaneProperty();
  if (prop)
    {
    xmlw->SetObject(prop);
    xmlw->CreateInNestedElement(elem, this->GetPlanePropertyElementName());
    }
 
  prop = obj->GetSelectedPlaneProperty();
  if (prop)
    {
    xmlw->SetObject(prop);
    xmlw->CreateInNestedElement(
      elem, this->GetSelectedPlanePropertyElementName());
    }

  xmlw->Delete();
 
  return 1;
}
