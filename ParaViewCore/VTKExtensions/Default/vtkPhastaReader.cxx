/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPhastaReader.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPhastaReader.h"

#include "vtkByteSwap.h"
#include "vtkCellData.h"
#include "vtkCellType.h" //added for constants such as VTK_TETRA etc...
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkPhastaReader);

vtkCxxSetObjectMacro(vtkPhastaReader, CachedGrid, vtkUnstructuredGrid);

#include <map>
#include <sstream>
#include <string>
#include <vector>

struct vtkPhastaReaderInternal
{
  struct FieldInfo
  {
    int StartIndexInPhastaArray;
    int NumberOfComponents;
    int DataDependency;   // 0-nodal, 1-elemental
    std::string DataType; // "int" or "double"
    std::string PhastaFieldTag;

    FieldInfo()
      : StartIndexInPhastaArray(-1)
      , NumberOfComponents(-1)
      , DataDependency(-1)
      , DataType("")
      , PhastaFieldTag("")
    {
    }
  };

  typedef std::map<std::string, FieldInfo> FieldInfoMapType;
  FieldInfoMapType FieldInfoMap;
};

// Begin of copy from phastaIO

#define swap_char(A, B)                                                                            \
  {                                                                                                \
    ucTmp = A;                                                                                     \
    A = B;                                                                                         \
    B = ucTmp;                                                                                     \
  }

std::map<int, char*> LastHeaderKey;
std::vector<FILE*> fileArray;
std::vector<int> byte_order;
std::vector<int> header_type;
int DataSize = 0;
int LastHeaderNotFound = 0;
int Wrong_Endian = 0;
int Strict_Error = 0;
int binary_format = 0;

// the caller has the responsibility to delete the returned string
char* vtkPhastaReader::StringStripper(const char istring[])
{
  size_t length = strlen(istring);
  char* dest = new char[length + 1];
  strcpy(dest, istring);
  dest[length] = '\0';

  if (char* p = strpbrk(dest, " "))
  {
    *p = '\0';
  }

  return dest;
}

