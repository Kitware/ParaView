#include "vtkPEnSightGoldBinaryReader.h"

#include "vtkByteSwap.h"
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
#include "vtkUnstructuredGrid.h"

#include <vtksys/SystemTools.hxx>

#include <ctype.h>
#include <string>

vtkStandardNewMacro(vtkPEnSightGoldBinaryReader);

// This is half the precision of an int.
#define MAXIMUM_PART_ID 65536

//----------------------------------------------------------------------------
vtkPEnSightGoldBinaryReader::vtkPEnSightGoldBinaryReader()
{
  this->IFile = NULL;
  this->FileSize = 0;
  this->Fortran = 0;
  this->NodeIdsListed = 0;
  this->ElementIdsListed = 0;

  this->FloatBufferSize = 1000;

  this->FloatBuffer = (float**)malloc(3 * sizeof(float*));
  this->FloatBuffer[0] = new float[this->FloatBufferSize];
  this->FloatBuffer[1] = new float[this->FloatBufferSize];
  this->FloatBuffer[2] = new float[this->FloatBufferSize];
  this->FloatBufferIndexBegin = -1;
  this->FloatBufferFilePosition = 0;
  this->FloatBufferNumberOfVectors = 0;
}

//----------------------------------------------------------------------------
vtkPEnSightGoldBinaryReader::~vtkPEnSightGoldBinaryReader()
{
  if (this->IFile)
  {
    this->IFile->close();
    delete this->IFile;
    this->IFile = NULL;
  }
  delete[] this->FloatBuffer[2];
  delete[] this->FloatBuffer[1];
  delete[] this->FloatBuffer[0];
  free(this->FloatBuffer);
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldBinaryReader::OpenFile(const char* filename)
{
  if (!filename)
  {
    vtkErrorMacro(<< "Missing filename.");
    return 0;
  }

  // Close file from any previous image
  if (this->IFile)
  {
    this->IFile->close();
    delete this->IFile;
    this->IFile = NULL;
  }

  // Open the new file
  vtkDebugMacro(<< "Opening file " << filename);
  vtksys::SystemTools::Stat_t fs;
  if (!vtksys::SystemTools::Stat(filename, &fs))
  {
    // Find out how big the file is.
    this->FileSize = (long)(fs.st_size);

#ifdef _WIN32
    this->IFile = new ifstream(filename, ios::in | ios::binary);
#else
    this->IFile = new ifstream(filename, ios::in);
#endif
  }
  else
  {
    vtkErrorMacro("stat failed.");
    return 0;
  }
  if (!this->IFile || this->IFile->fail())
  {
    vtkErrorMacro(<< "Could not open file " << filename);
    return 0;
  }

  // we now need to check for Fortran and byte ordering

  // we need to look at the first 4 bytes of the file, and the 84-87 bytes
  // of the file to correctly determine what it is. If we only check the first
  // 4 bytes we can get incorrect detection if it is a property file named "P"
  // we check the 84-87 bytes as that is the start of the next line on a fortran file

  char result[88];
  this->IFile->read(result, 88);
  if (this->IFile->eof() || this->IFile->fail())
  {
    vtkErrorMacro(<< filename << " is missing header information");
    return 0;
  }
  this->IFile->seekg(0, ios::beg); // reset the file to the start

  // if the first 4 bytes is the length, then this data is no doubt
  // a fortran data write!, copy the last 76 into the beginning
  char le_len[4] = { 0x50, 0x00, 0x00, 0x00 };
  char be_len[4] = { 0x00, 0x00, 0x00, 0x50 };

  // the fortran test here depends on the byte ordering. But if the user didn't
  // set any byte ordering then, we have to try both byte orderings. There was a
  // bug here which was resulting in binary-fortran-big-endian files being read
  // incorrectly on intel machines (BUG #10593). This dual-check avoids that
  // bug.
  bool le_isFortran = true;
  bool be_isFortran = true;
  for (int c = 0; c < 4; c++)
  {
    le_isFortran = le_isFortran && (result[c] == le_len[c]) && (result[c + 84] == le_len[c]);
    be_isFortran = be_isFortran && (result[c] == be_len[c]) && (result[c + 84] == be_len[c]);
  }

  switch (this->ByteOrder)
  {
    case FILE_BIG_ENDIAN:
      this->Fortran = be_isFortran;
      break;

    case FILE_LITTLE_ENDIAN:
      this->Fortran = le_isFortran;
      break;

    case FILE_UNKNOWN_ENDIAN:
      if (le_isFortran)
      {
        this->Fortran = true;
        this->ByteOrder = FILE_LITTLE_ENDIAN;
      }
      else if (be_isFortran)
      {
        this->Fortran = true;
        this->ByteOrder = FILE_BIG_ENDIAN;
      }
      else
      {
        this->Fortran = false;
      }
      break;
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldBinaryReader::InitializeFile(const char* fileName)
{
  char line[80], subLine[80];

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

  if (this->OpenFile(sfilename.c_str()) == 0)
  {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    return 0;
  }

  line[0] = '\0';
  subLine[0] = '\0';
  if (this->ReadLine(line) == 0)
  {
    vtkErrorMacro("Error with line reading upon file initialization");
    return 0;
  }

  if (sscanf(line, " %*s %s", subLine) != 1)
  {
    vtkErrorMacro("Error with subline extraction upon file initialization");
    return 0;
  }

  if (strncmp(subLine, "Binary", 6) != 0 && strncmp(subLine, "binary", 6) != 0)
  {
    vtkErrorMacro("This is not a binary data set. Try "
      << "vtkEnSightGoldReader.");
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldBinaryReader::ReadGeometryFile(
  const char* fileName, int timeStep, vtkMultiBlockDataSet* output)
{
  char line[80], subLine[80], nameline[80];
  int partId, realId;
  int lineRead, i;

  if (!this->InitializeFile(fileName))
  {
    return 0;
  }

  /* Disable this. This is too slow on big files and the CASE file
   * is supposed to be correct anyway...
   //this will close the file, so we need to reinitialize it
   int numberOfTimeStepsInFile=this->CountTimeSteps();

   if (!this->InitializeFile(fileName))
   {
   return 0;
   }
  */

  if (this->UseFileSets)
  {
    int realTimeStep = timeStep - 1;
    int j = 0;
    // Try to find the nearest time step for which we know the offset
    for (i = realTimeStep; i >= 0; i--)
    {
      if (this->FileOffsets.find(fileName) != this->FileOffsets.end() &&
        this->FileOffsets[fileName].find(i) != this->FileOffsets[fileName].end())
      {
        this->IFile->seekg(this->FileOffsets[fileName][i], ios::beg);
        j = i;
        break;
      }
    }

    // Hopefully we are not very far from the timestep we want to use
    // Find it (and cache any timestep we find on the way...)
    while (j++ < realTimeStep)
    {
      if (!this->SkipTimeStep())
      {
        return 0;
      }
      else
      {
        if (this->FileOffsets.find(fileName) == this->FileOffsets.end())
        {
          std::map<int, long> tsMap;
          this->FileOffsets[fileName] = tsMap;
        }
        this->FileOffsets[fileName][j] = this->IFile->tellg();
      }
    }

    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadLine(line);
    }
  }

  // Skip the 2 description lines.
  this->ReadLine(line);
  this->ReadLine(line);

  // Read the node id and element id lines.
  this->ReadLine(line);
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

  this->ReadLine(line);
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

  lineRead = this->ReadLine(line); // "extents" or "part"
  if (strncmp(line, "extents", 7) == 0)
  {
    // Skipping the extents.
    this->IFile->seekg(6 * sizeof(float), ios::cur);
    lineRead = this->ReadLine(line); // "part"
  }

  while (lineRead > 0 && strncmp(line, "part", 4) == 0)
  {
    this->ReadPartId(&partId);
    partId--; // EnSight starts #ing at 1.
    if (partId < 0 || partId >= MAXIMUM_PART_ID)
    {
      vtkErrorMacro("Invalid part id; check that ByteOrder is set correctly.");
      return 0;
    }
    realId = this->InsertNewPartId(partId);

    // Increment the number of geometry parts such that the measured geometry,
    // if any, can be properly combined into a vtkMultiBlockDataSet object.
    // --- fix to bug #7453
    this->NumberOfGeometryParts++;

    this->ReadLine(line); // part description line

    strncpy(nameline, line, 80); // 80 characters in line are allowed
    nameline[79] = '\0';         // Ensure NULL character at end of part name
    char* name = strdup(nameline);

    // fix to bug #0008237
    // The original "return 1" operation upon "strncmp(line, "interface", 9) == 0"
    // was removed here as 'interface' is NOT a keyword of an EnSight Gold file.

    this->ReadLine(line);

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
        if (this->IFile)
        {
          this->IFile->close();
          delete this->IFile;
          this->IFile = NULL;
        }
        return 0;
      }
    }
    free(name);
  }

  if (this->IFile)
  {
    this->IFile->close();
    delete this->IFile;
    this->IFile = NULL;
  }
  if (lineRead < 0)
  {
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldBinaryReader::CountTimeSteps()
{
  int count = 0;
  while (1)
  {
    int result = this->SkipTimeStep();
    if (result)
    {
      count++;
    }
    else
    {
      break;
    }
  }
  return count;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldBinaryReader::SkipTimeStep()
{
  char line[80], subLine[80];
  int lineRead;

  line[0] = '\0';
  while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
  {
    if (!this->ReadLine(line))
    {
      return 0;
    }
  }

  // Skip the 2 description lines.
  this->ReadLine(line);
  this->ReadLine(line);

  // Read the node id and element id lines.
  this->ReadLine(line);
  sscanf(line, " %*s %*s %s", subLine);
  if (strncmp(subLine, "given", 5) == 0 || strncmp(subLine, "ignore", 6) == 0)
  {
    this->NodeIdsListed = 1;
  }
  else
  {
    this->NodeIdsListed = 0;
  }

  this->ReadLine(line);
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

  lineRead = this->ReadLine(line); // "extents" or "part"
  if (strncmp(line, "extents", 7) == 0)
  {
    // Skipping the extents.
    this->IFile->seekg(6 * sizeof(float), ios::cur);
    lineRead = this->ReadLine(line); // "part"
  }

  while (lineRead > 0 && strncmp(line, "part", 4) == 0)
  {
    int tmpInt;
    this->ReadPartId(&tmpInt);
    if (tmpInt < 0 || tmpInt > MAXIMUM_PART_ID)
    {
      vtkErrorMacro("Invalid part id; check that ByteOrder is set correctly.");
      return 0;
    }
    this->ReadLine(line); // part description line
    this->ReadLine(line);

    if (strncmp(line, "block", 5) == 0)
    {
      if (sscanf(line, " %*s %s", subLine) == 1)
      {
        if (strncmp(subLine, "rectilinear", 11) == 0)
        {
          // block rectilinear
          lineRead = this->SkipRectilinearGrid(line);
        }
        else if (strncmp(subLine, "uniform,", 7) == 0)
        {
          // block uniform
          lineRead = this->SkipImageData(line);
        }
        else
        {
          // block iblanked
          lineRead = this->SkipStructuredGrid(line);
        }
      }
      else
      {
        // block
        lineRead = this->SkipStructuredGrid(line);
      }
    }
    else
    {
      lineRead = this->SkipUnstructuredGrid(line);
    }
  }

  if (lineRead < 0)
  {
    if (this->IFile)
    {
      this->IFile->close();
      delete this->IFile;
      this->IFile = NULL;
    }
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldBinaryReader::SkipStructuredGrid(char line[256])
{
  char subLine[80];
  int lineRead;
  int iblanked = 0;
  int dimensions[3];
  int numPts;

  if (sscanf(line, " %*s %s", subLine) == 1)
  {
    if (strncmp(subLine, "iblanked", 8) == 0)
    {
      iblanked = 1;
    }
  }

  this->ReadIntArray(dimensions, 3);
  numPts = dimensions[0] * dimensions[1] * dimensions[2];
  if (dimensions[0] < 0 || dimensions[0] * (int)sizeof(int) > this->FileSize ||
    dimensions[0] > this->FileSize || dimensions[1] < 0 ||
    dimensions[1] * (int)sizeof(int) > this->FileSize || dimensions[1] > this->FileSize ||
    dimensions[2] < 0 || dimensions[2] * (int)sizeof(int) > this->FileSize ||
    dimensions[2] > this->FileSize || numPts < 0 || numPts * (int)sizeof(int) > this->FileSize ||
    numPts > this->FileSize)
  {
    vtkErrorMacro("Invalid dimensions read; check that ByteOrder is set correctly.");
    return -1;
  }

  // Skip xCoords, yCoords and zCoords.
  this->IFile->seekg(sizeof(float) * numPts * 3, ios::cur);

  if (iblanked)
  { // skip iblank array.
    this->IFile->seekg(numPts * sizeof(int), ios::cur);
  }

  // reading next line to check for EOF
  lineRead = this->ReadLine(line);
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldBinaryReader::SkipUnstructuredGrid(char line[256])
{
  int lineRead = 1;
  int i;
  int numElements;
  int cellType;

  while (lineRead && strncmp(line, "part", 4) != 0)
  {
    if (strncmp(line, "coordinates", 11) == 0)
    {
      vtkDebugMacro("coordinates");
      int numPts;

      this->ReadInt(&numPts);
      if (numPts < 0 || numPts * (int)sizeof(int) > this->FileSize || numPts > this->FileSize)
      {
        vtkErrorMacro("Invalid number of points; check that ByteOrder is set correctly.");
        return -1;
      }

      vtkDebugMacro("num. points: " << numPts);

      if (this->NodeIdsListed)
      { // skip node ids.
        this->IFile->seekg(sizeof(int) * numPts, ios::cur);
      }

      // Skip xCoords, yCoords and zCoords.
      this->IFile->seekg(sizeof(float) * 3 * numPts, ios::cur);
    }
    else if (strncmp(line, "point", 5) == 0 || strncmp(line, "g_point", 7) == 0)
    {
      vtkDebugMacro("point");

      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of point cells; check that ByteOrder is set correctly.");
        return -1;
      }
      if (this->ElementIdsListed)
      { // skip element ids.
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      // Skip nodeIdList.
      this->IFile->seekg(sizeof(int) * numElements, ios::cur);
    }
    else if (strncmp(line, "bar2", 4) == 0 || strncmp(line, "g_bar2", 6) == 0)
    {
      vtkDebugMacro("bar2");

      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of bar2 cells; check that ByteOrder is set correctly.");
        return -1;
      }
      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      // Skip nodeIdList.
      this->IFile->seekg(sizeof(int) * 2 * numElements, ios::cur);
    }
    else if (strncmp(line, "bar3", 4) == 0 || strncmp(line, "g_bar3", 6) == 0)
    {
      vtkDebugMacro("bar3");
      vtkWarningMacro("Only vertex nodes of this element will be read.");

      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of bar3 cells; check that ByteOrder is set correctly.");
        return -1;
      }

      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      // Skip nodeIdList.
      this->IFile->seekg(sizeof(int) * 2 * numElements, ios::cur);
    }
    else if (strncmp(line, "nsided", 6) == 0 || strncmp(line, "g_nsided", 8) == 0)
    {
      vtkDebugMacro("nsided");
      int* numNodesPerElement;
      int numNodes = 0;

      // cellType = vtkPEnSightReader::NSIDED;
      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of nsided cells; check that ByteOrder is set correctly.");
        return -1;
      }

      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      numNodesPerElement = new int[numElements];
      this->ReadIntArray(numNodesPerElement, numElements);
      for (i = 0; i < numElements; i++)
      {
        numNodes += numNodesPerElement[i];
      }
      // Skip nodeIdList.
      this->IFile->seekg(sizeof(int) * numNodes, ios::cur);
      delete[] numNodesPerElement;
    }
    else if (strncmp(line, "tria3", 5) == 0 || strncmp(line, "tria6", 5) == 0 ||
      strncmp(line, "g_tria3", 7) == 0 || strncmp(line, "g_tria6", 7) == 0)
    {
      if (strncmp(line, "tria6", 5) == 0 || strncmp(line, "g_tria6", 7) == 0)
      {
        vtkDebugMacro("tria6");
        vtkWarningMacro("Only vertex nodes of this element will be read.");
        cellType = vtkPEnSightReader::TRIA6;
      }
      else
      {
        vtkDebugMacro("tria3");
        cellType = vtkPEnSightReader::TRIA3;
      }

      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of triangle cells; check that ByteOrder is set correctly.");
        return -1;
      }
      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      if (cellType == vtkPEnSightReader::TRIA6)
      {
        // Skip nodeIdList.
        this->IFile->seekg(sizeof(int) * 6 * numElements, ios::cur);
      }
      else
      {
        // Skip nodeIdList.
        this->IFile->seekg(sizeof(int) * 3 * numElements, ios::cur);
      }
    }
    else if (strncmp(line, "quad4", 5) == 0 || strncmp(line, "quad8", 5) == 0 ||
      strncmp(line, "g_quad4", 7) == 0 || strncmp(line, "g_quad8", 7) == 0)
    {
      if (strncmp(line, "quad8", 5) == 0 || strncmp(line, "g_quad8", 7) == 0)
      {
        vtkDebugMacro("quad8");
        vtkWarningMacro("Only vertex nodes of this element will be read.");
        cellType = vtkPEnSightReader::QUAD8;
      }
      else
      {
        vtkDebugMacro("quad4");
        cellType = vtkPEnSightReader::QUAD4;
      }

      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of quad cells; check that ByteOrder is set correctly.");
        return -1;
      }
      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      if (cellType == vtkPEnSightReader::QUAD8)
      {
        // Skip nodeIdList.
        this->IFile->seekg(sizeof(int) * 8 * numElements, ios::cur);
      }
      else
      {
        // Skip nodeIdList.
        this->IFile->seekg(sizeof(int) * 4 * numElements, ios::cur);
      }
    }
    else if (strncmp(line, "nfaced", 6) == 0)
    {
      vtkDebugMacro("nfaced");
      int* numFacesPerElement;
      int* numNodesPerFace;
      int numFaces = 0;
      int numNodes = 0;

      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of nfaced cells; check that ByteOrder is set correctly.");
        return -1;
      }

      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      numFacesPerElement = new int[numElements];
      this->ReadIntArray(numFacesPerElement, numElements);
      for (i = 0; i < numElements; i++)
      {
        numFaces += numFacesPerElement[i];
      }
      delete[] numFacesPerElement;
      numNodesPerFace = new int[numFaces];
      this->ReadIntArray(numNodesPerFace, numFaces);
      for (i = 0; i < numFaces; i++)
      {
        numNodes += numNodesPerFace[i];
      }
      // Skip nodeIdList.
      this->IFile->seekg(sizeof(int) * numNodes, ios::cur);
      delete[] numNodesPerFace;
    }
    else if (strncmp(line, "tetra4", 6) == 0 || strncmp(line, "tetra10", 7) == 0 ||
      strncmp(line, "g_tetra4", 8) == 0 || strncmp(line, "g_tetra10", 9) == 0)
    {
      if (strncmp(line, "tetra10", 7) == 0 || strncmp(line, "g_tetra10", 9) == 0)
      {
        vtkDebugMacro("tetra10");
        vtkWarningMacro("Only vertex nodes of this element will be read.");
        cellType = vtkPEnSightReader::TETRA10;
      }
      else
      {
        vtkDebugMacro("tetra4");
        cellType = vtkPEnSightReader::TETRA4;
      }

      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro(
          "Invalid number of tetrahedral cells; check that ByteOrder is set correctly.");
        return -1;
      }
      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      if (cellType == vtkPEnSightReader::TETRA10)
      {
        // Skip nodeIdList.
        this->IFile->seekg(sizeof(int) * 10 * numElements, ios::cur);
      }
      else
      {
        // Skip nodeIdList.
        this->IFile->seekg(sizeof(int) * 4 * numElements, ios::cur);
      }
    }
    else if (strncmp(line, "pyramid5", 8) == 0 || strncmp(line, "pyramid13", 9) == 0 ||
      strncmp(line, "g_pyramid5", 10) == 0 || strncmp(line, "g_pyramid13", 11) == 0)
    {
      if (strncmp(line, "pyramid13", 9) == 0 || strncmp(line, "g_pyramid13", 11) == 0)
      {
        vtkDebugMacro("pyramid13");
        vtkWarningMacro("Only vertex nodes of this element will be read.");
        cellType = vtkPEnSightReader::PYRAMID13;
      }
      else
      {
        vtkDebugMacro("pyramid5");
        cellType = vtkPEnSightReader::PYRAMID5;
      }

      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of pyramid cells; check that ByteOrder is set correctly.");
        return -1;
      }
      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      if (cellType == vtkPEnSightReader::PYRAMID13)
      {
        // Skip nodeIdList.
        this->IFile->seekg(sizeof(int) * 13 * numElements, ios::cur);
      }
      else
      {
        // Skip nodeIdList.
        this->IFile->seekg(sizeof(int) * 5 * numElements, ios::cur);
      }
    }
    else if (strncmp(line, "hexa8", 5) == 0 || strncmp(line, "hexa20", 6) == 0 ||
      strncmp(line, "g_hexa8", 7) == 0 || strncmp(line, "g_hexa20", 8) == 0)
    {
      if (strncmp(line, "hexa20", 6) == 0 || strncmp(line, "g_hexa20", 8) == 0)
      {
        vtkDebugMacro("hexa20");
        vtkWarningMacro("Only vertex nodes of this element will be read.");
        cellType = vtkPEnSightReader::HEXA20;
      }
      else
      {
        vtkDebugMacro("hexa8");
        cellType = vtkPEnSightReader::HEXA8;
      }

      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of hexahedral cells; check that ByteOrder is set correctly.");
        return -1;
      }
      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      if (cellType == vtkPEnSightReader::HEXA20)
      {
        // Skip nodeIdList.
        this->IFile->seekg(sizeof(int) * 20 * numElements, ios::cur);
      }
      else
      {
        // Skip nodeIdList.
        this->IFile->seekg(sizeof(int) * 8 * numElements, ios::cur);
      }
    }
    else if (strncmp(line, "penta6", 6) == 0 || strncmp(line, "penta15", 7) == 0 ||
      strncmp(line, "g_penta6", 8) == 0 || strncmp(line, "g_penta15", 9) == 0)
    {
      if (strncmp(line, "penta15", 7) == 0 || strncmp(line, "g_penta15", 9) == 0)
      {
        vtkDebugMacro("penta15");
        vtkWarningMacro("Only vertex nodes of this element will be read.");
        cellType = vtkPEnSightReader::PENTA15;
      }
      else
      {
        vtkDebugMacro("penta6");
        cellType = vtkPEnSightReader::PENTA6;
      }

      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of pentagonal cells; check that ByteOrder is set correctly.");
        return -1;
      }
      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      if (cellType == vtkPEnSightReader::PENTA15)
      {
        // Skip nodeIdList.
        this->IFile->seekg(sizeof(int) * 15 * numElements, ios::cur);
      }
      else
      {
        // Skip nodeIdList.
        this->IFile->seekg(sizeof(int) * 6 * numElements, ios::cur);
      }
    }
    else if (strncmp(line, "END TIME STEP", 13) == 0)
    {
      return 1;
    }
    else
    {
      vtkErrorMacro("undefined geometry file line");
      return -1;
    }
    lineRead = this->ReadLine(line);
  }
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldBinaryReader::SkipRectilinearGrid(char line[256])
{
  char subLine[80];
  int lineRead;
  int iblanked = 0;
  int dimensions[3];
  int numPts;

  if (sscanf(line, " %*s %*s %s", subLine) == 1)
  {
    if (strncmp(subLine, "iblanked", 8) == 0)
    {
      iblanked = 1;
    }
  }

  this->ReadIntArray(dimensions, 3);
  if (dimensions[0] < 0 || dimensions[0] * (int)sizeof(int) > this->FileSize ||
    dimensions[0] > this->FileSize || dimensions[1] < 0 ||
    dimensions[1] * (int)sizeof(int) > this->FileSize || dimensions[1] > this->FileSize ||
    dimensions[2] < 0 || dimensions[2] * (int)sizeof(int) > this->FileSize ||
    dimensions[2] > this->FileSize || (dimensions[0] + dimensions[1] + dimensions[2]) < 0 ||
    (dimensions[0] + dimensions[1] + dimensions[2]) * (int)sizeof(int) > this->FileSize ||
    (dimensions[0] + dimensions[1] + dimensions[2]) > this->FileSize)
  {
    vtkErrorMacro("Invalid dimensions read; check that BytetOrder is set correctly.");
    return -1;
  }

  numPts = dimensions[0] * dimensions[1] * dimensions[2];

  // Skip xCoords
  this->IFile->seekg(sizeof(float) * dimensions[0], ios::cur);
  // Skip yCoords
  this->IFile->seekg(sizeof(float) * dimensions[1], ios::cur);
  // Skip zCoords
  this->IFile->seekg(sizeof(float) * dimensions[2], ios::cur);

  if (iblanked)
  {
    vtkWarningMacro("VTK does not handle blanking for rectilinear grids.");
    this->IFile->seekg(sizeof(int) * numPts, ios::cur);
  }

  // reading next line to check for EOF
  lineRead = this->ReadLine(line);
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldBinaryReader::SkipImageData(char line[256])
{
  char subLine[80];
  int lineRead;
  int iblanked = 0;
  int dimensions[3];
  float origin[3], delta[3];
  int numPts;

  if (sscanf(line, " %*s %*s %s", subLine) == 1)
  {
    if (strncmp(subLine, "iblanked", 8) == 0)
    {
      iblanked = 1;
    }
  }

  this->ReadIntArray(dimensions, 3);
  this->ReadFloatArray(origin, 3);
  this->ReadFloatArray(delta, 3);

  if (iblanked)
  {
    vtkWarningMacro("VTK does not handle blanking for image data.");
    numPts = dimensions[0] * dimensions[1] * dimensions[2];
    if (dimensions[0] < 0 || dimensions[0] * (int)sizeof(int) > this->FileSize ||
      dimensions[0] > this->FileSize || dimensions[1] < 0 ||
      dimensions[1] * (int)sizeof(int) > this->FileSize || dimensions[1] > this->FileSize ||
      dimensions[2] < 0 || dimensions[2] * (int)sizeof(int) > this->FileSize ||
      dimensions[2] > this->FileSize || numPts < 0 || numPts * (int)sizeof(int) > this->FileSize ||
      numPts > this->FileSize)
    {
      return -1;
    }
    this->IFile->seekg(sizeof(int) * numPts, ios::cur);
  }

  // reading next line to check for EOF
  lineRead = this->ReadLine(line);
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldBinaryReader::ReadMeasuredGeometryFile(
  const char* fileName, int timeStep, vtkMultiBlockDataSet* output)
{
  char line[80], subLine[80];
  vtkIdType i;
  int* pointIds;
  float *xCoords, *yCoords, *zCoords;
  vtkPoints* points = vtkPoints::New();
  vtkPolyData* pd = vtkPolyData::New();

  this->NumberOfNewOutputs++;

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

  if (this->OpenFile(sfilename.c_str()) == 0)
  {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    return 0;
  }

  this->ReadLine(line);
  sscanf(line, " %*s %s", subLine);
  if (strncmp(subLine, "Binary", 6) != 0)
  {
    vtkErrorMacro("This is not a binary data set. Try "
      << "vtkEnSightGoldReader.");
    return 0;
  }

  if (this->UseFileSets)
  {
    int realTimeStep = timeStep - 1;
    int k, j = 0;
    // Try to find the nearest time step for which we know the offset
    for (k = realTimeStep; k >= 0; k--)
    {
      if (this->FileOffsets.find(fileName) != this->FileOffsets.end() &&
        this->FileOffsets[fileName].find(k) != this->FileOffsets[fileName].end())
      {
        this->IFile->seekg(this->FileOffsets[fileName][k], ios::beg);
        j = k;
        break;
      }
    }

    // Hopefully we are not very far from the timestep we want to use
    // Find it (and cache any timestep we find on the way...)
    while (j++ < realTimeStep)
    {
      while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
      {
        this->ReadLine(line);
      }
      // Skip the description line.
      this->ReadLine(line);

      this->ReadLine(line); // "particle coordinates"

      this->ReadInt(&this->NumberOfMeasuredPoints);

      // Skip pointIds
      // this->IFile->ignore(sizeof(int)*this->NumberOfMeasuredPoints);
      // Skip xCoords
      // this->IFile->ignore(sizeof(float)*this->NumberOfMeasuredPoints);
      // Skip yCoords
      // this->IFile->ignore(sizeof(float)*this->NumberOfMeasuredPoints);
      // Skip zCoords
      // this->IFile->ignore(sizeof(float)*this->NumberOfMeasuredPoints);
      this->IFile->seekg(
        (sizeof(float) * 3 + sizeof(int)) * this->NumberOfMeasuredPoints, ios::cur);
      this->ReadLine(line); // END TIME STEP
      if (this->FileOffsets.find(fileName) == this->FileOffsets.end())
      {
        std::map<int, long> tsMap;
        this->FileOffsets[fileName] = tsMap;
      }
      this->FileOffsets[fileName][j] = this->IFile->tellg();
    }
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadLine(line);
    }
  }

  // Skip the description line.
  this->ReadLine(line);

  this->ReadLine(line); // "particle coordinates"

  this->ReadInt(&this->NumberOfMeasuredPoints);

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
  this->PrepareStructuredDimensionsForDistribution(
    partId, dimensions, newDimensions, &splitDimension, &splitDimensionBeginIndex, 0, NULL, NULL);

  pointIds = new int[this->NumberOfMeasuredPoints];
  xCoords = new float[this->NumberOfMeasuredPoints];
  yCoords = new float[this->NumberOfMeasuredPoints];
  zCoords = new float[this->NumberOfMeasuredPoints];

  points->Allocate(this->GetPointIds(partId)->GetLocalNumberOfIds());
  pd->Allocate(this->GetPointIds(partId)->GetLocalNumberOfIds());

  // Extract the array of point indices. Note EnSight Manual v8.2 (pp. 559,
  // http://www-vis.lbl.gov/NERSC/Software/ensight/docs82/UserManual.pdf)
  // is wrong in describing the format of binary measured geometry files.
  // As opposed to this description, the actual format employs a 'hybrid'
  // storage scheme. Specifically, point indices are stored in an array,
  // whereas 3D coordinates follow the array in a tuple-by-tuple manner.
  // The following code segment (20+ lines) serves as a fix to bug #9245.
  this->ReadIntArray(pointIds, this->NumberOfMeasuredPoints);

  // Read point coordinates tuple by tuple while each tuple contains three
  // components: (x-cord, y-cord, z-cord)
  int floatSize = sizeof(float);
  for (i = 0; i < this->NumberOfMeasuredPoints; i++)
  {
    this->IFile->read((char*)(xCoords + i), floatSize);
    this->IFile->read((char*)(yCoords + i), floatSize);
    this->IFile->read((char*)(zCoords + i), floatSize);
  }

  if (this->ByteOrder == FILE_LITTLE_ENDIAN)
  {
    vtkByteSwap::Swap4LERange(xCoords, this->NumberOfMeasuredPoints);
    vtkByteSwap::Swap4LERange(yCoords, this->NumberOfMeasuredPoints);
    vtkByteSwap::Swap4LERange(zCoords, this->NumberOfMeasuredPoints);
  }
  else
  {
    vtkByteSwap::Swap4BERange(xCoords, this->NumberOfMeasuredPoints);
    vtkByteSwap::Swap4BERange(yCoords, this->NumberOfMeasuredPoints);
    vtkByteSwap::Swap4BERange(zCoords, this->NumberOfMeasuredPoints);
  }

  for (i = 0; i < this->NumberOfMeasuredPoints; i++)
  {
    int realId = this->GetPointIds(partId)->GetId(i);
    if (realId != -1)
    {
      vtkIdType tempId = realId;
      points->InsertNextPoint(xCoords[i], yCoords[i], zCoords[i]);
      pd->InsertNextCell(VTK_VERTEX, 1, &tempId);
    }
  }

  pd->SetPoints(points);
  this->AddToBlock(output, partId, pd);

  points->Delete();
  pd->Delete();
  delete[] pointIds;
  delete[] xCoords;
  delete[] yCoords;
  delete[] zCoords;

  if (this->IFile)
  {
    this->IFile->close();
    delete this->IFile;
    this->IFile = NULL;
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldBinaryReader::ReadScalarsPerNode(const char* fileName, const char* description,
  int timeStep, vtkMultiBlockDataSet* compositeOutput, int measured, int numberOfComponents,
  int component)
{
  char line[80];
  int partId, realId, numPts, i, lineRead;
  vtkFloatArray* scalars;
  float* scalarsRead;
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

  if (this->OpenFile(sfilename.c_str()) == 0)
  {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
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
        this->IFile->seekg(this->FileOffsets[fileName][i], ios::beg);
        j = i;
        break;
      }
    }

    // Hopefully we are not very far from the timestep we want to use
    // Find it (and cache any timestep we find on the way...)
    while (j++ < realTimeStep)
    {
      this->ReadLine(line);
      while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
      {
        this->ReadLine(line);
      }
      this->ReadLine(line); // skip the description line

      if (measured)
      {
        partId = this->UnstructuredPartIds->IsId(this->NumberOfGeometryParts);
        output = static_cast<vtkDataSet*>(this->GetDataSetFromBlock(compositeOutput, partId));
        numPts = this->GetPointIds(partId)->GetNumberOfIds();
        if (numPts)
        {
          this->ReadLine(line);
          // Skip sclalars
          this->IFile->seekg(sizeof(float) * numPts, ios::cur);
        }
      }

      while (this->ReadLine(line) && strncmp(line, "part", 4) == 0)
      {
        this->ReadPartId(&partId);
        partId--; // EnSight starts #ing with 1.
        realId = this->InsertNewPartId(partId);
        output = static_cast<vtkDataSet*>(this->GetDataSetFromBlock(compositeOutput, realId));
        numPts = this->GetPointIds(realId)->GetNumberOfIds();
        if (numPts)
        {
          this->ReadLine(line); // "coordinates" or "block"
          // Skip sclalars
          this->IFile->seekg(sizeof(float) * numPts, ios::cur);
        }
      }
      if (this->FileOffsets.find(fileName) == this->FileOffsets.end())
      {
        std::map<int, long> tsMap;
        this->FileOffsets[fileName] = tsMap;
      }
      this->FileOffsets[fileName][j] = this->IFile->tellg();
    }

    this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadLine(line);
    }
  }

  this->ReadLine(line); // skip the description line

  if (measured)
  {
    partId = this->UnstructuredPartIds->IsId(this->NumberOfGeometryParts);
    output = static_cast<vtkDataSet*>(this->GetDataSetFromBlock(compositeOutput, partId));
    numPts = this->GetPointIds(partId)->GetNumberOfIds();
    if (numPts)
    {
      // 'this->ReadLine(line)' was removed here, otherwise there would be a
      // problem with timestep retrieval of the measured scalars.
      // This bug was noticed while fixing bug #7453.
      scalars = vtkFloatArray::New();
      scalars->SetNumberOfComponents(numberOfComponents);
      scalars->SetNumberOfTuples(this->GetPointIds(partId)->GetLocalNumberOfIds());
      scalarsRead = new float[numPts];
      this->ReadFloatArray(scalarsRead, numPts);
      // Why are we setting only one component here?
      // Only one component is set because scalars are single-component arrays.
      // For complex scalars, there is a file for the real part and another
      // file for the imaginary part, but we are storing them as a 2-component
      // array.
      for (i = 0; i < numPts; i++)
      {
        this->InsertVariableComponent(
          scalars, i, component, &(scalarsRead[i]), partId, 0, SCALAR_PER_NODE);
      }
      scalars->SetName(description);
      output->GetPointData()->AddArray(scalars);
      if (!output->GetPointData()->GetScalars())
      {
        output->GetPointData()->SetScalars(scalars);
      }
      scalars->Delete();
      delete[] scalarsRead;
    }
    if (this->IFile)
    {
      this->IFile->close();
      delete this->IFile;
      this->IFile = NULL;
    }
    return 1;
  }

  lineRead = this->ReadLine(line);
  while (lineRead && strncmp(line, "part", 4) == 0)
  {
    this->ReadPartId(&partId);
    partId--; // EnSight starts #ing with 1.
    realId = this->InsertNewPartId(partId);
    output = this->GetDataSetFromBlock(compositeOutput, realId);
    numPts = this->GetPointIds(realId)->GetNumberOfIds();
    // If the part has no points, then only the part number is listed in
    // the variable file.
    if (numPts)
    {
      this->ReadLine(line); // "coordinates" or "block"
      if (component == 0)
      {
        scalars = vtkFloatArray::New();
        scalars->SetNumberOfComponents(numberOfComponents);
        scalars->SetNumberOfTuples(this->GetPointIds(realId)->GetLocalNumberOfIds());
      }
      else
      {
        scalars = (vtkFloatArray*)(output->GetPointData()->GetArray(description));
      }

      scalarsRead = new float[numPts];
      this->ReadFloatArray(scalarsRead, numPts);

      for (i = 0; i < numPts; i++)
      {
        this->InsertVariableComponent(
          scalars, i, component, &(scalarsRead[i]), realId, 0, SCALAR_PER_NODE);
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
      delete[] scalarsRead;
    }

    this->IFile->peek();
    if (this->IFile->eof())
    {
      lineRead = 0;
      continue;
    }
    lineRead = this->ReadLine(line);
  }

  if (this->IFile)
  {
    this->IFile->close();
    delete this->IFile;
    this->IFile = NULL;
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldBinaryReader::ReadVectorsPerNode(const char* fileName, const char* description,
  int timeStep, vtkMultiBlockDataSet* compositeOutput, int measured)
{
  char line[80];
  int partId, realId, numPts, i, lineRead;
  vtkFloatArray* vectors;
  float tuple[3];
  float *comp1, *comp2, *comp3;
  float* vectorsRead;
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

  if (this->OpenFile(sfilename.c_str()) == 0)
  {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
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
        this->IFile->seekg(this->FileOffsets[fileName][i], ios::beg);
        j = i;
        break;
      }
    }

    // Hopefully we are not very far from the timestep we want to use
    // Find it (and cache any timestep we find on the way...)
    while (j++ < realTimeStep)
    {
      this->ReadLine(line);
      while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
      {
        this->ReadLine(line);
      }
      this->ReadLine(line); // skip the description line

      if (measured)
      {
        partId = this->UnstructuredPartIds->IsId(this->NumberOfGeometryParts);
        output = static_cast<vtkDataSet*>(this->GetDataSetFromBlock(compositeOutput, partId));
        numPts = this->GetPointIds(partId)->GetNumberOfIds();
        if (numPts)
        {
          this->ReadLine(line);
          // Skip vectors.
          this->IFile->seekg(sizeof(float) * 3 * numPts, ios::cur);
        }
      }

      while (this->ReadLine(line) && strncmp(line, "part", 4) == 0)
      {
        this->ReadPartId(&partId);
        partId--; // EnSight starts #ing with 1.
        realId = this->InsertNewPartId(partId);
        output = static_cast<vtkDataSet*>(this->GetDataSetFromBlock(compositeOutput, realId));
        numPts = this->GetPointIds(realId)->GetNumberOfIds();
        if (numPts)
        {
          this->ReadLine(line); // "coordinates" or "block"
          // Skip comp1, comp2 and comp3
          this->IFile->seekg(sizeof(float) * 3 * numPts, ios::cur);
        }
      }
      if (this->FileOffsets.find(fileName) == this->FileOffsets.end())
      {
        std::map<int, long> tsMap;
        this->FileOffsets[fileName] = tsMap;
      }
      this->FileOffsets[fileName][j] = this->IFile->tellg();
    }

    this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadLine(line);
    }
  }

  this->ReadLine(line); // skip the description line

  if (measured)
  {
    partId = this->UnstructuredPartIds->IsId(this->NumberOfGeometryParts);
    output = static_cast<vtkDataSet*>(this->GetDataSetFromBlock(compositeOutput, partId));
    numPts = this->GetPointIds(partId)->GetNumberOfIds();
    if (numPts)
    {
      // NOTE: NO ReadLine() here since there is only one description
      // line (already read above), immediately followed by the actual data.

      vectors = vtkFloatArray::New();
      vectors->SetNumberOfComponents(3);
      vectors->SetNumberOfTuples(this->GetPointIds(partId)->GetNumberOfIds());
      vectorsRead = vectors->GetPointer(0);
      this->ReadFloatArray(vectorsRead, numPts * 3);
      vtkFloatArray* localVectors = vtkFloatArray::New();
      localVectors->SetNumberOfComponents(3);
      localVectors->SetNumberOfTuples(this->GetPointIds(partId)->GetLocalNumberOfIds());
      float* vec = new float[3];
      for (i = 0; i < numPts; i++)
      {
        vectors->GetTypedTuple(i, vec);
        this->InsertVariableComponent(localVectors, i, 0, vec, partId, 0, VECTOR_PER_NODE);
      }
      delete[] vec;
      localVectors->SetName(description);
      output->GetPointData()->AddArray(localVectors);
      if (!output->GetPointData()->GetVectors())
      {
        output->GetPointData()->SetVectors(localVectors);
      }
      vectors->Delete();
    }
    if (this->IFile)
    {
      this->IFile->close();
      delete this->IFile;
      this->IFile = NULL;
    }
    return 1;
  }

  lineRead = this->ReadLine(line);
  while (lineRead && strncmp(line, "part", 4) == 0)
  {
    vectors = vtkFloatArray::New();
    this->ReadPartId(&partId);
    partId--; // EnSight starts #ing with 1.
    realId = this->InsertNewPartId(partId);
    output = this->GetDataSetFromBlock(compositeOutput, realId);
    numPts = this->GetPointIds(realId)->GetNumberOfIds();
    if (numPts)
    {
      this->ReadLine(line); // "coordinates" or "block"
      vectors->SetNumberOfComponents(3);
      vectors->SetNumberOfTuples(this->GetPointIds(realId)->GetLocalNumberOfIds());
      comp1 = new float[numPts];
      comp2 = new float[numPts];
      comp3 = new float[numPts];
      this->ReadFloatArray(comp1, numPts);
      this->ReadFloatArray(comp2, numPts);
      this->ReadFloatArray(comp3, numPts);
      for (i = 0; i < numPts; i++)
      {
        tuple[0] = comp1[i];
        tuple[1] = comp2[i];
        tuple[2] = comp3[i];
        this->InsertVariableComponent(vectors, i, -1, tuple, realId, 0, VECTOR_PER_NODE);
      }
      vectors->SetName(description);
      output->GetPointData()->AddArray(vectors);
      if (!output->GetPointData()->GetVectors())
      {
        output->GetPointData()->SetVectors(vectors);
      }
      vectors->Delete();
      delete[] comp1;
      delete[] comp2;
      delete[] comp3;
    }

    this->IFile->peek();
    if (this->IFile->eof())
    {
      lineRead = 0;
      continue;
    }
    lineRead = this->ReadLine(line);
  }

  if (this->IFile)
  {
    this->IFile->close();
    delete this->IFile;
    this->IFile = NULL;
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldBinaryReader::ReadTensorsPerNode(const char* fileName, const char* description,
  int timeStep, vtkMultiBlockDataSet* compositeOutput)
{
  char line[80];
  int partId, realId, numPts, i, lineRead;
  vtkFloatArray* tensors;
  float *comp1, *comp2, *comp3, *comp4, *comp5, *comp6;
  float tuple[6];
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

  if (this->OpenFile(sfilename.c_str()) == 0)
  {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    return 0;
  }

  if (this->UseFileSets)
  {
    int realTimeStep = timeStep - 1;
    int j = 0;
    // Try to find the nearest time step for which we know the offset
    for (i = realTimeStep; i >= 0; i--)
    {
      if (this->FileOffsets.find(fileName) != this->FileOffsets.end() &&
        this->FileOffsets[fileName].find(i) != this->FileOffsets[fileName].end())
      {
        this->IFile->seekg(this->FileOffsets[fileName][i], ios::beg);
        j = i;
        break;
      }
    }

    // Hopefully we are not very far from the timestep we want to use
    // Find it (and cache any timestep we find on the way...)
    while (j++ < realTimeStep)
    {
      this->ReadLine(line);
      while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
      {
        this->ReadLine(line);
      }
      this->ReadLine(line); // skip the description line

      while (this->ReadLine(line) && strncmp(line, "part", 4) == 0)
      {
        this->ReadPartId(&partId);
        partId--; // EnSight starts #ing with 1.
        realId = this->InsertNewPartId(partId);
        output = this->GetDataSetFromBlock(compositeOutput, realId);
        numPts = this->GetPointIds(realId)->GetNumberOfIds();
        if (numPts)
        {
          this->ReadLine(line); // "coordinates" or "block"
          // Skip over comp1, comp2, ... comp6
          this->IFile->seekg(sizeof(float) * 6 * numPts, ios::cur);
        }
      }
      if (this->FileOffsets.find(fileName) == this->FileOffsets.end())
      {
        std::map<int, long> tsMap;
        this->FileOffsets[fileName] = tsMap;
      }
      this->FileOffsets[fileName][j] = this->IFile->tellg();
    }
    this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadLine(line);
    }
  }

  this->ReadLine(line); // skip the description line
  lineRead = this->ReadLine(line);

  while (lineRead && strncmp(line, "part", 4) == 0)
  {
    this->ReadPartId(&partId);
    partId--; // EnSight starts #ing with 1.
    realId = this->InsertNewPartId(partId);
    output = this->GetDataSetFromBlock(compositeOutput, realId);
    numPts = this->GetPointIds(realId)->GetNumberOfIds();
    if (numPts)
    {
      tensors = vtkFloatArray::New();
      this->ReadLine(line); // "coordinates" or "block"
      tensors->SetNumberOfComponents(6);
      tensors->SetNumberOfTuples(this->GetPointIds(realId)->GetLocalNumberOfIds());
      comp1 = new float[numPts];
      comp2 = new float[numPts];
      comp3 = new float[numPts];
      comp4 = new float[numPts];
      comp5 = new float[numPts];
      comp6 = new float[numPts];
      this->ReadFloatArray(comp1, numPts);
      this->ReadFloatArray(comp2, numPts);
      this->ReadFloatArray(comp3, numPts);
      this->ReadFloatArray(comp4, numPts);
      this->ReadFloatArray(comp6, numPts);
      this->ReadFloatArray(comp5, numPts);
      for (i = 0; i < numPts; i++)
      {
        tuple[0] = comp1[i];
        tuple[1] = comp2[i];
        tuple[2] = comp3[i];
        tuple[3] = comp4[i];
        tuple[4] = comp5[i];
        tuple[5] = comp6[i];
        this->InsertVariableComponent(tensors, i, -1, tuple, realId, 0, TENSOR_SYMM_PER_NODE);
      }
      tensors->SetName(description);
      output->GetPointData()->AddArray(tensors);
      tensors->Delete();
      delete[] comp1;
      delete[] comp2;
      delete[] comp3;
      delete[] comp4;
      delete[] comp5;
      delete[] comp6;
    }

    this->IFile->peek();
    if (this->IFile->eof())
    {
      lineRead = 0;
      continue;
    }
    lineRead = this->ReadLine(line);
  }

  if (this->IFile)
  {
    this->IFile->close();
    delete this->IFile;
    this->IFile = NULL;
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldBinaryReader::ReadScalarsPerElement(const char* fileName,
  const char* description, int timeStep, vtkMultiBlockDataSet* compositeOutput,
  int numberOfComponents, int component)
{
  char line[80];
  int partId, realId, numCells, numCellsPerElement, i, idx;
  vtkFloatArray* scalars;
  float* scalarsRead;
  int lineRead, elementType;
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

  if (this->OpenFile(sfilename.c_str()) == 0)
  {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
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
        this->IFile->seekg(this->FileOffsets[fileName][i], ios::beg);
        j = i;
        break;
      }
    }

    // Hopefully we are not very far from the timestep we want to use
    // Find it (and cache any timestep we find on the way...)
    while (j++ < realTimeStep)
    {
      this->ReadLine(line);
      while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
      {
        this->ReadLine(line);
      }
      this->ReadLine(line);            // skip the description line
      lineRead = this->ReadLine(line); // "part"

      while (lineRead && strncmp(line, "part", 4) == 0)
      {
        this->ReadPartId(&partId);
        partId--; // EnSight starts #ing with 1.
        realId = this->InsertNewPartId(partId);
        output = this->GetDataSetFromBlock(compositeOutput, realId);
        numCells = this->GetTotalNumberOfCellIds(realId);
        if (numCells)
        {
          this->ReadLine(line); // element type or "block"

          // need to find out from CellIds how many cells we have of this
          // element type (and what their ids are) -- IF THIS IS NOT A BLOCK
          // SECTION
          if (strncmp(line, "block", 5) == 0)
          {
            // Skip over float scalars.
            this->IFile->seekg(sizeof(float) * numCells, ios::cur);
            lineRead = this->ReadLine(line);
          }
          else
          {
            while (
              lineRead && strncmp(line, "part", 4) != 0 && strncmp(line, "END TIME STEP", 13) != 0)
            {
              elementType = this->GetElementType(line);
              if (elementType == -1)
              {
                vtkErrorMacro("Unknown element type \"" << line << "\"");
                if (this->IFile)
                {
                  this->IFile->close();
                  delete this->IFile;
                  this->IFile = NULL;
                }
                return 0;
              }
              idx = this->UnstructuredPartIds->IsId(realId);
              numCellsPerElement = this->GetCellIds(idx, elementType)->GetNumberOfIds();
              this->IFile->seekg(sizeof(float) * numCellsPerElement, ios::cur);
              lineRead = this->ReadLine(line);
            }
          } // end else
        }   // end if (numCells)
        else
        {
          lineRead = this->ReadLine(line);
        }
      } // end while
      if (this->FileOffsets.find(fileName) == this->FileOffsets.end())
      {
        std::map<int, long> tsMap;
        this->FileOffsets[fileName] = tsMap;
      }
      this->FileOffsets[fileName][j] = this->IFile->tellg();
    } // end for
    this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadLine(line);
    }
  }

  this->ReadLine(line);            // skip the description line
  lineRead = this->ReadLine(line); // "part"

  while (lineRead && strncmp(line, "part", 4) == 0)
  {
    this->ReadPartId(&partId);
    partId--; // EnSight starts #ing with 1.
    realId = this->InsertNewPartId(partId);
    output = this->GetDataSetFromBlock(compositeOutput, realId);
    numCells = this->GetTotalNumberOfCellIds(realId);
    if (numCells)
    {
      this->ReadLine(line); // element type or "block"
      if (component == 0)
      {
        scalars = vtkFloatArray::New();
        scalars->SetNumberOfComponents(numberOfComponents);
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
        scalarsRead = new float[numCells];
        this->ReadFloatArray(scalarsRead, numCells);
        for (i = 0; i < numCells; i++)
        {
          this->InsertVariableComponent(
            scalars, i, component, &(scalarsRead[i]), realId, 0, SCALAR_PER_ELEMENT);
        }
        if (this->IFile->eof())
        {
          lineRead = 0;
        }
        else
        {
          lineRead = this->ReadLine(line);
        }
        delete[] scalarsRead;
      }
      else
      {
        while (lineRead && strncmp(line, "part", 4) != 0 && strncmp(line, "END TIME STEP", 13) != 0)
        {
          elementType = this->GetElementType(line);
          if (elementType == -1)
          {
            vtkErrorMacro("Unknown element type \"" << line << "\"");
            if (this->IFile)
            {
              this->IFile->close();
              delete this->IFile;
              this->IFile = NULL;
            }
            if (component == 0)
            {
              scalars->Delete();
            }
            return 0;
          }
          idx = this->UnstructuredPartIds->IsId(realId);
          numCellsPerElement = this->GetCellIds(idx, elementType)->GetNumberOfIds();
          scalarsRead = new float[numCellsPerElement];
          this->ReadFloatArray(scalarsRead, numCellsPerElement);
          for (i = 0; i < numCellsPerElement; i++)
          {
            this->InsertVariableComponent(
              scalars, i, component, &(scalarsRead[i]), idx, elementType, SCALAR_PER_ELEMENT);
          }
          this->IFile->peek();
          if (this->IFile->eof())
          {
            lineRead = 0;
          }
          else
          {
            lineRead = this->ReadLine(line);
          }
          delete[] scalarsRead;
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
      this->IFile->peek();
      if (this->IFile->eof())
      {
        lineRead = 0;
      }
      else
      {
        lineRead = this->ReadLine(line);
      }
    }
  }

  if (this->IFile)
  {
    this->IFile->close();
    delete this->IFile;
    this->IFile = NULL;
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldBinaryReader::ReadVectorsPerElement(const char* fileName,
  const char* description, int timeStep, vtkMultiBlockDataSet* compositeOutput)
{
  char line[80];
  int partId, realId, numCells, numCellsPerElement, i, idx;
  vtkFloatArray* vectors;
  float *comp1, *comp2, *comp3;
  int lineRead, elementType;
  float tuple[3];
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

  if (this->OpenFile(sfilename.c_str()) == 0)
  {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
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
        this->IFile->seekg(this->FileOffsets[fileName][i], ios::beg);
        j = i;
        break;
      }
    }

    // Hopefully we are not very far from the timestep we want to use
    // Find it (and cache any timestep we find on the way...)
    while (j++ < realTimeStep)
    {
      this->ReadLine(line);
      while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
      {
        this->ReadLine(line);
      }
      this->ReadLine(line);            // skip the description line
      lineRead = this->ReadLine(line); // "part"

      while (lineRead && strncmp(line, "part", 4) == 0)
      {
        this->ReadPartId(&partId);
        partId--; // EnSight starts #ing with 1.
        realId = this->InsertNewPartId(partId);
        output = this->GetDataSetFromBlock(compositeOutput, realId);
        numCells = this->GetTotalNumberOfCellIds(realId);
        if (numCells)
        {
          this->ReadLine(line); // element type or "block"

          // need to find out from CellIds how many cells we have of this
          // element type (and what their ids are) -- IF THIS IS NOT A BLOCK
          // SECTION
          if (strncmp(line, "block", 5) == 0)
          {
            // Skip over comp1, comp2 and comp3
            this->IFile->seekg(sizeof(float) * 3 * numCells, ios::cur);
            lineRead = this->ReadLine(line);
          }
          else
          {
            while (
              lineRead && strncmp(line, "part", 4) != 0 && strncmp(line, "END TIME STEP", 13) != 0)
            {
              elementType = this->GetElementType(line);
              if (elementType == -1)
              {
                vtkErrorMacro("Unknown element type \"" << line << "\"");
                delete this->IS;
                this->IS = NULL;
                return 0;
              }
              idx = this->UnstructuredPartIds->IsId(realId);
              numCellsPerElement = this->GetCellIds(idx, elementType)->GetNumberOfIds();
              // Skip over comp1, comp2 and comp3
              this->IFile->seekg(sizeof(float) * 3 * numCellsPerElement, ios::cur);
              lineRead = this->ReadLine(line);
            } // end while
          }   // end else
        }
        else
        {
          lineRead = this->ReadLine(line);
        }
      }
      if (this->FileOffsets.find(fileName) == this->FileOffsets.end())
      {
        std::map<int, long> tsMap;
        this->FileOffsets[fileName] = tsMap;
      }
      this->FileOffsets[fileName][j] = this->IFile->tellg();
    }
    this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadLine(line);
    }
  }

  this->ReadLine(line);            // skip the description line
  lineRead = this->ReadLine(line); // "part"

  while (lineRead && strncmp(line, "part", 4) == 0)
  {
    this->ReadPartId(&partId);
    partId--; // EnSight starts #ing with 1.
    realId = this->InsertNewPartId(partId);
    output = this->GetDataSetFromBlock(compositeOutput, realId);
    numCells = this->GetTotalNumberOfCellIds(realId);
    if (numCells)
    {
      vectors = vtkFloatArray::New();
      this->ReadLine(line); // element type or "block"
      vectors->SetNumberOfComponents(3);
      vectors->SetNumberOfTuples(this->GetLocalTotalNumberOfCellIds(realId));
      // need to find out from CellIds how many cells we have of this element
      // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
      if (strncmp(line, "block", 5) == 0)
      {
        comp1 = new float[numCells];
        comp2 = new float[numCells];
        comp3 = new float[numCells];
        this->ReadFloatArray(comp1, numCells);
        this->ReadFloatArray(comp2, numCells);
        this->ReadFloatArray(comp3, numCells);
        for (i = 0; i < numCells; i++)
        {
          tuple[0] = comp1[i];
          tuple[1] = comp2[i];
          tuple[2] = comp3[i];
          this->InsertVariableComponent(vectors, i, -1, tuple, realId, 0, VECTOR_PER_ELEMENT);
        }
        this->IFile->peek();
        if (this->IFile->eof())
        {
          lineRead = 0;
        }
        else
        {
          lineRead = this->ReadLine(line);
        }
        delete[] comp1;
        delete[] comp2;
        delete[] comp3;
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
            this->IS = NULL;
            vectors->Delete();
            return 0;
          }
          idx = this->UnstructuredPartIds->IsId(realId);
          numCellsPerElement = this->GetCellIds(idx, elementType)->GetNumberOfIds();
          comp1 = new float[numCellsPerElement];
          comp2 = new float[numCellsPerElement];
          comp3 = new float[numCellsPerElement];
          this->ReadFloatArray(comp1, numCellsPerElement);
          this->ReadFloatArray(comp2, numCellsPerElement);
          this->ReadFloatArray(comp3, numCellsPerElement);
          for (i = 0; i < numCellsPerElement; i++)
          {
            tuple[0] = comp1[i];
            tuple[1] = comp2[i];
            tuple[2] = comp3[i];
            this->InsertVariableComponent(
              vectors, i, 0, tuple, idx, elementType, VECTOR_PER_ELEMENT);
          }
          this->IFile->peek();
          if (this->IFile->eof())
          {
            lineRead = 0;
          }
          else
          {
            lineRead = this->ReadLine(line);
          }
          delete[] comp1;
          delete[] comp2;
          delete[] comp3;
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
      this->IFile->peek();
      if (this->IFile->eof())
      {
        lineRead = 0;
      }
      else
      {
        lineRead = this->ReadLine(line);
      }
    }
  }

  if (this->IFile)
  {
    this->IFile->close();
    delete this->IFile;
    this->IFile = NULL;
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldBinaryReader::ReadTensorsPerElement(const char* fileName,
  const char* description, int timeStep, vtkMultiBlockDataSet* compositeOutput)
{
  char line[80];
  int partId, realId, numCells, numCellsPerElement, i, idx;
  vtkFloatArray* tensors;
  int lineRead, elementType;
  float *comp1, *comp2, *comp3, *comp4, *comp5, *comp6;
  float tuple[6];
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
    vtkDebugMacro("full path to  tensor per element file: " << sfilename.c_str());
  }
  else
  {
    sfilename = fileName;
  }

  if (this->OpenFile(sfilename.c_str()) == 0)
  {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
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
        this->IFile->seekg(this->FileOffsets[fileName][i], ios::beg);
        j = i;
        break;
      }
    }

    // Hopefully we are not very far from the timestep we want to use
    // Find it (and cache any timestep we find on the way...)
    while (j++ < realTimeStep)
    {
      this->ReadLine(line);
      while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
      {
        this->ReadLine(line);
      }
      this->ReadLine(line);            // skip the description line
      lineRead = this->ReadLine(line); // "part"

      while (lineRead && strncmp(line, "part", 4) == 0)
      {
        this->ReadPartId(&partId);
        partId--; // EnSight starts #ing with 1.
        realId = this->InsertNewPartId(partId);
        output = this->GetDataSetFromBlock(compositeOutput, realId);
        numCells = this->GetTotalNumberOfCellIds(realId);
        if (numCells)
        {
          this->ReadLine(line); // element type or "block"

          // need to find out from CellIds how many cells we have of this
          // element type (and what their ids are) -- IF THIS IS NOT A BLOCK
          // SECTION
          if (strncmp(line, "block", 5) == 0)
          {
            // Skip comp1 - comp6
            this->IFile->seekg(sizeof(float) * 6 * numCells, ios::cur);
            lineRead = this->ReadLine(line);
          }
          else
          {
            while (
              lineRead && strncmp(line, "part", 4) != 0 && strncmp(line, "END TIME STEP", 13) != 0)
            {
              elementType = this->GetElementType(line);
              if (elementType == -1)
              {
                vtkErrorMacro("Unknown element type \"" << line << "\"");
                delete this->IS;
                this->IS = NULL;
                return 0;
              }
              idx = this->UnstructuredPartIds->IsId(realId);
              numCellsPerElement = this->GetCellIds(idx, elementType)->GetNumberOfIds();
              // Skip over comp1->comp6
              this->IFile->seekg(sizeof(float) * 6 * numCellsPerElement, ios::cur);
              lineRead = this->ReadLine(line);
            } // end while
          }   // end else
        }     // end if (numCells)
        else
        {
          lineRead = this->ReadLine(line);
        }
      }
      if (this->FileOffsets.find(fileName) == this->FileOffsets.end())
      {
        std::map<int, long> tsMap;
        this->FileOffsets[fileName] = tsMap;
      }
      this->FileOffsets[fileName][j] = this->IFile->tellg();
    }
    this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadLine(line);
    }
  }

  this->ReadLine(line);            // skip the description line
  lineRead = this->ReadLine(line); // "part"

  while (lineRead && strncmp(line, "part", 4) == 0)
  {
    this->ReadPartId(&partId);
    partId--; // EnSight starts #ing with 1.
    realId = this->InsertNewPartId(partId);
    output = this->GetDataSetFromBlock(compositeOutput, realId);
    numCells = this->GetTotalNumberOfCellIds(realId);
    if (numCells)
    {
      tensors = vtkFloatArray::New();
      this->ReadLine(line); // element type or "block"
      tensors->SetNumberOfComponents(6);
      tensors->SetNumberOfTuples(this->GetLocalTotalNumberOfCellIds(realId));

      // need to find out from CellIds how many cells we have of this element
      // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
      if (strncmp(line, "block", 5) == 0)
      {
        comp1 = new float[numCells];
        comp2 = new float[numCells];
        comp3 = new float[numCells];
        comp4 = new float[numCells];
        comp5 = new float[numCells];
        comp6 = new float[numCells];
        this->ReadFloatArray(comp1, numCells);
        this->ReadFloatArray(comp2, numCells);
        this->ReadFloatArray(comp3, numCells);
        this->ReadFloatArray(comp4, numCells);
        this->ReadFloatArray(comp6, numCells);
        this->ReadFloatArray(comp5, numCells);
        for (i = 0; i < numCells; i++)
        {
          tuple[0] = comp1[i];
          tuple[1] = comp2[i];
          tuple[2] = comp3[i];
          tuple[3] = comp4[i];
          tuple[4] = comp5[i];
          tuple[5] = comp6[i];
          this->InsertVariableComponent(tensors, i, -1, tuple, realId, 0, TENSOR_SYMM_PER_ELEMENT);
        }
        this->IFile->peek();
        if (this->IFile->eof())
        {
          lineRead = 0;
        }
        else
        {
          lineRead = this->ReadLine(line);
        }
        delete[] comp1;
        delete[] comp2;
        delete[] comp3;
        delete[] comp4;
        delete[] comp5;
        delete[] comp6;
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
            this->IS = NULL;
            tensors->Delete();
            return 0;
          }
          idx = this->UnstructuredPartIds->IsId(realId);
          numCellsPerElement = this->GetCellIds(idx, elementType)->GetNumberOfIds();
          comp1 = new float[numCellsPerElement];
          comp2 = new float[numCellsPerElement];
          comp3 = new float[numCellsPerElement];
          comp4 = new float[numCellsPerElement];
          comp5 = new float[numCellsPerElement];
          comp6 = new float[numCellsPerElement];
          this->ReadFloatArray(comp1, numCellsPerElement);
          this->ReadFloatArray(comp2, numCellsPerElement);
          this->ReadFloatArray(comp3, numCellsPerElement);
          this->ReadFloatArray(comp4, numCellsPerElement);
          this->ReadFloatArray(comp6, numCellsPerElement);
          this->ReadFloatArray(comp5, numCellsPerElement);
          for (i = 0; i < numCellsPerElement; i++)
          {
            tuple[0] = comp1[i];
            tuple[1] = comp2[i];
            tuple[2] = comp3[i];
            tuple[3] = comp4[i];
            tuple[4] = comp5[i];
            tuple[5] = comp6[i];
            this->InsertVariableComponent(
              tensors, i, 0, tuple, idx, elementType, TENSOR_SYMM_PER_ELEMENT);
          }
          this->IFile->peek();
          if (this->IFile->eof())
          {
            lineRead = 0;
          }
          else
          {
            lineRead = this->ReadLine(line);
          }
          delete[] comp1;
          delete[] comp2;
          delete[] comp3;
          delete[] comp4;
          delete[] comp5;
          delete[] comp6;
        } // end while
      }   // end else
      tensors->SetName(description);
      output->GetCellData()->AddArray(tensors);
      tensors->Delete();
    }
    else
    {
      this->IFile->peek();
      if (this->IFile->eof())
      {
        lineRead = 0;
      }
      else
      {
        lineRead = this->ReadLine(line);
      }
    }
  }

  if (this->IFile)
  {
    this->IFile->close();
    delete this->IFile;
    this->IFile = NULL;
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldBinaryReader::CreateUnstructuredGridOutput(
  int partId, char line[80], const char* name, vtkMultiBlockDataSet* compositeOutput)
{
  int lineRead = 1;
  int i, j;
  vtkIdType* nodeIds;
  int* nodeIdList;
  int numElements;
  int idx, cellType;

  this->NumberOfNewOutputs++;

  if (this->GetDataSetFromBlock(compositeOutput, partId) == NULL ||
    !this->GetDataSetFromBlock(compositeOutput, partId)->IsA("vtkUnstructuredGrid"))
  {
    vtkDebugMacro("creating new unstructured output");
    vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::New();
    this->AddToBlock(compositeOutput, partId, ugrid);
    ugrid->Delete();

    this->UnstructuredPartIds->InsertNextId(partId);
  }

  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(this->GetDataSetFromBlock(compositeOutput, partId));
  this->SetBlockName(compositeOutput, partId, name);

  // Clear all cell ids from the last execution, if any.
  idx = this->UnstructuredPartIds->IsId(partId);
  for (i = 0; i < vtkPEnSightReader::NUMBER_OF_ELEMENT_TYPES; i++)
  {
    this->GetCellIds(idx, i)->Reset();
    this->GetPointIds(idx)->Reset();
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
      // keep coordinates offset in mind
      coordinatesOffset = -1;
      this->GetPointIds(idx)->Reset();
      this->LastPointId = 0;

      vtkDebugMacro("coordinates");
      vtkPoints* points = vtkPoints::New();
      coordinatesOffset = this->IFile->tellg();

      int pointsRead = this->ReadOrSkipCoordinates(points, coordinatesOffset, idx, true);
      points->Delete();
      if (pointsRead == -1)
      {
        return -1;
      }
    }
    else if (strncmp(line, "point", 5) == 0)
    {
      vtkDebugMacro("point");

      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of point cells; check that ByteOrder is set correctly.");
        return -1;
      }

      nodeIds = new vtkIdType[1];

      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      nodeIdList = new int[numElements];
      this->ReadIntArray(nodeIdList, numElements);

      for (i = 0; i < numElements; i++)
      {
        nodeIds[0] = nodeIdList[i] - 1;
        this->InsertNextCellAndId(
          output, VTK_VERTEX, 1, nodeIds, idx, vtkPEnSightReader::POINT, i, numElements);
      }

      delete[] nodeIds;
      delete[] nodeIdList;
    }
    else if (strncmp(line, "g_point", 7) == 0)
    {
      // skipping ghost cells
      vtkDebugMacro("g_point");

      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of g_point cells; check that ByteOrder is set correctly.");
        return -1;
      }
      if (this->ElementIdsListed)
      { // skip element ids.
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      // Skip nodeIdList.
      this->IFile->seekg(sizeof(int) * numElements, ios::cur);
    }
    else if (strncmp(line, "bar2", 4) == 0)
    {
      vtkDebugMacro("bar2");

      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of bar2 cells; check that ByteOrder is set correctly.");
        return -1;
      }
      nodeIds = new vtkIdType[2];
      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      nodeIdList = new int[numElements * 2];
      this->ReadIntArray(nodeIdList, numElements * 2);

      for (i = 0; i < numElements; i++)
      {
        for (j = 0; j < 2; j++)
        {
          nodeIds[j] = nodeIdList[2 * i + j] - 1;
        }
        this->InsertNextCellAndId(
          output, VTK_LINE, 2, nodeIds, idx, vtkPEnSightReader::BAR2, i, numElements);
      }

      delete[] nodeIds;
      delete[] nodeIdList;
    }
    else if (strncmp(line, "g_bar2", 6) == 0)
    {
      // skipping ghost cells
      vtkDebugMacro("g_bar2");

      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of g_bar2 cells; check that ByteOrder is set correctly.");
        return -1;
      }
      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      // Skip nodeIdList.
      this->IFile->seekg(sizeof(int) * 2 * numElements, ios::cur);
    }
    else if (strncmp(line, "bar3", 4) == 0)
    {
      vtkDebugMacro("bar3");

      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of bar3 cells; check that ByteOrder is set correctly.");
        return -1;
      }
      nodeIds = new vtkIdType[3];

      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      nodeIdList = new int[numElements * 3];
      this->ReadIntArray(nodeIdList, numElements * 3);

      for (i = 0; i < numElements; i++)
      {
        nodeIds[0] = nodeIdList[3 * i] - 1;
        nodeIds[1] = nodeIdList[3 * i + 2] - 1;
        nodeIds[2] = nodeIdList[3 * i + 1] - 1;
        this->InsertNextCellAndId(
          output, VTK_QUADRATIC_EDGE, 3, nodeIds, idx, vtkPEnSightReader::BAR3, i, numElements);
      }

      delete[] nodeIds;
      delete[] nodeIdList;
    }
    else if (strncmp(line, "g_bar3", 6) == 0)
    {
      // skipping ghost cells
      vtkDebugMacro("g_bar3");

      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of g_bar3 cells; check that ByteOrder is set correctly.");
        return -1;
      }

      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      // Skip nodeIdList.
      this->IFile->seekg(sizeof(int) * 2 * numElements, ios::cur);
    }
    else if (strncmp(line, "nsided", 6) == 0)
    {
      vtkDebugMacro("nsided");
      int* numNodesPerElement;
      int numNodes = 0;
      int nodeCount = 0;

      cellType = vtkPEnSightReader::NSIDED;
      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of nsided cells; check that ByteOrder is set correctly.");
        return -1;
      }

      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      numNodesPerElement = new int[numElements];
      this->ReadIntArray(numNodesPerElement, numElements);
      for (i = 0; i < numElements; i++)
      {
        numNodes += numNodesPerElement[i];
      }
      nodeIdList = new int[numNodes];
      this->ReadIntArray(nodeIdList, numNodes);

      for (i = 0; i < numElements; i++)
      {
        nodeIds = new vtkIdType[numNodesPerElement[i]];
        for (j = 0; j < numNodesPerElement[i]; j++)
        {
          nodeIds[j] = nodeIdList[nodeCount] - 1;
          nodeCount++;
        }
        this->InsertNextCellAndId(
          output, VTK_POLYGON, numNodesPerElement[i], nodeIds, idx, cellType, i, numElements);

        delete[] nodeIds;
      }

      delete[] nodeIdList;
      delete[] numNodesPerElement;
    }
    else if (strncmp(line, "g_nsided", 8) == 0)
    {
      // skipping ghost cells
      vtkDebugMacro("g_nsided");
      int* numNodesPerElement;
      int numNodes = 0;

      // cellType = vtkPEnSightReader::NSIDED;
      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of g_nsided cells; check that ByteOrder is set correctly.");
        return -1;
      }

      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      numNodesPerElement = new int[numElements];
      this->ReadIntArray(numNodesPerElement, numElements);
      for (i = 0; i < numElements; i++)
      {
        numNodes += numNodesPerElement[i];
      }
      // Skip nodeIdList.
      this->IFile->seekg(sizeof(int) * numNodes, ios::cur);
      delete[] numNodesPerElement;
    }
    else if (strncmp(line, "tria3", 5) == 0 || strncmp(line, "tria6", 5) == 0)
    {
      if (strncmp(line, "tria6", 5) == 0)
      {
        vtkDebugMacro("tria6");
        cellType = vtkPEnSightReader::TRIA6;
      }
      else
      {
        vtkDebugMacro("tria3");
        cellType = vtkPEnSightReader::TRIA3;
      }

      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of triangle cells; check that ByteOrder is set correctly.");
        return -1;
      }

      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      if (cellType == vtkPEnSightReader::TRIA6)
      {
        nodeIds = new vtkIdType[6];
        nodeIdList = new int[numElements * 6];
        this->ReadIntArray(nodeIdList, numElements * 6);
      }
      else
      {
        nodeIds = new vtkIdType[3];
        nodeIdList = new int[numElements * 3];
        this->ReadIntArray(nodeIdList, numElements * 3);
      }

      for (i = 0; i < numElements; i++)
      {
        if (cellType == vtkPEnSightReader::TRIA6)
        {
          for (j = 0; j < 6; j++)
          {
            nodeIds[j] = nodeIdList[6 * i + j] - 1;
          }
          this->InsertNextCellAndId(
            output, VTK_QUADRATIC_TRIANGLE, 6, nodeIds, idx, cellType, i, numElements);
        }
        else
        {
          for (j = 0; j < 3; j++)
          {
            nodeIds[j] = nodeIdList[3 * i + j] - 1;
          }
          this->InsertNextCellAndId(
            output, VTK_TRIANGLE, 3, nodeIds, idx, cellType, i, numElements);
        }
      }

      delete[] nodeIds;
      delete[] nodeIdList;
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

      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of triangle cells; check that ByteOrder is set correctly.");
        return -1;
      }
      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      if (cellType == vtkPEnSightReader::TRIA6)
      {
        // Skip nodeIdList.
        this->IFile->seekg(sizeof(int) * 6 * numElements, ios::cur);
      }
      else
      {
        // Skip nodeIdList.
        this->IFile->seekg(sizeof(int) * 3 * numElements, ios::cur);
      }
    }
    else if (strncmp(line, "quad4", 5) == 0 || strncmp(line, "quad8", 5) == 0)
    {
      if (strncmp(line, "quad8", 5) == 0)
      {
        vtkDebugMacro("quad8");
        cellType = vtkPEnSightReader::QUAD8;
      }
      else
      {
        vtkDebugMacro("quad4");
        cellType = vtkPEnSightReader::QUAD4;
      }

      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of quad cells; check that ByteOrder is set correctly.");
        return -1;
      }

      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      if (cellType == vtkPEnSightReader::QUAD8)
      {
        nodeIds = new vtkIdType[8];
        nodeIdList = new int[numElements * 8];
        this->ReadIntArray(nodeIdList, numElements * 8);
      }
      else
      {
        nodeIds = new vtkIdType[4];
        nodeIdList = new int[numElements * 4];
        this->ReadIntArray(nodeIdList, numElements * 4);
      }

      for (i = 0; i < numElements; i++)
      {
        if (cellType == vtkPEnSightReader::QUAD8)
        {
          for (j = 0; j < 8; j++)
          {
            nodeIds[j] = nodeIdList[8 * i + j] - 1;
          }
          this->InsertNextCellAndId(
            output, VTK_QUADRATIC_QUAD, 8, nodeIds, idx, cellType, i, numElements);
        }
        else
        {
          for (j = 0; j < 4; j++)
          {
            nodeIds[j] = nodeIdList[4 * i + j] - 1;
          }
          this->InsertNextCellAndId(output, VTK_QUAD, 4, nodeIds, idx, cellType, i, numElements);
        }
      }

      delete[] nodeIds;
      delete[] nodeIdList;
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

      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of quad cells; check that ByteOrder is set correctly.");
        return -1;
      }
      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      if (cellType == vtkPEnSightReader::QUAD8)
      {
        // Skip nodeIdList.
        this->IFile->seekg(sizeof(int) * 8 * numElements, ios::cur);
      }
      else
      {
        // Skip nodeIdList.
        this->IFile->seekg(sizeof(int) * 4 * numElements, ios::cur);
      }
    }
    else if (strncmp(line, "nfaced", 6) == 0)
    {
      vtkDebugMacro("nfaced");
      int* numFacesPerElement;
      int* numNodesPerFace;
      int* numNodesPerElement;
      int* nodeMarker;
      int numPts = 0;
      int numFaces = 0;
      int numNodes = 0;
      int faceCount = 0;
      int nodeCount = 0;
      int elementNodeCount = 0;

      cellType = vtkPEnSightReader::NFACED;
      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of nfaced cells; check that ByteOrder is set correctly.");
        return -1;
      }

      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      numFacesPerElement = new int[numElements];
      this->ReadIntArray(numFacesPerElement, numElements);
      for (i = 0; i < numElements; i++)
      {
        numFaces += numFacesPerElement[i];
      }
      numNodesPerFace = new int[numFaces];
      this->ReadIntArray(numNodesPerFace, numFaces);

      numNodesPerElement = new int[numElements];
      for (i = 0; i < numElements; i++)
      {
        numNodesPerElement[i] = 0;
        for (j = 0; j < numFacesPerElement[i]; j++)
        {
          numNodesPerElement[i] += numNodesPerFace[faceCount + j];
        }
        faceCount += numFacesPerElement[i];
      }

      delete[] numFacesPerElement;
      delete[] numNodesPerFace;

      for (i = 0; i < numElements; i++)
      {
        numNodes += numNodesPerElement[i];
      }

      // Points may have not been read here
      // but we have an estimation in ReadOrSkipCoordinates( .... , skip = true )
      numPts = this->GetPointIds(idx)->GetNumberOfIds();

      nodeMarker = new int[numPts];
      for (i = 0; i < numPts; i++)
      {
        nodeMarker[i] = -1;
      }

      nodeIdList = new int[numNodes];
      this->ReadIntArray(nodeIdList, numNodes);

      for (i = 0; i < numElements; i++)
      {
        // For each nfaced...
        elementNodeCount = 0;
        nodeIds = new vtkIdType[numNodesPerElement[i]];
        for (j = 0; j < numNodesPerElement[i]; j++)
        {
          // For each Node (Point) in Element (Nfaced) ...
          if (nodeMarker[nodeIdList[nodeCount] - 1] < i)
          {
            nodeIds[elementNodeCount] = nodeIdList[nodeCount] - 1;
            nodeMarker[nodeIdList[nodeCount] - 1] = i;
            elementNodeCount += 1;
          }
          nodeCount++;
        }
        this->InsertNextCellAndId(
          output, VTK_CONVEX_POINT_SET, elementNodeCount, nodeIds, idx, cellType, i, numElements);

        delete[] nodeIds;
      }

      delete[] nodeMarker;
      delete[] nodeIdList;
      delete[] numNodesPerElement;
    }
    else if (strncmp(line, "tetra4", 6) == 0 || strncmp(line, "tetra10", 7) == 0)
    {
      if (strncmp(line, "tetra10", 7) == 0)
      {
        vtkDebugMacro("tetra10");
        cellType = vtkPEnSightReader::TETRA10;
      }
      else
      {
        vtkDebugMacro("tetra4");
        cellType = vtkPEnSightReader::TETRA4;
      }

      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro(
          "Invalid number of tetrahedral cells; check that ByteOrder is set correctly.");
        return -1;
      }

      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      if (cellType == vtkPEnSightReader::TETRA10)
      {
        nodeIds = new vtkIdType[10];
        nodeIdList = new int[numElements * 10];
        this->ReadIntArray(nodeIdList, numElements * 10);
      }
      else
      {
        nodeIds = new vtkIdType[4];
        nodeIdList = new int[numElements * 4];
        this->ReadIntArray(nodeIdList, numElements * 4);
      }

      for (i = 0; i < numElements; i++)
      {
        if (cellType == vtkPEnSightReader::TETRA10)
        {
          for (j = 0; j < 10; j++)
          {
            nodeIds[j] = nodeIdList[10 * i + j] - 1;
          }
          this->InsertNextCellAndId(
            output, VTK_QUADRATIC_TETRA, 10, nodeIds, idx, cellType, i, numElements);
        }
        else
        {
          for (j = 0; j < 4; j++)
          {
            nodeIds[j] = nodeIdList[4 * i + j] - 1;
          }
          this->InsertNextCellAndId(output, VTK_TETRA, 4, nodeIds, idx, cellType, i, numElements);
        }
      }

      delete[] nodeIds;
      delete[] nodeIdList;
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

      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro(
          "Invalid number of tetrahedral cells; check that ByteOrder is set correctly.");
        return -1;
      }
      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      if (cellType == vtkPEnSightReader::TETRA10)
      {
        // Skip nodeIdList.
        this->IFile->seekg(sizeof(int) * 10 * numElements, ios::cur);
      }
      else
      {
        // Skip nodeIdList.
        this->IFile->seekg(sizeof(int) * 4 * numElements, ios::cur);
      }
    }
    else if (strncmp(line, "pyramid5", 8) == 0 || strncmp(line, "pyramid13", 9) == 0)
    {
      if (strncmp(line, "pyramid13", 9) == 0)
      {
        vtkDebugMacro("pyramid13");
        cellType = vtkPEnSightReader::PYRAMID13;
      }
      else
      {
        vtkDebugMacro("pyramid5");
        cellType = vtkPEnSightReader::PYRAMID5;
      }

      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of pyramid cells; check that ByteOrder is set correctly.");
        return -1;
      }

      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      if (cellType == vtkPEnSightReader::PYRAMID13)
      {
        nodeIds = new vtkIdType[13];
        nodeIdList = new int[numElements * 13];
        this->ReadIntArray(nodeIdList, numElements * 13);
      }
      else
      {
        nodeIds = new vtkIdType[5];
        nodeIdList = new int[numElements * 5];
        this->ReadIntArray(nodeIdList, numElements * 5);
      }

      for (i = 0; i < numElements; i++)
      {
        if (cellType == vtkPEnSightReader::PYRAMID13)
        {
          for (j = 0; j < 13; j++)
          {
            nodeIds[j] = nodeIdList[13 * i + j] - 1;
          }
          this->InsertNextCellAndId(
            output, VTK_QUADRATIC_PYRAMID, 13, nodeIds, idx, cellType, i, numElements);
        }
        else
        {
          for (j = 0; j < 5; j++)
          {
            nodeIds[j] = nodeIdList[5 * i + j] - 1;
          }
          this->InsertNextCellAndId(output, VTK_PYRAMID, 5, nodeIds, idx, cellType, i, numElements);
        }
      }

      delete[] nodeIds;
      delete[] nodeIdList;
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

      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of pyramid cells; check that ByteOrder is set correctly.");
        return -1;
      }
      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      if (cellType == vtkPEnSightReader::PYRAMID13)
      {
        // Skip nodeIdList.
        this->IFile->seekg(sizeof(int) * 13 * numElements, ios::cur);
      }
      else
      {
        // Skip nodeIdList.
        this->IFile->seekg(sizeof(int) * 5 * numElements, ios::cur);
      }
    }
    else if (strncmp(line, "hexa8", 5) == 0 || strncmp(line, "hexa20", 6) == 0)
    {
      if (strncmp(line, "hexa20", 6) == 0)
      {
        vtkDebugMacro("hexa20");
        cellType = vtkPEnSightReader::HEXA20;
      }
      else
      {
        vtkDebugMacro("hexa8");
        cellType = vtkPEnSightReader::HEXA8;
      }

      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of hexahedral cells; check that ByteOrder is set correctly.");
        return -1;
      }

      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      if (cellType == vtkPEnSightReader::HEXA20)
      {
        nodeIds = new vtkIdType[20];
        nodeIdList = new int[numElements * 20];
        this->ReadIntArray(nodeIdList, numElements * 20);
      }
      else
      {
        nodeIds = new vtkIdType[8];
        nodeIdList = new int[numElements * 8];
        this->ReadIntArray(nodeIdList, numElements * 8);
      }

      for (i = 0; i < numElements; i++)
      {
        if (cellType == vtkPEnSightReader::HEXA20)
        {
          for (j = 0; j < 20; j++)
          {
            nodeIds[j] = nodeIdList[20 * i + j] - 1;
          }
          this->InsertNextCellAndId(
            output, VTK_QUADRATIC_HEXAHEDRON, 20, nodeIds, idx, cellType, i, numElements);
        }
        else
        {
          for (j = 0; j < 8; j++)
          {
            nodeIds[j] = nodeIdList[8 * i + j] - 1;
          }
          this->InsertNextCellAndId(
            output, VTK_HEXAHEDRON, 8, nodeIds, idx, cellType, i, numElements);
        }
      }
      delete[] nodeIds;
      delete[] nodeIdList;
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

      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of hexahedral cells; check that ByteOrder is set correctly.");
        return -1;
      }
      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      if (cellType == vtkPEnSightReader::HEXA20)
      {
        // Skip nodeIdList.
        this->IFile->seekg(sizeof(int) * 20 * numElements, ios::cur);
      }
      else
      {
        // Skip nodeIdList.
        this->IFile->seekg(sizeof(int) * 8 * numElements, ios::cur);
      }
    }
    else if (strncmp(line, "penta6", 6) == 0 || strncmp(line, "penta15", 7) == 0)
    {
      if (strncmp(line, "penta15", 7) == 0)
      {
        vtkDebugMacro("penta15");
        cellType = vtkPEnSightReader::PENTA15;
      }
      else
      {
        vtkDebugMacro("penta6");
        cellType = vtkPEnSightReader::PENTA6;
      }

      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of pentagonal cells; check that ByteOrder is set correctly.");
        return -1;
      }

      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      if (cellType == vtkPEnSightReader::PENTA15)
      {
        nodeIds = new vtkIdType[15];
        nodeIdList = new int[numElements * 15];
        this->ReadIntArray(nodeIdList, numElements * 15);
      }
      else
      {
        nodeIds = new vtkIdType[6];
        nodeIdList = new int[numElements * 6];
        this->ReadIntArray(nodeIdList, numElements * 6);
      }

      for (i = 0; i < numElements; i++)
      {
        if (cellType == vtkPEnSightReader::PENTA15)
        {
          for (j = 0; j < 15; j++)
          {
            nodeIds[j] = nodeIdList[15 * i + j] - 1;
          }
          this->InsertNextCellAndId(
            output, VTK_QUADRATIC_WEDGE, 15, nodeIds, idx, cellType, i, numElements);
        }
        else
        {
          for (j = 0; j < 6; j++)
          {
            nodeIds[j] = nodeIdList[6 * i + j] - 1;
          }
          this->InsertNextCellAndId(output, VTK_WEDGE, 6, nodeIds, idx, cellType, i, numElements);
        }
      }

      delete[] nodeIds;
      delete[] nodeIdList;
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

      this->ReadInt(&numElements);
      if (numElements < 0 || numElements * (int)sizeof(int) > this->FileSize ||
        numElements > this->FileSize)
      {
        vtkErrorMacro("Invalid number of pentagonal cells; check that ByteOrder is set correctly.");
        return -1;
      }
      if (this->ElementIdsListed)
      {
        this->IFile->seekg(sizeof(int) * numElements, ios::cur);
      }

      if (cellType == vtkPEnSightReader::PENTA15)
      {
        // Skip nodeIdList.
        this->IFile->seekg(sizeof(int) * 15 * numElements, ios::cur);
      }
      else
      {
        // Skip nodeIdList.
        this->IFile->seekg(sizeof(int) * 6 * numElements, ios::cur);
      }
    }
    else if (strncmp(line, "END TIME STEP", 13) == 0)
    {
      // time to read coordinates at end if necessary
      if (this->CoordinatesAtEnd &&
        (this->InjectCoordinatesAtEnd(output, coordinatesOffset, idx) == -1))
      {
        return -1;
      }
      return 1;
    }
    else if (this->IS->fail())
    {
      // May want consistency check here?
      // time to read coordinates at end if necessary
      if (this->CoordinatesAtEnd &&
        (this->InjectCoordinatesAtEnd(output, coordinatesOffset, idx) == -1))
      {
        return -1;
      }
      // vtkWarningMacro("EOF on geometry file");
      return 1;
    }
    else
    {

      vtkErrorMacro("undefined geometry file line");
      return -1;
    }

    this->IFile->peek();
    if (this->IFile->eof())
    {
      lineRead = 0;
      // time to read coordinates at end if necessary
      if (this->CoordinatesAtEnd &&
        (this->InjectCoordinatesAtEnd(output, coordinatesOffset, idx) == -1))
      {
        return -1;
      }
      continue;
    }
    lineRead = this->ReadLine(line);
  }

  // time to read coordinates at end if necessary
  if (this->CoordinatesAtEnd &&
    (this->InjectCoordinatesAtEnd(output, coordinatesOffset, idx) == -1))
  {
    return -1;
  }

  return lineRead;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldBinaryReader::CreateStructuredGridOutput(
  int partId, char line[80], const char* name, vtkMultiBlockDataSet* compositeOutput)
{
  char subLine[80];
  int lineRead;
  int iblanked = 0;
  int dimensions[3];
  vtkIdType i;
  vtkPoints* points = vtkPoints::New();
  vtkIdType numPts;

  this->NumberOfNewOutputs++;

  vtkDataSet* ds = this->GetDataSetFromBlock(compositeOutput, partId);
  if (ds == NULL || !ds->IsA("vtkStructuredGrid"))
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

  this->ReadIntArray(dimensions, 3);
  // global number of points
  numPts = dimensions[0] * dimensions[1] * dimensions[2];
  if (dimensions[0] < 0 || dimensions[0] * (int)sizeof(int) > this->FileSize ||
    dimensions[0] > this->FileSize || dimensions[1] < 0 ||
    dimensions[1] * (int)sizeof(int) > this->FileSize || dimensions[1] > this->FileSize ||
    dimensions[2] < 0 || dimensions[2] * (int)sizeof(int) > this->FileSize ||
    dimensions[2] > this->FileSize || numPts < 0 || numPts * (int)sizeof(int) > this->FileSize ||
    numPts > this->FileSize)
  {
    vtkErrorMacro("Invalid dimensions read; check that ByteOrder is set correctly.");
    points->Delete();
    return -1;
  }

  int newDimensions[3];
  int splitDimension;
  int splitDimensionBeginIndex;
  vtkUnsignedCharArray* pointGhostArray = NULL;
  vtkUnsignedCharArray* cellGhostArray = NULL;
  if (this->GhostLevels == 0)
  {
    this->PrepareStructuredDimensionsForDistribution(
      partId, dimensions, newDimensions, &splitDimension, &splitDimensionBeginIndex, 0, NULL, NULL);
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

  long currentPositionInFile = this->IFile->tellg();

  // Buffer Read.
  this->FloatBufferFilePosition = currentPositionInFile;
  this->FloatBufferIndexBegin = 0;
  this->FloatBufferNumberOfVectors = numPts;
  long endFilePosition = currentPositionInFile + 3 * numPts * (long)sizeof(float);
  if (this->Fortran)
    endFilePosition += 24; // 4 * (begin + end) * number of components (3)
  this->UpdateFloatBuffer();
  this->IFile->seekg(endFilePosition);

  for (i = 0; i < numPts; i++)
  {
    int realPointId = this->GetPointIds(partId)->GetId(i);
    if (realPointId != -1)
    {
      float vec[3];
      this->GetVectorFromFloatBuffer(i, vec);
      points->InsertNextPoint(vec[0], vec[1], vec[2]);
    }
  }
  output->SetPoints(points);
  if (iblanked)
  {
    int* iblanks = new int[numPts];
    this->ReadIntArray(iblanks, numPts);

    for (i = 0; i < numPts; i++)
    {
      if (!iblanks[i])
      {
        int realPointId = this->GetPointIds(partId)->GetId(i);
        if (realPointId != -1)
          output->BlankPoint(realPointId);
      }
    }
    delete[] iblanks;
  }

  // Ghost level End
  if (this->GhostLevels > 0)
  {
    output->GetPointData()->AddArray(pointGhostArray);
    output->GetCellData()->AddArray(cellGhostArray);
  }

  points->Delete();

  this->IFile->peek();
  if (this->IFile->eof())
  {
    lineRead = 0;
  }
  else
  {
    lineRead = this->ReadLine(line);
  }

  if (strncmp(line, "node_ids", 8) == 0)
  {
    int* nodeIds = new int[numPts];
    this->ReadIntArray(nodeIds, numPts);
    lineRead = this->ReadLine(line);
    delete[] nodeIds;
  }
  if (strncmp(line, "element_ids", 11) == 0)
  {
    int numElements = (dimensions[0] - 1) * (dimensions[1] - 1) * (dimensions[2] - 1);
    int* elementIds = new int[numElements];
    this->ReadIntArray(elementIds, numElements);
    lineRead = this->ReadLine(line);
    delete[] elementIds;
  }

  return lineRead;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldBinaryReader::CreateRectilinearGridOutput(
  int partId, char line[80], const char* name, vtkMultiBlockDataSet* compositeOutput)
{
  char subLine[80];
  int lineRead;
  int iblanked = 0;
  int dimensions[3];
  int i;
  vtkFloatArray* xCoords = vtkFloatArray::New();
  vtkFloatArray* yCoords = vtkFloatArray::New();
  vtkFloatArray* zCoords = vtkFloatArray::New();
  float* tempCoords;
  int numPts;

  this->NumberOfNewOutputs++;

  vtkDataSet* ds = this->GetDataSetFromBlock(compositeOutput, partId);
  if (ds == NULL || !ds->IsA("vtkRectilinearGrid"))
  {
    vtkDebugMacro("creating new rectilinear grid output");
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

  this->ReadIntArray(dimensions, 3);
  if (dimensions[0] < 0 || dimensions[0] * (int)sizeof(int) > this->FileSize ||
    dimensions[0] > this->FileSize || dimensions[1] < 0 ||
    dimensions[1] * (int)sizeof(int) > this->FileSize || dimensions[1] > this->FileSize ||
    dimensions[2] < 0 || dimensions[2] * (int)sizeof(int) > this->FileSize ||
    dimensions[2] > this->FileSize || (dimensions[0] + dimensions[1] + dimensions[2]) < 0 ||
    (dimensions[0] + dimensions[1] + dimensions[2]) * (int)sizeof(int) > this->FileSize ||
    (dimensions[0] + dimensions[1] + dimensions[2]) > this->FileSize)
  {
    vtkErrorMacro("Invalid dimensions read; check that BytetOrder is set correctly.");
    xCoords->Delete();
    yCoords->Delete();
    zCoords->Delete();
    return -1;
  }

  int newDimensions[3];
  int splitDimension;
  int splitDimensionBeginIndex;
  vtkUnsignedCharArray* pointGhostArray = NULL;
  vtkUnsignedCharArray* cellGhostArray = NULL;
  if (this->GhostLevels == 0)
  {
    this->PrepareStructuredDimensionsForDistribution(
      partId, dimensions, newDimensions, &splitDimension, &splitDimensionBeginIndex, 0, NULL, NULL);
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

  tempCoords = new float[dimensions[0]];
  this->ReadFloatArray(tempCoords, dimensions[0]);
  for (i = beginDimension[0]; i < (beginDimension[0] + newDimensions[0]); i++)
  {
    xCoords->InsertNextTuple(&tempCoords[i]);
  }
  delete[] tempCoords;
  tempCoords = new float[dimensions[1]];
  this->ReadFloatArray(tempCoords, dimensions[1]);
  for (i = beginDimension[1]; i < (beginDimension[1] + newDimensions[1]); i++)
  {
    yCoords->InsertNextTuple(&tempCoords[i]);
  }
  delete[] tempCoords;
  tempCoords = new float[dimensions[2]];
  this->ReadFloatArray(tempCoords, dimensions[2]);
  for (i = beginDimension[2]; i < (beginDimension[2] + newDimensions[2]); i++)
  {
    zCoords->InsertNextTuple(&tempCoords[i]);
  }
  delete[] tempCoords;

  // Ghost level End
  if (this->GhostLevels > 0)
  {
    output->GetPointData()->AddArray(pointGhostArray);
    output->GetCellData()->AddArray(cellGhostArray);
  }

  if (iblanked)
  {
    vtkWarningMacro("VTK does not handle blanking for rectilinear grids.");
    numPts = dimensions[0] * dimensions[1] * dimensions[2];
    int* tempArray = new int[numPts];
    this->ReadIntArray(tempArray, numPts);
    delete[] tempArray;
  }

  output->SetXCoordinates(xCoords);
  output->SetYCoordinates(yCoords);
  output->SetZCoordinates(zCoords);

  xCoords->Delete();
  yCoords->Delete();
  zCoords->Delete();

  // reading next line to check for EOF
  lineRead = this->ReadLine(line);
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldBinaryReader::CreateImageDataOutput(
  int partId, char line[80], const char* name, vtkMultiBlockDataSet* compositeOutput)
{
  char subLine[80];
  int lineRead;
  int iblanked = 0;
  int dimensions[3];
  float origin[3], delta[3];
  int numPts;

  this->NumberOfNewOutputs++;

  vtkDataSet* ds = this->GetDataSetFromBlock(compositeOutput, partId);
  if (ds == NULL || !ds->IsA("vtkImageData"))
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

  this->ReadIntArray(dimensions, 3);

  int newDimensions[3];
  int splitDimension;
  int splitDimensionBeginIndex;
  vtkUnsignedCharArray* pointGhostArray = NULL;
  vtkUnsignedCharArray* cellGhostArray = NULL;
  if (this->GhostLevels == 0)
  {
    this->PrepareStructuredDimensionsForDistribution(
      partId, dimensions, newDimensions, &splitDimension, &splitDimensionBeginIndex, 0, NULL, NULL);
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

  this->ReadFloatArray(origin, 3);
  this->ReadFloatArray(delta, 3);

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
    vtkWarningMacro("VTK does not handle blanking for image data.");
    numPts = dimensions[0] * dimensions[1] * dimensions[2];
    if (dimensions[0] < 0 || dimensions[0] * (int)sizeof(int) > this->FileSize ||
      dimensions[0] > this->FileSize || dimensions[1] < 0 ||
      dimensions[1] * (int)sizeof(int) > this->FileSize || dimensions[1] > this->FileSize ||
      dimensions[2] < 0 || dimensions[2] * (int)sizeof(int) > this->FileSize ||
      dimensions[2] > this->FileSize || numPts < 0 || numPts * (int)sizeof(int) > this->FileSize ||
      numPts > this->FileSize)
    {
      return -1;
    }
    int* tempArray = new int[numPts];
    this->ReadIntArray(tempArray, numPts);
    delete[] tempArray;
  }

  // reading next line to check for EOF
  lineRead = this->ReadLine(line);
  return lineRead;
}

// Internal function to read in a line up to 80 characters.
// Returns zero if there was an error.
int vtkPEnSightGoldBinaryReader::ReadLine(char result[80])
{
  if (!(this->IFile->read(result, 80).good()))
  {
    // The read fails when reading the last part/array when there are no points.
    // I took out the error macro as a temporary fix.
    // We need to determine what EnSight does when the part with zero point
    // is not the last, and change the read array method.
    // int fixme; // I do not a file to test with yet.
    vtkDebugMacro("Read failed");
    return 0;
  }
  // fix to the memory leakage problem detected by Valgrind
  result[79] = '\0';

  if (this->Fortran)
  {
    strncpy(result, &result[4], 76);
    result[76] = 0;
    // better read an extra 8 bytes to prevent error next time
    char dummy[8];
    if (!(this->IFile->read(dummy, 8).good()))
    {
      vtkDebugMacro("Read (fortran) failed");
      return 0;
    }
  }

  return 1;
}

// Internal function to read a single integer.
// Returns zero if there was an error.
// Sets byte order so that part id is reasonable.
int vtkPEnSightGoldBinaryReader::ReadPartId(int* result)
{
  // first swap like normal.
  if (this->ReadInt(result) == 0)
  {
    vtkErrorMacro("Read failed");
    return 0;
  }

  // second: try an experimental byte swap.
  // Only experiment if byte order is not set.
  if (this->ByteOrder == FILE_UNKNOWN_ENDIAN)
  {
    int tmpLE = *result;
    int tmpBE = *result;
    vtkByteSwap::Swap4LE(&tmpLE);
    vtkByteSwap::Swap4BE(&tmpBE);

    if (tmpLE >= 0 && tmpLE < MAXIMUM_PART_ID)
    {
      this->ByteOrder = FILE_LITTLE_ENDIAN;
      *result = tmpLE;
      return 1;
    }
    if (tmpBE >= 0 && tmpBE < MAXIMUM_PART_ID)
    {
      this->ByteOrder = FILE_BIG_ENDIAN;
      *result = tmpBE;
      return 1;
    }
    vtkErrorMacro("Byte order could not be determined.");
    return 0;
  }

  return 1;
}

// Internal function to read a single integer.
// Returns zero if there was an error.
int vtkPEnSightGoldBinaryReader::ReadInt(int* result)
{
  char dummy[4];
  if (this->Fortran)
  {
    if (!this->IFile->read(dummy, 4).good())
    {
      vtkErrorMacro("Read (fortran) failed.");
      return 0;
    }
  }

  if (!this->IFile->read((char*)result, sizeof(int)).good())
  {
    vtkErrorMacro("Read failed");
    return 0;
  }

  if (this->ByteOrder == FILE_LITTLE_ENDIAN)
  {
    vtkByteSwap::Swap4LE(result);
  }
  else if (this->ByteOrder == FILE_BIG_ENDIAN)
  {
    vtkByteSwap::Swap4BE(result);
  }

  if (this->Fortran)
  {
    if (!this->IFile->read(dummy, 4).good())
    {
      vtkErrorMacro("Read (fortran) failed.");
      return 0;
    }
  }

  return 1;
}

// Internal function to read an integer array.
// Returns zero if there was an error.
int vtkPEnSightGoldBinaryReader::ReadIntArray(int* result, int numInts)
{
  if (numInts <= 0)
  {
    return 1;
  }

  char dummy[4];
  if (this->Fortran)
  {
    if (!this->IFile->read(dummy, 4).good())
    {
      vtkErrorMacro("Read (fortran) failed.");
      return 0;
    }
  }

  if (!this->IFile->read((char*)result, sizeof(int) * numInts).good())
  {
    vtkErrorMacro("Read failed.");
    return 0;
  }

  if (this->ByteOrder == FILE_LITTLE_ENDIAN)
  {
    vtkByteSwap::Swap4LERange(result, numInts);
  }
  else
  {
    vtkByteSwap::Swap4BERange(result, numInts);
  }

  if (this->Fortran)
  {
    if (!this->IFile->read(dummy, 4).good())
    {
      vtkErrorMacro("Read (fortran) failed.");
      return 0;
    }
  }

  return 1;
}

// Internal function to read a float array.
// Returns zero if there was an error.
int vtkPEnSightGoldBinaryReader::ReadFloatArray(float* result, int numFloats)
{
  if (numFloats <= 0)
  {
    return 1;
  }

  char dummy[4];
  if (this->Fortran)
  {
    if (!this->IFile->read(dummy, 4).good())
    {
      vtkErrorMacro("Read (fortran) failed.");
      return 0;
    }
  }

  if (!this->IFile->read((char*)result, sizeof(float) * numFloats).good())
  {
    vtkErrorMacro("Read failed");
    return 0;
  }

  if (this->ByteOrder == FILE_LITTLE_ENDIAN)
  {
    vtkByteSwap::Swap4LERange(result, numFloats);
  }
  else
  {
    vtkByteSwap::Swap4BERange(result, numFloats);
  }

  if (this->Fortran)
  {
    if (!this->IFile->read(dummy, 4).good())
    {
      vtkErrorMacro("Read (fortran) failed.");
      return 0;
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldBinaryReader::ReadOrSkipCoordinates(
  vtkPoints* points, long offset, int partId, bool skip)
{
  if (offset == -1)
  {
    return 0;
  }

  this->IFile->seekg(offset);

  int numPtsTmp;
  this->ReadInt(&numPtsTmp);
  vtkIdType numPts = numPtsTmp;
  if (numPts < 0 || numPts > this->FileSize || numPts * (int)sizeof(int) > this->FileSize)
  {
    vtkErrorMacro(
      "Invalid number of unstructured points read; check that ByteOrder is set correctly.");
    return -1;
  }

  vtkDebugMacro("num. points: " << numPts);

  if (this->NodeIdsListed)
  {
    this->IFile->seekg(sizeof(int) * numPts, ios::cur);
  }

  long currentPositionInFile = this->IFile->tellg();

  this->FloatBufferFilePosition = currentPositionInFile;
  this->FloatBufferIndexBegin = 0;
  this->FloatBufferNumberOfVectors = numPts;
  this->UpdateFloatBuffer();

  // Position to reach at the end of this method
  long endFilePosition = currentPositionInFile + 3 * numPts * (long)sizeof(float);
  if (this->Fortran)
    endFilePosition += 24; // 4 * (begin + end) * number of components (3)

  // Only to quickly visualize who reads what...
  if (skip)
  {
    // Inject real Number of points, as we cannot take the size of the vector as a reference
    // numPts comes from the file, it cannot be wrong
    // Needed in nfaced...
    this->GetPointIds(partId)->SetNumberOfIds(numPts);

    this->IFile->seekg(endFilePosition);

    return 0;
  }
  else
  {
    if (this->GetPointIds(partId)->GetNumberOfIds() == 0)
    {
      // No Point was injected at all For this Part. There is clearly a problem...
      // TODO: Do something ?
      // delete [] xCoords;
      // delete [] yCoords;
      // delete [] zCoords;
      this->IFile->seekg(endFilePosition);
      return 0;
    }
    else
    {
      // Inject really needed points
      vtkIdType i;
      int localNumberOfIds = this->GetPointIds(partId)->GetLocalNumberOfIds();
      points->Allocate(localNumberOfIds);
      points->SetNumberOfPoints(localNumberOfIds);
      int maxId = -1;
      int minId = -1;
      for (i = 0; i < numPts; i++)
      {
        float vec[3];
        int id = this->GetPointIds(partId)->GetId(i);
        if (id != -1)
        {
          if ((minId == -1) || (minId > id))
            minId = id;
          if ((maxId == -1) || (maxId < id))
            maxId = id;
          this->GetVectorFromFloatBuffer(i, vec);
          points->SetPoint(id, vec[0], vec[1], vec[2]);
        }
      }

      // Inject real Number of points, as we cannot take the size of the vector as a reference
      // numPts comes from the file, it cannot be wrong
      // Needed in nfaced...
      // In case read has never been skipped, we do this again here
      this->GetPointIds(partId)->SetNumberOfIds(numPts);

      // delete [] xCoords;
      // delete [] yCoords;
      // delete [] zCoords;
      this->IFile->seekg(endFilePosition);
      return localNumberOfIds;
    }
  }
}

//----------------------------------------------------------------------------
int vtkPEnSightGoldBinaryReader::InjectCoordinatesAtEnd(
  vtkUnstructuredGrid* output, long coordinatesOffset, int partId)
{
  bool eof = false;

  if (this->IFile->eof())
    eof = true;

  if (eof)
  {
    // remove the EOF flag to read back coordinates
    this->IFile->clear();
  }

  long currentFilePosition = this->IFile->tellg();
  vtkPoints* points = vtkPoints::New();
  int pointsRead = this->ReadOrSkipCoordinates(points, coordinatesOffset, partId, false);
  this->IFile->seekg(currentFilePosition);
  if (pointsRead == -1)
  {
    return -1;
  }
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

  if (eof)
  {
    // put the EOF flag back
    this->IFile->peek();
  }

  return pointsRead;
}

//----------------------------------------------------------------------------
void vtkPEnSightGoldBinaryReader::GetVectorFromFloatBuffer(vtkIdType i, float* vector)
{
  // We assume FloatBufferIndexBegin, FloatBufferFilePosition, and FloatBufferNumberOfVectors
  // were previously set.
  vtkIdType closestBufferBegin = (i / this->FloatBufferSize) * this->FloatBufferSize;
  if ((this->FloatBufferIndexBegin == -1) || (closestBufferBegin != this->FloatBufferIndexBegin))
  {
    this->FloatBufferIndexBegin = closestBufferBegin;
    this->UpdateFloatBuffer();
  }

  vtkIdType index = i - this->FloatBufferIndexBegin;
  vector[0] = this->FloatBuffer[0][index];
  vector[1] = this->FloatBuffer[1][index];
  vector[2] = this->FloatBuffer[2][index];
}

//----------------------------------------------------------------------------
void vtkPEnSightGoldBinaryReader::UpdateFloatBuffer()
{
  long currentPosition = this->IFile->tellg();

  vtkIdType sizeToRead;
  if (this->FloatBufferIndexBegin + this->FloatBufferSize > this->FloatBufferNumberOfVectors)
  {
    sizeToRead = this->FloatBufferNumberOfVectors - this->FloatBufferIndexBegin;
  }
  else
  {
    sizeToRead = this->FloatBufferSize;
  }

  for (vtkIdType i = 0; i < 3; i++)
  {
    // We cannot use ReadFloatArray method, because Fortran format has dummy things
    if (this->Fortran)
    {
      this->IFile->seekg(this->FloatBufferFilePosition + 4 +
        i * (this->FloatBufferNumberOfVectors * sizeof(float) + 8) +
        this->FloatBufferIndexBegin * sizeof(float));
    }
    else
    {
      vtkIdType ost = this->FloatBufferFilePosition +
        i * this->FloatBufferNumberOfVectors * sizeof(float) +
        this->FloatBufferIndexBegin * sizeof(float);
      this->IFile->seekg(ost);
      if (this->IFile->good() != true)
      {
        vtkErrorMacro("File seek failed");
      }
    }
    if (!this->IFile->read((char*)this->FloatBuffer[i], sizeof(float) * sizeToRead).good())
    {
      vtkErrorMacro("Read failed");
    }

    if (this->ByteOrder == FILE_LITTLE_ENDIAN)
    {
      vtkByteSwap::Swap4LERange(this->FloatBuffer[i], sizeToRead);
    }
    else
    {
      vtkByteSwap::Swap4BERange(this->FloatBuffer[i], sizeToRead);
    }
  }

  this->IFile->seekg(currentPosition);
}

//----------------------------------------------------------------------------
void vtkPEnSightGoldBinaryReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
