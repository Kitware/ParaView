// .NAME vtkDataMineDrillHoleReader.cxx
// Read DataMine binary files for single objects.
// point, perimeter (polyline), wframe<points/triangle>
// With or without properties (each property name < 8 characters)
// The binary file reading is done by 'dmfile.cxx'
//     99-04-12: Written by Jeremy Maccelari, visualn@iafrica.com

#include "vtkDataMineDrillHoleReader.h"
#include "PropertyStorage.h"
#include "dmfile.h"

#include "vtkCellArray.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkDataMineDrillHoleReader);

// --------------------------------------
vtkDataMineDrillHoleReader::vtkDataMineDrillHoleReader()
{
  this->CellMode = VTK_LINE;
}

// --------------------------------------
vtkDataMineDrillHoleReader::~vtkDataMineDrillHoleReader()
{
}

// --------------------------------------
void vtkDataMineDrillHoleReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// --------------------------------------
int vtkDataMineDrillHoleReader::CanReadFile(const char* fname)
{
  return this->CanRead(fname, drillhole);
}

// --------------------------------------
void vtkDataMineDrillHoleReader::Read(vtkPoints* points, vtkCellArray* cells)
{
  TDMFile* file = new TDMFile();

  file->LoadFileHeader(this->GetFileName());
  int numRecords = file->GetNumberOfRecords();
  int recordLength = file->nVars;

  // since the binary file will have these fields, but the order of
  // them is not known
  int X, Y, Z, ID, IDSize;
  ID = Z = Y = X = -1; // set them all
  IDSize = 0;

  char* varname = new char[256]; // make it really large so we don't run the bounds
  for (int i = 0; i < recordLength; i++)
  {
    file->Vars[i].GetName(varname);
    if (strncmp(varname, "X ", 2) == 0 && X < 0)
    {
      X = i;
    }
    else if (strncmp(varname, "Y ", 2) == 0 && Y < 0)
    {
      Y = i;
    }
    else if (strncmp(varname, "Z ", 2) == 0 && Z < 0)
    {
      Z = i;
    }
    else if (strncmp(varname, "BHID", 4) == 0)
    {
      if (ID == -1)
      {
        ID = i;
      }
      IDSize++;
    }

    this->AddProperty(varname, i, file->Vars[i].TypeIsNumerical(), numRecords);
  }
  delete[] varname;

  // read the point
  this->ParsePoints(points, cells, file, X, Y, Z, ID, IDSize);

  // cleanup
  delete file;
}

// --------------------------------------
void vtkDataMineDrillHoleReader::ParsePoints(vtkPoints* points, vtkCellArray* cells, TDMFile* file,
  const int& XID, const int& YID, const int& ZID, const int& BHID, const int& BHIDSize)
{
  Data* values = new Data[file->nVars];
  int numRecords = file->GetNumberOfRecords();
  double x, y, z;

  int cellCount = 0;
  int pointsInCell = 0;
  bool BHIDChanged = false;
  double currentCellNumber = 0;

  // setup the
  double* prevCellNumbers = new double[BHIDSize];
  for (int i = 0; i < BHIDSize; i++)
  {
    prevCellNumbers[i] = -1;
  }

  file->OpenRecVarFile(this->GetFileName());
  for (int i = 0; i < numRecords; i++)
  {
    file->GetRecVars(i, values);
    // logic for creating cells

    // need to see if the string or int id has changed
    for (int j = 0; j < BHIDSize; j++)
    {
      currentCellNumber = values[BHID + j].v;
      if (prevCellNumbers[j] != currentCellNumber)
      {
        // check for changes, and update prevCellNumbers at the same time
        BHIDChanged = true;
        prevCellNumbers[j] = currentCellNumber;
      }
    }
    if (BHIDChanged)
    {
      // close the previous cell
      if (cellCount > 0)
      {
        cells->UpdateCellCount(pointsInCell);
      }

      BHIDChanged = false;
      pointsInCell = 0;
      cellCount++;
      cells->InsertNextCell(1);
    }
    pointsInCell++;

    // add the point to the point index
    x = values[XID].v;
    y = values[YID].v;
    z = values[ZID].v;
    points->InsertNextPoint(x, y, z);
    cells->InsertCellPoint(i);

    this->ParseProperties(values);
  }
  file->CloseRecVarFile();

  cells->UpdateCellCount(pointsInCell);
  delete[] prevCellNumbers;
  delete[] values;
}