int vtkPhastaReader::cscompare(const char teststring[], const char targetstring[])
{

  char* s1 = const_cast<char*>(teststring);
  char* s2 = const_cast<char*>(targetstring);

  while (*s1 == ' ')
  {
    s1++;
  }
  while (*s2 == ' ')
  {
    s2++;
  }
  while ((*s1) && (*s2) && (*s2 != '?') && (tolower(*s1) == tolower(*s2)))
  {
    s1++;
    s2++;
    while (*s1 == ' ')
    {
      s1++;
    }
    while (*s2 == ' ')
    {
      s2++;
    }
  }
  if (!(*s1) || (*s1 == '?'))
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

void vtkPhastaReader::isBinary(const char iotype[])
{
  char* fname = StringStripper(iotype);
  if (cscompare(fname, "binary"))
  {
    binary_format = 1;
  }
  else
  {
    binary_format = 0;
  }
  delete[] fname;
}

size_t vtkPhastaReader::typeSize(const char typestring[])
{
  char* ts1 = StringStripper(typestring);

  if (cscompare("integer", ts1))
  {
    delete[] ts1;
    return sizeof(int);
  }
  else if (cscompare("double", ts1))
  {
    delete[] ts1;
    return sizeof(double);
  }
  else if (cscompare("float", ts1))
  {
    delete[] ts1;
    return sizeof(float);
  }
  else
  {
    vtkGenericWarningMacro(<< "unknown type : " << ts1 << endl);
    delete[] ts1;
    return 0;
  }
}

namespace
{
void xfgets(char* str, int num, FILE* stream)
{
  if (fgets(str, num, stream) == NULL)
  {
    vtkGenericWarningMacro(<< "Could not read or end of file" << endl);
  }
}

void xfread(void* ptr, size_t size, size_t count, FILE* stream)
{
  if (fread(ptr, size, count, stream) != count)
  {
    vtkGenericWarningMacro(<< "Could not read or end of file" << endl);
  }
}

template <typename... Args>
void xfscanf(FILE* stream, const char* format, Args... args)
{
  int ret = fscanf(stream, format, args...);
  (void)ret;
}
}

int vtkPhastaReader::readHeader(FILE* fileObject, const char phrase[], int* params, int expect)
{
  char* text_header;
  char* token;
  char Line[1024];
  char junk;
  int FOUND = 0;
  size_t real_length;
  int skip_size, integer_value;
  int rewind_count = 0;

  if (!fgets(Line, 1024, fileObject) && feof(fileObject))
  {
    rewind(fileObject);
    clearerr(fileObject);
    rewind_count++;

    xfgets(Line, 1024, fileObject);
  }

  while (!FOUND && (rewind_count < 2))
  {
    if ((Line[0] != '\n') && (real_length = strcspn(Line, "#")))
    {
      text_header = new char[real_length + 1];
      strncpy(text_header, Line, real_length);
      text_header[real_length] = static_cast<char>(NULL);
      token = strtok(text_header, ":");
      if (cscompare(phrase, token))
      {
        FOUND = 1;
        token = strtok(NULL, " ,;<>");
        skip_size = atoi(token);
        int i;
        for (i = 0; i < expect && (token = strtok(NULL, " ,;<>")); i++)
        {
          params[i] = atoi(token);
        }
        if (i < expect)
        {
          vtkGenericWarningMacro(<< "Expected # of ints not found for: " << phrase << endl);
        }
      }
      else if (cscompare(token, "byteorder magic number"))
      {
        if (binary_format)
        {
          xfread((void*)&integer_value, sizeof(int), 1, fileObject);
          xfread(&junk, sizeof(char), 1, fileObject);
          if (362436 != integer_value)
          {
            Wrong_Endian = 1;
          }
        }
        else
        {
          xfscanf(fileObject, "%d\n", &integer_value);
        }
      }
      else
      {
        /* some other header, so just skip over */
        token = strtok(NULL, " ,;<>");
        skip_size = atoi(token);
        if (binary_format)
        {
          fseek(fileObject, skip_size, SEEK_CUR);
        }
        else
        {
          for (int gama = 0; gama < skip_size; gama++)
          {
            xfgets(Line, 1024, fileObject);
          }
        }
      }
      delete[] text_header;
    }

    if (!FOUND)
    {
      if (!fgets(Line, 1024, fileObject) && feof(fileObject))
      {
        rewind(fileObject);
        clearerr(fileObject);
        rewind_count++;
        xfgets(Line, 1024, fileObject);
      }
    }
  }

  if (!FOUND)
  {
    vtkGenericWarningMacro(<< "Could not find: " << phrase << endl);
    return 1;
  }
  return 0;
}

void vtkPhastaReader::SwapArrayByteOrder(void* array, int nbytes, int nItems)
{
  /* This swaps the byte order for the array of nItems each
     of size nbytes , This will be called only locally  */
  int i, j;
  unsigned char ucTmp;
  unsigned char* ucDst = (unsigned char*)array;

  for (i = 0; i < nItems; i++)
  {
    for (j = 0; j < (nbytes / 2); j++)
    {
      swap_char(ucDst[j], ucDst[(nbytes - 1) - j]);
    }
    ucDst += nbytes;
  }
}

void vtkPhastaReader::openfile(const char filename[], const char mode[], int* fileDescriptor)
{
  FILE* file = NULL;
  *fileDescriptor = 0;
  // Stripping a filename is not correct, since
  // filenames can certainly have spaces.
  // char* fname = StringStripper( filename );
  const char* fname = filename;
  char* imode = StringStripper(mode);

  if (cscompare("read", imode))
  {
    file = fopen(fname, "rb");
  }
  else if (cscompare("write", imode))
  {
    file = fopen(fname, "wb");
  }
  else if (cscompare("append", imode))
  {
    file = fopen(fname, "ab");
  }

  if (!file)
  {
    vtkGenericWarningMacro(<< "unable to open file : " << fname << endl);
  }
  else
  {
    fileArray.push_back(file);
    byte_order.push_back(0);
    header_type.push_back(sizeof(int));
    *fileDescriptor = static_cast<int>(fileArray.size());
  }
  delete[] imode;
}

void vtkPhastaReader::closefile(int* fileDescriptor, const char mode[])
{
  char* imode = StringStripper(mode);

  if (cscompare("write", imode) || cscompare("append", imode))
  {
    fflush(fileArray[*fileDescriptor - 1]);
  }

  fclose(fileArray[*fileDescriptor - 1]);
  delete[] imode;
}

void vtkPhastaReader::readheader(int* fileDescriptor, const char keyphrase[], void* valueArray,
  int* nItems, const char datatype[], const char iotype[])
{
  int filePtr = *fileDescriptor - 1;
  FILE* fileObject;
  int* valueListInt;

  if (*fileDescriptor < 1 || *fileDescriptor > (int)fileArray.size())
  {
    vtkGenericWarningMacro(<< "No file associated with Descriptor " << *fileDescriptor << "\n"
                           << "openfile function has to be called before \n"
                           << "accessing the file\n "
                           << "fatal error: cannot continue, returning out of call\n");
    return;
  }

  LastHeaderKey[filePtr] = const_cast<char*>(keyphrase);
  LastHeaderNotFound = 0;

  fileObject = fileArray[filePtr];
  Wrong_Endian = byte_order[filePtr];

  isBinary(iotype);
  typeSize(datatype); // redundant call, just avoid a compiler warning.

  // right now we are making the assumption that we will only write integers
  // on the header line.

  valueListInt = static_cast<int*>(valueArray);
  int ierr = readHeader(fileObject, keyphrase, valueListInt, *nItems);

  byte_order[filePtr] = Wrong_Endian;

  if (ierr)
  {
    LastHeaderNotFound = 1;
  }

  return;
}

void vtkPhastaReader::readdatablock(int* fileDescriptor, const char keyphrase[], void* valueArray,
  int* nItems, const char datatype[], const char iotype[])
{
  int filePtr = *fileDescriptor - 1;
  FILE* fileObject;
  char junk;

  if (*fileDescriptor < 1 || *fileDescriptor > (int)fileArray.size())
  {
    vtkGenericWarningMacro(<< "No file associated with Descriptor " << *fileDescriptor << "\n"
                           << "openfile function has to be called before \n"
                           << "accessing the file\n "
                           << "fatal error: cannot continue, returning out of call\n");
    return;
  }

  // error check..
  // since we require that a consistent header always precede the data block
  // let us check to see that it is actually the case.

  if (!cscompare(LastHeaderKey[filePtr], keyphrase))
  {
    vtkGenericWarningMacro(<< "Header not consistent with data block\n"
                           << "Header: " << LastHeaderKey[filePtr] << "\n"
                           << "DataBlock: " << keyphrase << "\n"
                           << "Please recheck read sequence \n") if (Strict_Error)
    {
      vtkGenericWarningMacro(<< "fatal error: cannot continue, returning out of call\n");
      return;
    }
  }

  if (LastHeaderNotFound)
  {
    return;
  }

  fileObject = fileArray[filePtr];
  Wrong_Endian = byte_order[filePtr];

  size_t type_size = typeSize(datatype);
  int nUnits = *nItems;
  isBinary(iotype);

  if (binary_format)
  {
    xfread(valueArray, type_size, nUnits, fileObject);
    xfread(&junk, sizeof(char), 1, fileObject);
    if (Wrong_Endian)
    {
      SwapArrayByteOrder(valueArray, static_cast<int>(type_size), nUnits);
    }
  }
  else
  {
    char* ts1 = StringStripper(datatype);
    if (cscompare("integer", ts1))
    {
      for (int n = 0; n < nUnits; n++)
      {
        xfscanf(fileObject, "%d\n", (int*)((int*)valueArray + n));
      }
    }
    else if (cscompare("double", ts1))
    {
      for (int n = 0; n < nUnits; n++)
      {
        xfscanf(fileObject, "%lf\n", (double*)((double*)valueArray + n));
      }
    }
    delete[] ts1;
  }

  return;
}

// End of copy from phastaIO

vtkPhastaReader::vtkPhastaReader()
{
  this->GeometryFileName = NULL;
  this->FieldFileName = NULL;
  this->SetNumberOfInputPorts(0);
  this->Internal = new vtkPhastaReaderInternal;
  this->CachedGrid = 0;
}

vtkPhastaReader::~vtkPhastaReader()
{
  if (this->GeometryFileName)
  {
    delete[] this->GeometryFileName;
  }
  if (this->FieldFileName)
  {
    delete[] this->FieldFileName;
  }
  delete this->Internal;
  this->SetCachedGrid(0);
}

void vtkPhastaReader::ClearFieldInfo()
{
  this->Internal->FieldInfoMap.clear();
}

void vtkPhastaReader::SetFieldInfo(const char* paraviewFieldTag, const char* phastaFieldTag,
  int index, int numOfComps, int dataDependency, const char* dataType)
{
  vtkPhastaReaderInternal::FieldInfo& info = this->Internal->FieldInfoMap[paraviewFieldTag];

  info.PhastaFieldTag = phastaFieldTag;
  info.StartIndexInPhastaArray = index;
  info.NumberOfComponents = numOfComps;
  info.DataDependency = dataDependency;
  info.DataType = dataType;
}

int vtkPhastaReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  int firstVertexNo = 0;
  int fvn = 0;
  int noOfNodes, noOfCells, noOfDatas;

  // get the data object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (this->GetCachedGrid())
  {
    // shallow the cached grid that was previously set...
    vtkDebugMacro("Using a cached copy of the grid.");
    output->ShallowCopy(this->GetCachedGrid());
  }
  else
  {
    vtkPoints* points;

    output->Allocate(10000, 2100);

    points = vtkPoints::New();

    vtkDebugMacro(<< "Reading Phasta file...");

    if (!this->GeometryFileName || !this->FieldFileName)
    {
      vtkErrorMacro(<< "All input parameters not set.");
      return 0;
    }
    vtkDebugMacro(<< "Updating ensa with ....");
    vtkDebugMacro(<< "Geom File : " << this->GeometryFileName);
    vtkDebugMacro(<< "Field File : " << this->FieldFileName);

    fvn = firstVertexNo;
    this->ReadGeomFile(this->GeometryFileName, firstVertexNo, points, noOfNodes, noOfCells);
    /* set the points over here, this is because vtkUnStructuredGrid
       only insert points once, next insertion overwrites the previous one */
    // acbauer is not sure why the above comment is about...
    output->SetPoints(points);
    points->Delete();
  }

  if (!this->Internal->FieldInfoMap.size())
  {
    vtkDataSetAttributes* field = output->GetPointData();
    this->ReadFieldFile(this->FieldFileName, fvn, field, noOfNodes);
  }
  else
  {
    this->ReadFieldFile(this->FieldFileName, fvn, output, noOfDatas);
  }

  // if there exists point arrays called coordsX, coordsY and coordsZ,
  // create another array of point data and set the output to use this
  vtkPointData* pointData = output->GetPointData();
  vtkDoubleArray* coordsX = vtkDoubleArray::SafeDownCast(pointData->GetArray("coordsX"));
  vtkDoubleArray* coordsY = vtkDoubleArray::SafeDownCast(pointData->GetArray("coordsY"));
  vtkDoubleArray* coordsZ = vtkDoubleArray::SafeDownCast(pointData->GetArray("coordsZ"));
  if (coordsX && coordsY && coordsZ)
  {
    vtkIdType numPoints = output->GetPoints()->GetNumberOfPoints();
    if (numPoints != coordsX->GetNumberOfTuples() || numPoints != coordsY->GetNumberOfTuples() ||
      numPoints != coordsZ->GetNumberOfTuples())
    {
      vtkWarningMacro("Wrong number of points for moving mesh.  Using original points.");
      return 0;
    }
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    points->DeepCopy(output->GetPoints());
    for (vtkIdType i = 0; i < numPoints; i++)
    {
      points->SetPoint(i, coordsX->GetValue(i), coordsY->GetValue(i), coordsZ->GetValue(i));
    }
    output->SetPoints(points);
  }

  return 1;
}

