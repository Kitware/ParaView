/*=========================================================================

  Module:    vtkXMLPiecewiseFunctionWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPiecewiseFunctionWriter.h"

#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLPiecewiseFunctionWriter);
vtkCxxRevisionMacro(vtkXMLPiecewiseFunctionWriter, "1.5");

//----------------------------------------------------------------------------
char* vtkXMLPiecewiseFunctionWriter::GetRootElementName()
{
  return "PiecewiseFunction";
}

//----------------------------------------------------------------------------
char* vtkXMLPiecewiseFunctionWriter::GetPointElementName()
{
  return "Point";
}

//----------------------------------------------------------------------------
int vtkXMLPiecewiseFunctionWriter::AddAttributes(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddAttributes(elem))
    {
    return 0;
    }

  vtkPiecewiseFunction *obj = vtkPiecewiseFunction::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The PiecewiseFunction is not set!");
    return 0;
    }

  elem->SetIntAttribute("Size", obj->GetSize());

  elem->SetIntAttribute("Clamping", obj->GetClamping());

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLPiecewiseFunctionWriter::AddNestedElements(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddNestedElements(elem))
    {
    return 0;
    }

  vtkPiecewiseFunction *obj = vtkPiecewiseFunction::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The PiecewiseFunction is not set!");
    return 0;
    }

  // Iterate over all points and create a point XML data element for each one

  int size = obj->GetSize();
  double *data_ptr = obj->GetDataPointer();

  if (size && data_ptr)
    {
    for (int i = 0; i < size; i++, data_ptr += 2)
      {
      vtkXMLDataElement *point_elem = this->NewDataElement();
      elem->AddNestedElement(point_elem);
      point_elem->Delete();
      point_elem->SetName(this->GetPointElementName());
      point_elem->SetDoubleAttribute("X", data_ptr[0]);
      point_elem->SetDoubleAttribute("Value", data_ptr[1]);
      }
    }

  return 1;
}


