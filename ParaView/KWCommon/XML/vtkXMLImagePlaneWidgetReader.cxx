/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
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
#include "vtkXMLImagePlaneWidgetReader.h"

#include "vtkImagePlaneWidget.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkTextProperty.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLImagePlaneWidgetWriter.h"
#include "vtkXMLPropertyReader.h"
#include "vtkXMLTextPropertyReader.h"

vtkStandardNewMacro(vtkXMLImagePlaneWidgetReader);
vtkCxxRevisionMacro(vtkXMLImagePlaneWidgetReader, "1.1.4.2");

//----------------------------------------------------------------------------
char* vtkXMLImagePlaneWidgetReader::GetRootElementName()
{
  return "ImagePlaneWidget";
}

//----------------------------------------------------------------------------
int vtkXMLImagePlaneWidgetReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtkImagePlaneWidget *obj = vtkImagePlaneWidget::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The ImagePlaneWidget is not set!");
    return 0;
    }

  // Get attributes

  float fbuffer3[3];
  int ival;

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

  if (elem->GetScalarAttribute("ResliceInterpolate", ival))
    {
    obj->SetResliceInterpolate(ival);
    }

  if (elem->GetScalarAttribute("RestrictPlaneToVolume", ival))
    {
    obj->SetRestrictPlaneToVolume(ival);
    }

  if (elem->GetScalarAttribute("TextureInterpolate", ival))
    {
    obj->SetTextureInterpolate(ival);
    }

  if (elem->GetScalarAttribute("TextureVisibility", ival))
    {
    obj->SetTextureVisibility(ival);
    }

  if (elem->GetScalarAttribute("DisplayText", ival))
    {
    obj->SetDisplayText(ival);
    }

  // Get nested elements
  
  // Plane properties

  vtkXMLPropertyReader *xmlr = vtkXMLPropertyReader::New();
  vtkProperty *prop;

  prop = obj->GetPlaneProperty();
  if (prop)
    {
    xmlr->SetObject(prop);
    xmlr->ParseInNestedElement(
      elem, vtkXMLImagePlaneWidgetWriter::GetPlanePropertyElementName());
    }

  prop = obj->GetSelectedPlaneProperty();
  if (prop)
    {
    xmlr->SetObject(prop);
    xmlr->ParseInNestedElement(
      elem, 
      vtkXMLImagePlaneWidgetWriter::GetSelectedPlanePropertyElementName());
    }

  prop = obj->GetCursorProperty();
  if (prop)
    {
    xmlr->SetObject(prop);
    xmlr->ParseInNestedElement(
      elem, vtkXMLImagePlaneWidgetWriter::GetCursorPropertyElementName());
    }

  prop = obj->GetMarginProperty();
  if (prop)
    {
    xmlr->SetObject(prop);
    xmlr->ParseInNestedElement(
      elem, vtkXMLImagePlaneWidgetWriter::GetMarginPropertyElementName());
    }

  prop = obj->GetTexturePlaneProperty();
  if (prop)
    {
    xmlr->SetObject(prop);
    xmlr->ParseInNestedElement(
      elem, vtkXMLImagePlaneWidgetWriter::GetTexturePlanePropertyElementName());
    }

  xmlr->Delete();

  // Text properties

  vtkXMLTextPropertyReader *xmltr = vtkXMLTextPropertyReader::New();
  vtkTextProperty *tprop;

  tprop = obj->GetTextProperty();
  if (tprop)
    {
    xmltr->SetObject(tprop);
    xmltr->ParseInNestedElement(
      elem, vtkXMLImagePlaneWidgetWriter::GetTextPropertyElementName());
    }

  xmltr->Delete();

  return 1;
}