/* firstVertexNo is useful when reading multiple geom files and coalescing
   them into one, ReadGeomfile can then be called repeatedly from Execute with
   firstVertexNo forming consecutive series of vertex numbers */

void vtkPhastaReader::ReadGeomFile(
  char* geomFileName, int& firstVertexNo, vtkPoints* points, int& num_nodes, int& num_cells)
{

  /* variables for vtk */
  vtkUnstructuredGrid* output = this->GetOutput();
  double* coordinates;
  vtkIdType* nodes;
  int cell_type;

  //  int num_tpblocks;
  /* variables for the geom data file */
  /* nodal information */
  // int byte_order;
  // int data[11], data1[7];
  int dim;
  int num_int_blocks;
  double* pos;
  // int *nlworkdata;
  /* element information */
  int num_elems, num_vertices, num_per_line;
  int* connectivity = NULL;

  /* misc variables*/
  int i, j, k, item;
  int geomfile;

  openfile(geomFileName, "read", &geomfile);
  // geomfile = fopen(GeometryFileName,"rb");

  if (!geomfile)
  {
    vtkErrorMacro(<< "Cannot open file " << geomFileName);
    return;
  }

  int expect;
  int array[10];
  expect = 1;

  /* read number of nodes */

  readheader(&geomfile, "number of nodes", array, &expect, "integer", "binary");
  num_nodes = array[0];

  /* read number of elements */
  readheader(&geomfile, "number of interior elements", array, &expect, "integer", "binary");
  num_elems = array[0];
  num_cells = array[0];

  /* read number of interior */
  readheader(&geomfile, "number of interior tpblocks", array, &expect, "integer", "binary");
  num_int_blocks = array[0];

  vtkDebugMacro(<< "Nodes: " << num_nodes << "Elements: " << num_elems
                << "tpblocks: " << num_int_blocks);

  /* read coordinates */
  expect = 2;
  readheader(&geomfile, "co-ordinates", array, &expect, "double", "binary");
  // TEST *******************
  num_nodes = array[0];
  // TEST *******************
  if (num_nodes != array[0])
  {
    vtkErrorMacro(<< "Ambiguous information in geom.data file, number of nodes does not match the "
                     "co-ordinates size. Nodes: "
                  << num_nodes << " Coordinates: " << array[0]);
    return;
  }
  dim = array[1];

  /* read the coordinates */

  coordinates = new double[dim];
  if (coordinates == NULL)
  {
    vtkErrorMacro(<< "Unable to allocate memory for nodal info");
    return;
  }

  pos = new double[num_nodes * dim];
  if (pos == NULL)
  {
    vtkErrorMacro(<< "Unable to allocate memory for nodal info");
    delete[] coordinates;
    return;
  }

  item = num_nodes * dim;
  readdatablock(&geomfile, "co-ordinates", pos, &item, "double", "binary");

  for (i = 0; i < num_nodes; i++)
  {
    for (j = 0; j < dim; j++)
    {
      coordinates[j] = pos[j * num_nodes + i];
    }
    switch (dim)
    {
      case 1:
        points->InsertPoint(i + firstVertexNo, coordinates[0], 0, 0);
        break;
      case 2:
        points->InsertPoint(i + firstVertexNo, coordinates[0], coordinates[1], 0);
        break;
      case 3:
        points->InsertNextPoint(coordinates);
        break;
      default:
        vtkErrorMacro(<< "Unrecognized dimension in " << geomFileName) return;
    }
  }

  /* read the connectivity information */
  expect = 7;

  for (k = 0; k < num_int_blocks; k++)
  {
    readheader(&geomfile, "connectivity interior", array, &expect, "integer", "binary");

    /* read information about the block*/
    num_elems = array[0];
    num_vertices = array[1];
    num_per_line = array[3];
    connectivity = new int[num_elems * num_per_line];

    if (connectivity == NULL)
    {
      vtkErrorMacro(<< "Unable to allocate memory for connectivity info");
      return;
    }

    item = num_elems * num_per_line;
    readdatablock(&geomfile, "connectivity interior", connectivity, &item, "integer", "binary");

    /* insert cells */
    for (i = 0; i < num_elems; i++)
    {
      nodes = new vtkIdType[num_vertices];

      // connectivity starts from 1 so node[j] will never be -ve
      for (j = 0; j < num_vertices; j++)
      {
        nodes[j] = connectivity[i + num_elems * j] + firstVertexNo - 1;
      }

      /* 1 is subtracted from the connectivity info to reflect that in vtk
         vertex  numbering start from 0 as opposed to 1 in geomfile */

      // find out element type
      switch (num_vertices)
      {
        case 4:
          cell_type = VTK_TETRA;
          break;
        case 5:
          cell_type = VTK_PYRAMID;
          break;
        case 6:
          cell_type = VTK_WEDGE;
          break;
        case 8:
          cell_type = VTK_HEXAHEDRON;

          break;
        default:
          delete[] nodes;
          vtkErrorMacro(<< "Unrecognized CELL_TYPE in " << geomFileName);
          return;
      }

      /* insert the element */
      output->InsertNextCell(cell_type, num_vertices, nodes);
      delete[] nodes;
    }
  }
  // update the firstVertexNo so that next slice/partition can be read
  firstVertexNo = firstVertexNo + num_nodes;

  // clean up
  closefile(&geomfile, "read");
  delete[] coordinates;
  delete[] pos;
  delete[] connectivity;
}

