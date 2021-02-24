// .NAME DataMinePointReader.cxx
// Read DataMine binary files for single objects.
// point, perimeter (polyline), wframe<points/triangle>
// With or without properties (each property name < 8 characters)
// The binary file reading is done by 'dmfile.cxx'
//     99-04-12: Written by Jeremy Maccelari, visualn@iafrica.com

#include "vtkDataMinePerimeterReader.h"
#include "ThirdParty/PropertyStorage.h"
#include "ThirdParty/dmfile.h"

#include "vtkCellArray.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkDataMinePerimeterReader);

// Constructor
vtkDataMinePerimeterReader::vtkDataMinePerimeterReader()
{
  this->CellMode = VTK_LINE;
}

// --------------------------------------
// Destructor
vtkDataMinePerimeterReader::~vtkDataMinePerimeterReader() = default;

// --------------------------------------
void vtkDataMinePerimeterReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// --------------------------------------
int vtkDataMinePerimeterReader::CanReadFile(const char* fname)
{
  return this->CanRead(fname, perimeter);
}

// --------------------------------------
void vtkDataMinePerimeterReader::Read(vtkPoints* points, vtkCellArray* cells)
{
  TDMFile* file = new TDMFile();
  file->LoadFileHeader(this->GetFileName());
  int numRecords = file->GetNumberOfRecords();
  int recordLength = file->nVars;

  // since the binary file will have these fields, but the order of
  // them is not known
  int PV, PTN, X, Y, Z;

  char* varname = new char[256]; // make it really large so we don't run the bounds
  for (int i = 0; i < recordLength; i++)
  {
    file->Vars[i].GetName(varname);
    if (strncmp(varname, "XP", 2) == 0)
    {
      X = i;
    }
    else if (strncmp(varname, "YP", 2) == 0)
    {
      Y = i;
    }
    else if (strncmp(varname, "ZP", 2) == 0)
    {
      Z = i;
    }
    else if (strncmp(varname, "PTN", 3) == 0)
    {
      PTN = i;
    }
    else if (strncmp(varname, "PVALUE", 6) == 0)
    {
      PV = i;
    }

    this->AddProperty(varname, i, file->Vars[i].TypeIsNumerical(), numRecords);
  }
  delete[] varname;

  this->ParsePoints(points, cells, file, X, Y, Z, PTN, PV);

  // cleanup
  delete file;
}

// --------------------------------------
void vtkDataMinePerimeterReader::ParsePoints(vtkPoints* points, vtkCellArray* cells, TDMFile* file,
  const int& XID, const int& YID, const int& ZID, const int& vtkNotUsed(PTN), const int& PV)
{
  Data* values = new Data[file->nVars];
  int numRecords = file->GetNumberOfRecords();
  double x, y, z;

  int cellCount = 0;
  int pointsInCell = 0;
  double currentCellNumber = 0;
  double prevCellNumber = -1;

  file->OpenRecVarFile(this->GetFileName());
  for (int i = 0; i < numRecords; i++)
  {
    file->GetRecVars(i, values);
    // logic for creating cells
    currentCellNumber = values[PV].v;
    if (currentCellNumber != prevCellNumber)
    {
      // close the previous cell
      if (cellCount > 0)
      {
        cells->UpdateCellCount(pointsInCell);
      }
      pointsInCell = 0;
      cellCount++;
      prevCellNumber = currentCellNumber;
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
  cells->UpdateCellCount(pointsInCell);

  file->CloseRecVarFile();
  delete[] values;
}
