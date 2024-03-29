// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
// .NAME DataMineBlockMpdelReader.cxx
// Read DataMine binary files for single objects.
// point, perimeter (polyline), wframe<points/triangle>
// With or without properties (each property name < 8 characters)
// The binary file reading is done by 'dmfile.cxx'
//     99-04-12: Written by Jeremy Maccelari, visualn@iafrica.com

#include "vtkDataMineBlockReader.h"
#include "ThirdParty/PropertyStorage.h"
#include "ThirdParty/dmfile.h"

#include "vtkCellArray.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkDataMineBlockReader);

// --------------------------------------
vtkDataMineBlockReader::vtkDataMineBlockReader()
{
  this->CellMode = VTK_VERTEX;
}

// --------------------------------------
vtkDataMineBlockReader::~vtkDataMineBlockReader() = default;

// --------------------------------------
void vtkDataMineBlockReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// --------------------------------------
int vtkDataMineBlockReader::CanReadFile(const char* fname)
{
  return this->CanRead(fname, blockmodel);
}

// --------------------------------------
void vtkDataMineBlockReader::Read(vtkPoints* points, vtkCellArray* cells)
{
  TDMFile* file = new TDMFile();

  file->LoadFileHeader(this->GetFileName());
  int numRecords = file->GetNumberOfRecords();
  int recordLength = file->nVars;

  // since the binary file will have these fields, but the order of
  // them is not known
  int X, Y, Z;

  char* varname = new char[256]; // make it really large so we don't run the bounds
  for (int i = 0; i < recordLength; i++)
  {
    file->Vars[i].GetName(varname);
    if (strncmp(varname, "XC", 2) == 0)
    {
      X = i;
    }
    else if (strncmp(varname, "YC", 2) == 0)
    {
      Y = i;
    }
    else if (strncmp(varname, "ZC", 2) == 0)
    {
      Z = i;
    }

    this->AddProperty(varname, i, file->Vars[i].TypeIsNumerical(), numRecords);
  }
  delete[] varname;

  this->ParsePoints(points, cells, file, X, Y, Z);

  // cleanup
  delete file;
}

// --------------------------------------
void vtkDataMineBlockReader::ParsePoints(vtkPoints* points, vtkCellArray* cells, TDMFile* file,
  const int& XID, const int& YID, const int& ZID)
{
  Data* values = new Data[file->nVars];
  int numRecords = file->GetNumberOfRecords();
  float x, y, z;

  file->OpenRecVarFile(this->GetFileName());
  for (int i = 0; i < numRecords; i++)
  {
    file->GetRecVars(i, values);
    x = values[XID].v;
    y = values[YID].v;
    z = values[ZID].v;

    points->InsertNextPoint(x, y, z);

    this->ParseProperties(values);

    cells->InsertNextCell(1);
    cells->InsertCellPoint(i);
  }

  file->CloseRecVarFile();

  delete[] values;
}
