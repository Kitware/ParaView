/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPlaneWidgetReader.cxx
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
#include "vtkXMLPlaneWidgetReader.h"

#include "vtkPlaneWidget.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLPropertyReader.h"
#include "vtkXMLPlaneWidgetWriter.h"

vtkStandardNewMacro(vtkXMLPlaneWidgetReader);
vtkCxxRevisionMacro(vtkXMLPlaneWidgetReader, "1.1");

//----------------------------------------------------------------------------
char* vtkXMLPlaneWidgetReader::GetRootElementName()
{
  return "PlaneWidget";
}

//----------------------------------------------------------------------------
int vtkXMLPlaneWidgetReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtkPlaneWidget *obj = vtkPlaneWidget::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The PlaneWidget is not set!");
    return 0;
    }

  // Get attributes

  float fbuffer3[3];
  int ival;

  if (elem->GetScalarAttribute("Resolution", ival))
    {
    obj->SetResolution(ival);
    }

  if (elem->GetVectorAttribute("Origin", 3, fbuffer3) == 3)
    {
    obj->SetOrigin(fbuffer3);
    }

  if (elem->GetVectorAttribute("Point1", 3, fbuffer3) == 3)
    {
    obj->SetPoint1(fbuffer3);
    }

  if (elem->GetVectorAttribute("Point2", 3, fbuffer3) == 3)
    {
    obj->SetPoint2(fbuffer3);
    }

  if (elem->GetVectorAttribute("Center", 3, fbuffer3) == 3)
    {
    obj->SetCenter(fbuffer3);
    }

  if (elem->GetVectorAttribute("Normal", 3, fbuffer3) == 3)
    {
    obj->SetNormal(fbuffer3);
    }

  if (elem->GetScalarAttribute("Representation", ival))
    {
    obj->SetRepresentation(ival);
    }

  if (elem->GetScalarAttribute("NormalToXAxis", ival))
    {
    obj->SetNormalToXAxis(ival);
    }

  if (elem->GetScalarAttribute("NormalToYAxis", ival))
    {
    obj->SetNormalToYAxis(ival);
    }

  if (elem->GetScalarAttribute("NormalToZAxis", ival))
    {
    obj->SetNormalToZAxis(ival);
    }

  // Get nested elements
  
  // Handle and plane property

  vtkXMLPropertyReader *xmlr = vtkXMLPropertyReader::New();
  vtkProperty *prop;

  prop = obj->GetHandleProperty();
  if (prop)
    {
    xmlr->SetObject(prop);
    xmlr->ParseInNestedElement(
      elem, vtkXMLPlaneWidgetWriter::GetHandlePropertyElementName());
    }

  prop = obj->GetSelectedHandleProperty();
  if (prop)
    {
    xmlr->SetObject(prop);
    xmlr->ParseInNestedElement(
      elem, vtkXMLPlaneWidgetWriter::GetSelectedHandlePropertyElementName());
    }

  if (xmlr->IsInNestedElement(
        elem, vtkXMLPlaneWidgetWriter::GetPlanePropertyElementName()))
    {
    prop = obj->GetPlaneProperty();
    if (!prop)
      {
      prop = vtkProperty::New();
      obj->SetPlaneProperty(prop);
      prop->Delete();
      }
    xmlr->SetObject(prop);
    xmlr->ParseInNestedElement(
      elem, vtkXMLPlaneWidgetWriter::GetPlanePropertyElementName());
    }

  prop = obj->GetSelectedPlaneProperty();
  if (prop)
    {
    xmlr->SetObject(prop);
    xmlr->ParseInNestedElement(
      elem, vtkXMLPlaneWidgetWriter::GetSelectedPlanePropertyElementName());
    }

  xmlr->Delete();

  return 1;
}
