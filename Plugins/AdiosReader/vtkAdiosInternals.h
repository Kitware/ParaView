/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAdiosInternals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtksys/ios/sstream>
#include <vtkstd/string>
#include <vtkstd/map>
#include <vtkstd/set>

#include <sys/stat.h>
#include <assert.h>

#include "vtkSmartPointer.h"

#include <vtkTriangle.h>

#include <vtkRectilinearGrid.h>
#include <vtkStructuredGrid.h>
#include <vtkUnstructuredGrid.h>
#include <vtkDataSet.h>
#include <vtkPointData.h>

#include <vtkDataArray.h>
#include <vtkCharArray.h>
#include <vtkUnsignedCharArray.h>
#include <vtkShortArray.h>
#include <vtkUnsignedShortArray.h>
#include <vtkIntArray.h>
#include <vtkUnsignedIntArray.h>
#include <vtkLongArray.h>
#include <vtkUnsignedLongArray.h>
#include <vtkFloatArray.h>
#include <vtkDoubleArray.h>
#include <vtkPoints.h>

#include <vtksys/ios/sstream>
using vtksys_ios::ostringstream;

// MUST be before adios include
#ifdef ADIOS_NO_MPI
#define _NOMPI
#else
//#include <mpi.h>
#include <vtkMultiProcessController.h>
#include <vtkCommunicator.h>
#include <vtkMPICommunicator.h>
#include <vtkMPICommunicator.h>
#include <vtkMPI.h>
#endif
// MUST be before adios include

extern "C"
{
#include <adios_read.h>
}

