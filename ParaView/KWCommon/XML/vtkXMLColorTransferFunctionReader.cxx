/*=========================================================================

  Module:    vtkXMLColorTransferFunctionReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLColorTransferFunctionReader.h"

#include "vtkColorTransferFunction.h"
#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLColorTransferFunctionWriter.h"

vtkStandardNewMacro(vtkXMLColorTransferFunctionReader);
vtkCxxRevisionMacro(vtkXMLColorTransferFunctionReader, "1.4");

//----------------------------------------------------------------------------
char* vtkXMLColorTransferFunctionReader::GetRootElementName()
{
  return "ColorTransferFunction";
}

//----------------------------------------------------------------------------
int vtkXMLColorTransferFunctionReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtkColorTransferFunction *obj = 
    vtkColorTransferFunction::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The ColorTransferFunction is not set!");
    return 0;
    }

  // Get attributes

  int ival;

  if (elem->GetScalarAttribute("Clamping", ival))
    {
    obj->SetClamping(ival);
    }

  if (elem->GetScalarAttribute("ColorSpace", ival))
    {
    obj->SetColorSpace(ival);
    }

  // Get the points

  obj->RemoveAllPoints();

  int nb_nested_elems = elem->GetNumberOfNestedElements();
  for (int idx = 0; idx < nb_nested_elems; idx++)
    {
    vtkXMLDataElement *nested_elem = elem->GetNestedElement(idx);
    if (!strcmp(nested_elem->GetName(), 
                vtkXMLColorTransferFunctionWriter::GetPointElementName()))
      {
      float x, fbuffer3[3];
      if (nested_elem->GetScalarAttribute("X", x) &&
          nested_elem->GetVectorAttribute("Value", 3, fbuffer3) == 3)
        {
        obj->AddRGBPoint(x, fbuffer3[0], fbuffer3[1], fbuffer3[2]);
        }
      }
    }
  
  return 1;
}


