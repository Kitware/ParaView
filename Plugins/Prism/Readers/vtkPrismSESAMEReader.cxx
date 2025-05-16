// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPrismSESAMEReader.h"

#include "vtkDoubleArray.h"
#include "vtkErrorCode.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridGeometryFilter.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"

#include "vtksys/SystemTools.hxx"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkPrismSESAMEReader);

//------------------------------------------------------------------------------
vtkPrismSESAMEReader::vtkPrismSESAMEReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(2);
}

//------------------------------------------------------------------------------
vtkPrismSESAMEReader::~vtkPrismSESAMEReader()
{
  this->SetFileName(nullptr);
  this->SetXArrayName(nullptr);
  this->SetYArrayName(nullptr);
  this->SetZArrayName(nullptr);
}

//------------------------------------------------------------------------------
void vtkPrismSESAMEReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "TableId: " << this->TableId << endl;
  os << indent << "TableIds: " << endl;
  for (auto i = 0; i < this->TableIds->GetNumberOfValues(); ++i)
  {
    os << indent << indent << this->TableIds->GetValue(i) << endl;
  }
  os << indent << "SurfaceTableIds: " << endl;
  for (auto i = 0; i < this->SurfaceTableIds->GetNumberOfValues(); ++i)
  {
    os << indent << indent << this->SurfaceTableIds->GetValue(i) << endl;
  }
  os << indent << "CurveTableIds: " << endl;
  for (auto i = 0; i < this->CurveTableIds->GetNumberOfValues(); ++i)
  {
    os << indent << indent << this->CurveTableIds->GetValue(i) << endl;
  }
  os << indent << "ArraysOfTables: " << endl;
  for (const auto& array : this->ArraysOfTables)
  {
    os << indent << indent << "Table: " << array.first << endl;
    for (auto i = 0; i < array.second->GetNumberOfValues(); ++i)
    {
      os << indent << indent << indent << array.second->GetValue(i) << endl;
    }
  }
  os << indent << "XArrayName: " << (this->XArrayName ? this->XArrayName : "(none)") << endl;
  os << indent << "YArrayName: " << (this->YArrayName ? this->YArrayName : "(none)") << endl;
  os << indent << "ZArrayName: " << (this->ZArrayName ? this->ZArrayName : "(none)") << endl;
  os << indent << "ReadCurves: " << this->ReadCurves << endl;
  os << indent << "VariableConversionValues: " << endl;
  for (vtkIdType i = 0; i < this->VariableConversionValues->GetNumberOfValues(); ++i)
  {
    os << indent << indent << this->VariableConversionValues->GetValue(i) << endl;
  }
}

namespace
{
constexpr int SESAME_NUM_CHARS = 512;
const std::string TableLineFormat = "%2i%6i%6i";

//------------------------------------------------------------------------------
struct TablesInformation
{
  static int TableIndex(int tableIndex)
  {
    for (int i = 0; i < TablesInformation::NUMBER_OF_SUPPORTED_TABLES; ++i)
    {
      if (TablesInformation::TablesInfo[i].TableIndex == tableIndex)
      {
        return i;
      }
    }
    return -1;
  }

  struct TableInformation
  {
    int TableIndex;
    std::vector<std::string> ArrayNames;
  };

  static const int NUMBER_OF_SUPPORTED_TABLES = 17;
  static const std::array<TableInformation, NUMBER_OF_SUPPORTED_TABLES> TablesInfo;
};

const std::array<TablesInformation::TableInformation, TablesInformation::NUMBER_OF_SUPPORTED_TABLES>
  TablesInformation::TablesInfo = { { { 301,
                                        { "Density", "Temperature", "Total EOS (Pressure)",
                                          "Total EOS (Energy)", "Total EOS (Free Energy)",
                                          "Total EOS (Speed)" } },
    { 303,
      { "Density", "Temperature", "Total EOS (Pressure)", "Total EOS (Energy)",
        "Total EOS (Free Energy)", "Total EOS (Speed)" } },
    { 304,
      { "Density", "Temperature", "Electron EOS (Pressure)", "Electron EOS (Energy)",
        "Electron EOS (Free Energy)", "Total EOS (Speed)" } },

    { 305,
      { "Density", "Temperature", "Total EOS (Pressure)", "Total EOS (Energy)",
        "Total EOS (Free Energy)", "Total EOS (Speed)" } },

    { 306, { "Density", "Total EOS (Pressure)", "Total EOS (Energy)", "Total EOS (Free Energy)" } },

    { 401,
      { "Vapor Pressure", "Temperature", "Vapor Density", "Density of Liquid or Solid",
        "Internal Energy of Vapor", "Internal Energy of Liquid or Solid", "Free Energy of Vapor",
        "Free Energy of Liquid or Solid" } },

    { 411,
      { "Density of Solid on Melt", "Melt Temperature", "Melt Pressure", "Internal Energy of Solid",
        "Free Energy of Solid" } },

    { 412,
      { "Density of Solid on Melt", "Melt Temperature", "Melt Pressure",
        "Internal Energy of Liquid", "Free Energy of Liquid" } },

    { 502, { "Density", "Temperature", "Rosseland Mean Opacity" } },

    { 503, { "Density", "Temperature", "Electron Conductive Opacity1" } },

    { 504, { "Density", "Temperature", "Mean Ion Charge1" } },

    { 505, { "Density", "Temperature", "Planck Mean Opacity" } },

    { 601, { "Density", "Temperature", "Mean Ion Charge2" } },

    { 602, { "Density", "Temperature", "Electrical Conductivity" } },

    { 603, { "Density", "Temperature", "Thermal Conductivity" } },

    { 604, { "Density", "Temperature", "Thermoelectric Coefficient" } },

    { 605, { "Density", "Temperature", "Electron Conductive Opacity2" } } } };

} // end anonymous namespace

//------------------------------------------------------------------------------
void vtkPrismSESAMEReader::SetFileName(const char* filename)
{
  vtkSetStringBodyMacro(FileName, filename);
  this->Reset();
}

//------------------------------------------------------------------------------
void vtkPrismSESAMEReader::SetTableId(int tableId)
{
  if (this->TableId != tableId)
  {
    if (TablesInformation::TableIndex(tableId) != -1)
    {
      this->TableId = tableId;
      this->Modified();
    }
  }
}

//------------------------------------------------------------------------------
vtkStringArray* vtkPrismSESAMEReader::GetArraysOfTable(int tableId)
{
  if (this->ArraysOfTables.find(tableId) != this->ArraysOfTables.end())
  {
    return this->ArraysOfTables[tableId];
  }
  return nullptr;
}

