/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLCornerAnnotationWriter.cxx
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
#include "vtkXMLCornerAnnotationWriter.h"

#include "vtkCornerAnnotation.h"
#include "vtkObjectFactory.h"
#include "vtkTextProperty.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLTextPropertyWriter.h"

vtkStandardNewMacro(vtkXMLCornerAnnotationWriter);
vtkCxxRevisionMacro(vtkXMLCornerAnnotationWriter, "1.1");

//----------------------------------------------------------------------------
char* vtkXMLCornerAnnotationWriter::GetRootElementName()
{
  return "CornerAnnotation";
}

//----------------------------------------------------------------------------
int vtkXMLCornerAnnotationWriter::AddAttributes(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddAttributes(elem))
    {
    return 0;
    }

  vtkCornerAnnotation *obj = vtkCornerAnnotation::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The CornerAnnotation is not set!");
    return 0;
    }

  elem->SetIntAttribute("Visibility", obj->GetVisibility());

  for (int i = 0; i < 4; i++)
    {
    ostrstream text_name;
    text_name << "Text" << i << ends;
    elem->SetAttribute(text_name.str(), obj->GetText(i));
    text_name.rdbuf()->freeze(0);
    }

  elem->SetFloatAttribute("MaximumLineHeight", obj->GetMaximumLineHeight());

  elem->SetIntAttribute("MinimumFontSize", obj->GetMinimumFontSize());

  elem->SetFloatAttribute("LevelShift", obj->GetLevelShift());

  elem->SetFloatAttribute("LevelScale", obj->GetLevelScale());

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLCornerAnnotationWriter::AddNestedElements(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddNestedElements(elem))
    {
    return 0;
    }

  vtkCornerAnnotation *obj = vtkCornerAnnotation::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The CornerAnnotation is not set!");
    return 0;
    }

  // Text property

  vtkTextProperty *tprop = obj->GetTextProperty();
  if (tprop)
    {
    vtkXMLDataElement *nested_elem = vtkXMLDataElement::New();
    elem->AddNestedElement(nested_elem);
    nested_elem->Delete();
    vtkXMLTextPropertyWriter *xmlw = vtkXMLTextPropertyWriter::New();
    xmlw->SetObject(tprop);
    xmlw->Create(nested_elem);
    xmlw->Delete();
    }
  
  return 1;
}
