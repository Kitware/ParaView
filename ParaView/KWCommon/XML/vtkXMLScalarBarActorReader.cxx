/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLScalarBarActorReader.cxx
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
#include "vtkXMLScalarBarActorReader.h"

#include "vtkObjectFactory.h"
#include "vtkScalarBarActor.h"
#include "vtkTextProperty.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLScalarBarActorWriter.h"
#include "vtkXMLTextPropertyReader.h"

vtkStandardNewMacro(vtkXMLScalarBarActorReader);
vtkCxxRevisionMacro(vtkXMLScalarBarActorReader, "1.2");

//----------------------------------------------------------------------------
char* vtkXMLScalarBarActorReader::GetRootElementName()
{
  return "ScalarBarActor";
}

//----------------------------------------------------------------------------
int vtkXMLScalarBarActorReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtkScalarBarActor *obj = vtkScalarBarActor::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The ScalarBarActor is not set!");
    return 0;
    }

  // Get attributes

  int ival;
  const char *cptr;

  if (elem->GetScalarAttribute("MaximumNumberOfColors", ival))
    {
    obj->SetMaximumNumberOfColors(ival);
    }

  if (elem->GetScalarAttribute("NumberOfLabels", ival))
    {
    obj->SetNumberOfLabels(ival);
    }

  if (elem->GetScalarAttribute("Orientation", ival))
    {
    obj->SetOrientation(ival);
    }
  
  cptr = elem->GetAttribute("LabelFormat");
  if (cptr)
    {
    obj->SetLabelFormat(cptr);
    }

  cptr = elem->GetAttribute("Title");
  if (cptr)
    {
    obj->SetTitle(cptr);
    }
  
  // Get nested elements
  
  // Title and label text property

  vtkXMLTextPropertyReader *xmlr = vtkXMLTextPropertyReader::New();

  if (xmlr->IsInNestedElement(
        elem, vtkXMLScalarBarActorWriter::GetTitleTextPropertyElementName()))
    {
    vtkTextProperty *tprop = obj->GetTitleTextProperty();
    if (!tprop)
      {
      tprop = vtkTextProperty::New();
      obj->SetTitleTextProperty(tprop);
      tprop->Delete();
      }
    xmlr->SetObject(tprop);
    xmlr->ParseInNestedElement(
      elem, vtkXMLScalarBarActorWriter::GetTitleTextPropertyElementName());
    }

  if (xmlr->IsInNestedElement(
        elem, vtkXMLScalarBarActorWriter::GetLabelTextPropertyElementName()))
    {
    vtkTextProperty *tprop = obj->GetLabelTextProperty();
    if (!tprop)
      {
      tprop = vtkTextProperty::New();
      obj->SetLabelTextProperty(tprop);
      tprop->Delete();
      }
    xmlr->SetObject(tprop);
    xmlr->ParseInNestedElement(
      elem, vtkXMLScalarBarActorWriter::GetLabelTextPropertyElementName());
    }

  xmlr->Delete();

  return 1;
}