//------------------------------------------------------------------------------
vtkStringArray* vtkPrismSESAMEReader::GetArraysOfSelectedTable()
{
  return this->GetArraysOfTable(this->TableId);
}

//------------------------------------------------------------------------------
vtkStringArray* vtkPrismSESAMEReader::GetFlatArraysOfTables()
{
  this->FlatArraysOfTables->Initialize();
  for (auto& arraysOfTable : this->ArraysOfTables)
  {
    this->FlatArraysOfTables->InsertNextValue(std::to_string(arraysOfTable.first));
    auto tableArrays = arraysOfTable.second;
    for (auto i = 0; i < tableArrays->GetNumberOfValues(); ++i)
    {
      this->FlatArraysOfTables->InsertNextValue(tableArrays->GetValue(i));
    }
  }
  return this->FlatArraysOfTables;
}

//------------------------------------------------------------------------------
void vtkPrismSESAMEReader::SetNumberOfVariableConversionValues(int value)
{
  this->VariableConversionValues->SetNumberOfValues(value);
}

//------------------------------------------------------------------------------
void vtkPrismSESAMEReader::SetVariableConversionValue(int i, double value)
{
  if (this->VariableConversionValues->GetValue(i) != value)
  {
    this->VariableConversionValues->SetValue(i, value);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
double vtkPrismSESAMEReader::GetVariableConversionValue(int i)
{
  return this->VariableConversionValues->GetValue(i);
}

//------------------------------------------------------------------------------
bool vtkPrismSESAMEReader::GetCurvesAvailable()
{
  return this->CurveTableIds->GetNumberOfValues() > 0;
}

//------------------------------------------------------------------------------
void vtkPrismSESAMEReader::Reset()
{
  this->TableIds->Initialize();
  this->SurfaceTableIds->Initialize();
  this->CurveTableIds->Initialize();
  this->TableId = -1;
  this->TableLocations.clear();
  this->ArraysOfTables.clear();
}

//------------------------------------------------------------------------------
bool vtkPrismSESAMEReader::OpenFile(std::FILE*& file)
{
  // check filename
  if (!this->FileName || *this->FileName == 0)
  {
    vtkErrorMacro(<< "A FileName must be specified.");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return false;
  }
  // Initialize
  file = vtksys::SystemTools::Fopen(this->FileName, "rb");
  if (file == nullptr)
  {
    vtkErrorMacro(<< "File " << this->FileName << " not found");
    this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
    return false;
  }
  // check that it is valid
  int a;
  if (!this->ReadTableHeader(file, a))
  {
    vtkErrorMacro(<< this->GetFileName() << " is not a valid SESAME file");
    fclose(file);
    file = nullptr;
    return false;
  }
  rewind(file);
  return true;
}

//------------------------------------------------------------------------------
bool vtkPrismSESAMEReader::ReadTableHeader(char* buffer, int& tableId)
{
  int dummy;
  int internalId;
  int table;

  // see if the line matches the  " 0 9999 602" format
  if (sscanf(buffer, TableLineFormat.c_str(), &dummy, &internalId, &table) == 3)
  {
    tableId = table;
    this->FileFormat = SESAMEFormat::LANL;
    return true;
  }
  else
  {
    std::string header = buffer;
    std::transform(header.begin(), header.end(), header.begin(), tolower);
    auto recordPos = header.find("record");
    auto typePos = header.find("type");
    auto indexPos = header.find("index");
    auto matidPos = header.find("matid");

    if (recordPos != std::string::npos && typePos != std::string::npos)
    {
      char buffer2[SESAME_NUM_CHARS];
      if (sscanf(buffer, "%s%s%s%d%s", buffer2, buffer2, buffer2, &table, buffer2) == 5)
      {
        tableId = table;
        this->FileFormat = SESAMEFormat::ASC;
        return true;
      }
      else
      {
        tableId = -1;
        return false;
      }
    }
    else if (indexPos != std::string::npos && matidPos != std::string::npos)
    {
      tableId = -1;
      return true;
    }
    else
    {
      tableId = -1;
      return false;
    }
  }
}

//------------------------------------------------------------------------------
bool vtkPrismSESAMEReader::ReadTableHeader(std::FILE* f, int& tableId)
{
  if (f)
  {
    char buffer[SESAME_NUM_CHARS];
    if (fgets(buffer, SESAME_NUM_CHARS, f) != nullptr)
    {
      return this->ReadTableHeader(buffer, tableId);
    }
  }
  return false;
}

//------------------------------------------------------------------------------
int vtkPrismSESAMEReader::ReadTableValueLine(
  std::FILE* file, float* v1, float* v2, float* v3, float* v4, float* v5)
{
  // by definition, a line of this file is 80 characters long
  // when we start reading the data values, for the LANL format the end of the
  // line is a tag (see note below), which we have to ignore in order to read
  // the data properly The ASC format doesn't have the end of line tag.
  char buffer[SESAME_NUM_CHARS + 1];
  buffer[SESAME_NUM_CHARS] = '\0';
  int numRead = 0;
  if (fgets(buffer, SESAME_NUM_CHARS, file) != nullptr)
  {
    int tableId;
    // see if the line matches the  " 0 9999 602" format
    if (this->ReadTableHeader(buffer, tableId))
    {
      // this is the start of a new table
      numRead = 0;
    }
    else
    {
      if (this->FileFormat == SESAMEFormat::LANL)
      {
        // ignore the last 5 characters of the line (see notes above)
        buffer[75] = '\0';
      }
      numRead = sscanf(buffer, "%e%e%e%e%e", v1, v2, v3, v4, v5);
    }
  }

  return numRead;
}

//------------------------------------------------------------------------------
int vtkPrismSESAMEReader::JumpToTable(std::FILE* file, int tableId)
{
  for (auto i = 0; i < this->TableIds->GetNumberOfValues(); ++i)
  {
    if (this->TableIds->GetValue(i) == tableId)
    {
      fseek(file, this->TableLocations[i], SEEK_SET);
      return 1;
    }
  }

  return 0;
}

//------------------------------------------------------------------------------
void vtkPrismSESAMEReader::ReadTable(std::FILE* file, vtkPolyData* output, int tableId)
{
  // read the file
  this->JumpToTable(file, tableId);
  if (tableId == 301 || (tableId >= 303 && tableId <= 305) || (tableId >= 502 && tableId <= 505) ||
    (tableId >= 601 && tableId <= 605))
  {
    this->ReadSurfaceTable(file, output, tableId);
  }
  else if (tableId == 306 || tableId == 411 || tableId == 412)
  {
    this->ReadCurveTable(file, output, tableId);
  }
  else if (tableId == 401)
  {
    this->ReadVaporizationCurveTable(file, output, tableId);
  }
  else
  {
    vtkErrorMacro("Table " << tableId << " is not supported.");
  }
}

//------------------------------------------------------------------------------
void vtkPrismSESAMEReader::ReadSurfaceTable(std::FILE* file, vtkPolyData* output, int tableId)
{
  // We are going to read a table of the following format to create a 2D rectilinear grid with
  // scalar point arrays:
  //
  // 1) The first word (NR) contains the number of densities
  // 2) The second word (NT) contains the number of temperatures
  // 3) The following NR words are used to create the density array
  // 4) The following NT words are used to create the temperature array
  // 5) while there are words
  // 6)   The following NR*NT words are used to define a scalar point array

  const auto tableArrays = this->ArraysOfTables[tableId];
  float v[5] = { 0.0, 0.0, 0.0, 0.0, 0.0 };

  // get the table's first line to extract NR and NT
  int readFromTable = this->ReadTableValueLine(file, &(v[0]), &(v[1]), &(v[2]), &(v[3]), &(v[4]));
  if (readFromTable == 0)
  {
    vtkErrorMacro("Error reading table " << tableId);
    return;
  }

  const int NR = (int)(v[0]);     // number of density values
  const int NT = (int)(v[1]);     // number of temperature values
  const int NRxNT = NR * NT;      // size of the scalar arrays
  const int NR_plus_NT = NR + NT; // constant used to read the table

  // allocate space
  vtkNew<vtkFloatArray> xCoords;
  vtkNew<vtkFloatArray> yCoords;
  vtkNew<vtkFloatArray> zCoords;
  xCoords->Allocate(NR);
  yCoords->Allocate(NT);
  zCoords->Allocate(1);
  zCoords->InsertNextValue(0.0);

  // create scalar arrays
  std::vector<vtkSmartPointer<vtkFloatArray>> scalars;
  for (auto i = 0; i < tableArrays->GetNumberOfValues(); ++i)
  {
    vtkNew<vtkFloatArray> newArray;
    scalars.emplace_back(newArray);
    if (newArray)
    {
      newArray->Allocate(NRxNT);
      newArray->SetName(tableArrays->GetValue(i).c_str());
    }
  }

  // 500/600 tables hold log10 values, so we need to "unlog" them as we are reading the values in.
  const bool inverse_log_scale_needed = (tableId >= 500 && tableId < 700);
  // number of read values
  int valuesRead = 0;
  // index of the scalar array to fill, start with 2 because the first two arrays are the x and y
  unsigned int scalarIndex = 2;
  // number of values read for the current scalar array
  int scalarCount = 0;
  // since we already have utilized the first 2 values of the first line, we need to read the rest
  int kBegin = 2;
  do
  {
    for (int k = kBegin; k < readFromTable; ++k)
    {
      // invert the log scale if needed
      if (inverse_log_scale_needed)
      {
        v[k] = std::pow(10.0f, v[k]);
      }
      // read the first NR values into the xCoords array
      if (valuesRead < NR)
      {
        xCoords->InsertNextValue(v[k]);
      }
      // read the next NT values into the yCoords array
      else if (valuesRead < NR_plus_NT)
      {
        yCoords->InsertNextValue(v[k]);
      }
      // as long as there are words, read the next NRxNT values into a scalar array
      else
      {
        ++scalarCount;
        if (scalarCount > NRxNT)
        {
          scalarCount = 1;
          ++scalarIndex;
        }
        if (tableArrays->GetNumberOfValues() > scalarIndex)
        {
          scalars[scalarIndex]->InsertNextValue(v[k]);
        }
      }
      ++valuesRead;
    }
    // for all the next lines, we need to read all the values
    kBegin = 0;
  } while ((readFromTable =
               this->ReadTableValueLine(file, &(v[0]), &(v[1]), &(v[2]), &(v[3]), &(v[4]))) != 0);

  // in case there are empty scalars arrays, fill them with zeros
  for (vtkIdType i = scalarIndex + 1; i < tableArrays->GetNumberOfValues(); ++i)
  {
    scalars[i]->SetNumberOfValues(NRxNT);
    scalars[i]->Fill(0.0);
  }

  // fill 1st and 2nd scalar arrays with zeros which represent the x and y coordinates
  if (tableArrays->GetNumberOfValues() > 0)
  {
    scalars[0]->SetNumberOfValues(NRxNT);
    scalars[0]->Fill(0.0);
  }
  if (tableArrays->GetNumberOfValues() > 1)
  {
    scalars[1]->SetNumberOfValues(NRxNT);
    scalars[1]->Fill(0.0);
  }

  // create the rectilinear grid
  vtkNew<vtkRectilinearGrid> rGrid;
  rGrid->SetDimensions(NR, NT, 1);
  rGrid->SetXCoordinates(xCoords);
  rGrid->SetYCoordinates(yCoords);
  rGrid->SetZCoordinates(zCoords);

  // add the scalar arrays to the rectilinear grid
  for (const auto& scalarArray : scalars)
  {
    if (scalarArray)
    {
      rGrid->GetPointData()->AddArray(scalarArray);
    }
  }

  // extract the surface of the rectilinear grid
  vtkNew<vtkRectilinearGridGeometryFilter> geomFilter;
  geomFilter->SetInputData(rGrid);
  geomFilter->Update();
  output->ShallowCopy(geomFilter->GetOutput());

  vtkPoints* inPts = output->GetPoints();
  vtkPointData* pd = output->GetPointData();
  const vtkIdType numPts = inPts->GetNumberOfPoints();

  auto xArray = vtkFloatArray::SafeDownCast(pd->GetArray(0));
  auto yArray = vtkFloatArray::SafeDownCast(pd->GetArray(1));

  // 1st and 2nd scalar arrays are the x and y coordinates which need to be filled by taking the
  // point coordinates
  vtkSMPTools::For(0, numPts,
    [&](vtkIdType begin, vtkIdType end)
    {
      double p[3];
      for (vtkIdType pointId = begin; pointId < end; ++pointId)
      {
        inPts->GetPoint(pointId, p);
        xArray->SetValue(pointId, p[0]);
        yArray->SetValue(pointId, p[1]);
      }
    });
  xArray->Modified();
  yArray->Modified();
}

//------------------------------------------------------------------------------
void vtkPrismSESAMEReader::ReadCurveTable(std::FILE* file, vtkPolyData* output, int tableId)
{
  // We are going to read a table of the following format to create a polydata curve consisted of
  // lines with scalar point arrays:
  //
  // 1) The first word (NR) contains the number of densities
  // 2) The second word (NT) contains the number of temperatures which is always 1
  // 3) The following NR words are used to create the density array
  // 4) The next word is the temperature value which is always 0
  // 5) while there are words
  // 6)   The following NR words are used to define a scalar point array

  const auto tableArrays = this->ArraysOfTables[tableId];
  float v[5] = { 0.0, 0.0, 0.0, 0.0, 0.0 };

  // get the table's first line to extract NR
  int readFromTable = this->ReadTableValueLine(file, &(v[0]), &(v[1]), &(v[2]), &(v[3]), &(v[4]));
  if (readFromTable == 0)
  {
    vtkErrorMacro("Error reading table " << tableId);
    return;
  }

  const int NR = (int)(v[0]); // number of density values
  const int NT = (int)(v[1]); // number of temperature values
  (void)NT;                   // unused variable

  // create scalar arrays
  std::vector<vtkSmartPointer<vtkFloatArray>> scalars;
  for (auto i = 0; i < tableArrays->GetNumberOfValues(); ++i)
  {
    vtkNew<vtkFloatArray> newArray;
    scalars.emplace_back(newArray);
    if (newArray)
    {
      newArray->Allocate(NR);
      newArray->SetName(tableArrays->GetValue(i).c_str());
    }
  }

  // number of read values
  int valuesRead = 0;
  // index of the scalar array to fill
  unsigned int scalarIndex = 0;
  // number of values read for the current scalar array
  int scalarCount = 0;
  // since we already have utilized the first 2 values of the first line, we need to read the rest
  int kBegin = 2;
  do
  {
    for (int k = kBegin; k < readFromTable; ++k)
    {
      // Avoid reading the temperature value which is 0
      if (valuesRead != NR)
      {
        // as long as there are words, read the next NR values into a scalar array
        ++scalarCount;
        if (scalarCount > NR)
        {
          scalarCount = 1;
          ++scalarIndex;
        }
        if (tableArrays->GetNumberOfValues() > scalarIndex)
        {
          scalars[scalarIndex]->InsertNextValue(v[k]);
        }
      }
      ++valuesRead;
    }
    // for all the next lines, we need to read all the values
    kBegin = 0;
  } while ((readFromTable =
               this->ReadTableValueLine(file, &(v[0]), &(v[1]), &(v[2]), &(v[3]), &(v[4]))) != 0);

  // in case there are empty scalars arrays, fill them with zeros
  for (vtkIdType i = scalarIndex + 1; i < tableArrays->GetNumberOfValues(); ++i)
  {
    scalars[i]->SetNumberOfValues(NR);
    scalars[i]->Fill(0.0);
  }

  if (scalars.size() >= 3 && scalars[0]->GetNumberOfValues() == NR &&
    scalars[1]->GetNumberOfValues() == NR && scalars[2]->GetNumberOfValues() == NR)
  {
    auto xArray = scalars[0];
    auto yArray = scalars[1];
    auto zArray = scalars[2];

    // create the points
    vtkNew<vtkFloatArray> coords;
    coords->SetNumberOfComponents(3);
    coords->SetNumberOfTuples(NR);
    vtkSMPTools::For(0, NR,
      [&](vtkIdType begin, vtkIdType end)
      {
        float* coordsPtr = coords->GetPointer(3 * begin);
        for (vtkIdType pointId = begin; pointId < end; ++pointId, coordsPtr += 3)
        {
          coordsPtr[0] = xArray->GetValue(pointId);
          coordsPtr[1] = yArray->GetValue(pointId);
          coordsPtr[2] = zArray->GetValue(pointId);
        }
      });
    vtkNew<vtkPoints> points;
    points->SetData(coords);
    output->SetPoints(points);

    // create the connectivity array for the lines of the curve
    vtkNew<vtkIdTypeArray> connectivity;
    connectivity->SetNumberOfValues(2 * (NR - 1));
    vtkSMPTools::For(0, NR - 1,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType pointId = begin; pointId < end; ++pointId)
        {
          connectivity->SetValue(2 * pointId, pointId);
          connectivity->SetValue(2 * pointId + 1, pointId + 1);
        }
      });
    // create the offsets array for the lines of the curve
    vtkNew<vtkIdTypeArray> offsets;
    offsets->SetNumberOfValues(NR);
    vtkSMPTools::For(0, NR,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType pointId = begin; pointId < end; ++pointId)
        {
          offsets->SetValue(pointId, 2 * pointId);
        }
      });
    // create the lines of the curve
    vtkNew<vtkCellArray> lines;
    lines->SetData(offsets, connectivity);
    output->SetLines(lines);

    // add the scalar arrays
    for (const auto& scalarArray : scalars)
    {
      if (scalarArray && scalarArray->GetNumberOfValues() == NR)
      {
        output->GetPointData()->AddArray(scalarArray);
      }
    }
  }
  else
  {
    vtkErrorMacro("The number of values in the scalar arrays is not equal");
    return;
  }
}

