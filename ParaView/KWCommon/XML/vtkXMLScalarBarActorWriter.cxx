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
#include "vtkXMLScalarBarActorWriter.h"

#include "vtkObjectFactory.h"
#include "vtkScalarBarActor.h"
#include "vtkTextProperty.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLTextPropertyWriter.h"

vtkStandardNewMacro(vtkXMLScalarBarActorWriter);
vtkCxxRevisionMacro(vtkXMLScalarBarActorWriter, "1.3");

//----------------------------------------------------------------------------
char* vtkXMLScalarBarActorWriter::GetRootElementName()
{
  return "ScalarBarActor";
}

//----------------------------------------------------------------------------
char* vtkXMLScalarBarActorWriter::GetTitleTextPropertyElementName()
{
  return "TitleTextProperty";
}

//----------------------------------------------------------------------------
char* vtkXMLScalarBarActorWriter::GetLabelTextPropertyElementName()
{
  return "LabelTextProperty";
}

//----------------------------------------------------------------------------
int vtkXMLScalarBarActorWriter::AddAttributes(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddAttributes(elem))
    {
    return 0;
    }

  vtkScalarBarActor *obj = vtkScalarBarActor::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The ScalarBarActor is not set!");
    return 0;
    }

  elem->SetIntAttribute(
    "MaximumNumberOfColors", obj->GetMaximumNumberOfColors());

  elem->SetIntAttribute("NumberOfLabels", obj->GetNumberOfLabels());

  elem->SetIntAttribute("Orientation", obj->GetOrientation());

  elem->SetAttribute("LabelFormat", obj->GetLabelFormat());

  elem->SetAttribute("Title", obj->GetTitle());

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLScalarBarActorWriter::AddNestedElements(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddNestedElements(elem))
    {
    return 0;
    }

  vtkScalarBarActor *obj = vtkScalarBarActor::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The ScalarBarActor is not set!");
    return 0;
    }

  // Title text property

  vtkTextProperty *tprop = obj->GetTitleTextProperty();
  if (tprop)
    {
    vtkXMLTextPropertyWriter *xmlw = vtkXMLTextPropertyWriter::New();
    xmlw->SetObject(tprop);
    xmlw->CreateInNestedElement(elem, this->GetTitleTextPropertyElementName());
    xmlw->Delete();
    }
 
  // Label text property

  vtkTextProperty *lprop = obj->GetLabelTextProperty();
  if (lprop)
    {
    vtkXMLTextPropertyWriter *xmlw = vtkXMLTextPropertyWriter::New();
    xmlw->SetObject(lprop);
    xmlw->CreateInNestedElement(elem, this->GetLabelTextPropertyElementName());
    xmlw->Delete();
    }
 
  return 1;
}