void vtkPhastaReader::ReadFieldFile(
  char* fieldFileName, int, vtkDataSetAttributes* field, int& noOfNodes)
{

  int i, j;
  int item;
  double* data;
  int fieldfile;

  openfile(fieldFileName, "read", &fieldfile);
  // fieldfile = fopen(FieldFileName,"rb");

  if (!fieldfile)
  {
    vtkErrorMacro(<< "Cannot open file " << FieldFileName) return;
  }
  int array[10], expect;

  /* read the solution */
  vtkDoubleArray* pressure = vtkDoubleArray::New();
  pressure->SetName("pressure");
  vtkDoubleArray* velocity = vtkDoubleArray::New();
  velocity->SetName("velocity");
  velocity->SetNumberOfComponents(3);
  vtkDoubleArray* temperature = vtkDoubleArray::New();
  temperature->SetName("temperature");

  expect = 3;
  readheader(&fieldfile, "solution", array, &expect, "double", "binary");
  noOfNodes = array[0];
  this->NumberOfVariables = array[1];

  vtkDoubleArray* sArrays[4];
  for (i = 0; i < 4; i++)
  {
    sArrays[i] = 0;
  }
  item = noOfNodes * this->NumberOfVariables;
  data = new double[item];
  if (data == NULL)
  {
    vtkErrorMacro(<< "Unable to allocate memory for field info");
    return;
  }

  readdatablock(&fieldfile, "solution", data, &item, "double", "binary");

  for (i = 5; i < this->NumberOfVariables; i++)
  {
    int idx = i - 5;
    sArrays[idx] = vtkDoubleArray::New();
    std::ostringstream aName;
    aName << "s" << idx + 1 << ends;
    sArrays[idx]->SetName(aName.str().c_str());
    sArrays[idx]->SetNumberOfTuples(noOfNodes);
  }

  pressure->SetNumberOfTuples(noOfNodes);
  velocity->SetNumberOfTuples(noOfNodes);
  temperature->SetNumberOfTuples(noOfNodes);
  for (i = 0; i < noOfNodes; i++)
  {
    pressure->SetTuple1(i, data[i]);
    velocity->SetTuple3(i, data[noOfNodes + i], data[2 * noOfNodes + i], data[3 * noOfNodes + i]);
    temperature->SetTuple1(i, data[4 * noOfNodes + i]);
    for (j = 5; j < this->NumberOfVariables; j++)
    {
      sArrays[j - 5]->SetTuple1(i, data[j * noOfNodes + i]);
    }
  }

  field->AddArray(pressure);
  field->SetActiveScalars("pressure");
  pressure->Delete();
  field->AddArray(velocity);
  field->SetActiveVectors("velocity");
  velocity->Delete();
  field->AddArray(temperature);
  temperature->Delete();

  for (i = 5; i < this->NumberOfVariables; i++)
  {
    int idx = i - 5;
    field->AddArray(sArrays[idx]);
    sArrays[idx]->Delete();
  }

  // clean up
  closefile(&fieldfile, "read");
  delete[] data;

} // closes ReadFieldFile