//------------------------------------------------------------------------------
void vtkPrismSESAMEReader::ReadVaporizationCurveTable(
  std::FILE* file, vtkPolyData* output, int tableId)
{
  // We are going to read a table of the following format to create a polydata curve consisted of
  // lines with scalar point arrays:
  //
  // 1) The first word (NT) contains the number of temperatures
  // 2) while there are words
  // 3)   The following NT words are used to define a scalar point array

  const auto tableArrays = this->ArraysOfTables[tableId];
  float v[5] = { 0.0, 0.0, 0.0, 0.0, 0.0 };

  // get the table's first line to extract NR
  int readFromTable = this->ReadTableValueLine(file, &(v[0]), &(v[1]), &(v[2]), &(v[3]), &(v[4]));
  if (readFromTable == 0)
  {
    vtkErrorMacro("Error reading table " << tableId);
    return;
  }
  const int NT = (int)(v[0]); // number of temperatures

  // create scalar arrays
  std::vector<vtkSmartPointer<vtkFloatArray>> scalars;
  for (auto i = 0; i < tableArrays->GetNumberOfValues(); ++i)
  {
    vtkNew<vtkFloatArray> newArray;
    scalars.emplace_back(newArray);
    if (newArray)
    {
      newArray->Allocate(NT);
      newArray->SetName(tableArrays->GetValue(i).c_str());
    }
  }

  // number of read values
  int valuesRead = 0;
  // index of the scalar array to fill
  unsigned int scalarIndex = 0;
  // number of values read for the current scalar array
  int scalarCount = 0;
  // since we already have utilized the first 1 values of the first line, we need to read the rest
  int kBegin = 1;
  do
  {
    for (int k = kBegin; k < readFromTable; ++k)
    {
      // as long as there are words, read the next NT values into a scalar array
      ++scalarCount;
      if (scalarCount > NT)
      {
        scalarCount = 1;
        ++scalarIndex;
      }
      if (tableArrays->GetNumberOfValues() > scalarIndex)
      {
        scalars[scalarIndex]->InsertNextValue(v[k]);
      }
      ++valuesRead;
    }
    // for all the next lines, we need to read all the values
    kBegin = 0;
  } while ((readFromTable =
               this->ReadTableValueLine(file, &(v[0]), &(v[1]), &(v[2]), &(v[3]), &(v[4]))) != 0);

  // in case there are empty scalars arrays, fill them with zeros
  for (vtkIdType i = scalarIndex + 1; i < tableArrays->GetNumberOfValues(); ++i)
  {
    scalars[i]->SetNumberOfValues(NT);
    scalars[i]->Fill(0.0);
  }

  if (scalars.size() >= 3 && scalars[0]->GetNumberOfValues() == NT &&
    scalars[1]->GetNumberOfValues() == NT && scalars[2]->GetNumberOfValues() == NT)
  {
    auto xArray = scalars[0];
    auto yArray = scalars[1];
    auto zArray = scalars[2];

    // create the points
    vtkNew<vtkFloatArray> coords;
    coords->SetNumberOfComponents(3);
    coords->SetNumberOfTuples(NT);
    vtkSMPTools::For(0, NT,
      [&](vtkIdType begin, vtkIdType end)
      {
        float* coordsPtr = coords->GetPointer(3 * begin);
        for (vtkIdType pointId = begin; pointId < end; ++pointId, coordsPtr += 3)
        {
          coordsPtr[0] = xArray->GetValue(pointId);
          coordsPtr[1] = yArray->GetValue(pointId);
          coordsPtr[2] = zArray->GetValue(pointId);
        }
      });
    vtkNew<vtkPoints> points;
    points->SetData(coords);
    output->SetPoints(points);

    // create the connectivity array for the lines of the curve
    vtkNew<vtkIdTypeArray> connectivity;
    connectivity->SetNumberOfValues(2 * (NT - 1));
    vtkSMPTools::For(0, NT - 1,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType pointId = begin; pointId < end; ++pointId)
        {
          connectivity->SetValue(2 * pointId, pointId);
          connectivity->SetValue(2 * pointId + 1, pointId + 1);
        }
      });
    // create the offsets array for the lines of the curve
    vtkNew<vtkIdTypeArray> offsets;
    offsets->SetNumberOfValues(NT);
    vtkSMPTools::For(0, NT,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType pointId = begin; pointId < end; ++pointId)
        {
          offsets->SetValue(pointId, 2 * pointId);
        }
      });
    // create the lines of the curve
    vtkNew<vtkCellArray> lines;
    lines->SetData(offsets, connectivity);
    output->SetLines(lines);

    // add the scalar arrays
    for (const auto& scalarArray : scalars)
    {
      if (scalarArray && scalarArray->GetNumberOfValues() == NT)
      {
        output->GetPointData()->AddArray(scalarArray);
      }
    }
  }
  else
  {
    vtkErrorMacro("The number of values in the scalar arrays is not equal");
    return;
  }
}

