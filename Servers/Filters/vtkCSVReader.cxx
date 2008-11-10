/*=========================================================================

  Program:   ParaView
  Module:    vtkCSVReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCSVReader.h"

#include "vtkDataSetAttributes.h"
#include "vtkDelimitedTextReader.h"
#include "vtkDoubleArray.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkSmartPointer.h"
#include "vtkVariant.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkCSVReader);
vtkCxxRevisionMacro(vtkCSVReader, "1.4");

//-----------------------------------------------------------------------------
vtkCSVReader::vtkCSVReader()
{
}

//-----------------------------------------------------------------------------
vtkCSVReader::~vtkCSVReader()
{
}

//-----------------------------------------------------------------------------
int vtkCSVReader::RequestData(vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestData(request, inputVector, outputVector))
    {
    return 0;
    }

  vtkTable* table = vtkTable::GetData(outputVector, 0);
  vtkIdType numColumns = table->GetNumberOfColumns();

  /// *** Determine column types.
  // The reader reads the data into a vtkTable with all string arrays.
  // For the data to be any useful, we need to convert non-string data
  // into non-string data arrays.
  // Hence, for each column we determine if it can be converted to a double array.
  vtkstd::vector<vtkSmartPointer<vtkAbstractArray> > outputArrays;
  for (vtkIdType cc=0; cc < numColumns; cc++)
    {
    vtkStringArray* strArray = 
      vtkStringArray::SafeDownCast(table->GetColumn(cc));
    if (!strArray)
      {
      // just push it to the output as is.
      outputArrays.push_back(table->GetColumn(cc));
      continue;
      }
    vtkSmartPointer<vtkDoubleArray> doubleArray = 
      vtkSmartPointer<vtkDoubleArray>::New();

    // iterate over all string values converting them to doubles
    // if possible
    doubleArray->SetNumberOfComponents(strArray->GetNumberOfComponents());
    doubleArray->SetNumberOfTuples(strArray->GetNumberOfTuples());
    doubleArray->SetName(strArray->GetName());

    vtkIdType numValues = strArray->GetNumberOfValues();

    bool valid = false;
    for (vtkIdType ii=0; ii < numValues; ii++)
      {
      vtkVariant v(strArray->GetValue(ii));
      doubleArray->SetValue(ii, v.ToDouble(&valid));
      if (!valid)
        {
        // conversion failed, cannot convert array.
        break; 
        }
      }
    // Push the doubleArray if conversion was successful, else the original
    // string array.
    if (valid)
      {
      outputArrays.push_back(doubleArray);
      }
    else
      {
      outputArrays.push_back(strArray);
      }
    }

  table->GetRowData()->Initialize();
  for (unsigned int kk=0; kk < outputArrays.size(); kk++)
    {
    table->GetRowData()->AddArray(outputArrays[kk]);
    }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkCSVReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