void vtkPhastaReader::ReadFieldFile(
  char* fieldFileName, int, vtkUnstructuredGrid* output, int& noOfDatas)
{

  int i, j, numOfVars;
  int item;
  int fieldfile;

  openfile(fieldFileName, "read", &fieldfile);
  // fieldfile = fopen(FieldFileName,"rb");

  if (!fieldfile)
  {
    vtkErrorMacro(<< "Cannot open file " << FieldFileName) return;
  }
  int array[10], expect;

  int activeScalars = 0, activeTensors = 0;

  vtkPhastaReaderInternal::FieldInfoMapType::iterator it = this->Internal->FieldInfoMap.begin();
  vtkPhastaReaderInternal::FieldInfoMapType::iterator itend = this->Internal->FieldInfoMap.end();
  for (; it != itend; it++)
  {
    const char* paraviewFieldTag = it->first.c_str();
    const char* phastaFieldTag = it->second.PhastaFieldTag.c_str();
    int index = it->second.StartIndexInPhastaArray;
    int numOfComps = it->second.NumberOfComponents;
    int dataDependency = it->second.DataDependency;
    const char* dataType = it->second.DataType.c_str();

    vtkDataSetAttributes* field;
    if (dataDependency)
      field = output->GetCellData();
    else
      field = output->GetPointData();

    // void *data;
    int dtype; // (0=double, 1=float)
    vtkDataArray* dataArray;
    /* read the field data */
    if (strcmp(dataType, "double") == 0)
    {
      dataArray = vtkDoubleArray::New();
      dtype = 0;
    }
    else if (strcmp(dataType, "float") == 0)
    {
      dataArray = vtkFloatArray::New();
      dtype = 1;
    }
    else
    {
      vtkErrorMacro("Data type [" << dataType << "] NOT supported");
      continue;
    }

    dataArray->SetName(paraviewFieldTag);
    dataArray->SetNumberOfComponents(numOfComps);

    expect = 3;
    readheader(&fieldfile, phastaFieldTag, array, &expect, dataType, "binary");
    noOfDatas = array[0];
    this->NumberOfVariables = array[1];
    numOfVars = array[1];
    dataArray->SetNumberOfTuples(noOfDatas);

    if (index < 0 || index > numOfVars - 1)
    {
      vtkErrorMacro("index [" << index << "] is out of range [num. of vars.:" << numOfVars
                              << "] for field [paraview field tag:" << paraviewFieldTag
                              << ", phasta field tag:" << phastaFieldTag << "]");

      dataArray->Delete();
      continue;
    }

    if (numOfComps < 0 || index + numOfComps > numOfVars)
    {
      vtkErrorMacro("index [" << index << "] with num. of comps. [" << numOfComps
                              << "] is out of range [num. of vars.:" << numOfVars
                              << "] for field [paraview field tag:" << paraviewFieldTag
                              << ", phasta field tag:" << phastaFieldTag << "]");

      dataArray->Delete();
      continue;
    }

    item = numOfVars * noOfDatas;
    if (dtype == 0)
    { // data is type double

      double* data;
      data = new double[item];

      if (data == NULL)
      {
        vtkErrorMacro(<< "Unable to allocate memory for field info");

        dataArray->Delete();
        continue;
      }

      readdatablock(&fieldfile, phastaFieldTag, data, &item, dataType, "binary");

      switch (numOfComps)
      {
        case 1:
        {
          int offset = index * noOfDatas;

          if (!activeScalars)
            field->SetActiveScalars(paraviewFieldTag);
          else
            activeScalars = 1;
          for (i = 0; i < noOfDatas; i++)
          {
            dataArray->SetTuple1(i, data[offset + i]);
          }
        }
        break;
        case 3:
        {
          int offset[3];
          for (j = 0; j < 3; j++)
            offset[j] = (index + j) * noOfDatas;

          if (!activeScalars)
            field->SetActiveVectors(paraviewFieldTag);
          else
            activeScalars = 1;
          for (i = 0; i < noOfDatas; i++)
          {
            dataArray->SetTuple3(i, data[offset[0] + i], data[offset[1] + i], data[offset[2] + i]);
          }
        }
        break;
        case 9:
        {
          int offset[9];
          for (j = 0; j < 9; j++)
            offset[j] = (index + j) * noOfDatas;

          if (!activeTensors)
            field->SetActiveTensors(paraviewFieldTag);
          else
            activeTensors = 1;
          for (i = 0; i < noOfDatas; i++)
          {
            dataArray->SetTuple9(i, data[offset[0] + i], data[offset[1] + i], data[offset[2] + i],
              data[offset[3] + i], data[offset[4] + i], data[offset[5] + i], data[offset[6] + i],
              data[offset[7] + i], data[offset[8] + i]);
          }
        }
        break;
        default:
          vtkErrorMacro("number of components [" << numOfComps << "] NOT supported");

          dataArray->Delete();
          delete[] data;
          continue;
      }

      // clean up
      delete[] data;
    }
    else if (dtype == 1)
    { // data is type float

      float* data;
      data = new float[item];

      if (data == NULL)
      {
        vtkErrorMacro(<< "Unable to allocate memory for field info");

        dataArray->Delete();
        continue;
      }

      readdatablock(&fieldfile, phastaFieldTag, data, &item, dataType, "binary");

      switch (numOfComps)
      {
        case 1:
        {
          int offset = index * noOfDatas;

          if (!activeScalars)
            field->SetActiveScalars(paraviewFieldTag);
          else
            activeScalars = 1;
          for (i = 0; i < noOfDatas; i++)
          {
            // double tmpval = (double) data[offset+i];
            // dataArray->SetTuple1(i, tmpval);
            dataArray->SetTuple1(i, data[offset + i]);
          }
        }
        break;
        case 3:
        {
          int offset[3];
          for (j = 0; j < 3; j++)
            offset[j] = (index + j) * noOfDatas;

          if (!activeScalars)
            field->SetActiveVectors(paraviewFieldTag);
          else
            activeScalars = 1;
          for (i = 0; i < noOfDatas; i++)
          {
            // double tmpval[3];
            // for(j=0;j<3;j++)
            //   tmpval[j] = (double) data[offset[j]+i];
            // dataArray->SetTuple3(i,
            //                    tmpval[0], tmpval[1], tmpval[3]);
            dataArray->SetTuple3(i, data[offset[0] + i], data[offset[1] + i], data[offset[2] + i]);
          }
        }
        break;
        case 9:
        {
          int offset[9];
          for (j = 0; j < 9; j++)
            offset[j] = (index + j) * noOfDatas;

          if (!activeTensors)
            field->SetActiveTensors(paraviewFieldTag);
          else
            activeTensors = 1;
          for (i = 0; i < noOfDatas; i++)
          {
            // double tmpval[9];
            // for(j=0;j<9;j++)
            //   tmpval[j] = (double) data[offset[j]+i];
            // dataArray->SetTuple9(i,
            //                     tmpval[0],
            //                     tmpval[1],
            //                     tmpval[2],
            //                     tmpval[3],
            //                     tmpval[4],
            //                     tmpval[5],
            //                     tmpval[6],
            //                     tmpval[7],
            //                     tmpval[8]);
            dataArray->SetTuple9(i, data[offset[0] + i], data[offset[1] + i], data[offset[2] + i],
              data[offset[3] + i], data[offset[4] + i], data[offset[5] + i], data[offset[6] + i],
              data[offset[7] + i], data[offset[8] + i]);
          }
        }
        break;
        default:
          vtkErrorMacro("number of components [" << numOfComps << "] NOT supported");

          dataArray->Delete();
          delete[] data;
          continue;
      }

      // clean up
      delete[] data;
    }
    else
    {
      vtkErrorMacro("Data type [" << dataType << "] NOT supported");
      continue;
    }

    field->AddArray(dataArray);

    // clean up
    dataArray->Delete();
    // delete [] data;
  }

  // close up
  closefile(&fieldfile, "read");

} // closes ReadFieldFile

void vtkPhastaReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent
     << "GeometryFileName: " << (this->GeometryFileName ? this->GeometryFileName : "(none)")
     << endl;
  os << indent << "FieldFileName: " << (this->FieldFileName ? this->FieldFileName : "(none)")
     << endl;
  os << indent << "CachedGrid: " << this->CachedGrid << endl;
}