//------------------------------------------------------------------------------
int vtkPrismSESAMEReader::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  }
  else if (port == 1)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPartitionedDataSetCollection");
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkPrismSESAMEReader::RequestDataObject(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  bool resultSurface = vtkDataObjectAlgorithm::SetOutputDataObject(
    VTK_POLY_DATA, outputVector->GetInformationObject(0), /*exact*/ true);
  bool resultCurves = vtkDataObjectAlgorithm::SetOutputDataObject(
    VTK_PARTITIONED_DATA_SET_COLLECTION, outputVector->GetInformationObject(1), /*exact*/ true);
  return static_cast<int>(resultSurface && resultCurves);
}

//------------------------------------------------------------------------------
int vtkPrismSESAMEReader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector))
{
  // exit if table ids are already populated
  if (this->TableIds->GetNumberOfValues() != 0)
  {
    return 1;
  }

  std::FILE* file = nullptr;
  // open the file
  if (!this->OpenFile(file))
  {
    return 1;
  }

  // get the table ids and table locations
  char buffer[SESAME_NUM_CHARS];
  int tableId;

  while (fgets(buffer, SESAME_NUM_CHARS, file) != nullptr)
  {
    // see if the line matches the  " 0 9999 602" format
    if (this->ReadTableHeader(buffer, tableId))
    {
      if (TablesInformation::TableIndex(tableId) != -1)
      {
        this->TableIds->InsertNextValue(tableId);
        if (tableId == 306 || tableId == 401 || tableId == 411 || tableId == 412)
        {
          this->CurveTableIds->InsertNextValue(tableId);
        }
        else
        {
          this->SurfaceTableIds->InsertNextValue(tableId);
        }
        long loc = ftell(file);
        this->TableLocations.push_back(loc);
      }
    }
  }
  // if table id has not been set, set the first surface table as the default
  if (this->TableId != 1 && this->SurfaceTableIds->GetNumberOfValues() > 0)
  {
    this->TableId = this->SurfaceTableIds->GetValue(0);
  }

  // parse tables to extract the array names
  for (vtkIdType i = 0; i < this->TableIds->GetNumberOfValues(); ++i)
  {
    tableId = this->TableIds->GetValue(i);

    float v[5] = { 0.0, 0.0, 0.0, 0.0, 0.0 };
    int NR;                      // number of densities
    int NT;                      // number of temperatures
    int readFromTable;           // number of values in read line
    int valuesRead = 0;          // number of read values
    unsigned int numberOfArrays; // number of scalar arrays
    int scalarCount = 0;         // number of scalars read
    int kBegin;                  // begin index of the read values
    bool tableProcessed = false; // flag to indicate if the table has been processed

    // see ReadSurfaceTable function to understand the format of what we are reading
    if (tableId == 301 || (tableId >= 303 && tableId <= 305) ||
      (tableId >= 502 && tableId <= 505) || (tableId >= 601 && tableId <= 605))
    {
      this->JumpToTable(file, tableId);

      readFromTable = this->ReadTableValueLine(file, &(v[0]), &(v[1]), &(v[2]), &(v[3]), &(v[4]));
      if (readFromTable == 0)
      {
        vtkErrorMacro("Error reading table " << tableId);
        fclose(file);
        return 1;
      }
      NR = (int)(v[0]);
      NT = (int)(v[1]);
      const int NRxNT = NR * NT;
      const int NR_plus_NT = NR + NT;

      numberOfArrays = 2;
      kBegin = 2;
      do
      {
        for (int k = kBegin; k < readFromTable; ++k)
        {
          if (valuesRead >= NR_plus_NT)
          {
            ++scalarCount;
            if (scalarCount == NRxNT)
            {
              scalarCount = 0;
              ++numberOfArrays;
            }
          }
          ++valuesRead;
        }
        kBegin = 0;
      } while ((readFromTable = this->ReadTableValueLine(
                  file, &(v[0]), &(v[1]), &(v[2]), &(v[3]), &(v[4]))) != 0);
      tableProcessed = true;
    }
    // see ReadCurveTable function to understand the format of what we are reading
    else if (tableId == 306 || tableId == 411 || tableId == 412)
    {
      this->JumpToTable(file, tableId);

      readFromTable = this->ReadTableValueLine(file, &(v[0]), &(v[1]), &(v[2]), &(v[3]), &(v[4]));
      if (readFromTable == 0)
      {
        vtkErrorMacro("Error reading table " << tableId);
        fclose(file);
        return 1;
      }
      NR = (int)(v[0]);
      NT = (int)(v[1]);
      (void)NT;

      numberOfArrays = 0;
      kBegin = 2;
      do
      {
        for (int k = kBegin; k < readFromTable; ++k)
        {
          if (valuesRead != NR)
          {
            ++scalarCount;
            if (scalarCount == NR)
            {
              scalarCount = 0;
              ++numberOfArrays;
            }
          }
          ++valuesRead;
        }
        kBegin = 0;
      } while ((readFromTable = this->ReadTableValueLine(
                  file, &(v[0]), &(v[1]), &(v[2]), &(v[3]), &(v[4]))) != 0);
      tableProcessed = true;
    }
    // see ReadVaporizationCurveTable function to understand the format of what we are reading
    else if (tableId == 401)
    {
      this->JumpToTable(file, tableId);

      readFromTable = this->ReadTableValueLine(file, &(v[0]), &(v[1]), &(v[2]), &(v[3]), &(v[4]));
      if (readFromTable == 0)
      {
        vtkErrorMacro("Error reading table " << tableId);
        fclose(file);
        return 1;
      }
      NT = (int)(v[0]);

      numberOfArrays = 0;
      kBegin = 1;
      do
      {
        for (int k = kBegin; k < readFromTable; ++k)
        {
          ++scalarCount;
          if (scalarCount == NT)
          {
            scalarCount = 0;
            ++numberOfArrays;
          }
        }
        kBegin = 0;
      } while ((readFromTable = this->ReadTableValueLine(
                  file, &(v[0]), &(v[1]), &(v[2]), &(v[3]), &(v[4]))) != 0);
      tableProcessed = true;
    }
    else
    {
      numberOfArrays = 0;
    }
    if (tableProcessed)
    {
      // get the names of the arrays in the table
      int tableIndex = TablesInformation::TableIndex(tableId);
      auto numVarNames = TablesInformation::TablesInfo[tableIndex].ArrayNames.size();
      auto tableArrays = vtkSmartPointer<vtkStringArray>::New();
      for (unsigned int j = 0; j < numberOfArrays; ++j)
      {
        const std::string arrayName = j < numVarNames
          ? TablesInformation::TablesInfo[tableIndex].ArrayNames[j]
          : "Variable " + std::to_string(j + 1);
        tableArrays->InsertNextValue(arrayName);
      }
      this->ArraysOfTables[tableId] = tableArrays;
    }
  }
  fclose(file);

  return 1;
}

