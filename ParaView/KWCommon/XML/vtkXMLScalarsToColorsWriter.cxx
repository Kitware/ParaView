/*=========================================================================

  Module:    vtkXMLScalarsToColorsWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLScalarsToColorsWriter.h"

#include "vtkObjectFactory.h"
#include "vtkScalarsToColors.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLScalarsToColorsWriter);
vtkCxxRevisionMacro(vtkXMLScalarsToColorsWriter, "1.3");

//----------------------------------------------------------------------------
char* vtkXMLScalarsToColorsWriter::GetRootElementName()
{
  return "ScalarsToColors";
}

//----------------------------------------------------------------------------
int vtkXMLScalarsToColorsWriter::AddAttributes(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddAttributes(elem))
    {
    return 0;
    }

  vtkScalarsToColors *obj = vtkScalarsToColors::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The ScalarsToColors is not set!");
    return 0;
    }

  elem->SetFloatAttribute("Alpha", obj->GetAlpha());

  elem->SetIntAttribute("VectorMode", obj->GetVectorMode());

  elem->SetIntAttribute("VectorComponent", obj->GetVectorComponent());

  return 1;
}


