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

#include "vtkCellData.h"
#include "vtkDelimitedTextReader.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariant.h"
#include "vtkDoubleArray.h"

#include <vtksys/RegularExpression.hxx>
#include <vtkstd/string>
#include <vtkstd/vector>

vtkStandardNewMacro(vtkCSVReader);
vtkCxxRevisionMacro(vtkCSVReader, "1.1");

//-----------------------------------------------------------------------------
vtkCSVReader::vtkCSVReader()
{
  this->FileName = 0;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->FieldDelimiterCharacters = 0;
  this->SetFieldDelimiterCharacters(",");
  this->StringDelimiter = '"';
  this->UseStringDelimiter = true;
  this->HaveHeaders = false;
  this->MergeConsecutiveDelimiters = false;
}

//-----------------------------------------------------------------------------
vtkCSVReader::~vtkCSVReader()
{
  this->SetFileName(0);
  this->SetFieldDelimiterCharacters(0);

}

enum ColumnCategories
{
  POINT_X=0,
  POINT_Y=1,
  POINT_Z=2,
  POINT_DATA,
  CELL_DATA,
  INDETERMINATE
};

struct ColumnInfo
{
  ColumnCategories Type;
  vtkstd::string ArrayName;
};

//-----------------------------------------------------------------------------
int vtkCSVReader::RequestData(vtkInformation*, vtkInformationVector**, 
  vtkInformationVector* outputVector)
{
  vtkRectilinearGrid* output = vtkRectilinearGrid::GetData(outputVector, 0); 

  // Create the internal reader and read the file.
  // TODO: we need to pass the update request correctly to this reader.
  vtkSmartPointer<vtkDelimitedTextReader> reader = 
    vtkSmartPointer<vtkDelimitedTextReader>::New();
  reader->SetFileName(this->FileName);
  reader->SetFieldDelimiterCharacters(this->FieldDelimiterCharacters);
  reader->SetStringDelimiter(this->StringDelimiter);
  reader->SetUseStringDelimiter(this->UseStringDelimiter);
  reader->SetHaveHeaders(this->HaveHeaders);
  reader->SetMergeConsecutiveDelimiters(this->MergeConsecutiveDelimiters);

  reader->Update();

  vtkTable* table = reader->GetOutput();

  /// *** Determine column categories.
  // We look at CSV headers to check if the CSV  provides additional 
  // information about the columns, such as which columns are 
  // cell data, point data, points etc. If no such information is
  // available, we create a 1D rectilinear grid with all arrays
  // as cell data.

  vtkstd::vector<ColumnInfo> columnInfos;

  vtksys::RegularExpression regX("^X::Point$");
  vtksys::RegularExpression regY("^Y::Point$");
  vtksys::RegularExpression regZ("^Z::Point$");
  vtksys::RegularExpression regPointData("^(.*)::PointData$");
  vtksys::RegularExpression regCellData("^(.*)::CellData$");

  vtkIdType cc;
  vtkIdType numColumns = table->GetNumberOfColumns();

  // these flags are used to ensure that only 1 array 
  // is maked for each point component.
  bool x_added =false;
  bool y_added =false;
  bool z_added =false;

  for (cc=0; cc < numColumns; cc++)
    {
    ColumnInfo info;
    info.Type = INDETERMINATE;
    vtkstd::string col_name = table->GetColumnName(cc);
    if (this->HaveHeaders)
      {
      if (!x_added && regX.find(col_name))
        {
        info.Type = POINT_X;
        info.ArrayName = col_name;
        x_added = true;
        }
      else if (!y_added && regY.find(col_name))
        {
        info.Type = POINT_Y;
        info.ArrayName = col_name;
        y_added = true;
        }
      else if (!z_added && regZ.find(col_name))
        {
        info.Type = POINT_Z;
        info.ArrayName = col_name;
        z_added = true;
        }
      else if (regPointData.find(col_name))
        {
        info.Type = POINT_DATA;
        info.ArrayName =regPointData.match(1);
        }
      else if (regCellData.find(col_name))
        {
        info.Type = CELL_DATA;
        info.ArrayName =regCellData.match(1);
        }
      }

    if (info.Type == INDETERMINATE)
      {
      info.Type = CELL_DATA;
      info.ArrayName = col_name;
      }
    columnInfos.push_back(info);
    }

  // if no points were found, all the point data/cell data distinction is
  // bogus anyways.
  bool ignore_classification = !(x_added && y_added && z_added);

  /// *** Determine column types.
  // The reader reader the data into a vtkTable with all string arrays.
  // For the data to be any useful, we need to convert non-string data
  // into non-string data arrays.
  // Hence, for each column we determine if it can be converted to a double array.
  vtkstd::vector<vtkSmartPointer<vtkAbstractArray> > outputArrays;
  for (cc=0; cc < numColumns; cc++)
    {
    vtkSmartPointer<vtkDoubleArray> doubleArray = 
      vtkSmartPointer<vtkDoubleArray>::New();
    vtkStringArray* strArray = 
      vtkStringArray::SafeDownCast(table->GetColumn(cc));
    if (!strArray)
      {
      // just push it to the output as is.
      outputArrays.push_back(table->GetColumn(cc));
      continue;
      }

    // iterate over all string values converting them to doubles
    // if possible
    doubleArray->SetNumberOfComponents(strArray->GetNumberOfComponents());
    doubleArray->SetNumberOfTuples(0);
    doubleArray->SetName(strArray->GetName());

    bool valid = false;
    for (vtkIdType ii=0; ii < strArray->GetNumberOfValues(); ii++)
      {
      vtkVariant v(strArray->GetValue(ii));
      if (v.ToString() == "")
        {
        // empty values -- it's possible for end rows to have empty values.
        // However, once an empty value is encoundered in any column, 
        // there can be no more non-empty values in it.
        valid = true;
        break;
        }
      else
        {
        doubleArray->InsertNextTuple1(v.ToDouble(&valid));
        }

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

  /// *** Determine dimensions. 
  // CSV cannot directly give any information about the dimensionality of
  // the rectilinear grid. If the CSV explicity flags a few columns
  // as points, we try to estimate dimensionality by using the range for 
  // point components (we can be smarter here by counting distinct
  // point components, but its unnecessarily complicated, and may not
  // be as useful since this reader must typically generate only
  // 1D rectlinear grids). 
  // If CSV does not identify points columns at all,
  // then we are simply making a 1D rectilinear grid.
  int dimensions[3] = {0,0,0};
  for (cc=0; cc < numColumns && !ignore_classification; cc++)
    {
    ColumnInfo info = columnInfos[cc];
    vtkDataArray* columnArray = vtkDataArray::SafeDownCast(outputArrays[cc]);
    if (info.Type > POINT_Z)
      {
      continue;
      }
    if (!columnArray)
      {
      // A points array is non-numeric, we simply ditch the 
      // classification and put everything in cell data.
      ignore_classification = true;
      break;
      }
    double range[2];
    columnArray->GetRange(range);
    dimensions[info.Type] = (range[0] == range[1])? 1: columnArray->GetNumberOfTuples();
    }

  if (!ignore_classification)
    {
    // We ensure that the output is going to be a 1D rectilinear grid
    // with out interpretation for the dimensions.
    int non_unit_dimensions = 0;
    non_unit_dimensions += dimensions[0]>1? 1: 0;
    non_unit_dimensions += dimensions[1]>1? 1: 0;
    non_unit_dimensions += dimensions[2]>1? 1: 0;
    if (non_unit_dimensions > 1)
      {
      // output as we are interpreting won't be a 1D rectilinear grid,
      // hence ditch the classification.
      ignore_classification = true;
      }
    }

  if (ignore_classification)
    {
    // Create points for the output.
    dimensions[0] = table->GetNumberOfRows() + 1;
    dimensions[1] = 1;
    dimensions[2] = 1;

    vtkSmartPointer<vtkDoubleArray> yzCoordinates = 
      vtkSmartPointer<vtkDoubleArray>::New();
    yzCoordinates->SetNumberOfComponents(1);
    yzCoordinates->SetNumberOfTuples(1);
    yzCoordinates->SetTuple1(0, 0);
    output->SetYCoordinates(yzCoordinates);
    output->SetZCoordinates(yzCoordinates);

    vtkSmartPointer<vtkDoubleArray> xCoordinates = 
      vtkSmartPointer<vtkDoubleArray>::New();
    xCoordinates->SetNumberOfComponents(1);
    xCoordinates->SetNumberOfTuples(dimensions[0]);
    for (cc=0; cc < dimensions[0]; cc++)
      {
      xCoordinates->SetTuple1(cc, cc);
      }
    output->SetXCoordinates(xCoordinates);
    }

  output->SetDimensions(dimensions);

  vtkPointData* pd = output->GetPointData();
  vtkCellData* cd = output->GetCellData();

  vtkIdType numPoints = output->GetNumberOfPoints();
  vtkIdType numCells = output->GetNumberOfCells();

  // *** Add arrays to the output vtkRectilinearGrid.
  for (cc=0; cc < numColumns; cc++)
    {
    ColumnInfo info = columnInfos[cc];
    vtkAbstractArray* columnArray = outputArrays[cc];
    if (ignore_classification)
      {
        if (columnArray->GetNumberOfTuples() < numCells)
          {
          vtkWarningMacro("Igoring column \"" << columnArray->GetName()
            << "\" since it does not have enough values for cell data.");
          }
        else
          {
          columnArray->SetNumberOfTuples(numCells);
          cd->AddArray(columnArray);
          }
      }
    else
      {
      switch (info.Type)
        {
      case POINT_X:
        output->SetXCoordinates(vtkDataArray::SafeDownCast(columnArray));
        break;
      case POINT_Y:
        output->SetYCoordinates(vtkDataArray::SafeDownCast(columnArray));
        break;
      case POINT_Z:
        output->SetZCoordinates(vtkDataArray::SafeDownCast(columnArray));
        break;

      case POINT_DATA:
        if (columnArray->GetNumberOfTuples() < numPoints)
          {
          vtkWarningMacro("Igoring column \"" << columnArray->GetName()
            << "\" since it does not have enough values for point data.");
          }
        else
          {
          // we'll truncate additional tuples.
          columnArray->SetNumberOfTuples(numPoints);
          columnArray->SetName(info.ArrayName.c_str());
          pd->AddArray(columnArray);
          }
        break;

      case CELL_DATA:
      default:
        if (columnArray->GetNumberOfTuples() < numCells)
          {
          vtkWarningMacro("Igoring column \"" << columnArray->GetName()
            << "\" since it does not have enough values for cell data.");
          }
        else
          {
          // we'll truncate additional tuples.
          columnArray->SetNumberOfTuples(numCells);
          columnArray->SetName(info.ArrayName.c_str());
          cd->AddArray(columnArray);
          }
        break;
        }
      }
    }
  return 1;
} 

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void vtkCSVReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " 
     << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "Field delimiters: '" << this->FieldDelimiterCharacters
     << "'" << endl;
  os << indent << "String delimiter: '" << this->StringDelimiter
     << "'" << endl;
  os << indent << "UseStringDelimiter: " 
     << (this->UseStringDelimiter ? "true" : "false") << endl;
  os << indent << "HaveHeaders: " 
     << (this->HaveHeaders ? "true" : "false") << endl;
  os << indent << "MergeConsecutiveDelimiters: " 
     << (this->MergeConsecutiveDelimiters ? "true" : "false") << endl;
}