//------------------------------------------------------------------------------
void vtkPrismSESAMEReader::RequestCurvesData(
  std::FILE* file, vtkPartitionedDataSetCollection* curves)
{
  // This array is needed to be able to identify  if IsSimulationData is true or not for
  // vtkPrismGeometryRepresentation.
  vtkNew<vtkUnsignedCharArray> prismData;
  prismData->SetName("PRISM_DATA");
  prismData->InsertNextValue(1);
  curves->GetFieldData()->AddArray(prismData);

  const auto numberOfCurves = static_cast<unsigned int>(this->CurveTableIds->GetNumberOfValues());
  curves->SetNumberOfPartitionedDataSets(numberOfCurves);
  for (unsigned int curveIndex = 0; curveIndex < numberOfCurves; ++curveIndex)
  {
    int tableId = this->CurveTableIds->GetValue(static_cast<int>(curveIndex));

    // read the table
    vtkNew<vtkPolyData> curve;
    this->ReadTable(file, curve, tableId);

    // create the array map from curve to surface
    const int numberOfVariables = curve->GetPointData()->GetNumberOfArrays();
    std::vector<int> curveToSurfaceMap(static_cast<size_t>(numberOfVariables));
    switch (tableId)
    {
      case 306:
        // Density
        curveToSurfaceMap[0] = 0;
        // Pressure
        curveToSurfaceMap[1] = 2;
        // Energy
        curveToSurfaceMap[2] = 3;
        // Free Energy
        curveToSurfaceMap[3] = 4;
        break;
      case 401:
        // Pressure
        curveToSurfaceMap[0] = 2;
        // Temperature
        curveToSurfaceMap[1] = 1;
        // Vapor Density
        curveToSurfaceMap[2] = 0;
        // Density of Liquid or Solid
        curveToSurfaceMap[3] = 0;
        // Internal Energy of Vapor
        curveToSurfaceMap[4] = 3;
        // Internal Energy of Liquid or Solid
        curveToSurfaceMap[5] = 3;
        // Free Energy of Vapor
        curveToSurfaceMap[6] = 4;
        // Free Energy of Liquid or Solid
        curveToSurfaceMap[7] = 4;
        break;
      case 411:
        // Density of Solid on Melt
        curveToSurfaceMap[0] = 0;
        // Melt Temperature
        curveToSurfaceMap[1] = 1;
        // Melt Pressure
        curveToSurfaceMap[2] = 2;
        // Internal Energy of Solid
        curveToSurfaceMap[3] = 3;
        // Free Energy of Solid
        curveToSurfaceMap[4] = 4;
        break;
      case 412:
      default:
        // Density of Solid on Melt
        curveToSurfaceMap[0] = 0;
        // Melt Temperature
        curveToSurfaceMap[1] = 1;
        // Melt Pressure
        curveToSurfaceMap[2] = 2;
        // Internal Energy of Liquid
        curveToSurfaceMap[3] = 3;
        // Free Energy of Liquid
        curveToSurfaceMap[4] = 4;
        break;
    }
    // convert variables of curves based on the map
    // conversion values need to be at least 5 because table 301 has at least 5 arrays
    if (this->VariableConversionValues->GetNumberOfValues() >= 5)
    {
      vtkNew<vtkDoubleArray> conversionValues;
      conversionValues->SetNumberOfValues(numberOfVariables);
      for (int i = 0; i < numberOfVariables; ++i)
      {
        conversionValues->SetValue(
          i, this->VariableConversionValues->GetValue(curveToSurfaceMap[i]));
      }

      const auto numberOfConversionValues = static_cast<int>(conversionValues->GetNumberOfValues());
      const vtkIdType numberOfPoints = curve->GetNumberOfPoints();
      for (int i = 0; i < numberOfConversionValues; ++i)
      {
        if (auto array = vtkFloatArray::SafeDownCast(curve->GetPointData()->GetArray(i)))
        {
          const double conversionValue = conversionValues->GetValue(i);
          vtkSMPTools::For(0, numberOfPoints,
            [&](vtkIdType begin, vtkIdType end)
            {
              for (vtkIdType j = begin; j < end; ++j)
              {
                array->SetValue(j, static_cast<float>(array->GetValue(j) * conversionValue));
              }
            });
        }
      }
    }

    // set the points based on the selected arrays
    if (tableId == 305 || tableId == 411 || tableId == 412)
    {
      // we match the correct array based on the surface
      std::map<std::string, int> arrayNameToIndexMap;
      for (int i = 0; i < numberOfVariables; ++i)
      {
        arrayNameToIndexMap[this->GetArraysOfSelectedTable()->GetValue(i)] = curveToSurfaceMap[i];
      }

      vtkSmartPointer<vtkFloatArray> xArray;
      vtkSmartPointer<vtkFloatArray> yArray;
      vtkSmartPointer<vtkFloatArray> zArray;
      for (const auto& arrayNameToIndex : arrayNameToIndexMap)
      {
        const auto& arrayName = arrayNameToIndex.first;
        const auto& arrayIndex = arrayNameToIndex.second;
        if (arrayName == this->GetXArrayName())
        {
          xArray = vtkFloatArray::SafeDownCast(curve->GetPointData()->GetArray(arrayIndex));
        }
        else if (arrayName == this->GetYArrayName())
        {
          yArray = vtkFloatArray::SafeDownCast(curve->GetPointData()->GetArray(arrayIndex));
        }
        else if (arrayName == this->GetZArrayName())
        {
          zArray = vtkFloatArray::SafeDownCast(curve->GetPointData()->GetArray(arrayIndex));
        }
      }
      // set the points based on the selected arrays
      const vtkIdType numberOfPoints = curve->GetNumberOfPoints();
      auto coords = vtkFloatArray::SafeDownCast(curve->GetPoints()->GetData());
      vtkSMPTools::For(0, numberOfPoints,
        [&](vtkIdType begin, vtkIdType end)
        {
          float* coordsPtr = coords->GetPointer(3 * begin);
          for (vtkIdType pointId = begin; pointId < end; ++pointId, coordsPtr += 3)
          {
            coordsPtr[0] = xArray ? xArray->GetValue(pointId) : 0.0;
            coordsPtr[1] = yArray ? yArray->GetValue(pointId) : 0.0;
            coordsPtr[2] = zArray ? zArray->GetValue(pointId) : 0.0;
          }
        });
      curve->GetPoints()->Modified();

      // add curve number to the point data
      vtkNew<vtkUnsignedCharArray> curveNumber;
      curveNumber->SetName("Curve Number");
      curveNumber->SetNumberOfValues(numberOfPoints);
      curveNumber->FillValue(0);
      curve->GetPointData()->AddArray(curveNumber);

      // set dataset
      curves->SetPartition(curveIndex, 0, curve);
    }
    else // tableId == 401
    {
      // we match the correct array based on the surface
      std::map<std::string, std::vector<int>> arrayNameToComboIndexMap;
      std::vector<int> indexes(2);
      // Density
      indexes[0] = 2; // Vapor Density on Coexistence Line
      indexes[1] = 3; // Density of Liquid or Solid on Coexistence Line
      arrayNameToComboIndexMap[this->GetArraysOfSelectedTable()->GetValue(0)] = indexes;
      // Temperature
      indexes[0] = 1; // Temperature
      indexes[1] = 1;
      arrayNameToComboIndexMap[this->GetArraysOfSelectedTable()->GetValue(1)] = indexes;
      // Pressure
      indexes[0] = 0; // Vapor Pressure
      indexes[1] = 0;
      arrayNameToComboIndexMap[this->GetArraysOfSelectedTable()->GetValue(2)] = indexes;
      // Energy
      indexes[0] = 4; // Internal Energy of Vapor
      indexes[1] = 5; // Internal Energy of Liquid or Solid
      arrayNameToComboIndexMap[this->GetArraysOfSelectedTable()->GetValue(3)] = indexes;
      // Free Energy
      indexes[0] = 6; // Free Energy of Vapor
      indexes[1] = 7; // Free Energy of Liquid
      arrayNameToComboIndexMap[this->GetArraysOfSelectedTable()->GetValue(4)] = indexes;

      vtkSmartPointer<vtkFloatArray> xArray[2];
      vtkSmartPointer<vtkFloatArray> yArray[2];
      vtkSmartPointer<vtkFloatArray> zArray[2];

      for (const auto& arrayNameToComboIndex : arrayNameToComboIndexMap)
      {
        const auto& arrayName = arrayNameToComboIndex.first;
        const auto& comboIndex = arrayNameToComboIndex.second;
        for (int i = 0; i < 2; ++i)
        {
          if (arrayName == this->GetXArrayName())
          {
            xArray[i] = vtkFloatArray::SafeDownCast(curve->GetPointData()->GetArray(comboIndex[i]));
          }
          else if (arrayName == this->GetYArrayName())
          {
            yArray[i] = vtkFloatArray::SafeDownCast(curve->GetPointData()->GetArray(comboIndex[i]));
          }
          else if (arrayName == this->GetZArrayName())
          {
            zArray[i] = vtkFloatArray::SafeDownCast(curve->GetPointData()->GetArray(comboIndex[i]));
          }
        }
      }

      // set the points based on the selected arrays
      const vtkIdType numberOfPoints = curve->GetNumberOfPoints();
      for (int i = 0; i < 2; ++i)
      {
        vtkNew<vtkFloatArray> coords;
        coords->SetNumberOfComponents(3);
        coords->SetNumberOfTuples(numberOfPoints);

        vtkSMPTools::For(0, numberOfPoints,
          [&](vtkIdType begin, vtkIdType end)
          {
            float* coordsPtr = coords->GetPointer(3 * begin);
            for (vtkIdType pointId = begin; pointId < end; ++pointId, coordsPtr += 3)
            {
              coordsPtr[0] = xArray[i] ? xArray[i]->GetValue(pointId) : 0.0;
              coordsPtr[1] = yArray[i] ? yArray[i]->GetValue(pointId) : 0.0;
              coordsPtr[2] = zArray[i] ? zArray[i]->GetValue(pointId) : 0.0;
            }
          });

        vtkNew<vtkPoints> points;
        points->SetData(coords);

        vtkNew<vtkPolyData> subCurve;
        subCurve->ShallowCopy(curve);
        subCurve->SetPoints(points);

        // add curve number to the point data
        vtkNew<vtkUnsignedCharArray> curveNumber;
        curveNumber->SetName("Curve Number");
        curveNumber->SetNumberOfValues(numberOfPoints);
        curveNumber->FillValue(i);
        subCurve->GetPointData()->AddArray(curveNumber);

        // set dataset
        curves->SetPartition(curveIndex, i, subCurve);
      }
    }
    // set partitioned dataset name
    const auto curveName = "Table " + std::to_string(tableId);
    curves->GetMetaData(curveIndex)->Set(vtkCompositeDataSet::NAME(), curveName.c_str());
  }
}

