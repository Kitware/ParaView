/*=========================================================================

  Module:    vtkXMLColorTransferFunctionWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLColorTransferFunctionWriter.h"

#include "vtkObjectFactory.h"
#include "vtkColorTransferFunction.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLColorTransferFunctionWriter);
vtkCxxRevisionMacro(vtkXMLColorTransferFunctionWriter, "1.6");

//----------------------------------------------------------------------------
char* vtkXMLColorTransferFunctionWriter::GetRootElementName()
{
  return "ColorTransferFunction";
}

//----------------------------------------------------------------------------
char* vtkXMLColorTransferFunctionWriter::GetPointElementName()
{
  return "Point";
}

//----------------------------------------------------------------------------
int vtkXMLColorTransferFunctionWriter::AddAttributes(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddAttributes(elem))
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

  elem->SetIntAttribute("Size", obj->GetSize());

  elem->SetIntAttribute("Clamping", obj->GetClamping());

  elem->SetIntAttribute("ColorSpace", obj->GetColorSpace());

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLColorTransferFunctionWriter::AddNestedElements(
  vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddNestedElements(elem))
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

  // Iterate over all points and create a point XML data element for each one

  int size = obj->GetSize();
  double *data_ptr = obj->GetDataPointer();

  if (size && data_ptr)
    {
    for (int i = 0; i < size; i++, data_ptr += 4)
      {
      vtkXMLDataElement *point_elem = this->NewDataElement();
      elem->AddNestedElement(point_elem);
      point_elem->Delete();
      point_elem->SetName(this->GetPointElementName());
      point_elem->SetDoubleAttribute("X", data_ptr[0]);
      point_elem->SetVectorAttribute("Value", 3, data_ptr + 1);
      }
    }

  return 1;
}