//*****************************************************************************
class AdiosData;
typedef vtkstd::map<vtkstd::string, AdiosData> AdiosDataMap;
typedef AdiosDataMap::const_iterator AdiosDataMapIterator;
class AdiosVariable;
typedef vtkstd::map<vtkstd::string, AdiosVariable> AdiosVariableMap;
typedef AdiosVariableMap::const_iterator AdiosVariableMapIterator;
//*****************************************************************************
class AdiosVariable
{
public:
  AdiosVariable()
    {
    this->NumberOfValues     = 0;
    this->NumberOfComponents = 0;
    this->GroupIndex         = -1;
    this->VarIndex           = -1;
    this->TimeIndexComponent = -1;

    this->Extents[0] = this->Extents[1] = 0.0;

    this->Start[0]  = this->Start[1]  = this->Start[2]  = 0;
    this->Count[0]  = this->Count[1]  = this->Count[2]  = 0;
    this->Global[0] = this->Global[1] = this->Global[2] = 0;

    this->Type = adios_unknown;
    }
  // --------------------------------------------------------------------------
  AdiosVariable(const char* name, int groupIdx, ADIOS_VARINFO *varInfo)
    {
    this->Name = name;
    this->GroupIndex = groupIdx;
    this->Type = varInfo->type;
    this->TimeIndexComponent = varInfo->timedim; // -1 if no time steps
    this->VarIndex = varInfo->varid;
    this->NumberOfComponents = varInfo->ndim - ((varInfo->timedim==-1)?0:1);

    this->Extents[0] = this->Extents[1] = 0;
    if (varInfo->gmin && varInfo->gmax)
      {
      if (this->Type == adios_integer)
        {
        this->Extents[0] = (double)(*((int*)varInfo->gmin));
        this->Extents[1] = (double)(*((int*)varInfo->gmax));
        }
      else if (this->Type == adios_real)
        {
        this->Extents[0] = (double)(*((float*)varInfo->gmin));
        this->Extents[1] = (double)(*((float*)varInfo->gmax));
        }
      else if (this->Type == adios_double)
        {
        this->Extents[0] = (double)(*((double*)varInfo->gmin));
        this->Extents[1] = (double)(*((double*)varInfo->gmax));
        }
      }

    int i = 0; // varInfo's index
    int j = 0; // vtk index
    // 1. process dimensions before the time dimension
    // Note that this is empty loop with current ADIOS/C++ (timedim = 0 or -1)
    for (; i < vtkstd::min(varInfo->timedim,3); i++)
      {
      this->Start[j] = 0;
      this->Count[j] = 1;
      this->Global[j] = 1;
      if (i < varInfo->ndim)
        this->Count[j] = this->Global[j] = varInfo->dims[i];
      j++;
      }
    // 2. skip time dimension if it has one
    if (varInfo->timedim >= 0)
        i++;
    // 3. process dimensions after the time dimension
    for (; i < (varInfo->timedim == -1 ? 3 : 4); i++)
      {
      this->Start[j] = 0;
      this->Count[j] = 1;
      this->Global[j] = 1;
      if (i < varInfo->ndim)
        this->Count[j] = this->Global[j] = varInfo->dims[i];
      j++;
      }

    this->NumberOfValues = 1;
    for(int k=0;k<varInfo->ndim;k++)
      {
      if(varInfo->dims[k] != 0)
        {
        this->NumberOfValues *= varInfo->dims[k];
        }
      }

    this->SwapIndices();
    }
  // --------------------------------------------------------------------------
  virtual ~AdiosVariable()
    {
    }
  // --------------------------------------------------------------------------
  void GetReadArrays(int timestep, uint64_t *start, uint64_t *count, int &nbTuples)
    {
    nbTuples = 1;

    int i=0;  // VTK var dimension index
    int j=0;  // adios var dimension index
    // timedim=-1 for non-timed variables, 0..n for others
    // 1. up to time index, or max 3
    // This loop is empty with current ADIOS/C++ (timedim = -1 or 0)
    for (; i < vtkstd::min(this->TimeIndexComponent,3); i++)
      {
      nbTuples *= (int)this->Count[i];
      start[j] = this->Start[i];
      count[j] = this->Count[i];
      j++;
      }
    // 2. handle time index if the variable has time
    if (this->TimeIndexComponent >= 0)
      {
      start[j] = timestep;
      count[j] = 1;
      j++;
      }
    // 3. the rest of indices (all if no time dimension)
    for (; i<3; i++)
      {
      nbTuples *= (int)this->Count[i];
      start[j] = this->Start[i];
      count[j] = this->Count[i];
      j++;
      }

    this->SwapIndices(this->NumberOfComponents, start);
    this->SwapIndices(this->NumberOfComponents, count);
    }
  // --------------------------------------------------------------------------
  void SwapIndices()
    {
    this->SwapIndices(this->NumberOfComponents, this->Start);
    this->SwapIndices(this->NumberOfComponents, this->Count);
    this->SwapIndices(this->NumberOfComponents, this->Global);
    }
  // --------------------------------------------------------------------------
  template <class T>
  void SwapIndices(int size, T* array) // Swap first and last values in the array
    {
    if (size <= 1) return;
    T tmp = array[0];
    size--;
    array[0] = array[size];
    array[size] = tmp;
    }
  // --------------------------------------------------------------------------
  void Print() const
    {
    cout << "Var " << this->Name.c_str() << endl
        << " - Start: " << this->Start[0] << " " << this->Start[1] << " " << this->Start[2] << endl
        << " - Count: " << this->Count[0] << " " << this->Count[1] << " " << this->Count[2] << endl
        << " - Global: " << this->Global[0] << " " << this->Global[1] << " " << this->Global[2] << endl;
    }
  // --------------------------------------------------------------------------
public:
  vtkstd::string Name;
  int NumberOfValues;
  int NumberOfComponents;
  int GroupIndex;
  int VarIndex;
  int TimeIndexComponent;
  uint64_t Start[3]; // We support only tuple size of 1 to 3
  uint64_t Count[3]; // We support only tuple size of 1 to 3
  uint64_t Global[3];// We support only tuple size of 1 to 3
  double Extents[2];
  ADIOS_DATATYPES Type;
};
//*****************************************************************************
class AdiosData
{
public:
  AdiosData()
    {
    this->Init("Undefined", adios_unknown, NULL);
    }
  // --------------------------------------------------------------------------
  AdiosData(const char* name, ADIOS_VARINFO *varInfo)
    {
    this->Init(name, varInfo->type, varInfo->value);
    }
  // --------------------------------------------------------------------------
  AdiosData(const char* name, ADIOS_DATATYPES type, void *data)
    {
    this->Init(name, type, data);
    }
  // --------------------------------------------------------------------------
  virtual ~AdiosData()
    {
    if(this->Data)
      free(this->Data);
    this->Data = NULL;
    this->Size = 0;
    }
  // --------------------------------------------------------------------------
  bool IsInt()    const {return this->Type == adios_integer;}
  bool IsFloat()  const {return this->Type == adios_real;}
  bool IsDouble() const {return this->Type == adios_double;}
  bool IsString() const {return this->Type == adios_string;}
  // --------------------------------------------------------------------------
  int AsInt() const
    {
    int v;
    memcpy(&v,this->Data,sizeof(int));
    return v;
    }
  float AsFloat() const
    {
    float v;
    memcpy(&v,this->Data,sizeof(float));
    return v;
    }
  double AsDouble() const
    {
    double v;
    memcpy(&v,this->Data,sizeof(double));
    return v;
    }
  vtkstd::string AsString() const
    {
    vtkstd::string v = (char *)this->Data;
    return v;
    }
  // --------------------------------------------------------------------------
  AdiosData& operator=(const AdiosData &other) {
    if (this != &other) // make sure not the same object
      {
      this->Name = other.Name;
      this->Size = other.Size;
      this->Type = other.Type;
      this->Data = new unsigned char[this->Size];
      memcpy(this->Data, other.Data, this->Size);
      }
    return *this;
    }
  // --------------------------------------------------------------------------
private:
  void Init(const char* name, ADIOS_DATATYPES type, void *data)
    {
    this->Name = name;
    this->Type = type;
    this->Data = NULL;
    this->Size = 0;
    if (type != adios_unknown)
      {
      this->Size = adios_type_size(type, data);
      }
    if (this->Size > 0)
      {
      this->Data = malloc(this->Size);
      memcpy(this->Data, data, this->Size);
      }
    }
  // --------------------------------------------------------------------------
public:
  vtkstd::string Name;
  size_t Size;
  void *Data;
  ADIOS_DATATYPES Type;
};
//*****************************************************************************
class AdiosFile
{
public:
  AdiosFile(const char* fileName)
    {
    this->File = NULL;
    this->Groups = NULL;
    this->FileName = fileName;
    }
  // --------------------------------------------------------------------------
  virtual ~AdiosFile()
    {
    this->Close();
    }
  // --------------------------------------------------------------------------
  bool IsOpen() const {return this->File != NULL;}
  // --------------------------------------------------------------------------
  bool Open()
    {
    if (this->IsOpen())
      return true;

#ifdef _NOMPI
    cout << "NO mpi for the adios reader" << endl;
    this->File = adios_fopen(this->FileName.c_str(), 0);
#else
    cout << "Use the mpi communicator for the adios reader" << endl;
    MPI_Comm *comm = NULL;
    vtkMPICommunicator *mpiComm = vtkMPICommunicator::SafeDownCast(vtkMultiProcessController::GetGlobalController()->GetCommunicator());
    if(mpiComm && mpiComm->GetMPIComm())
      {
      comm = mpiComm->GetMPIComm()->GetHandle();
      }
    this->File = adios_fopen(this->FileName.c_str(), (comm) ? *comm : 0);
#endif

    if (this->File == NULL)
    {
      cout << "Error opening bp file " <<  this->FileName.c_str() << ":\n"
           << adios_errmsg() << endl;
      return false;  // Throw exception
    }

    // Load groups
    this->Groups = (ADIOS_GROUP **) malloc(this->File->groups_count * sizeof(ADIOS_GROUP *));
    if (this->Groups == NULL)
      {
      cout << "The file could not be opened. Not enough memory" << endl;
      return false;  // Throw exception
      }

    if(false)
      {
      cout << "ADIOS BP file: " << this->FileName.c_str() << endl;
      cout << " - time steps: " << this->File->ntimesteps << " from " << this->File->tidx_start << endl;
      cout << " - groups: " << this->File->groups_count << endl;
      cout << " - variables: " << this->File->vars_count << endl;
      cout << " - attributes:" << this->File->attrs_count << endl;
      }

    // Timestep init
    bool hasTimeSteps = (this->File->ntimesteps != -1);
    int realTime = 1;

    // Read in variables/scalars.
    this->Variables.clear();
    this->Scalars.clear();
    for (int groupIdx=0; groupIdx < this->File->groups_count; groupIdx++)
      {
      //cout <<  "  group " << his->File->group_namelist[groupIdx] << ":" << endl;
      this->Groups[groupIdx] = adios_gopen_byid(this->File, groupIdx);
      if (this->Groups[groupIdx] == NULL)
        {
        cout << "Error opening group " << this->File->group_namelist[groupIdx]
             << " in bp file " << this->FileName.c_str() << ":" << endl
             << adios_errmsg() << endl;
        return false; // Throw exception
        }

      // Load variables
      for (int varIdx=0; varIdx < this->Groups[groupIdx]->vars_count; varIdx++)
        {
        ADIOS_VARINFO *varInfo = adios_inq_var_byid(this->Groups[groupIdx], varIdx);
        if (varInfo == NULL)
          {
          cout << "Error opening inquiring variable "
               << this->Groups[groupIdx]->var_namelist[varIdx]
               << " in group "   << this->File->group_namelist[groupIdx]
               << " of bp file " << this->FileName.c_str() << ":" << endl
               << adios_errmsg() << endl;
          return false;
          }

        if (SupportedVariable(varInfo))
          {
          if (varInfo->ndim == 0)
            {
            // Scalar
            AdiosData scalar(this->Groups[groupIdx]->var_namelist[varIdx], varInfo);
            this->Scalars[scalar.Name] = scalar;
            }
          else
            {
            // Variable
            // add variable to map, map id = variable path without the '/' in the beginning
            AdiosVariable variable(this->Groups[groupIdx]->var_namelist[varIdx], groupIdx, varInfo);
            this->Variables[variable.Name] = variable;

            // Find out real timestep values
            if(hasTimeSteps && ((realTime = this->ExtractTimeStep(variable.Name)) != -1))
              {
              this->RealTimeSteps.insert(realTime);
              }
            }
          }
        else
          {
          cout << "Skip variable: " << this->Groups[groupIdx]->var_namelist[varIdx]
              << " dimension: " << varInfo->ndim
              << " type: " << adios_type_to_string(varInfo->type) << endl;
          }
        adios_free_varinfo(varInfo);
        }

      // Load attributes
      for (int attrIdx = 0; attrIdx < this->Groups[groupIdx]->attrs_count; attrIdx++)
        {
        int size;
        void *data = NULL;
        ADIOS_DATATYPES attrType;

        if (adios_get_attr_byid(this->Groups[groupIdx], attrIdx, &attrType, &size, &data) != 0)
          {
          cout << "Failed to get attribute: "
               << this->Groups[groupIdx]->attr_namelist[attrIdx] << endl;
          continue;
          }

        AdiosData attribute(this->Groups[groupIdx]->attr_namelist[attrIdx], attrType, data);
        this->Attributes[attribute.Name] = attribute;

        free(data);
        }

      // Free group
      adios_gclose(this->Groups[groupIdx]);
      this->Groups[groupIdx] = NULL;
      }
    return true;
    }
  // --------------------------------------------------------------------------
  int ExtractTimeStep(vtkstd::string &variableName)
    {
    const std::string prefix("/Timestep_");

    int realTS = -1;
    std::string::size_type index;
    if (variableName.substr(0, prefix.size()) == prefix)
    {
        index = variableName.find("/", 1);
        realTS = atoi(variableName.substr(prefix.size(), index-prefix.size()).c_str());
    }
    return realTS;
    }
  // --------------------------------------------------------------------------
  void Close()
    {
    if (this->File && this->Groups)
      {
      for (int groupIdx=0; groupIdx < this->File->groups_count; groupIdx++)
        if (this->Groups[groupIdx] != NULL)
          {
          adios_gclose(this->Groups[groupIdx]);
          this->Groups[groupIdx] = NULL;
          }
      }

    if (this->Groups)
      free(this->Groups);
    if (this->File)
      adios_fclose(this->File);

    this->File = NULL;
    this->Groups = NULL;
    }
  // --------------------------------------------------------------------------
  bool IsPixieFileType()
    {
    vtkstd::string schema;
    return (this->GetStringAttribute("/schema/name", schema) && schema == "Pixie");
    }
  // --------------------------------------------------------------------------
  int GetDataType(const char* variableName)
    {
    vtkstd::string name = variableName;
    vtkstd::string::size_type index = name.rfind("/");

    if(index == vtkstd::string::npos)
      {
      return VTK_RECTILINEAR_GRID;
      }

    vtkstd::string head = name.substr(0, index);
    index = head.rfind("/");
    vtkstd::string head2 = head.substr(0, index);

    // Pixie specific coordinate management
    vtkstd::string c1Key = head + "/coords/coord1";
    vtkstd::string c2Key = head + "/coords/coord2";
    vtkstd::string c3Key = head + "/coords/coord3";

    vtkstd::string c1Value;
    vtkstd::string c2Value;
    vtkstd::string c3Value;

    if(this->GetStringAttribute(c1Key, c1Value) &&
       this->GetStringAttribute(c2Key, c2Value) &&
       this->GetStringAttribute(c3Key, c3Value) )
      {
      // Caution: This might be wrong, we also have to check the size of
      //          both arrays

      AdiosVariableMapIterator varIter;
      varIter = this->Variables.find(vtkstd::string(variableName));
      if(varIter == this->Variables.end())
        {
        cout << "No var with name " << variableName << endl;
        return VTK_RECTILINEAR_GRID;
        }
      AdiosVariable requestedVar = varIter->second;
      vtkstd::string coordVarName = head2 + c1Value;
      varIter = this->Variables.find(coordVarName);
      if(varIter == this->Variables.end())
        {
        cout << "No var with name " << coordVarName.c_str() << endl;
        return VTK_RECTILINEAR_GRID;
        }
      AdiosVariable pointCoordVar = varIter->second;

      if(pointCoordVar.NumberOfValues == requestedVar.NumberOfValues)
        return VTK_STRUCTURED_GRID;
      }

    // Get the rectilinear size
    return VTK_RECTILINEAR_GRID;
    }
  // --------------------------------------------------------------------------
  vtkDataSet* GetPixieRectilinearGrid(int timestep)
    {
    // Make sure the file is open and metadata loaded
    this->Open();

    // Retreive Rectilinear mesh for Pixie format
    const char axis[3] = {'X', 'Y', 'Z'};
    uint64_t gridSize[3] = {1, 1, 1};

    // Build coordinate arrays
    vtkFloatArray *coords[3];
    for(int i = 0; i < 3; i++)
      {
      float min, max, value, delta;
      coords[i] = vtkFloatArray::New();
      coords[i]->SetNumberOfComponents(1);

      ostringstream stream;
      stream << "/Timestep_" << timestep << "/cells/" << axis[i];
      AdiosVariableMapIterator varIter = this->Variables.find(stream.str());
      if(varIter != this->Variables.end())
        {
        // Coord extents are available
        min = varIter->second.Extents[0];
        max = varIter->second.Extents[1];

        gridSize[0] = varIter->second.Global[0] + 1;  // nbCell + 1 <=> nbPoints
        gridSize[1] = varIter->second.Global[1] + 1;  // nbCell + 1 <=> nbPoints
        gridSize[2] = varIter->second.Global[2] + 1;  // nbCell + 1 <=> nbPoints
        }
      else
        {
        cout << "No var found with name " << stream.str().c_str() << endl;
        // Use the dimension as extents
        min = 0.0;
        max = (float)gridSize[i];
        }

      // Compute coordinates based on the size and the extent of the grid
      coords[i]->SetNumberOfTuples(gridSize[i]);
      value = min;
      delta = (max - min) / (float)(coords[i]->GetNumberOfTuples());
      for(vtkIdType j = 0; j < coords[i]->GetNumberOfTuples(); j++, value += delta)
        {
        coords[i]->SetValue(j, value);
        }
      }

    // Setup the grid
    vtkRectilinearGrid *rectilinearGrid = vtkRectilinearGrid::New();
    rectilinearGrid->SetDimensions(gridSize[0],
                                   gridSize[1],
                                   gridSize[2]);

    rectilinearGrid->SetXCoordinates(coords[0]);
    coords[0]->FastDelete();

    rectilinearGrid->SetYCoordinates(coords[1]);
    coords[1]->FastDelete();

    rectilinearGrid->SetZCoordinates(coords[2]);
    coords[2]->FastDelete();

    return rectilinearGrid;
    }
  // --------------------------------------------------------------------------
  vtkDataSet* GetPixieStructuredGrid(const char* varName, int timestep)
    {
    // Retreive mesh for Pixie format
    AdiosVariableMapIterator varIter;
    varIter = this->Variables.find(vtkstd::string(varName));
    if(varIter == this->Variables.end())
      {
      cout << "No var with name " << varName << endl;
      return NULL;
      }

    // Find parent property for coordinates
    vtkstd::string name = varName;
    vtkstd::string::size_type index = name.rfind("/");
    vtkstd::string head = name.substr(0, index);
    vtkstd::string c1Key = head + "/coords/coord1";
    vtkstd::string c2Key = head + "/coords/coord2";
    vtkstd::string c3Key = head + "/coords/coord3";
    vtkstd::string coordName[3];
    this->GetStringAttribute(c1Key, coordName[0]);
    this->GetStringAttribute(c2Key, coordName[1]);
    this->GetStringAttribute(c3Key, coordName[2]);

    // Manage specific data type
    vtkDataArray *rawCoords[3];
    for (int i = 0; i < 3; i++)
      {
      ostringstream stream;
      stream << "/Timestep_" << timestep << coordName[i].c_str();
      rawCoords[i] = this->ReadVariable(stream.str().c_str(), timestep);
      }

    // Make sure to convert coord dataArray into vtkFloatArray
    vtkFloatArray *coords[3];
    if(vtkFloatArray::SafeDownCast(rawCoords[0]))
      {
      cout << "coord are float" << endl;
      // We are already the right type
      coords[0] = vtkFloatArray::SafeDownCast(rawCoords[0]);
      coords[1] = vtkFloatArray::SafeDownCast(rawCoords[1]);
      coords[2] = vtkFloatArray::SafeDownCast(rawCoords[2]);
      }
    else
      {
      cout << "coord need conversion" << endl;
      // Need data conversion
      for(vtkIdType k=0; k < 3; k++)
        {
        coords[k] = vtkFloatArray::New();
        coords[k]->SetNumberOfComponents(1);
        cout << "nb tuples: " << rawCoords[k]->GetNumberOfTuples() << endl;
        coords[k]->Allocate(rawCoords[k]->GetNumberOfTuples());
        for(vtkIdType i=0; i < rawCoords[k]->GetNumberOfTuples(); i++)
          {
          coords[k]->InsertNextTuple1(rawCoords[k]->GetTuple1(i));
          }
        rawCoords[k]->Delete();
        rawCoords[k] = NULL;
        }
      }


    // Setup point coordinates
    vtkPoints *points = vtkPoints::New();
    vtkIdType nbPoints = coords[0]->GetNumberOfTuples();
    points->SetNumberOfPoints(nbPoints);

    for (vtkIdType index = 0; index < nbPoints; index++)
      {
      points->SetPoint(index, coords[0]->GetValue(index),
                              coords[1]->GetValue(index),
                              coords[2]->GetValue(index));
      }

    coords[0]->Delete();
    coords[1]->Delete();
    coords[2]->Delete();

    // Setup the grid
    vtkStructuredGrid *structuredGrid  = vtkStructuredGrid::New();
    structuredGrid->SetDimensions(varIter->second.Global[0],
                                  varIter->second.Global[1],
                                  varIter->second.Global[2]);

    structuredGrid->SetPoints(points);
    points->FastDelete();

    return structuredGrid;
    }
  // --------------------------------------------------------------------------
  // CAUTION: the GetXGCMesh and AddPointDataToXGCMesh methods can not be called
  // from the same instance of AdiosFile since the mesh and
  // the data are in two different files
  vtkUnstructuredGrid* GetXGCMesh()
    {
    // Retreive mesh for XCG format
    int nbNodes, nbTriangles;
    this->GetIntegerAttribute("/nnodes", nbNodes);
    this->GetIntegerAttribute("/cell_set[0]/nbcells", nbTriangles);

    vtkPoints *points = this->ReadPoints("/coordinates/values", 0);// 0 because there is no timestep...
    vtkUnstructuredGrid *grid = vtkUnstructuredGrid::New();
    grid->SetPoints(points);
    points->Delete();

    vtkDataArray *rawData = this->ReadVariable("/cell_set[0]/node_connect_list",
                                               0); // 0 because there is no timestep...
    if(!rawData)
      {
      cout << "Invalid cell connectivity. No data read." << endl;
      grid->Delete();
      return NULL;
      }

    vtkIntArray *cells = vtkIntArray::SafeDownCast(rawData);
    if(!cells)
      {
      cout << "Invalid cell connectivity array type. " << rawData->GetClassName() << endl;
      grid->Delete();
      return NULL;
      }

    vtkIdType cell[3];
    grid->Allocate(cells->GetNumberOfTuples());
    for(vtkIdType idx = 0; idx < cells->GetNumberOfTuples(); idx+=3)
      {
      cell[0] = cells->GetValue(idx);
      cell[1] = cells->GetValue(idx + 1);
      cell[2] = cells->GetValue(idx + 2);
      grid->InsertNextCell(VTK_TRIANGLE, 3, cell);
      }
    cells->Delete();
    return grid;
    }
  // --------------------------------------------------------------------------
  // CAUTION: the GetXGCMesh and AddPointDataToXGCMesh methods can not be called
  // from the same instance of AdiosFile since the mesh and
  // the data are in two different files
  void AddPointDataToXGCMesh(vtkUnstructuredGrid* mesh)
    {
    // Attach point data to the grid
    int nbPointData = 0;
    if(this->GetIntegerAttribute("/nnode_data", nbPointData))
      {
      for (int i = 0; i < nbPointData; i++)
        {
        ostringstream labelKey;
        labelKey << "/node_data[" << i << "]/labels";
        vtkstd::string arrayName;
        if(!this->GetStringAttribute(labelKey.str(), arrayName))
          continue;

        ostringstream varKey;
        varKey << "/node_data[" << i << "]/values";
        vtkDataArray* array = this->ReadVariable(varKey.str().c_str(), 0);
        if(array)
          {
          array->SetName(arrayName.c_str());
          mesh->GetPointData()->AddArray(array);
          array->FastDelete();
          }
        }
      }
    }
  // --------------------------------------------------------------------------
  bool GetIntegerAttribute(const vtkstd::string &key, int &value)
    {
    this->Open();
    AdiosDataMapIterator iter = this->Attributes.find(key);
    if (iter == this->Attributes.end() || !iter->second.IsInt())
      return false;
    value = iter->second.AsInt();
    return true;
    }
  // --------------------------------------------------------------------------
  bool GetStringAttribute(const vtkstd::string &key, vtkstd::string &value)
    {
    this->Open();
    AdiosDataMapIterator iter = this->Attributes.find(key);
    if (iter == this->Attributes.end() || !iter->second.IsString())
      return false;
    value = iter->second.AsString();
    return true;
  }
  // --------------------------------------------------------------------------
  vtkPoints* ReadPoints(const char* name, int timestep)
    {
    this->Open();
    AdiosVariableMapIterator iter = this->Variables.find(vtkstd::string(name));
    if(iter == this->Variables.end())
      {
      cout << "ERROR in ReadPoints: Variable " << name << " not found." << endl;
      return NULL;
      }

    // Read data info
    int nbTuples = -1;
    uint64_t start[4] = {0,0,0,0};
    uint64_t count[4] = {0,0,0,0};

    AdiosVariable var = iter->second;
    var.GetReadArrays(timestep, start, count, nbTuples);
    nbTuples /= var.NumberOfComponents;

    // Create points object
    vtkPoints* points = NULL;
    switch (var.Type)
    {
      case adios_real:
        points = vtkPoints::New(VTK_FLOAT);
        break;
      case adios_double:
        points = vtkPoints::New(VTK_DOUBLE);
        break;
      default:
        cout << "Inavlid point type" << endl;
        return NULL;
    }
    points->SetNumberOfPoints(nbTuples);

      // Allocate buffer used to read data
      void* dataTmpBuffer = NULL;
      if (var.Type == adios_real)
        {
        cout << "There will be point convertion error" << endl; // FIXME
        dataTmpBuffer = malloc(nbTuples*3*sizeof(float));
        }
      else if (var.Type == adios_double)
        {
        dataTmpBuffer = malloc(nbTuples*3*sizeof(double));
        }
      else if (var.Type == adios_integer)
        {
        cout << "There will be point convertion error" << endl; // FIXME
        dataTmpBuffer = malloc(nbTuples*3*sizeof(int));
        }

      // Read buffer
      this->OpenGroup( var.GroupIndex );
      uint64_t retval = adios_read_var_byid( this->Groups[var.GroupIndex],
                                             var.VarIndex, start, count,
                                             dataTmpBuffer );
      this->CloseGroup( var.GroupIndex );
      if(retval < 0)
        {
        cout << adios_errmsg() << endl;
        // Error while reading so return NULL
        points->Delete();
        free(dataTmpBuffer);
        return NULL;
        }
      else
        {
        // Fill result
        double coord[3] = {0,0,0};
        int bufferIdx = 0;
        int nbComp = var.NumberOfComponents;
        int i;
        for(vtkIdType idx=0; idx < points->GetNumberOfPoints(); idx++)
          {
          for(i=0; i < nbComp; i++)
            {
            coord[i] = ((double*)dataTmpBuffer)[bufferIdx++]; // FIXME if dataTmpBuffer type is not of double type !!!!
            }
          points->SetPoint(idx, coord);
          }

        // Free tmp
        free(dataTmpBuffer);
        }
    return points;
    }
  // --------------------------------------------------------------------------
  vtkDataArray* ReadVariable(const char* name, int timestep)
    {
    this->Open();
    AdiosVariableMapIterator varIter = this->Variables.find(vtkstd::string(name));
    if( varIter == this->Variables.end() )
      {
      cout << "ERROR in ReadVariable: Variable " << name << " not found !" << endl;
      return false;
      }

    // Read data info
//    cout << "ReadVariable: " << name << endl;

    AdiosVariable var = varIter->second;
    int nbTuples = -1;
    uint64_t start[4] = {0,0,0,0};
    uint64_t count[4] = {0,0,0,0};
    var.GetReadArrays(0, start, count, nbTuples);

    // Create data array based on its type
    vtkDataArray *array = NULL;
    switch (var.Type)
    {
    case adios_unsigned_byte:
    case adios_string:
      array = vtkCharArray::New();
      break;

    case adios_byte:
      array = vtkUnsignedCharArray::New();
      break;
    case adios_unsigned_short:
      array = vtkUnsignedShortArray::New();
      break;
    case adios_short:
      array = vtkShortArray::New();
      break;

    case adios_unsigned_integer:
      array = vtkUnsignedIntArray::New();
      break;
    case adios_integer:
      array = vtkIntArray::New();
      break;

    case adios_unsigned_long:
      array = vtkUnsignedLongArray::New();
      break;
    case adios_long:
      array = vtkLongArray::New();
      break;

    case adios_real:
      array = vtkFloatArray::New();
      break;

    case adios_double:
      array = vtkDoubleArray::New();
      break;

    case adios_long_double: // 16 bytes
    case adios_complex:     //  8 bytes
    case adios_double_complex: // 16 bytes
    default:
      cout << "ERROR: Invalid data type" << endl;
      break;
    }

    if(!array)
      return NULL;

    array->SetNumberOfComponents(1);
    //array->Allocate(nbTuples * 3); // ?????????????
    array->SetNumberOfTuples(nbTuples); // Allocate array memory

    // Create a nice array name
    vtkstd::string arrayName = name;
    vtkstd::string::size_type index = arrayName.rfind("/");
    if(index != vtkstd::string::npos)
      {
      arrayName = arrayName.substr( index + 1, arrayName.size() - 1 );
      }
    array->SetName(arrayName.c_str());


    int groupIdx = var.GroupIndex;
    this->OpenGroup(groupIdx);

    cout << "start: " << start[0] << " " << start[1] << " " << start[2] << " " << start[3] << endl;
    cout << "count: " << count[0] << " " << count[1] << " " << count[2] << " " << count[3] << endl;
    cout << "varIdx: " << var.VarIndex << endl;
    cout << "array "<< arrayName.c_str() <<" of type: "<< array->GetClassName() << " with size " << array->GetNumberOfTuples() << endl;

    if( array && (adios_read_var_byid(this->Groups[groupIdx],
                                      var.VarIndex, start, count,
                                      array->GetVoidPointer(0)) < 0))
      {
      cout << "Error while reading data array." << endl;
      cout << adios_errmsg() << endl;
      array->Delete();
      array = NULL; // Impossible to read so set the output to NULL
      }
    this->CloseGroup(groupIdx);

    return array;
    }
  // --------------------------------------------------------------------------
  int GetNumberOfTimeSteps()
    {
    this->Open();
    return this->RealTimeSteps.size();
    //return this->File->ntimesteps;
    }
  // --------------------------------------------------------------------------
  int GetRealTimeStep(int timeStepIndex)
    {
    vtkstd::set<int>::iterator iter = this->RealTimeSteps.begin();
    for(int i = 0;i<timeStepIndex; i++)
      iter++;
    return *(iter);
    }
  // --------------------------------------------------------------------------
  bool OpenGroup(int groupIndex) // return true if OK
    {
    if(!this->Groups)
      return false;

    if(!this->Groups[groupIndex])
      {
      this->Groups[groupIndex] = adios_gopen_byid(this->File, groupIndex);
      if(this->Groups[groupIndex] == NULL)
        {
        cout << "Error when opening group "
             << this->File->group_namelist[groupIndex]
             << " in file " << this->FileName << endl;
        return false;
        }
      }
    return true;
    }
  // --------------------------------------------------------------------------
  bool CloseGroup(int groupIndex) // return true if OK
    {
    if(!this->Groups)
      return false;

    if(this->Groups[groupIndex])
      {
      int error = adios_gclose(this->Groups[groupIndex]);
      this->Groups[groupIndex] = NULL;
      if(error < 0)
        {
        cout << "Error closing group "
             << this->File->group_namelist[groupIndex]
             << " in file " << this->FileName << endl;
        cout << adios_errno << " : " << adios_errmsg() << endl;
        }
      return !error;
      }
    return true; // Already closed
    }
  // --------------------------------------------------------------------------
  bool SupportedVariable(ADIOS_VARINFO *varInfo)
    {
    return !((varInfo->ndim == 1 && varInfo->timedim >= 0) ||  // scalar with time
             (varInfo->ndim > 3 && varInfo->timedim == -1) ||  // xD array with no time
             (varInfo->ndim > 4 && varInfo->timedim >= 0)  ||  // xD array with time
             varInfo->type == adios_long_double ||
             varInfo->type == adios_complex ||
             varInfo->type == adios_double_complex);
    }
  // --------------------------------------------------------------------------
  void PrintInfo()
    {
    this->Open();
    cout << "Groups:" << endl;
    for(int i=0;i<this->File->groups_count;i++)
      {
      cout << " - " << this->File->group_namelist[i] << endl;
      }

    cout << "Attributes:" << endl;
    AdiosDataMapIterator iter = this->Attributes.begin();
    for(; iter != this->Attributes.end(); iter++)
      {
      cout << " - " << iter->first << endl;
      }

    cout << "Variables:" << endl;
    AdiosVariableMapIterator iter2 = this->Variables.begin();
    for(; iter2 != this->Variables.end(); iter2++)
      {
      cout << " - " << iter2->first << endl;
      }

    cout << "Pixie file ? " << (this->IsPixieFileType()) << endl;
    int nbTime = this->GetNumberOfTimeSteps();
    cout << "Number of timesteps: " << nbTime << endl;
    for(int i=0;i<nbTime;i++)
      {
      cout << " - " << this->GetRealTimeStep(i) << endl;
      }
    this->Close();
    }
  // --------------------------------------------------------------------------
public:
  vtkstd::string FileName;
  ADIOS_FILE *File;
  ADIOS_GROUP **Groups;

  AdiosDataMap Scalars;
  AdiosDataMap Attributes;
  AdiosVariableMap Variables;
  vtkstd::set<int> RealTimeSteps;
};
//*****************************************************************************