//------------------------------------------------------------------------------
int vtkPrismSESAMEReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* surfaceOutInfo = outputVector->GetInformationObject(0);
  auto surfaceOutput = vtkPolyData::GetData(surfaceOutInfo);

  vtkInformation* curvesOutInfo = outputVector->GetInformationObject(1);
  auto curvesOutput = vtkPartitionedDataSetCollection::GetData(curvesOutInfo);

  // Return all data in the first piece ...
  if (surfaceOutInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) > 0 ||
    curvesOutInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) > 0)
  {
    // if curves will be generated, ensure that all nodes/pieces have the same structure
    if (this->TableId == 301 && this->GetCurvesAvailable() && this->ReadCurves)
    {
      auto numberOfCurves = static_cast<unsigned int>(this->CurveTableIds->GetNumberOfValues());
      curvesOutput->SetNumberOfPartitionedDataSets(numberOfCurves);
      for (unsigned int curveIndex = 0; curveIndex < numberOfCurves; ++curveIndex)
      {
        const int tableId = this->CurveTableIds->GetValue(static_cast<int>(curveIndex));
        curvesOutput->SetNumberOfPartitions(curveIndex, tableId != 401 ? 1 : 2);
        const auto curveName = "Table " + std::to_string(tableId);
        curvesOutput->GetMetaData(curveIndex)->Set(vtkCompositeDataSet::NAME(), curveName.c_str());
      }
    }
    return 1;
  }
  // open file
  std::FILE* file = nullptr;
  if (!this->OpenFile(file))
  {
    return 0;
  }
  // read the table
  this->ReadTable(file, surfaceOutput, this->TableId);

  const vtkIdType numberOfPoints = surfaceOutput->GetNumberOfPoints();
  // convert the data if any conversion is needed
  if (this->VariableConversionValues->GetNumberOfValues() > 0)
  {
    const auto numberOfConversionValues =
      static_cast<int>(this->VariableConversionValues->GetNumberOfValues());
    for (int i = 0; i < numberOfConversionValues; ++i)
    {
      if (auto array = vtkFloatArray::SafeDownCast(surfaceOutput->GetPointData()->GetArray(i)))
      {
        const double conversionValue = this->VariableConversionValues->GetValue(i);
        vtkSMPTools::For(0, numberOfPoints,
          [&](vtkIdType begin, vtkIdType end)
          {
            for (vtkIdType j = begin; j < end; ++j)
            {
              array->SetValue(j, static_cast<float>(array->GetValue(j) * conversionValue));
            }
          });
      }
    }
  }

  // check if array names are nullptr
  if (!this->XArrayName || !this->YArrayName || !this->ZArrayName)
  {
    vtkErrorMacro("X, Y and Z array names must be specified.");
    fclose(file);
    return 0;
  }

  auto xArray =
    vtkFloatArray::SafeDownCast(surfaceOutput->GetPointData()->GetArray(this->XArrayName));
  auto yArray =
    vtkFloatArray::SafeDownCast(surfaceOutput->GetPointData()->GetArray(this->YArrayName));
  auto zArray =
    vtkFloatArray::SafeDownCast(surfaceOutput->GetPointData()->GetArray(this->ZArrayName));

  // check if the arrays exist
  if (!xArray || !yArray || !zArray)
  {
    vtkErrorMacro("X or Y or Z arrays were not part of the table.");
    fclose(file);
    return 0;
  }

  // set the points based on the selected arrays
  auto coords = vtkFloatArray::SafeDownCast(surfaceOutput->GetPoints()->GetData());
  vtkSMPTools::For(0, numberOfPoints,
    [&](vtkIdType begin, vtkIdType end)
    {
      float* coordsPtr = coords->GetPointer(3 * begin);
      for (vtkIdType pointId = begin; pointId < end; ++pointId, coordsPtr += 3)
      {
        coordsPtr[0] = xArray->GetValue(pointId);
        coordsPtr[1] = yArray->GetValue(pointId);
        coordsPtr[2] = zArray->GetValue(pointId);
      }
    });
  surfaceOutput->GetPoints()->Modified();

  // This array is needed to be able to identify  if IsSimulationData is true or not for
  // vtkPrismGeometryRepresentation.
  vtkNew<vtkUnsignedCharArray> prismData;
  prismData->SetName("PRISM_DATA");
  prismData->InsertNextValue(1);
  surfaceOutput->GetFieldData()->AddArray(prismData);

  // set the arrays names as field data

  // this is needed to be able to set the x axis name the PrismView.
  vtkNew<vtkStringArray> xTitle;
  xTitle->SetName("XTitle");
  xTitle->InsertNextValue(this->XArrayName);
  surfaceOutput->GetFieldData()->AddArray(xTitle);

  // this is needed to be able to set the y axis name the PrismView.
  vtkNew<vtkStringArray> yTitle;
  yTitle->SetName("YTitle");
  yTitle->InsertNextValue(this->YArrayName);
  surfaceOutput->GetFieldData()->AddArray(yTitle);

  // this is needed to be able to set the z axis name the PrismView.
  vtkNew<vtkStringArray> zTitle;
  zTitle->SetName("ZTitle");
  zTitle->InsertNextValue(this->ZArrayName);
  surfaceOutput->GetFieldData()->AddArray(zTitle);

  if (this->TableId == 301 && this->GetCurvesAvailable() && this->ReadCurves)
  {
    // read the curves
    this->RequestCurvesData(file, curvesOutput);
  }
  // if we have reached this point, the file is open, therefore we can close it
  fclose(file);

  return 1;
}
