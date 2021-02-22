#include "vtkPEnSightGoldReader.h"

#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkImageData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include "vtksys/FStream.hxx"

#include <ctype.h>
#include <sstream>
#include <string>
#include <vector>

vtkStandardNewMacro(vtkPEnSightGoldReader);

class UndefPartialInternal
{
public:
  double UndefCoordinates;
  double UndefBlock;
  double UndefElementTypes;
  std::vector<vtkIdType> PartialCoordinates;
  std::vector<vtkIdType> PartialBlock;
  std::vector<vtkIdType> PartialElementTypes;
};

//----------------------------------------------------------------------------
vtkPEnSightGoldReader::vtkPEnSightGoldReader()
{
  this->UndefPartial = new UndefPartialInternal;

  this->NodeIdsListed = 0;
  this->ElementIdsListed = 0;
  // this->DebugOn();
}
//----------------------------------------------------------------------------

vtkPEnSightGoldReader::~vtkPEnSightGoldReader()
{
  delete UndefPartial;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldReader::ReadGeometryFile(
  const char* fileName, int timeStep, vtkMultiBlockDataSet* output)
{
  char line[256], subLine[256];
  int partId, realId, i;
  int lineRead;

  // init line and subLine in case ReadLine(.), ReadNextDataLine(.), or
  // sscanf(...) fails while strncmp(..) is still subsequently performed
  // on these two un-assigned char arrays to cause memory leakage, as
  // detected by Valgrind. As an example, VTKData/Data/EnSight/test.geo
  // makes the first sscanf(...) below fail to assign 'subLine' that is
  // though then accessed by strnmp(..) for comparing two char arrays.
  line[0] = '\0';
  subLine[0] = '\0';

  // Initialize
  //
  if (!fileName)
  {
    vtkErrorMacro("A GeometryFileName must be specified in the case file.");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    vtkDebugMacro("full path to geometry file: " << sfilename.c_str());
  }
  else
  {
    sfilename = fileName;
  }

  // Opening the text file as binary. If not, the reader fails to read
  // files with Unix line endings on Windows machines.
  this->IS = new vtksys::ifstream(sfilename.c_str(), ios::in | ios::binary);
  if (this->IS->fail())
  {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    delete this->IS;
    this->IS = nullptr;
    return 0;
  }

  this->ReadNextDataLine(line);
  sscanf(line, " %*s %s", subLine);
  if (strncmp(subLine, "Binary", 6) == 0)
  {
    vtkErrorMacro("This is a binary data set. Try "
      << "vtkEnSightGoldBinaryReader.");
    return 0;
  }

  if (this->UseFileSets)
  {
    int realTimeStep = timeStep - 1;
    // Try to find the nearest time step for which we know the offset
    int j = 0;
    for (i = realTimeStep; i >= 0; i--)
    {
      if (this->FileOffsets.find(fileName) != this->FileOffsets.end() &&
        this->FileOffsets[fileName].find(i) != this->FileOffsets[fileName].end())
      {
        this->IS->seekg(this->FileOffsets[fileName][i], ios::beg);
        j = i;
        break;
      }
    }

    // Hopefully we are not very far from the timestep we want to use
    // Find it (and cache any timestep we find on the way...)
    while (j++ < realTimeStep)
    {
      while (strncmp(line, "END TIME STEP", 13) != 0)
      {
        this->ReadLine(line);
      }
      this->ReadLine(line);
      if (this->FileOffsets.find(fileName) == this->FileOffsets.end())
      {
        std::map<int, long> tsMap;
        this->FileOffsets[fileName] = tsMap;
      }
      this->FileOffsets[fileName][j] = this->IS->tellg();
    }

    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadNextDataLine(line);
    }
    this->ReadLine(line);
  }

  // Skip description lines.  Using ReadLine instead of
  // ReadNextDataLine because the description line could be blank.
  this->ReadLine(line);

  // Read the node id and element id lines.
  this->ReadNextDataLine(line);
  sscanf(line, " %*s %*s %s", subLine);
  if (strncmp(subLine, "given", 5) == 0)
  {
    this->NodeIdsListed = 1;
  }
  else if (strncmp(subLine, "ignore", 6) == 0)
  {
    this->NodeIdsListed = 1;
  }
  else
  {
    this->NodeIdsListed = 0;
  }

  this->ReadNextDataLine(line);
  sscanf(line, " %*s %*s %s", subLine);
  if (strncmp(subLine, "given", 5) == 0)
  {
    this->ElementIdsListed = 1;
  }
  else if (strncmp(subLine, "ignore", 6) == 0)
  {
    this->ElementIdsListed = 1;
  }
  else
  {
    this->ElementIdsListed = 0;
  }

  lineRead = this->ReadNextDataLine(line); // "extents" or "part"
  if (strncmp(line, "extents", 7) == 0)
  {
    // Skipping the extent lines for now.
    this->ReadNextDataLine(line);
    this->ReadNextDataLine(line);
    this->ReadNextDataLine(line);
    lineRead = this->ReadNextDataLine(line); // "part"
  }

  while (lineRead && strncmp(line, "part", 4) == 0)
  {
    this->NumberOfGeometryParts++;
    this->ReadNextDataLine(line);
    partId = atoi(line) - 1; // EnSight starts #ing at 1.

    realId = this->InsertNewPartId(partId);

    this->ReadNextDataLine(line); // part description line
    if (strncmp(line, "interface", 9) == 0)
    {
      return 1; // ignore it and move on
    }
    char* name = strdup(line);

    this->ReadNextDataLine(line);

    if (strncmp(line, "block", 5) == 0)
    {
      if (sscanf(line, " %*s %s", subLine) == 1)
      {
        if (strncmp(subLine, "rectilinear", 11) == 0)
        {
          // block rectilinear
          lineRead = this->CreateRectilinearGridOutput(realId, line, name, output);
        }
        else if (strncmp(subLine, "uniform", 7) == 0)
        {
          // block uniform
          lineRead = this->CreateImageDataOutput(realId, line, name, output);
        }
        else
        {
          // block iblanked
          lineRead = this->CreateStructuredGridOutput(realId, line, name, output);
        }
      }
      else
      {
        // block
        lineRead = this->CreateStructuredGridOutput(realId, line, name, output);
      }
    }
    else
    {
      lineRead = this->CreateUnstructuredGridOutput(realId, line, name, output);
      if (lineRead < 0)
      {
        free(name);
        delete this->IS;
        this->IS = nullptr;
        return 0;
      }
    }
    free(name);
  }

  delete this->IS;
  this->IS = nullptr;
  return 1;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldReader::ReadMeasuredGeometryFile(
  const char* fileName, int timeStep, vtkMultiBlockDataSet* output)
{
  char line[256], subLine[256];
  vtkPoints* newPoints;
  int i;
  int tempId;
  vtkIdType id;
  float coords[3];
  vtkPolyData* geom;

  // Initialize
  //
  if (!fileName)
  {
    vtkErrorMacro("A MeasuredFileName must be specified in the case file.");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    vtkDebugMacro("full path to measured geometry file: " << sfilename.c_str());
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new vtksys::ifstream(sfilename.c_str(), ios::in | ios::binary);
  if (this->IS->fail())
  {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    delete this->IS;
    this->IS = nullptr;
    return 0;
  }

  // Skip the description line.  Using ReadLine instead of ReadNextDataLine
  // because the description line could be blank.
  this->ReadLine(line);

  if (sscanf(line, " %*s %s", subLine) == 1)
  {
    if (strncmp(subLine, "Binary", 6) == 0)
    {
      vtkErrorMacro("This is a binary data set. Try "
        << "vtkEnSight6BinaryReader.");
      return 0;
    }
  }

  if (this->UseFileSets)
  {
    int realTimeStep = timeStep - 1;
    // Try to find the nearest time step for which we know the offset
    int j = 0;
    for (i = realTimeStep; i >= 0; i--)
    {
      if (this->FileOffsets.find(fileName) != this->FileOffsets.end() &&
        this->FileOffsets[fileName].find(i) != this->FileOffsets[fileName].end())
      {
        this->IS->seekg(this->FileOffsets[fileName][i], ios::beg);
        j = i;
        break;
      }
    }

    // Hopefully we are not very far from the timestep we want to use
    // Find it (and cache any timestep we find on the way...)
    while (j++ < realTimeStep)
    {
      while (strncmp(line, "END TIME STEP", 13) != 0)
      {
        this->ReadLine(line);
      }
      this->ReadLine(line);
      if (this->FileOffsets.find(fileName) == this->FileOffsets.end())
      {
        std::map<int, long> tsMap;
        this->FileOffsets[fileName] = tsMap;
      }
      this->FileOffsets[fileName][j] = this->IS->tellg();
    }

    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadNextDataLine(line);
    }
    this->ReadLine(line);
  }

  this->ReadLine(line); // "particle coordinates"
  this->ReadLine(line);
  this->NumberOfMeasuredPoints = atoi(line);

  this->UnstructuredPartIds->InsertNextId(this->NumberOfGeometryParts);
  int partId = this->UnstructuredPartIds->IsId(this->NumberOfGeometryParts);

  this->GetPointIds(partId)->Reset();
  this->GetCellIds(partId, 0)->Reset();

  // Measured Points are like a n * 1 * 1 IJK Structure...
  this->GetPointIds(partId)->SetMode(IMPLICIT_STRUCTURED_MODE);
  this->GetCellIds(partId, 0)->SetMode(IMPLICIT_STRUCTURED_MODE);
  int dimensions[3];
  dimensions[0] = this->NumberOfMeasuredPoints;
  dimensions[1] = 1;
  dimensions[2] = 1;
  int newDimensions[3];
  int splitDimension;
  int splitDimensionBeginIndex;
  this->PrepareStructuredDimensionsForDistribution(partId, dimensions, newDimensions,
    &splitDimension, &splitDimensionBeginIndex, 0, nullptr, nullptr);

  vtkDataSet* ds = this->GetDataSetFromBlock(output, partId);
  if (ds == nullptr || !ds->IsA("vtkPolyData"))
  {
    vtkDebugMacro("creating new measured geometry output");
    vtkPolyData* pd = vtkPolyData::New();
    pd->Allocate(this->GetPointIds(partId)->GetLocalNumberOfIds());
    this->AddToBlock(output, partId, pd);
    ds = pd;
    pd->Delete();
  }

  geom = vtkPolyData::SafeDownCast(ds);

  newPoints = vtkPoints::New();
  newPoints->Allocate(this->GetPointIds(partId)->GetLocalNumberOfIds());

  for (i = 0; i < this->NumberOfMeasuredPoints; i++)
  {
    this->ReadLine(line);
    sscanf(line, " %8d %12e %12e %12e", &tempId, &coords[0], &coords[1], &coords[2]);

    // It seems EnSight always enumerate point indices from 1 to N
    // (not from 0 to N-1) and therefore there is no need to determine
    // flag 'ParticleCoordinatesByIndex'. Instead let's just use 'i',
    // or probably more safely (tempId - 1), as the point index. In this
    // way the geometry that is defined by the datasets mentioned in
    // bug #0008236 can be properly constructed. Fix to bug #0008236.
    id = i;

    int realId = this->GetPointIds(partId)->GetId(id);
    if (realId != -1)
    {
      vtkIdType tmp = realId;
      newPoints->InsertNextPoint(coords);
      geom->InsertNextCell(VTK_VERTEX, 1, &tmp);
    }
  }

  geom->SetPoints(newPoints);
  newPoints->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldReader::ReadScalarsPerNode(const char* fileName, const char* description,
  int timeStep, vtkMultiBlockDataSet* compositeOutput, int measured, int numberOfComponents,
  int component)
{
  char line[256], formatLine[256], tempLine[256];
  int partId, realId, numPts, i, j, numLines, moreScalars;
  vtkFloatArray* scalars;
  float scalarsRead[6];
  vtkDataSet* output;

  // Initialize
  //
  if (!fileName)
  {
    vtkErrorMacro("NULL ScalarPerNode variable file name");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    vtkDebugMacro("full path to scalar per node file: " << sfilename.c_str());
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new vtksys::ifstream(sfilename.c_str(), ios::in | ios::binary);
  if (this->IS->fail())
  {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    delete this->IS;
    this->IS = nullptr;
    return 0;
  }

  if (this->UseFileSets)
  {
    int realTimeStep = timeStep - 1;
    // Try to find the nearest time step for which we know the offset
    j = 0;
    for (i = realTimeStep; i >= 0; i--)
    {
      if (this->FileOffsets.find(fileName) != this->FileOffsets.end() &&
        this->FileOffsets[fileName].find(i) != this->FileOffsets[fileName].end())
      {
        this->IS->seekg(this->FileOffsets[fileName][i], ios::beg);
        j = i;
        break;
      }
    }

    // Hopefully we are not very far from the timestep we want to use
    // Find it (and cache any timestep we find on the way...)
    while (j++ < realTimeStep)
    {
      this->ReadLine(line);
      while (strncmp(line, "END TIME STEP", 13) != 0)
      {
        this->ReadLine(line);
      }
      if (this->FileOffsets.find(fileName) == this->FileOffsets.end())
      {
        std::map<int, long> tsMap;
        this->FileOffsets[fileName] = tsMap;
      }
      this->FileOffsets[fileName][j] = this->IS->tellg();
    }

    this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadLine(line);
    }
  }

  this->ReadNextDataLine(line); // skip the description line

  if (measured)
  {
    partId = this->UnstructuredPartIds->IsId(this->NumberOfGeometryParts);
    output = this->GetDataSetFromBlock(compositeOutput, partId);
    numPts = this->GetPointIds(partId)->GetNumberOfIds();
    if (numPts)
    {
      numLines = numPts / 6;
      moreScalars = numPts % 6;

      scalars = vtkFloatArray::New();
      scalars->SetNumberOfComponents(numberOfComponents);
      scalars->SetNumberOfTuples(this->GetPointIds(partId)->GetLocalNumberOfIds());
      // scalars->Allocate(numPts * numberOfComponents);

      this->ReadNextDataLine(line);

      for (i = 0; i < numLines; i++)
      {
        sscanf(line, " %12e %12e %12e %12e %12e %12e", &scalarsRead[0], &scalarsRead[1],
          &scalarsRead[2], &scalarsRead[3], &scalarsRead[4], &scalarsRead[5]);
        for (j = 0; j < 6; j++)
        {
          // scalars->InsertComponent(i*6 + j, component, scalarsRead[j]);
          this->InsertVariableComponent(
            scalars, i * 6 + j, component, &(scalarsRead[j]), partId, 0, SCALAR_PER_NODE);
        }
        this->ReadNextDataLine(line);
      }
      strcpy(formatLine, "");
      strcpy(tempLine, "");
      for (j = 0; j < moreScalars; j++)
      {
        strcat(formatLine, " %12e");
        sscanf(line, formatLine, &scalarsRead[j]);
        // scalars->InsertComponent(i*6 + j, component, scalarsRead[j]);
        this->InsertVariableComponent(
          scalars, i * 6 + j, component, &(scalarsRead[j]), partId, 0, SCALAR_PER_NODE);
        strcat(tempLine, " %*12e");
        strcpy(formatLine, tempLine);
      }
      scalars->SetName(description);
      output->GetPointData()->AddArray(scalars);
      if (!output->GetPointData()->GetScalars())
      {
        output->GetPointData()->SetScalars(scalars);
      }
      scalars->Delete();
    }
    delete this->IS;
    this->IS = nullptr;
    return 1;
  }

  while (this->ReadNextDataLine(line) && strncmp(line, "part", 4) == 0)
  {
    this->ReadNextDataLine(line);
    partId = atoi(line) - 1; // EnSight starts #ing with 1.
    realId = this->InsertNewPartId(partId);
    output = this->GetDataSetFromBlock(compositeOutput, realId);
    // numPts = output->GetNumberOfPoints();
    numPts = this->GetPointIds(realId)->GetNumberOfIds();
    if (numPts)
    {
      this->ReadNextDataLine(line); // "coordinates" or "block"
      int partial = this->CheckForUndefOrPartial(line);
      if (component == 0)
      {
        scalars = vtkFloatArray::New();
        // scalars->SetNumberOfTuples(numPts);
        scalars->SetNumberOfComponents(numberOfComponents);
        // scalars->Allocate(numPts * numberOfComponents);
        scalars->SetNumberOfTuples(this->GetPointIds(realId)->GetLocalNumberOfIds());
      }
      else
      {
        scalars = (vtkFloatArray*)(output->GetPointData()->GetArray(description));
      }

      // If the keyword 'partial' was found, we should replace unspecified
      // coordinate to take the value specified in the 'undef' field
      if (partial)
      {
        int l = 0;
        // double val;
        float val;
        for (i = 0; i < numPts; i++)
        {
          if (i == this->UndefPartial->PartialCoordinates[l])
          {
            this->ReadNextDataLine(line);
            val = atof(line);
          }
          else
          {
            val = this->UndefPartial->UndefCoordinates;
            l++;
          }
          // scalars->InsertComponent(i, component, val);
          this->InsertVariableComponent(scalars, i, component, &val, realId, 0, SCALAR_PER_NODE);
        }
      }
      else
      {
        float val;
        for (i = 0; i < numPts; i++)
        {
          this->ReadNextDataLine(line);
          val = atof(line);
          // scalars->InsertComponent(i, component, atof(line));
          this->InsertVariableComponent(scalars, i, component, &val, realId, 0, SCALAR_PER_NODE);
        }
      }

      if (component == 0)
      {
        scalars->SetName(description);
        output->GetPointData()->AddArray(scalars);
        if (!output->GetPointData()->GetScalars())
        {
          output->GetPointData()->SetScalars(scalars);
        }
        scalars->Delete();
      }
      else
      {
        output->GetPointData()->AddArray(scalars);
      }
    }
  }

  delete this->IS;
  this->IS = nullptr;
  return 1;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldReader::ReadVectorsPerNode(const char* fileName, const char* description,
  int timeStep, vtkMultiBlockDataSet* compositeOutput, int measured)
{
  char line[256], formatLine[256], tempLine[256];
  int partId, realId, numPts, i, j, numLines, moreVectors;
  vtkFloatArray* vectors;
  float vector1[3], vector2[3];
  vtkDataSet* output;

  // Initialize
  //
  if (!fileName)
  {
    vtkErrorMacro("NULL VectorPerNode variable file name");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    vtkDebugMacro("full path to vector per node file: " << sfilename.c_str());
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new vtksys::ifstream(sfilename.c_str(), ios::in | ios::binary);
  if (this->IS->fail())
  {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    delete this->IS;
    this->IS = nullptr;
    return 0;
  }

  if (this->UseFileSets)
  {
    int realTimeStep = timeStep - 1;
    // Try to find the nearest time step for which we know the offset
    j = 0;
    for (i = realTimeStep; i >= 0; i--)
    {
      if (this->FileOffsets.find(fileName) != this->FileOffsets.end() &&
        this->FileOffsets[fileName].find(i) != this->FileOffsets[fileName].end())
      {
        this->IS->seekg(this->FileOffsets[fileName][i], ios::beg);
        j = i;
        break;
      }
    }

    // Hopefully we are not very far from the timestep we want to use
    // Find it (and cache any timestep we find on the way...)
    while (j++ < realTimeStep)
    {
      this->ReadLine(line);
      while (strncmp(line, "END TIME STEP", 13) != 0)
      {
        this->ReadLine(line);
      }
      if (this->FileOffsets.find(fileName) == this->FileOffsets.end())
      {
        std::map<int, long> tsMap;
        this->FileOffsets[fileName] = tsMap;
      }
      this->FileOffsets[fileName][j] = this->IS->tellg();
    }

    this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadLine(line);
    }
  }

  this->ReadNextDataLine(line); // skip the description line

  if (measured)
  {
    partId = this->UnstructuredPartIds->IsId(this->NumberOfGeometryParts);
    output = this->GetDataSetFromBlock(compositeOutput, partId);
    numPts = this->GetPointIds(partId)->GetNumberOfIds();
    if (numPts)
    {
      this->ReadNextDataLine(line);
      numLines = numPts / 2;
      moreVectors = ((numPts * 3) % 6) / 3;
      vectors = vtkFloatArray::New();
      vectors->SetNumberOfComponents(3);
      // vectors->SetNumberOfTuples(numPts);
      vectors->SetNumberOfTuples(this->GetPointIds(partId)->GetNumberOfIds());
      // vectors->Allocate(numPts*3);
      for (i = 0; i < numLines; i++)
      {
        sscanf(line, " %12e %12e %12e %12e %12e %12e", &vector1[0], &vector1[1], &vector1[2],
          &vector2[0], &vector2[1], &vector2[2]);
        // vectors->InsertTuple(i*2, vector1);
        this->InsertVariableComponent(vectors, i * 2, 0, vector1, partId, 0, VECTOR_PER_NODE);
        // vectors->InsertTuple(i*2 + 1, vector2);
        this->InsertVariableComponent(vectors, i * 2 + 1, 0, vector2, partId, 0, VECTOR_PER_NODE);
        this->ReadNextDataLine(line);
      }
      strcpy(formatLine, "");
      strcpy(tempLine, "");
      for (j = 0; j < moreVectors; j++)
      {
        strcat(formatLine, " %12e %12e %12e");
        sscanf(line, formatLine, &vector1[0], &vector1[1], &vector1[2]);
        // vectors->InsertTuple(i*2 + j, vector1);
        this->InsertVariableComponent(vectors, i * 2 + j, 0, vector1, partId, 0, VECTOR_PER_NODE);
        strcat(tempLine, " %*12e %*12e %*12e");
        strcpy(formatLine, tempLine);
      }
      vectors->SetName(description);
      output->GetPointData()->AddArray(vectors);
      if (!output->GetPointData()->GetVectors())
      {
        output->GetPointData()->SetVectors(vectors);
      }
      vectors->Delete();
    }
    delete this->IS;
    this->IS = nullptr;
    return 1;
  }

  while (this->ReadNextDataLine(line) && strncmp(line, "part", 4) == 0)
  {
    this->ReadNextDataLine(line);
    partId = atoi(line) - 1; // EnSight starts #ing with 1.
    realId = this->InsertNewPartId(partId);
    output = this->GetDataSetFromBlock(compositeOutput, realId);
    // numPts = output->GetNumberOfPoints();
    numPts = this->GetPointIds(realId)->GetNumberOfIds();
    if (numPts)
    {
      vectors = vtkFloatArray::New();
      this->ReadNextDataLine(line); // "coordinates" or "block"
      // vectors->SetNumberOfTuples(numPts);
      vectors->SetNumberOfComponents(3);
      vectors->SetNumberOfTuples(this->GetPointIds(realId)->GetLocalNumberOfIds());
      // vectors->Allocate(numPts*3);
      float val;
      for (i = 0; i < 3; i++)
      {
        for (j = 0; j < numPts; j++)
        {
          this->ReadNextDataLine(line);
          val = atof(line);
          // vectors->InsertComponent(j, i, atof(line));
          // Here we use the SCALAR_PER_NODE behaviour of the
          // InsertVariableComponent method, as components of the vectors
          // are far, far away from each other: we inject data as component,
          // and not as tuple.
          this->InsertVariableComponent(vectors, j, i, &val, realId, 0, SCALAR_PER_NODE);
        }
      }
      vectors->SetName(description);
      output->GetPointData()->AddArray(vectors);
      if (!output->GetPointData()->GetVectors())
      {
        output->GetPointData()->SetVectors(vectors);
      }
      vectors->Delete();
    }
  }

  delete this->IS;
  this->IS = nullptr;
  return 1;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldReader::ReadTensorsPerNode(const char* fileName, const char* description,
  int timeStep, vtkMultiBlockDataSet* compositeOutput)
{
  char line[256];
  int symmTensorOrder[6] = { 0, 1, 2, 3, 5, 4 };
  int partId, realId, numPts, i, j;
  vtkFloatArray* tensors;
  vtkDataSet* output;

  // Initialize
  //
  if (!fileName)
  {
    vtkErrorMacro("NULL TensorPerNode variable file name");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    vtkDebugMacro("full path to tensor per node file: " << sfilename.c_str());
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new vtksys::ifstream(sfilename.c_str(), ios::in | ios::binary);
  if (this->IS->fail())
  {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    delete this->IS;
    this->IS = nullptr;
    return 0;
  }

  if (this->UseFileSets)
  {
    int realTimeStep = timeStep - 1;
    // Try to find the nearest time step for which we know the offset
    j = 0;
    for (i = realTimeStep; i >= 0; i--)
    {
      if (this->FileOffsets.find(fileName) != this->FileOffsets.end() &&
        this->FileOffsets[fileName].find(i) != this->FileOffsets[fileName].end())
      {
        this->IS->seekg(this->FileOffsets[fileName][i], ios::beg);
        j = i;
        break;
      }
    }

    // Hopefully we are not very far from the timestep we want to use
    // Find it (and cache any timestep we find on the way...)
    while (j++ < realTimeStep)
    {
      this->ReadLine(line);
      while (strncmp(line, "END TIME STEP", 13) != 0)
      {
        this->ReadLine(line);
      }
      if (this->FileOffsets.find(fileName) == this->FileOffsets.end())
      {
        std::map<int, long> tsMap;
        this->FileOffsets[fileName] = tsMap;
      }
      this->FileOffsets[fileName][j] = this->IS->tellg();
    }

    this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadLine(line);
    }
  }

  this->ReadNextDataLine(line); // skip the description line

  while (this->ReadNextDataLine(line) && strncmp(line, "part", 4) == 0)
  {
    this->ReadNextDataLine(line);
    partId = atoi(line) - 1; // EnSight starts #ing with 1.
    realId = this->InsertNewPartId(partId);
    output = this->GetDataSetFromBlock(compositeOutput, realId);
    // numPts = output->GetNumberOfPoints();
    numPts = this->GetPointIds(realId)->GetNumberOfIds();
    if (numPts)
    {
      tensors = vtkFloatArray::New();
      this->ReadNextDataLine(line); // "coordinates" or "block"
      // tensors->SetNumberOfTuples(numPts);
      tensors->SetNumberOfComponents(6);
      tensors->SetNumberOfTuples(this->GetPointIds(realId)->GetLocalNumberOfIds());
      // tensors->Allocate(numPts*6);
      float val;
      for (i = 0; i < 6; i++)
      {
        for (j = 0; j < numPts; j++)
        {
          this->ReadNextDataLine(line);
          val = atof(line);
          // tensors->InsertComponent(j, i, atof(line));
          // Same behaviour as Vector Per Node variables: we inject data as component,
          // and not as tuple.
          this->InsertVariableComponent(
            tensors, j, symmTensorOrder[i], &val, realId, 0, SCALAR_PER_NODE);
        }
      }
      tensors->SetName(description);
      output->GetPointData()->AddArray(tensors);
      tensors->Delete();
    }
  }

  delete this->IS;
  this->IS = nullptr;
  return 1;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldReader::ReadScalarsPerElement(const char* fileName, const char* description,
  int timeStep, vtkMultiBlockDataSet* compositeOutput, int numberOfComponents, int component)
{
  char line[256];
  int partId, realId, numCells, numCellsPerElement, i, idx;
  vtkFloatArray* scalars;
  int lineRead, elementType;
  float scalar;
  vtkDataSet* output;

  // Initialize
  //
  if (!fileName)
  {
    vtkErrorMacro("NULL ScalarPerElement variable file name");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    vtkDebugMacro("full path to scalar per element file: " << sfilename.c_str());
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new vtksys::ifstream(sfilename.c_str(), ios::in | ios::binary);
  if (this->IS->fail())
  {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    delete this->IS;
    this->IS = nullptr;
    return 0;
  }

  if (this->UseFileSets)
  {
    int realTimeStep = timeStep - 1;
    // Try to find the nearest time step for which we know the offset
    int j = 0;
    for (i = realTimeStep; i >= 0; i--)
    {
      if (this->FileOffsets.find(fileName) != this->FileOffsets.end() &&
        this->FileOffsets[fileName].find(i) != this->FileOffsets[fileName].end())
      {
        this->IS->seekg(this->FileOffsets[fileName][i], ios::beg);
        j = i;
        break;
      }
    }

    // Hopefully we are not very far from the timestep we want to use
    // Find it (and cache any timestep we find on the way...)
    while (j++ < realTimeStep)
    {
      this->ReadLine(line);
      while (strncmp(line, "END TIME STEP", 13) != 0)
      {
        this->ReadLine(line);
      }
      if (this->FileOffsets.find(fileName) == this->FileOffsets.end())
      {
        std::map<int, long> tsMap;
        this->FileOffsets[fileName] = tsMap;
      }
      this->FileOffsets[fileName][j] = this->IS->tellg();
    }

    this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadLine(line);
    }
  }

  this->ReadNextDataLine(line);            // skip the description line
  lineRead = this->ReadNextDataLine(line); // "part"

  while (lineRead && strncmp(line, "part", 4) == 0)
  {
    this->ReadNextDataLine(line);
    partId = atoi(line) - 1; // EnSight starts #ing with 1.
    realId = this->InsertNewPartId(partId);
    output = this->GetDataSetFromBlock(compositeOutput, realId);
    // numCells = output->GetNumberOfCells();
    numCells = this->GetTotalNumberOfCellIds(realId);
    if (numCells)
    {
      this->ReadNextDataLine(line); // element type or "block"
      if (component == 0)
      {
        scalars = vtkFloatArray::New();
        scalars->SetNumberOfComponents(numberOfComponents);
        // scalars->SetNumberOfTuples(numCells);
        scalars->SetNumberOfTuples(this->GetLocalTotalNumberOfCellIds(realId));
      }
      else
      {
        scalars = (vtkFloatArray*)(output->GetCellData()->GetArray(description));
      }

      // need to find out from CellIds how many cells we have of this element
      // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
      if (strncmp(line, "block", 5) == 0)
      {
        for (i = 0; i < numCells; i++)
        {
          this->ReadNextDataLine(line);
          scalar = atof(line);
          // scalars->InsertComponent(i, component, scalar);
          this->InsertVariableComponent(
            scalars, i, component, &scalar, realId, 0, SCALAR_PER_ELEMENT);
        }
        lineRead = this->ReadNextDataLine(line);
      }
      else
      {
        while (lineRead && strncmp(line, "part", 4) != 0 && strncmp(line, "END TIME STEP", 13) != 0)
        {
          elementType = this->GetElementType(line);
          // Check if line contains either 'partial' or 'undef' keyword
          int partial = this->CheckForUndefOrPartial(line);
          if (elementType == -1)
          {
            vtkErrorMacro("Unknown element type \"" << line << "\"");
            delete this->IS;
            this->IS = nullptr;
            if (component == 0)
            {
              scalars->Delete();
            }
            return 0;
          }
          idx = this->UnstructuredPartIds->IsId(realId);
          numCellsPerElement = this->GetCellIds(idx, elementType)->GetNumberOfIds();
          // If the 'partial' keyword was found, we should replace
          // unspecified coordinate with value specified in the 'undef' section
          if (partial)
          {
            int j = 0;
            for (i = 0; i < numCellsPerElement; i++)
            {
              if (i == this->UndefPartial->PartialElementTypes[j])
              {
                this->ReadNextDataLine(line);
                scalar = atof(line);
              }
              else
              {
                scalar = this->UndefPartial->UndefElementTypes;
                j++; // go on to the next value in the partial list
              }
              // scalars->InsertComponent( this->GetCellIds(idx,
              //  elementType)->GetId(i), component, scalar);
              this->InsertVariableComponent(
                scalars, i, component, &scalar, idx, elementType, SCALAR_PER_ELEMENT);
            }
          }
          else
          {
            for (i = 0; i < numCellsPerElement; i++)
            {
              this->ReadNextDataLine(line);
              scalar = atof(line);
              // scalars->InsertComponent( this->GetCellIds(idx,
              //  elementType)->GetId(i), component, scalar);
              this->InsertVariableComponent(
                scalars, i, component, &scalar, idx, elementType, SCALAR_PER_ELEMENT);
            }
          }
          lineRead = this->ReadNextDataLine(line);
        } // end while
      }   // end else
      if (component == 0)
      {
        scalars->SetName(description);
        output->GetCellData()->AddArray(scalars);
        if (!output->GetCellData()->GetScalars())
        {
          output->GetCellData()->SetScalars(scalars);
        }
        scalars->Delete();
      }
      else
      {
        output->GetCellData()->AddArray(scalars);
      }
    }
    else
    {
      lineRead = this->ReadNextDataLine(line);
    }
  }

  delete this->IS;
  this->IS = nullptr;
  return 1;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldReader::ReadVectorsPerElement(const char* fileName, const char* description,
  int timeStep, vtkMultiBlockDataSet* compositeOutput)
{
  char line[256];
  int partId, realId, numCells, numCellsPerElement, i, j, idx;
  vtkFloatArray* vectors;
  int lineRead, elementType;
  float value;
  vtkDataSet* output;

  // Initialize
  //
  if (!fileName)
  {
    vtkErrorMacro("NULL VectorPerElement variable file name");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    vtkDebugMacro("full path to vector per element file: " << sfilename.c_str());
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new vtksys::ifstream(sfilename.c_str(), ios::in | ios::binary);
  if (this->IS->fail())
  {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    delete this->IS;
    this->IS = nullptr;
    return 0;
  }

  if (this->UseFileSets)
  {
    int realTimeStep = timeStep - 1;
    // Try to find the nearest time step for which we know the offset
    j = 0;
    for (i = realTimeStep; i >= 0; i--)
    {
      if (this->FileOffsets.find(fileName) != this->FileOffsets.end() &&
        this->FileOffsets[fileName].find(i) != this->FileOffsets[fileName].end())
      {
        this->IS->seekg(this->FileOffsets[fileName][i], ios::beg);
        j = i;
        break;
      }
    }

    // Hopefully we are not very far from the timestep we want to use
    // Find it (and cache any timestep we find on the way...)
    while (j++ < realTimeStep)
    {
      this->ReadLine(line);
      while (strncmp(line, "END TIME STEP", 13) != 0)
      {
        this->ReadLine(line);
      }
      if (this->FileOffsets.find(fileName) == this->FileOffsets.end())
      {
        std::map<int, long> tsMap;
        this->FileOffsets[fileName] = tsMap;
      }
      this->FileOffsets[fileName][j] = this->IS->tellg();
    }

    this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadLine(line);
    }
  }

  this->ReadNextDataLine(line);            // skip the description line
  lineRead = this->ReadNextDataLine(line); // "part"

  while (lineRead && strncmp(line, "part", 4) == 0)
  {
    this->ReadNextDataLine(line);
    partId = atoi(line) - 1; // EnSight starts #ing with 1.
    realId = this->InsertNewPartId(partId);
    output = this->GetDataSetFromBlock(compositeOutput, realId);
    // numCells = output->GetNumberOfCells();
    numCells = this->GetTotalNumberOfCellIds(realId);
    if (numCells)
    {
      vectors = vtkFloatArray::New();
      this->ReadNextDataLine(line); // element type or "block"
      vectors->SetNumberOfComponents(3);
      // vectors->SetNumberOfTuples(numCells);
      vectors->SetNumberOfTuples(this->GetLocalTotalNumberOfCellIds(realId));
      // vectors->Allocate(numCells*3);

      // need to find out from CellIds how many cells we have of this element
      // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
      if (strncmp(line, "block", 5) == 0)
      {
        for (i = 0; i < 3; i++)
        {
          for (j = 0; j < numCells; j++)
          {
            this->ReadNextDataLine(line);
            value = atof(line);
            // vectors->InsertComponent(j, i, value);
            this->InsertVariableComponent(vectors, j, i, &value, realId, 0, SCALAR_PER_ELEMENT);
          }
        }
        lineRead = this->ReadNextDataLine(line);
      }
      else
      {
        while (lineRead && strncmp(line, "part", 4) != 0 && strncmp(line, "END TIME STEP", 13) != 0)
        {
          elementType = this->GetElementType(line);
          if (elementType == -1)
          {
            vtkErrorMacro("Unknown element type \"" << line << "\"");
            delete this->IS;
            this->IS = nullptr;
            vectors->Delete();
            return 0;
          }
          idx = this->UnstructuredPartIds->IsId(realId);
          numCellsPerElement = this->GetCellIds(idx, elementType)->GetNumberOfIds();
          for (i = 0; i < 3; i++)
          {
            for (j = 0; j < numCellsPerElement; j++)
            {
              this->ReadNextDataLine(line);
              value = atof(line);
              // vectors->InsertComponent(this->GetCellIds(idx, elementType)->GetId(j),
              //                         i, value);
              this->InsertVariableComponent(
                vectors, j, i, &value, idx, elementType, SCALAR_PER_ELEMENT);
            }
          }
          lineRead = this->ReadNextDataLine(line);
        } // end while
      }   // end else
      vectors->SetName(description);
      output->GetCellData()->AddArray(vectors);
      if (!output->GetCellData()->GetVectors())
      {
        output->GetCellData()->SetVectors(vectors);
      }
      vectors->Delete();
    }
    else
    {
      lineRead = this->ReadNextDataLine(line);
    }
  }

  delete this->IS;
  this->IS = nullptr;
  return 1;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldReader::ReadTensorsPerElement(const char* fileName, const char* description,
  int timeStep, vtkMultiBlockDataSet* compositeOutput)
{
  char line[256];
  int symmTensorOrder[6] = { 0, 1, 2, 3, 5, 4 };
  int partId, realId, numCells, numCellsPerElement, i, j, idx;
  vtkFloatArray* tensors;
  int lineRead, elementType;
  float value;
  vtkDataSet* output;

  // Initialize
  //
  if (!fileName)
  {
    vtkErrorMacro("NULL TensorPerElement variable file name");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    vtkDebugMacro("full path to tensor per element file: " << sfilename.c_str());
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new vtksys::ifstream(sfilename.c_str(), ios::in | ios::binary);
  if (this->IS->fail())
  {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    delete this->IS;
    this->IS = nullptr;
    return 0;
  }

  if (this->UseFileSets)
  {
    int realTimeStep = timeStep - 1;
    // Try to find the nearest time step for which we know the offset
    j = 0;
    for (i = realTimeStep; i >= 0; i--)
    {
      if (this->FileOffsets.find(fileName) != this->FileOffsets.end() &&
        this->FileOffsets[fileName].find(i) != this->FileOffsets[fileName].end())
      {
        this->IS->seekg(this->FileOffsets[fileName][i], ios::beg);
        j = i;
        break;
      }
    }

    // Hopefully we are not very far from the timestep we want to use
    // Find it (and cache any timestep we find on the way...)
    while (j++ < realTimeStep)
    {
      this->ReadLine(line);
      while (strncmp(line, "END TIME STEP", 13) != 0)
      {
        this->ReadLine(line);
      }
      if (this->FileOffsets.find(fileName) == this->FileOffsets.end())
      {
        std::map<int, long> tsMap;
        this->FileOffsets[fileName] = tsMap;
      }
      this->FileOffsets[fileName][j] = this->IS->tellg();
    }

    this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadLine(line);
    }
  }

  this->ReadNextDataLine(line);            // skip the description line
  lineRead = this->ReadNextDataLine(line); // "part"

  while (lineRead && strncmp(line, "part", 4) == 0)
  {
    this->ReadNextDataLine(line);
    partId = atoi(line) - 1; // EnSight starts #ing with 1.
    realId = this->InsertNewPartId(partId);
    output = this->GetDataSetFromBlock(compositeOutput, realId);
    // numCells = output->GetNumberOfCells();
    numCells = this->GetTotalNumberOfCellIds(realId);
    if (numCells)
    {
      tensors = vtkFloatArray::New();
      this->ReadNextDataLine(line); // element type or "block"
      // tensors->SetNumberOfTuples(numCells);
      tensors->SetNumberOfComponents(6);
      tensors->SetNumberOfTuples(this->GetLocalTotalNumberOfCellIds(realId));
      // tensors->Allocate(numCells*6);

      // need to find out from CellIds how many cells we have of this element
      // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
      if (strncmp(line, "block", 5) == 0)
      {
        for (i = 0; i < 6; i++)
        {
          for (j = 0; j < numCells; j++)
          {
            this->ReadNextDataLine(line);
            value = atof(line);
            // tensors->InsertComponent(j, i, value);
            this->InsertVariableComponent(
              tensors, j, symmTensorOrder[i], &value, realId, 0, SCALAR_PER_ELEMENT);
          }
        }
        lineRead = this->ReadNextDataLine(line);
      }
      else
      {
        while (lineRead && strncmp(line, "part", 4) != 0 && strncmp(line, "END TIME STEP", 13) != 0)
        {
          elementType = this->GetElementType(line);
          if (elementType == -1)
          {
            vtkErrorMacro("Unknown element type \"" << line << "\"");
            delete[] this->IS;
            this->IS = nullptr;
            tensors->Delete();
            return 0;
          }
          idx = this->UnstructuredPartIds->IsId(realId);
          numCellsPerElement = this->GetCellIds(idx, elementType)->GetNumberOfIds();
          for (i = 0; i < 6; i++)
          {
            for (j = 0; j < numCellsPerElement; j++)
            {
              this->ReadNextDataLine(line);
              value = atof(line);
              // tensors->InsertComponent(this->GetCellIds(idx, elementType)->GetId(j),
              //                         i, value);
              this->InsertVariableComponent(
                tensors, j, symmTensorOrder[i], &value, idx, elementType, SCALAR_PER_ELEMENT);
            }
          }
          lineRead = this->ReadNextDataLine(line);
        } // end while
      }   // end else
      tensors->SetName(description);
      output->GetCellData()->AddArray(tensors);
      tensors->Delete();
    }
    else
    {
      lineRead = this->ReadNextDataLine(line);
    }
  }

  delete this->IS;
  this->IS = nullptr;
  return 1;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldReader::CreateUnstructuredGridOutput(
  int partId, char line[256], const char* name, vtkMultiBlockDataSet* compositeOutput)
{
  int lineRead = 1;
  char subLine[256];
  int i, j, k;
  vtkIdType* nodeIds;
  int* intIds;
  int numElements;
  int idx, cellType;

  this->NumberOfNewOutputs++;

  vtkDataSet* ds = this->GetDataSetFromBlock(compositeOutput, partId);
  if (ds == nullptr || !ds->IsA("vtkUnstructuredGrid"))
  {
    vtkDebugMacro("creating new unstructured output");
    vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::New();
    this->AddToBlock(compositeOutput, partId, ugrid);
    ugrid->Delete();
    ds = ugrid;

    this->UnstructuredPartIds->InsertNextId(partId);
  }

  vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(ds);

  this->SetBlockName(compositeOutput, partId, name);

  // Clear all cell ids from the last execution, if any.
  idx = this->UnstructuredPartIds->IsId(partId);
  for (i = 0; i < 16; i++)
  {
    this->GetCellIds(idx, i)->Reset();
  }

  output->Allocate(1000);

  long coordinatesOffset = -1;
  this->CoordinatesAtEnd = false;
  this->InjectGlobalNodeIds = false;
  this->InjectGlobalElementIds = false;
  this->LastPointId = 0;

  while (lineRead && strncmp(line, "part", 4) != 0)
  {
    if (strncmp(line, "coordinates", 11) == 0)
    {
      vtkDebugMacro("coordinates");
      vtkPoints* points = vtkPoints::New();

      // keep coordinates offset in mind
      coordinatesOffset = -1;
      this->GetPointIds(idx)->Reset();
      this->LastPointId = 0;

      coordinatesOffset = this->IS->tellg();

      int pointsRead =
        this->ReadOrSkipCoordinates(points, coordinatesOffset, idx, &lineRead, line, true);
      if (pointsRead == -1)
        return -1;
      points->Delete();
    }
    else if (strncmp(line, "point", 5) == 0)
    {
      int* elementIds;
      vtkDebugMacro("point");

      nodeIds = new vtkIdType[1];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      elementIds = new int[numElements];

      for (i = 0; i < numElements; i++)
      {
        this->ReadNextDataLine(line);
        elementIds[i] = atoi(line);
      }
      lineRead = this->ReadNextDataLine(line);
      sscanf(line, " %s", subLine);
      if (isdigit(subLine[0]))
      {
        for (i = 0; i < numElements; i++)
        {
          nodeIds[0] = atoi(line) - 1; // because EnSight ids start at 1
          // cellId = output->InsertNextCell(VTK_VERTEX, 1, nodeIds);
          // this->GetCellIds(idx, vtkPEnSightReader::POINT)->InsertNextId(cellId);
          this->InsertNextCellAndId(
            output, VTK_VERTEX, 1, nodeIds, idx, vtkPEnSightReader::POINT, i, numElements);
          lineRead = this->ReadNextDataLine(line);
        }
      }
      else
      {
        for (i = 0; i < numElements; i++)
        {
          nodeIds[0] = elementIds[i] - 1;
          // cellId = output->InsertNextCell(VTK_VERTEX, 1, nodeIds);
          // this->GetCellIds(idx, vtkPEnSightReader::POINT)->InsertNextId(cellId);
          this->InsertNextCellAndId(
            output, VTK_VERTEX, 1, nodeIds, idx, vtkPEnSightReader::POINT, i, numElements);
        }
      }

      delete[] nodeIds;
      delete[] elementIds;
    }
    else if (strncmp(line, "g_point", 7) == 0)
    {
      // skipping ghost cells
      vtkDebugMacro("g_point");

      this->ReadNextDataLine(line);
      numElements = atoi(line);

      for (i = 0; i < numElements; i++)
      {
        this->ReadNextDataLine(line);
      }
      lineRead = this->ReadNextDataLine(line);
      sscanf(line, " %s", subLine);
      if (isdigit(subLine[0]))
      {
        for (i = 0; i < numElements; i++)
        {
          lineRead = this->ReadNextDataLine(line);
        }
      }
    }
    else if (strncmp(line, "bar2", 4) == 0)
    {
      vtkDebugMacro("bar2");

      nodeIds = new vtkIdType[2];
      intIds = new int[2];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d", &intIds[0], &intIds[1]) != 2)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        sscanf(line, " %d %d", &intIds[0], &intIds[1]);
        for (j = 0; j < 2; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        // cellId = output->InsertNextCell(VTK_LINE, 2, nodeIds);
        // this->GetCellIds(idx, vtkPEnSightReader::BAR2)->InsertNextId(cellId);
        this->InsertNextCellAndId(
          output, VTK_LINE, 2, nodeIds, idx, vtkPEnSightReader::BAR2, i, numElements);
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] nodeIds;
      delete[] intIds;
    }
    else if (strncmp(line, "g_bar2", 6) == 0)
    {
      // skipping ghost cells
      vtkDebugMacro("g_bar2");

      intIds = new int[2];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d", &intIds[0], &intIds[1]) != 2)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] intIds;
    }
    else if (strncmp(line, "bar3", 4) == 0)
    {
      vtkDebugMacro("bar3");
      nodeIds = new vtkIdType[3];
      intIds = new int[3];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d", &intIds[0], &intIds[1], &intIds[2]) != 3)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        sscanf(line, " %d %d %d", &intIds[0], &intIds[1], &intIds[2]);
        for (j = 0; j < 3; j++)
        {
          intIds[j]--;
        }
        nodeIds[0] = intIds[0];
        nodeIds[1] = intIds[2];
        nodeIds[2] = intIds[1];

        // cellId = output->InsertNextCell(VTK_QUADRATIC_EDGE, 3, nodeIds);
        // this->GetCellIds(idx, vtkPEnSightReader::BAR3)->InsertNextId(cellId);
        this->InsertNextCellAndId(
          output, VTK_QUADRATIC_EDGE, 3, nodeIds, idx, vtkPEnSightReader::BAR3, i, numElements);
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] nodeIds;
      delete[] intIds;
    }
    else if (strncmp(line, "g_bar3", 6) == 0)
    {
      // skipping ghost cells
      vtkDebugMacro("g_bar3");
      intIds = new int[2];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %*d %d", &intIds[0], &intIds[1]) != 2)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] intIds;
    }
    else if (strncmp(line, "nsided", 6) == 0)
    {
      int* numNodesPerElement;
      int numNodes;
      std::stringstream* lineStream = new std::stringstream(std::stringstream::out);
      std::stringstream* formatStream = new std::stringstream(std::stringstream::out);
      std::stringstream* tempStream = new std::stringstream(std::stringstream::out);

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      if (this->ElementIdsListed)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }

      numNodesPerElement = new int[numElements];
      for (i = 0; i < numElements; i++)
      {
        this->ReadNextDataLine(line);
        numNodesPerElement[i] = atoi(line);
      }

      lineRead = this->ReadNextDataLine(line);
      for (i = 0; i < numElements; i++)
      {
        numNodes = numNodesPerElement[i];
        nodeIds = new vtkIdType[numNodes];
        intIds = new int[numNodesPerElement[i]];

        formatStream->str("");
        tempStream->str("");
        lineStream->str(line);
        lineStream->seekp(0, std::stringstream::end);
        while (!lineRead)
        {
          lineRead = this->ReadNextDataLine(line);
          lineStream->write(line, strlen(line));
          lineStream->seekp(0, std::stringstream::end);
        }
        for (j = 0; j < numNodes; j++)
        {
          formatStream->write(" %d", 3);
          formatStream->seekp(0, std::stringstream::end);
          sscanf(lineStream->str().c_str(), formatStream->str().c_str(), &intIds[numNodes - j - 1]);
          tempStream->write(" %*d", 4);
          tempStream->seekp(0, std::stringstream::end);
          formatStream->str(tempStream->str());
          formatStream->seekp(0, std::stringstream::end);
          intIds[numNodes - j - 1]--;
          nodeIds[numNodes - j - 1] = intIds[numNodes - j - 1];
        }
        // cellId = output->InsertNextCell(VTK_POLYGON, numNodes, nodeIds);
        // this->GetCellIds(idx, vtkPEnSightReader::NSIDED)->InsertNextId(cellId);
        this->InsertNextCellAndId(output, VTK_POLYGON, numNodesPerElement[i], nodeIds, idx,
          vtkPEnSightReader::NSIDED, i, numElements);
        lineRead = this->ReadNextDataLine(line);
        delete[] nodeIds;
        delete[] intIds;
      }
      delete lineStream;
      delete formatStream;
      delete tempStream;
      delete[] numNodesPerElement;
    }
    else if (strncmp(line, "g_nsided", 8) == 0)
    {
      // skipping ghost cells
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      for (i = 0; i < numElements * 2; i++)
      {
        this->ReadNextDataLine(line);
      }
      lineRead = this->ReadNextDataLine(line);
      if (lineRead)
      {
        sscanf(line, " %s", subLine);
      }
      if (lineRead && isdigit(subLine[0]))
      {
        // We still need to read in the node ids for each element.
        for (i = 0; i < numElements; i++)
        {
          lineRead = this->ReadNextDataLine(line);
        }
      }
    }
    else if (strncmp(line, "tria3", 5) == 0)
    {
      vtkDebugMacro("tria3");
      cellType = vtkPEnSightReader::TRIA3;

      nodeIds = new vtkIdType[3];
      intIds = new int[3];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d", &intIds[0], &intIds[1], &intIds[2]) != 3)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        sscanf(line, " %d %d %d", &intIds[0], &intIds[1], &intIds[2]);
        for (j = 0; j < 3; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        // cellId = output->InsertNextCell(VTK_TRIANGLE, 3, nodeIds);
        // this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        this->InsertNextCellAndId(output, VTK_TRIANGLE, 3, nodeIds, idx, cellType, i, numElements);
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] nodeIds;
      delete[] intIds;
    }
    else if (strncmp(line, "tria6", 5) == 0)
    {
      vtkDebugMacro("tria6");
      cellType = vtkPEnSightReader::TRIA6;

      nodeIds = new vtkIdType[6];
      intIds = new int[6];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2], &intIds[3],
            &intIds[4], &intIds[5]) != 6)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        sscanf(line, " %d %d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2], &intIds[3],
          &intIds[4], &intIds[5]);
        for (j = 0; j < 6; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        // cellId = output->InsertNextCell(VTK_QUADRATIC_TRIANGLE, 6, nodeIds);
        // this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        this->InsertNextCellAndId(
          output, VTK_QUADRATIC_TRIANGLE, 6, nodeIds, idx, cellType, i, numElements);
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] nodeIds;
      delete[] intIds;
    }
    else if (strncmp(line, "g_tria3", 7) == 0 || strncmp(line, "g_tria6", 7) == 0)
    {
      // skipping ghost cells
      if (strncmp(line, "g_tria6", 7) == 0)
      {
        vtkDebugMacro("g_tria6");
        cellType = vtkPEnSightReader::TRIA6;
      }
      else
      {
        vtkDebugMacro("g_tria3");
        cellType = vtkPEnSightReader::TRIA3;
      }

      intIds = new int[3];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d", &intIds[0], &intIds[1], &intIds[2]) != 3)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] intIds;
    }
    else if (strncmp(line, "quad4", 5) == 0)
    {
      vtkDebugMacro("quad4");
      cellType = vtkPEnSightReader::QUAD4;

      nodeIds = new vtkIdType[4];
      intIds = new int[4];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d", &intIds[0], &intIds[1], &intIds[2], &intIds[3]) != 4)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        sscanf(line, " %d %d %d %d", &intIds[0], &intIds[1], &intIds[2], &intIds[3]);
        for (j = 0; j < 4; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        // cellId = output->InsertNextCell(VTK_QUAD, 4, nodeIds);
        // this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        this->InsertNextCellAndId(output, VTK_QUAD, 4, nodeIds, idx, cellType, i, numElements);
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] nodeIds;
      delete[] intIds;
    }
    else if (strncmp(line, "quad8", 5) == 0)
    {
      vtkDebugMacro("quad8");
      cellType = vtkPEnSightReader::QUAD8;

      nodeIds = new vtkIdType[8];
      intIds = new int[8];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2], &intIds[3],
            &intIds[4], &intIds[5], &intIds[6], &intIds[7]) != 8)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        sscanf(line, " %d %d %d %d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2], &intIds[3],
          &intIds[4], &intIds[5], &intIds[6], &intIds[7]);
        for (j = 0; j < 8; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        // cellId = output->InsertNextCell(VTK_QUADRATIC_QUAD, 8, nodeIds);
        // this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        this->InsertNextCellAndId(
          output, VTK_QUADRATIC_QUAD, 8, nodeIds, idx, cellType, i, numElements);
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] nodeIds;
      delete[] intIds;
    }
    else if (strncmp(line, "g_quad4", 7) == 0 || strncmp(line, "g_quad8", 7) == 0)
    {
      // skipping ghost cells
      if (strncmp(line, "g_quad8", 7) == 0)
      {
        vtkDebugMacro("g_quad8");
        cellType = vtkPEnSightReader::QUAD8;
      }
      else
      {
        vtkDebugMacro("g_quad4");
        cellType = vtkPEnSightReader::QUAD4;
      }

      intIds = new int[4];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d", &intIds[0], &intIds[1], &intIds[2], &intIds[3]) != 4)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] intIds;
    }
    else if (strncmp(line, "nfaced", 6) == 0)
    {
      int* numFacesPerElement;
      int* numNodesPerFace;
      int* nodeMarker;
      int numPts = 0;
      int numFaces = 0;
      int numNodes = 0;
      int faceCount = 0;
      int elementNodeCount = 0;
      std::stringstream* lineStream = new std::stringstream(std::stringstream::out);
      std::stringstream* formatStream = new std::stringstream(std::stringstream::out);
      std::stringstream* tempStream = new std::stringstream(std::stringstream::out);

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      if (this->ElementIdsListed)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }

      numFacesPerElement = new int[numElements];
      for (i = 0; i < numElements; i++)
      {
        this->ReadNextDataLine(line);
        numFacesPerElement[i] = atoi(line);
        numFaces += numFacesPerElement[i];
      }

      numNodesPerFace = new int[numFaces];
      for (i = 0; i < numFaces; i++)
      {
        this->ReadNextDataLine(line);
        numNodesPerFace[i] = atoi(line);
      }

      // numPts = output->GetNumberOfPoints();
      // Points may have not been read here
      // but we have an estimation in ReadOrSkipCoordinates( .... , skip = true )
      numPts = this->GetPointIds(idx)->GetNumberOfIds();

      nodeMarker = new int[numPts];
      for (i = 0; i < numPts; i++)
      {
        nodeMarker[i] = -1;
      }

      lineRead = this->ReadNextDataLine(line);
      for (i = 0; i < numElements; i++)
      {
        numNodes = 0;
        for (j = 0; j < numFacesPerElement[i]; j++)
        {
          numNodes += numNodesPerFace[faceCount + j];
        }
        intIds = new int[numNodes];

        // Read element node ids
        elementNodeCount = 0;
        for (j = 0; j < numFacesPerElement[i]; j++)
        {
          formatStream->str("");
          tempStream->str("");
          lineStream->str(line);
          lineStream->seekp(0, std::stringstream::end);
          while (!lineRead)
          {
            lineRead = this->ReadNextDataLine(line);
            lineStream->write(line, strlen(line));
            lineStream->seekp(0, std::stringstream::end);
          }
          for (k = 0; k < numNodesPerFace[faceCount + j]; k++)
          {
            formatStream->write(" %d", 3);
            formatStream->seekp(0, std::stringstream::end);
            sscanf(
              lineStream->str().c_str(), formatStream->str().c_str(), &intIds[elementNodeCount]);
            tempStream->write(" %*d", 4);
            tempStream->seekp(0, std::stringstream::end);
            formatStream->str(tempStream->str());
            formatStream->seekp(0, std::stringstream::end);
            elementNodeCount += 1;
          }
          lineRead = this->ReadNextDataLine(line);
        }

        // prepare an array of Ids describing the vtkPolyhedron object
        int nodeIndx = 0; // indexing the raw array of point Ids
        // vtkPolyhedron's info of faces
        std::vector<vtkIdType> faceArray(elementNodeCount + numFacesPerElement[i]);
        vtkIdType faceArrayIdx = 0;
        for (j = 0; j < numFacesPerElement[i]; j++)
        {
          // number of points constituting this face
          faceArray[faceArrayIdx++] = numNodesPerFace[faceCount + j];
          for (k = 0; k < numNodesPerFace[faceCount + j]; k++)
          {
            // convert EnSight 1-based indexing to VTK 0-based indexing
            faceArray[faceArrayIdx++] = intIds[nodeIndx++] - 1;
          }
        }

        faceCount += numFacesPerElement[i];

        // Build element
        nodeIds = new vtkIdType[numNodes];
        elementNodeCount = 0;
        for (j = 0; j < numNodes; j++)
        {
          if (nodeMarker[intIds[j] - 1] < i)
          {
            nodeIds[elementNodeCount] = intIds[j] - 1;
            nodeMarker[intIds[j] - 1] = i;
            elementNodeCount += 1;
          }
        }
        /*cellId = output->InsertNextCell(VTK_CONVEX_POINT_SET,
          elementNodeCount,
          nodeIds);
          this->GetCellIds(idx, vtkPEnSightReader::NFACED)->InsertNextId(cellId);*/
        this->InsertNextCellAndId(output, VTK_POLYHEDRON, elementNodeCount, nodeIds, idx,
          vtkPEnSightReader::NFACED, i, numElements, faceArray);

        delete[] nodeIds;
        delete[] intIds;
      }

      delete lineStream;
      delete formatStream;
      delete tempStream;

      delete[] nodeMarker;
      delete[] numNodesPerFace;
      delete[] numFacesPerElement;
    }
    else if (strncmp(line, "tetra4", 6) == 0)
    {
      vtkDebugMacro("tetra4");
      cellType = vtkPEnSightReader::TETRA4;

      nodeIds = new vtkIdType[4];
      intIds = new int[4];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d", &intIds[0], &intIds[1], &intIds[2], &intIds[3]) != 4)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        sscanf(line, " %d %d %d %d", &intIds[0], &intIds[1], &intIds[2], &intIds[3]);
        for (j = 0; j < 4; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        // cellId = output->InsertNextCell(VTK_TETRA, 4, nodeIds);
        // this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        this->InsertNextCellAndId(output, VTK_TETRA, 4, nodeIds, idx, cellType, i, numElements);
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] nodeIds;
      delete[] intIds;
    }
    else if (strncmp(line, "tetra10", 7) == 0)
    {
      vtkDebugMacro("tetra10");
      cellType = vtkPEnSightReader::TETRA10;

      nodeIds = new vtkIdType[10];
      intIds = new int[10];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d %d %d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2],
            &intIds[3], &intIds[4], &intIds[5], &intIds[6], &intIds[7], &intIds[8],
            &intIds[9]) != 10)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        sscanf(line, " %d %d %d %d %d %d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2],
          &intIds[3], &intIds[4], &intIds[5], &intIds[6], &intIds[7], &intIds[8], &intIds[9]);
        for (j = 0; j < 10; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        // cellId = output->InsertNextCell(VTK_QUADRATIC_TETRA, 10, nodeIds);
        // this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        this->InsertNextCellAndId(
          output, VTK_QUADRATIC_TETRA, 10, nodeIds, idx, cellType, i, numElements);
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] nodeIds;
      delete[] intIds;
    }
    else if (strncmp(line, "g_tetra4", 8) == 0 || strncmp(line, "g_tetra10", 9) == 0)
    {
      // skipping ghost cells
      if (strncmp(line, "g_tetra10", 9) == 0)
      {
        vtkDebugMacro("g_tetra10");
        cellType = vtkPEnSightReader::TETRA10;
      }
      else
      {
        vtkDebugMacro("g_tetra4");
        cellType = vtkPEnSightReader::TETRA4;
      }

      intIds = new int[4];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d", &intIds[0], &intIds[1], &intIds[2], &intIds[3]) != 4)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] intIds;
    }
    else if (strncmp(line, "pyramid5", 8) == 0)
    {
      vtkDebugMacro("pyramid5");
      cellType = vtkPEnSightReader::PYRAMID5;

      nodeIds = new vtkIdType[5];
      intIds = new int[5];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2], &intIds[3],
            &intIds[4]) != 5)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        sscanf(line, " %d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2], &intIds[3], &intIds[4]);
        for (j = 0; j < 5; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        // cellId = output->InsertNextCell(VTK_PYRAMID, 5, nodeIds);
        // this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        this->InsertNextCellAndId(output, VTK_PYRAMID, 5, nodeIds, idx, cellType, i, numElements);
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] nodeIds;
      delete[] intIds;
    }
    else if (strncmp(line, "pyramid13", 9) == 0)
    {
      vtkDebugMacro("pyramid13");
      cellType = vtkPEnSightReader::PYRAMID13;

      nodeIds = new vtkIdType[13];
      intIds = new int[13];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d %d %d %d %d %d %d %d %d %d", &intIds[0], &intIds[1],
            &intIds[2], &intIds[3], &intIds[4], &intIds[5], &intIds[6], &intIds[7], &intIds[8],
            &intIds[9], &intIds[10], &intIds[11], &intIds[12]) != 13)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        sscanf(line, " %d %d %d %d %d %d %d %d %d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2],
          &intIds[3], &intIds[4], &intIds[5], &intIds[6], &intIds[7], &intIds[8], &intIds[9],
          &intIds[10], &intIds[11], &intIds[12]);
        for (j = 0; j < 13; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        // cellId = output->InsertNextCell(VTK_QUADRATIC_PYRAMID, 13, nodeIds);
        // this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        this->InsertNextCellAndId(
          output, VTK_QUADRATIC_PYRAMID, 13, nodeIds, idx, cellType, i, numElements);
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] nodeIds;
      delete[] intIds;
    }
    else if (strncmp(line, "g_pyramid5", 10) == 0 || strncmp(line, "g_pyramid13", 11) == 0)
    {
      // skipping ghost cells
      if (strncmp(line, "g_pyramid13", 11) == 0)
      {
        vtkDebugMacro("g_pyramid13");
        cellType = vtkPEnSightReader::PYRAMID13;
      }
      else
      {
        vtkDebugMacro("g_pyramid5");
        cellType = vtkPEnSightReader::PYRAMID5;
      }

      intIds = new int[5];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2], &intIds[3],
            &intIds[4]) != 5)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] intIds;
    }
    else if (strncmp(line, "hexa8", 5) == 0)
    {
      vtkDebugMacro("hexa8");
      cellType = vtkPEnSightReader::HEXA8;

      nodeIds = new vtkIdType[8];
      intIds = new int[8];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2], &intIds[3],
            &intIds[4], &intIds[5], &intIds[6], &intIds[7]) != 8)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        sscanf(line, " %d %d %d %d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2], &intIds[3],
          &intIds[4], &intIds[5], &intIds[6], &intIds[7]);
        for (j = 0; j < 8; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        // cellId = output->InsertNextCell(VTK_HEXAHEDRON, 8, nodeIds);
        // this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        this->InsertNextCellAndId(
          output, VTK_HEXAHEDRON, 8, nodeIds, idx, cellType, i, numElements);
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] nodeIds;
      delete[] intIds;
    }
    else if (strncmp(line, "hexa20", 6) == 0)
    {
      vtkDebugMacro("hexa20");
      cellType = vtkPEnSightReader::HEXA20;

      nodeIds = new vtkIdType[20];
      intIds = new int[20];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", &intIds[0],
            &intIds[1], &intIds[2], &intIds[3], &intIds[4], &intIds[5], &intIds[6], &intIds[7],
            &intIds[8], &intIds[9], &intIds[10], &intIds[11], &intIds[12], &intIds[13], &intIds[14],
            &intIds[15], &intIds[16], &intIds[17], &intIds[18], &intIds[19]) != 20)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        sscanf(line, " %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", &intIds[0],
          &intIds[1], &intIds[2], &intIds[3], &intIds[4], &intIds[5], &intIds[6], &intIds[7],
          &intIds[8], &intIds[9], &intIds[10], &intIds[11], &intIds[12], &intIds[13], &intIds[14],
          &intIds[15], &intIds[16], &intIds[17], &intIds[18], &intIds[19]);
        for (j = 0; j < 20; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        // cellId = output->InsertNextCell(VTK_QUADRATIC_HEXAHEDRON, 20, nodeIds);
        // this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        this->InsertNextCellAndId(
          output, VTK_QUADRATIC_HEXAHEDRON, 20, nodeIds, idx, cellType, i, numElements);
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] nodeIds;
      delete[] intIds;
    }
    else if (strncmp(line, "g_hexa8", 7) == 0 || strncmp(line, "g_hexa20", 8) == 0)
    {
      // skipping ghost cells
      if (strncmp(line, "g_hexa20", 8) == 0)
      {
        vtkDebugMacro("g_hexa20");
        cellType = vtkPEnSightReader::HEXA20;
      }
      else
      {
        vtkDebugMacro("g_hexa8");
        cellType = vtkPEnSightReader::HEXA8;
      }

      intIds = new int[8];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2], &intIds[3],
            &intIds[4], &intIds[5], &intIds[6], &intIds[7]) != 8)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] intIds;
    }
    else if (strncmp(line, "penta6", 6) == 0)
    {
      vtkDebugMacro("penta6");
      cellType = vtkPEnSightReader::PENTA6;

      nodeIds = new vtkIdType[6];
      intIds = new int[6];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2], &intIds[3],
            &intIds[4], &intIds[5]) != 6)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        sscanf(line, " %d %d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2], &intIds[3],
          &intIds[4], &intIds[5]);
        for (j = 0; j < 6; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        // cellId = output->InsertNextCell(VTK_WEDGE, 6, nodeIds);
        // this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        this->InsertNextCellAndId(output, VTK_WEDGE, 6, nodeIds, idx, cellType, i, numElements);
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] nodeIds;
      delete[] intIds;
    }
    else if (strncmp(line, "penta15", 7) == 0)
    {
      vtkDebugMacro("penta15");
      cellType = vtkPEnSightReader::PENTA15;

      nodeIds = new vtkIdType[15];
      intIds = new int[15];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", &intIds[0], &intIds[1],
            &intIds[2], &intIds[3], &intIds[4], &intIds[5], &intIds[6], &intIds[7], &intIds[8],
            &intIds[9], &intIds[10], &intIds[11], &intIds[12], &intIds[13], &intIds[14]) != 15)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        sscanf(line, " %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", &intIds[0], &intIds[1],
          &intIds[2], &intIds[3], &intIds[4], &intIds[5], &intIds[6], &intIds[7], &intIds[8],
          &intIds[9], &intIds[10], &intIds[11], &intIds[12], &intIds[13], &intIds[14]);
        for (j = 0; j < 15; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        // cellId = output->InsertNextCell(VTK_QUADRATIC_WEDGE, 15, nodeIds);
        // this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        this->InsertNextCellAndId(
          output, VTK_QUADRATIC_WEDGE, 15, nodeIds, idx, cellType, i, numElements);
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] nodeIds;
      delete[] intIds;
    }
    else if (strncmp(line, "g_penta6", 8) == 0 || strncmp(line, "g_penta15", 9) == 0)
    {
      // skipping ghost cells
      if (strncmp(line, "g_penta15", 9) == 0)
      {
        vtkDebugMacro("g_penta15");
        cellType = vtkPEnSightReader::PENTA15;
      }
      else
      {
        vtkDebugMacro("g_penta6");
        cellType = vtkPEnSightReader::PENTA6;
      }

      intIds = new int[6];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2], &intIds[3],
            &intIds[4], &intIds[5]) != 6)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        lineRead = this->ReadNextDataLine(line);
      }
      delete[] intIds;
    }
    else if (strncmp(line, "END TIME STEP", 13) == 0)
    {
      // time to read coordinates at end if necessary
      if (this->CoordinatesAtEnd &&
        (this->InjectCoordinatesAtEnd(output, coordinatesOffset, idx) == -1))
        return -1;
      return 1;
    }
    else if (this->IS->fail())
    {
      // May want consistency check here?
      // vtkWarningMacro("EOF on geometry file");
      // time to read coordinates at end if necessary
      if (this->CoordinatesAtEnd &&
        (this->InjectCoordinatesAtEnd(output, coordinatesOffset, idx) == -1))
        return -1;
      return 1;
    }
    else
    {
      vtkErrorMacro("undefined geometry file line");
      return -1;
    }
  }

  // time to read coordinates at end if necessary
  if (this->CoordinatesAtEnd &&
    (this->InjectCoordinatesAtEnd(output, coordinatesOffset, idx) == -1))
    return -1;

  return lineRead;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldReader::CreateStructuredGridOutput(
  int partId, char line[256], const char* name, vtkMultiBlockDataSet* compositeOutput)
{
  char subLine[256];
  int lineRead;
  int iblanked = 0;
  int dimensions[3];
  int i;
  vtkPoints* points = vtkPoints::New();
  double point[3];
  int numPts;

  this->NumberOfNewOutputs++;

  vtkDataSet* ds = this->GetDataSetFromBlock(compositeOutput, partId);
  if (ds == nullptr || !ds->IsA("vtkStructuredGrid"))
  {
    vtkDebugMacro("creating new structured grid output");
    vtkStructuredGrid* sgrid = vtkStructuredGrid::New();
    this->AddToBlock(compositeOutput, partId, sgrid);
    sgrid->Delete();
    ds = sgrid;
  }
  if (this->StructuredPartIds->IsId(partId) == -1)
    this->StructuredPartIds->InsertNextId(partId);

  vtkStructuredGrid* output = vtkStructuredGrid::SafeDownCast(ds);

  this->SetBlockName(compositeOutput, partId, name);

  if (sscanf(line, " %*s %s", subLine) == 1)
  {
    if (strncmp(subLine, "iblanked", 8) == 0)
    {
      iblanked = 1;
    }
  }

  this->ReadNextDataLine(line);
  sscanf(line, " %d %d %d", &dimensions[0], &dimensions[1], &dimensions[2]);

  numPts = dimensions[0] * dimensions[1] * dimensions[2];

  int newDimensions[3];
  int splitDimension;
  int splitDimensionBeginIndex;
  vtkUnsignedCharArray* pointGhostArray = nullptr;
  vtkUnsignedCharArray* cellGhostArray = nullptr;
  if (this->GhostLevels == 0)
  {
    this->PrepareStructuredDimensionsForDistribution(partId, dimensions, newDimensions,
      &splitDimension, &splitDimensionBeginIndex, 0, nullptr, nullptr);
  }
  else
  {
    pointGhostArray = vtkUnsignedCharArray::New();
    pointGhostArray->SetName(vtkDataSetAttributes::GhostArrayName());
    cellGhostArray = vtkUnsignedCharArray::New();
    cellGhostArray->SetName(vtkDataSetAttributes::GhostArrayName());
    this->PrepareStructuredDimensionsForDistribution(partId, dimensions, newDimensions,
      &splitDimension, &splitDimensionBeginIndex, this->GhostLevels, pointGhostArray,
      cellGhostArray);
  }

  output->SetDimensions(newDimensions);
  //   output->SetWholeExtent(
  //                          0, newDimensions[0]-1, 0, newDimensions[1]-1, 0, newDimensions[2]-1);
  points->Allocate(this->GetPointIds(partId)->GetLocalNumberOfIds());

  for (i = 0; i < numPts; i++)
  {
    this->ReadNextDataLine(line);
    int realPointId = this->GetPointIds(partId)->GetId(i);
    if (realPointId != -1)
    {
      points->InsertNextPoint(atof(line), 0.0, 0.0);
    }
  }
  for (i = 0; i < numPts; i++)
  {
    this->ReadNextDataLine(line);
    int realPointId = this->GetPointIds(partId)->GetId(i);
    if (realPointId != -1)
    {
      points->GetPoint(realPointId, point);
      points->SetPoint(realPointId, point[0], atof(line), point[2]);
    }
  }
  for (i = 0; i < numPts; i++)
  {
    this->ReadNextDataLine(line);
    int realPointId = this->GetPointIds(partId)->GetId(i);
    if (realPointId != -1)
    {
      points->GetPoint(realPointId, point);
      points->SetPoint(realPointId, point[0], point[1], atof(line));
    }
  }

  output->SetPoints(points);

  if (iblanked)
  {
    for (i = 0; i < numPts; i++)
    {
      this->ReadNextDataLine(line);
      int realPointId = this->GetPointIds(partId)->GetId(i);
      if ((realPointId != -1) && (!atoi(line)))
        output->BlankPoint(realPointId);
    }
  }

  // Ghost level End
  if (this->GhostLevels > 0)
  {
    output->GetPointData()->AddArray(pointGhostArray);
    output->GetCellData()->AddArray(cellGhostArray);
  }

  points->Delete();

  // reading next line to check for EOF
  lineRead = this->ReadNextDataLine(line);
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldReader::CreateRectilinearGridOutput(
  int partId, char line[256], const char* name, vtkMultiBlockDataSet* compositeOutput)
{
  char subLine[256];
  int lineRead;
  int iblanked = 0;
  int dimensions[3];
  int i;
  vtkFloatArray* xCoords = vtkFloatArray::New();
  vtkFloatArray* yCoords = vtkFloatArray::New();
  vtkFloatArray* zCoords = vtkFloatArray::New();
  int numPts = 0;

  this->NumberOfNewOutputs++;

  vtkDataSet* ds = this->GetDataSetFromBlock(compositeOutput, partId);
  if (ds == nullptr || !ds->IsA("vtkRectilinearGrid"))
  {
    vtkDebugMacro("creating new structured grid output");
    vtkRectilinearGrid* rgrid = vtkRectilinearGrid::New();
    this->AddToBlock(compositeOutput, partId, rgrid);
    rgrid->Delete();

    ds = rgrid;
  }
  if (this->StructuredPartIds->IsId(partId) == -1)
    this->StructuredPartIds->InsertNextId(partId);

  vtkRectilinearGrid* output = vtkRectilinearGrid::SafeDownCast(ds);

  this->SetBlockName(compositeOutput, partId, name);

  if (sscanf(line, " %*s %*s %s", subLine) == 1)
  {
    if (strncmp(subLine, "iblanked", 8) == 0)
    {
      iblanked = 1;
    }
  }

  this->ReadNextDataLine(line);
  sscanf(line, " %d %d %d", &dimensions[0], &dimensions[1], &dimensions[2]);

  int newDimensions[3];
  int splitDimension;
  int splitDimensionBeginIndex;
  vtkUnsignedCharArray* pointGhostArray = nullptr;
  vtkUnsignedCharArray* cellGhostArray = nullptr;
  if (this->GhostLevels == 0)
  {
    this->PrepareStructuredDimensionsForDistribution(partId, dimensions, newDimensions,
      &splitDimension, &splitDimensionBeginIndex, 0, nullptr, nullptr);
  }
  else
  {
    pointGhostArray = vtkUnsignedCharArray::New();
    pointGhostArray->SetName(vtkDataSetAttributes::GhostArrayName());
    cellGhostArray = vtkUnsignedCharArray::New();
    cellGhostArray->SetName(vtkDataSetAttributes::GhostArrayName());
    this->PrepareStructuredDimensionsForDistribution(partId, dimensions, newDimensions,
      &splitDimension, &splitDimensionBeginIndex, this->GhostLevels, pointGhostArray,
      cellGhostArray);
  }

  output->SetDimensions(newDimensions);
  //   output->SetWholeExtent(
  //                          0, newDimensions[0]-1, 0, newDimensions[1]-1, 0, newDimensions[2]-1);
  xCoords->Allocate(newDimensions[0]);
  yCoords->Allocate(newDimensions[1]);
  zCoords->Allocate(newDimensions[2]);

  int beginDimension[3];

  beginDimension[splitDimension] = splitDimensionBeginIndex;
  beginDimension[(splitDimension + 1) % 3] = 0;
  beginDimension[(splitDimension + 2) % 3] = 0;

  float val;
  for (i = 0; i < dimensions[0]; i++)
  {
    this->ReadNextDataLine(line);
    if ((i >= beginDimension[0]) && (i < (beginDimension[0] + newDimensions[0])))
    {
      val = atof(line);
      xCoords->InsertNextTuple(&val);
    }
  }
  for (i = 0; i < dimensions[1]; i++)
  {
    this->ReadNextDataLine(line);
    if ((i >= beginDimension[1]) && (i < (beginDimension[1] + newDimensions[1])))
    {
      val = atof(line);
      yCoords->InsertNextTuple(&val);
    }
  }
  for (i = 0; i < dimensions[2]; i++)
  {
    this->ReadNextDataLine(line);
    if ((i >= beginDimension[2]) && (i < (beginDimension[2] + newDimensions[2])))
    {
      val = atof(line);
      zCoords->InsertNextTuple(&val);
    }
  }

  // Ghost level End
  if (this->GhostLevels > 0)
  {
    output->GetPointData()->AddArray(pointGhostArray);
    output->GetCellData()->AddArray(cellGhostArray);
  }

  if (iblanked)
  {
    vtkDebugMacro("VTK does not handle blanking for rectilinear grids.");
    for (i = 0; i < numPts; i++)
    {
      this->ReadNextDataLine(line);
    }
  }

  output->SetXCoordinates(xCoords);
  output->SetYCoordinates(yCoords);
  output->SetZCoordinates(zCoords);

  xCoords->Delete();
  yCoords->Delete();
  zCoords->Delete();

  // reading next line to check for EOF
  lineRead = this->ReadNextDataLine(line);
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldReader::CreateImageDataOutput(
  int partId, char line[256], const char* name, vtkMultiBlockDataSet* compositeOutput)
{
  char subLine[256];
  int lineRead;
  int iblanked = 0;
  int dimensions[3];
  int i;
  float origin[3], delta[3];
  int numPts;

  this->NumberOfNewOutputs++;

  vtkDataSet* ds = this->GetDataSetFromBlock(compositeOutput, partId);
  if (ds == nullptr || !ds->IsA("vtkImageData"))
  {
    vtkDebugMacro("creating new image data output");
    vtkImageData* idata = vtkImageData::New();
    this->AddToBlock(compositeOutput, partId, idata);
    idata->Delete();
    ds = idata;
  }
  if (this->StructuredPartIds->IsId(partId) == -1)
    this->StructuredPartIds->InsertNextId(partId);

  vtkImageData* output = vtkImageData::SafeDownCast(ds);

  this->SetBlockName(compositeOutput, partId, name);

  if (sscanf(line, " %*s %*s %s", subLine) == 1)
  {
    if (strncmp(subLine, "iblanked", 8) == 0)
    {
      iblanked = 1;
    }
  }

  this->ReadNextDataLine(line);
  sscanf(line, " %d %d %d", &dimensions[0], &dimensions[1], &dimensions[2]);

  int newDimensions[3];
  int splitDimension;
  int splitDimensionBeginIndex;
  vtkUnsignedCharArray* pointGhostArray = nullptr;
  vtkUnsignedCharArray* cellGhostArray = nullptr;
  if (this->GhostLevels == 0)
  {
    this->PrepareStructuredDimensionsForDistribution(partId, dimensions, newDimensions,
      &splitDimension, &splitDimensionBeginIndex, 0, nullptr, nullptr);
  }
  else
  {
    pointGhostArray = vtkUnsignedCharArray::New();
    pointGhostArray->SetName(vtkDataSetAttributes::GhostArrayName());
    cellGhostArray = vtkUnsignedCharArray::New();
    cellGhostArray->SetName(vtkDataSetAttributes::GhostArrayName());
    this->PrepareStructuredDimensionsForDistribution(partId, dimensions, newDimensions,
      &splitDimension, &splitDimensionBeginIndex, this->GhostLevels, pointGhostArray,
      cellGhostArray);
  }

  output->SetDimensions(newDimensions);
  //   output->SetWholeExtent(
  //                          0, newDimensions[0]-1, 0, newDimensions[1]-1, 0, newDimensions[2]-1);

  for (i = 0; i < 3; i++)
  {
    this->ReadNextDataLine(line);
    sscanf(line, " %f", &origin[i]);
  }
  for (i = 0; i < 3; i++)
  {
    this->ReadNextDataLine(line);
    sscanf(line, " %f", &delta[i]);
  }

  // Compute new origin
  float newOrigin[3];
  newOrigin[splitDimension] =
    origin[splitDimension] + ((float)splitDimensionBeginIndex) * delta[splitDimension];
  newOrigin[(splitDimension + 1) % 3] = origin[(splitDimension + 1) % 3];
  newOrigin[(splitDimension + 2) % 3] = origin[(splitDimension + 2) % 3];

  output->SetOrigin(newOrigin[0], newOrigin[1], newOrigin[2]);
  output->SetSpacing(delta[0], delta[1], delta[2]);

  // Ghost level End
  if (this->GhostLevels > 0)
  {
    output->GetPointData()->AddArray(pointGhostArray);
    output->GetCellData()->AddArray(cellGhostArray);
  }

  if (iblanked)
  {
    vtkDebugMacro("VTK does not handle blanking for image data.");
    numPts = dimensions[0] * dimensions[1] * dimensions[2];
    for (i = 0; i < numPts; i++)
    {
      this->ReadNextDataLine(line);
    }
  }

  // reading next line to check for EOF
  lineRead = this->ReadNextDataLine(line);
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldReader::CheckForUndefOrPartial(const char* line)
{
  char undefvar[16];
  // Look for keyword 'partial' or 'undef':
  int r = sscanf(line, "%*s %s", undefvar);
  if (r == 1)
  {
    char subline[80];
    if (strcmp(undefvar, "undef") == 0)
    {
      vtkDebugMacro("undef: " << line);
      this->ReadNextDataLine(subline);
      double val = atof(subline);
      switch (this->GetSectionType(line))
      {
        case vtkPEnSightReader::COORDINATES:
          this->UndefPartial->UndefCoordinates = val;
          break;
        case vtkPEnSightReader::BLOCK:
          this->UndefPartial->UndefBlock = val;
          break;
        case vtkPEnSightReader::ELEMENT:
          this->UndefPartial->UndefElementTypes = val;
          break;
        default:
          vtkErrorMacro(<< "Unknown section type: " << subline);
      }
      return 0; // meaning 'undef', so no other steps is necesserary
    }
    else if (strcmp(undefvar, "partial") == 0)
    {
      vtkDebugMacro("partial: " << line);
      this->ReadNextDataLine(subline);
      int nLines = atoi(subline);
      vtkIdType val;
      int i;
      switch (this->GetSectionType(line))
      {
        case vtkPEnSightReader::COORDINATES:
          for (i = 0; i < nLines; ++i)
          {
            this->ReadNextDataLine(subline);
            val = atoi(subline) - 1; // EnSight start at 1
            this->UndefPartial->PartialCoordinates.push_back(val);
          }
          break;
        case vtkPEnSightReader::BLOCK:
          for (i = 0; i < nLines; ++i)
          {
            this->ReadNextDataLine(subline);
            val = atoi(subline) - 1; // EnSight start at 1
            this->UndefPartial->PartialBlock.push_back(val);
          }
          break;
        case vtkPEnSightReader::ELEMENT:
          for (i = 0; i < nLines; ++i)
          {
            this->ReadNextDataLine(subline);
            val = atoi(subline) - 1; // EnSight start at 1
            this->UndefPartial->PartialElementTypes.push_back(val);
          }
          break;
        default:
          vtkErrorMacro(<< "Unknown section type: " << subline);
      }
      return 1; // meaning 'partial', so other steps are necesserary
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldReader::ReadOrSkipCoordinates(
  vtkPoints* points, long offset, int partId, int* lineRead, char line[256], bool skip)
{
  int numPts, i;
  char subLine[256];
  double point[3];

  if (offset == -1)
  {
    return 0;
  }

  // seek to file offset
  // In Ascii reader, IS is used instead of IFile
  this->IS->seekg(offset);

  this->ReadNextDataLine(line);
  numPts = atoi(line);

  if (skip)
  {
    vtkDebugMacro("SKIP. num. points: " << numPts);
    // Inject real Number of points, as we cannot take the size of the vector as a reference
    // numPts comes from the file, it cannot be wrong
    // Needed in nfaced...
    this->GetPointIds(partId)->SetNumberOfIds(numPts);

    // now "seek" to the end
    // there is no other way to do this, as we must skip DATA lines, and not juste lines.
    for (i = 0; i < (3 * numPts); i++)
    {
      this->ReadNextDataLine(line);
    }
    *lineRead = this->ReadNextDataLine(line);
    sscanf(line, " %s", subLine);
    char* endptr;
    double value = strtod(subLine, &endptr); // Testing if we can convert this string to double
    (void)value;                             // Prevent Warning to show up
    if (subLine != endptr)
    { // necessary if node ids were listed
      for (i = 0; i < numPts; i++)
      {
        *lineRead = this->ReadNextDataLine(line);
      }
    }

    return 0;
  }
  else
  {
    if (this->GetPointIds(partId)->GetNumberOfIds() == 0)
    {
      // No Point was injected at all For this Part. There is clearly a problem...
      // TODO: I should remove this part...
      for (i = 0; i < (3 * numPts); i++)
      {
        this->ReadNextDataLine(line);
      }
      *lineRead = this->ReadNextDataLine(line);
      sscanf(line, " %s", subLine);
      char* endptr;
      double value = strtod(subLine, &endptr); // Testing if we can convert this string to double
      (void)value;                             // Prevent Warning to show up
      if (subLine != endptr)
      { // necessary if node ids were listed
        for (i = 0; i < numPts; i++)
        {
          *lineRead = this->ReadNextDataLine(line);
        }
      }
      return 0;
    }
    else
    {
      // Inject really needed points
      int localNumberOfIds = this->GetPointIds(partId)->GetLocalNumberOfIds();
      points->Allocate(localNumberOfIds);
      points->SetNumberOfPoints(localNumberOfIds);

      for (i = 0; i < numPts; i++)
      {
        this->ReadNextDataLine(line);
        int id = this->GetPointIds(partId)->GetId(i);
        if (id != -1)
        {
          points->SetPoint(id, atof(line), 0, 0);
        }
      }
      for (i = 0; i < numPts; i++)
      {
        this->ReadNextDataLine(line);
        int id = this->GetPointIds(partId)->GetId(i);
        if (id != -1)
        {
          points->GetPoint(id, point);
          points->SetPoint(id, point[0], atof(line), 0);
        }
      }
      for (i = 0; i < numPts; i++)
      {
        this->ReadNextDataLine(line);
        int id = this->GetPointIds(partId)->GetId(i);
        if (id != -1)
        {
          points->GetPoint(id, point);
          points->SetPoint(id, point[0], point[1], atof(line));
        }
      }

      *lineRead = this->ReadNextDataLine(line);
      sscanf(line, " %s", subLine);

      char* endptr;
      double value = strtod(subLine, &endptr); // Testing if we can convert this string to double
      (void)value;                             // Prevent Warning to show up
      if (subLine != endptr)
      { // necessary if node ids were listed
        for (i = 0; i < numPts; i++)
        {
          int id = this->GetPointIds(partId)->GetId(i);
          if (id != -1)
          {
            points->GetPoint(id, point);
            points->SetPoint(id, point[1], point[2], atof(line));
          }
          *lineRead = this->ReadNextDataLine(line);
        }
      }
      this->GetPointIds(partId)->SetNumberOfIds(numPts);

      return localNumberOfIds;
    }
  }
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldReader::InjectCoordinatesAtEnd(
  vtkUnstructuredGrid* output, long coordinatesOffset, int partId)
{
  int fakeLineRead;
  char fakeLine[256];
  ios::iostate state;

  state = this->IS->rdstate();
  if (this->IS->fail() || this->IS->eof())
  {
    // we must read coordinates, so we clear error flags
    this->IS->clear();
  }

  long currentFilePosition = this->IS->tellg();
  vtkPoints* points = vtkPoints::New();
  int pointsRead =
    this->ReadOrSkipCoordinates(points, coordinatesOffset, partId, &fakeLineRead, fakeLine, false);
  this->IS->seekg(currentFilePosition);
  if (pointsRead == -1)
    return -1;
  output->SetPoints(points);
  points->Delete();
  this->CoordinatesAtEnd = false;

  // Inject Global Node Ids
  vtkPointData* pointData = output->GetPointData();
  vtkDataArray* globalNodeIds = this->GetPointIds(partId)->GenerateGlobalIdsArray("GlobalNodeId");
  pointData->SetGlobalIds(globalNodeIds);
  globalNodeIds->Delete();

  // We do not inject global Element Ids: It is not required for D3, for example,
  // and it consumes a lot of memory

  this->IS->setstate(state);

  return pointsRead;
}

//----------------------------------------------------------------------------
void vtkPEnSightGoldReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
