/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLTextPropertyReader.cxx
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
#include "vtkXMLTextPropertyReader.h"

#include "vtkObjectFactory.h"
#include "vtkTextProperty.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLTextPropertyReader);
vtkCxxRevisionMacro(vtkXMLTextPropertyReader, "1.1");

//----------------------------------------------------------------------------
char* vtkXMLTextPropertyReader::GetRootElementName()
{
  return "TextProperty";
}

//----------------------------------------------------------------------------
int vtkXMLTextPropertyReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtkTextProperty *obj = vtkTextProperty::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The TextProperty is not set!");
    return 0;
    }

  // Get attributes

  float fbuffer3[3], fval;
  int ival;

  if (elem->GetVectorAttribute("Color", 3, fbuffer3) == 3)
    {
    obj->SetColor(fbuffer3);
    }

  if (elem->GetScalarAttribute("Opacity", fval))
    {
    obj->SetOpacity(fval);
    }

  if (elem->GetScalarAttribute("FontFamily", ival))
    {
    obj->SetFontFamily(ival);
    }

  if (elem->GetScalarAttribute("FontSize", ival))
    {
    obj->SetFontSize(ival);
    }

  if (elem->GetScalarAttribute("Bold", ival))
    {
    obj->SetBold(ival);
    }

  if (elem->GetScalarAttribute("Italic", ival))
    {
    obj->SetItalic(ival);
    }

  if (elem->GetScalarAttribute("Shadow", ival))
    {
    obj->SetShadow(ival);
    }

  if (elem->GetScalarAttribute("AntiAliasing", ival))
    {
    obj->SetAntiAliasing(ival);
    }

  if (elem->GetScalarAttribute("Justification", ival))
    {
    obj->SetJustification(ival);
    }

  if (elem->GetScalarAttribute("VerticalJustification", ival))
    {
    obj->SetVerticalJustification(ival);
    }

  if (elem->GetScalarAttribute("LineOffset", fval))
    {
    obj->SetLineOffset(fval);
    }

  if (elem->GetScalarAttribute("LineSpacing", fval))
    {
    obj->SetLineSpacing(fval);
    }

  return 1;
}
