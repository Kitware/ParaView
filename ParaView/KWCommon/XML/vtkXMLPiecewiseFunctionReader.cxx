/*=========================================================================

  Module:    vtkXMLPiecewiseFunctionReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPiecewiseFunctionReader.h"

#include "vtkPiecewiseFunction.h"
#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLPiecewiseFunctionWriter.h"

vtkStandardNewMacro(vtkXMLPiecewiseFunctionReader);
vtkCxxRevisionMacro(vtkXMLPiecewiseFunctionReader, "1.3");

//----------------------------------------------------------------------------
char* vtkXMLPiecewiseFunctionReader::GetRootElementName()
{
  return "PiecewiseFunction";
}

//----------------------------------------------------------------------------
int vtkXMLPiecewiseFunctionReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtkPiecewiseFunction *obj = vtkPiecewiseFunction::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The PiecewiseFunction is not set!");
    return 0;
    }

  // Get attributes

  int ival;

  if (elem->GetScalarAttribute("Clamping", ival))
    {
    obj->SetClamping(ival);
    }

  // Get the points

  obj->RemoveAllPoints();

  int nb_nested_elems = elem->GetNumberOfNestedElements();
  for (int idx = 0; idx < nb_nested_elems; idx++)
    {
    vtkXMLDataElement *nested_elem = elem->GetNestedElement(idx);
    if (!strcmp(nested_elem->GetName(), 
                vtkXMLPiecewiseFunctionWriter::GetPointElementName()))
      {
      float x, val;
      if (nested_elem->GetScalarAttribute("X", x) &&
          nested_elem->GetScalarAttribute("Value", val))
        {
        obj->AddPoint(x, val);
        }
      }
    }
  
  return 1;
}


