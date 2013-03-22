/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVWebUtilities.h"

#include "vtkDataSetAttributes.h"
#include "vtkDataSet.h"
#include "vtkJavaScriptDataWriter.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSplitColumnComponents.h"
#include "vtkTable.h"


#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkPVWebUtilities);
//----------------------------------------------------------------------------
vtkPVWebUtilities::vtkPVWebUtilities()
{
}

//----------------------------------------------------------------------------
vtkPVWebUtilities::~vtkPVWebUtilities()
{
}

//----------------------------------------------------------------------------
std::string vtkPVWebUtilities::WriteAttributesToJavaScript(
  int field_type, vtkDataSet* dataset)
{
  if (dataset == NULL || (
      field_type != vtkDataObject::POINT &&
      field_type != vtkDataObject::CELL) )
    {
    return "[]";
    }

  vtksys_ios::ostringstream stream;

  vtkNew<vtkDataSetAttributes> clone;
  clone->PassData(dataset->GetAttributes(field_type));
  clone->RemoveArray("vtkValidPointMask");

  vtkNew<vtkTable> table;
  table->SetRowData(clone.GetPointer());

  vtkNew<vtkSplitColumnComponents> splitter;
  splitter->SetInputDataObject(table.GetPointer());
  splitter->Update();

  vtkNew<vtkJavaScriptDataWriter> writer;
  writer->SetOutputStream(&stream);
  writer->SetInputDataObject(splitter->GetOutputDataObject(0));
  writer->SetVariableName(NULL);
  writer->SetIncludeFieldNames(false);
  writer->Write();

  return stream.str();
}

//----------------------------------------------------------------------------
std::string vtkPVWebUtilities::WriteAttributeHeadersToJavaScript(
  int field_type, vtkDataSet* dataset)
{
  if (dataset == NULL || (
      field_type != vtkDataObject::POINT &&
      field_type != vtkDataObject::CELL) )
    {
    return "[]";
    }

  vtksys_ios::ostringstream stream;
  stream << "[";

  vtkDataSetAttributes* dsa = dataset->GetAttributes(field_type);
  vtkNew<vtkDataSetAttributes> clone;
  clone->CopyAllocate(dsa, 0);
  clone->RemoveArray("vtkValidPointMask");

  vtkNew<vtkTable> table;
  table->SetRowData(clone.GetPointer());

  vtkNew<vtkSplitColumnComponents> splitter;
  splitter->SetInputDataObject(table.GetPointer());
  splitter->Update();

  dsa = vtkTable::SafeDownCast(
    splitter->GetOutputDataObject(0))->GetRowData();

  for (int cc=0; cc < dsa->GetNumberOfArrays(); cc++)
    {
    const char* name = dsa->GetArrayName(cc);
    if (cc != 0)
      {
      stream << ", ";
      }
    stream << "\"" << (name? name : "") << "\"";
    }
  stream << "]";
  return stream.str();
}

//----------------------------------------------------------------------------
void vtkPVWebUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
