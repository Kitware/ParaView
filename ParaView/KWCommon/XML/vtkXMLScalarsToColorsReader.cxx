/*=========================================================================

  Module:    vtkXMLScalarsToColorsReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLScalarsToColorsReader.h"

#include "vtkObjectFactory.h"
#include "vtkScalarsToColors.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLScalarsToColorsReader);
vtkCxxRevisionMacro(vtkXMLScalarsToColorsReader, "1.3");

//----------------------------------------------------------------------------
char* vtkXMLScalarsToColorsReader::GetRootElementName()
{
  return "ScalarsToColors";
}

//----------------------------------------------------------------------------
int vtkXMLScalarsToColorsReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtkScalarsToColors *obj = vtkScalarsToColors::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The ScalarsToColors is not set!");
    return 0;
    }

  // Get attributes

  float fval;
  int ival;

  if (elem->GetScalarAttribute("Alpha", fval))
    {
    obj->SetAlpha(fval);
    }

  if (elem->GetScalarAttribute("VectorMode", ival))
    {
    obj->SetVectorMode(ival);
    }

  if (elem->GetScalarAttribute("VectorComponent", ival))
    {
    obj->SetVectorComponent(ival);
    }

  return 1;
}


