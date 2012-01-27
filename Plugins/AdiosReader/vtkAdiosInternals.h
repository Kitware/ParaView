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
#include <string>
#include <map>
#include <set>

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
#include <vtkExtentTranslator.h>
#include <vtkCellData.h>
#include <vtkPointData.h>

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
#include <globals.h>
#include <adios_error.h>
}

#ifndef __vtkAdiosInternals_h
#define __vtkAdiosInternals_h

//*****************************************************************************
class AdiosData;
typedef std::map<std::string, AdiosData> AdiosDataMap;
typedef AdiosDataMap::const_iterator AdiosDataMapIterator;
class AdiosVariable;
typedef std::map<std::string, AdiosVariable> AdiosVariableMap;
typedef AdiosVariableMap::const_iterator AdiosVariableMapIterator;
//*****************************************************************************
class AdiosVariable
{
public:
  AdiosVariable()
    {
    this->NumberOfElements   = -1;
    this->Dimension          = -1;
    this->GroupIndex         = -1;
    this->VarIndex           = -1;
    this->TimeIndexComponent = -1;
    this->TimeStepOffset      = 0;

    this->Range[0] = this->Range[1] = 0.0;
    // Extent = [minX, maxX, minY, maxY, minZ, maxZ, minTime, maxTime]
    this->Extent[0] = this->Extent[1] = this->Extent[2] = this->Extent[3] = 0;
    this->Extent[4] = this->Extent[5] = this->Extent[6] = this->Extent[7] = 0;

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
    this->Dimension = varInfo->ndim - ((varInfo->timedim == -1) ? 0 : 1); // The dimension here do not take the time into account

    // Reset range and extent
    this->Range[0] = this->Range[1] = 0;
    this->Extent[0] = this->Extent[1] = this->Extent[2] = this->Extent[3] = 0;
    this->Extent[4] = this->Extent[5] = this->Extent[6] = this->Extent[7] = 0;

    if (varInfo->gmin && varInfo->gmax)
      {
      if (this->Type == adios_integer)
        {
        this->Range[0] = (double)(*((int*)varInfo->gmin));
        this->Range[1] = (double)(*((int*)varInfo->gmax));
        }
      else if (this->Type == adios_real)
        {
        this->Range[0] = (double)(*((float*)varInfo->gmin));
        this->Range[1] = (double)(*((float*)varInfo->gmax));
        }
      else if (this->Type == adios_double)
        {
        this->Range[0] = (double)(*((double*)varInfo->gmin));
        this->Range[1] = (double)(*((double*)varInfo->gmax));
        }
      }

    // CAUTION: We suppose that if the variable has time it MUST BE the first
    //          component one and the dimension of the array CAN NOT be bigger
    //          than (time + 3D) = 4
    if(this->Dimension > 3 || varInfo->timedim > 0)
      {
      // Error this case is not supported for now !!!!
      cerr << "The variable " << name
           << " do not follow a correct structure type for the current adios Reader."
           << endl << "The application might CRASH at some point." << endl;
      return;
      }

    // Build extent + compute number of elements
    this->NumberOfElements = 1;
    for(int i=0; i < 4; i++)
      {
      this->Extent[i*2 + 0] = 0;                                       //Offset
      this->Extent[7 - i*2] = (i < varInfo->ndim) ? varInfo->dims[i]:0;//Size
      // Count the total number of elements
      if(this->Extent[7 - i*2] != 0)
        {
        this->NumberOfElements *= this->Extent[i*2 + 1];
        }
      }

    // Shift left if no time
    // i.e. 1D     [0, 0, 0, 0, 0, 0, 0, X] => [0, 0, 0, 0, 0, X, 0, 0] <<
    // i.e. 2D     [0, 0, 0, 0, 0, X, 0, Y] => [0, 0, 0, X, 0, Y, 0, 0] <<
    // i.e. 2D + T [0, 0, 0, X, 0, Y, 0, T] => [0, 0, 0, X, 0, Y, 0, T] (no change)
    // i.e. 3D     [0, 0, 0, X, 0, Y, 0, Z] => [0, X, 0, Y, 0, Z, 0, 0] <<
    if(!this->IsTimeDependent())
      {
      for(int j=0; j<3; j++)
        {
        this->Extent[2*j + 1] = this->Extent[2*(j+1) + 1];
        this->Extent[2*(j+1) + 1] = 0;
        }
      }
    // i.e. 1D     [0, 0, 0, 0, 0, X, 0, 0] => [0, X, 0, 0, 0, 0, 0, 0] << x2
    // i.e. 2D     [0, 0, 0, 0, 0, X, 0, Y] => [0, X, 0, Y, 0, 0, 0, 0] << x1
    // i.e. 2D + T [0, 0, 0, X, 0, Y, 0, T] => [0, X, 0, Y, 0, 0, 0, T] << x1
    // i.e. 3D     [0, 0, 0, X, 0, Y, 0, Z] => [0, X, 0, Y, 0, Z, 0, 0] (no change)
    int nbShift = 3 - this->Dimension;
    for(int i=0;i<nbShift;i++)
      {
      this->Extent[2*i] = this->Extent[2*(i+1)];
      this->Extent[2*(i+1)] = 0;
      }
    }
  // --------------------------------------------------------------------------
  /// Fill the provided array with the corresponding data size mapped in 3D.
  /// Remember that it could be a 1D or 2D data. So the missing dimension is
  /// set to 0. [x, y, z]
  void GetDimension3D(vtkIdType *matrixSize) const
    {
    // Set values [X, Y, Z] <=> [0, X, 0, Y, 0, Z, 0, Time]
    matrixSize[0] = this->Extent[1];
    matrixSize[1] = this->Extent[3];
    matrixSize[2] = this->Extent[5];
    }
  // --------------------------------------------------------------------------
  bool IsTimeDependent() const
    {
    return this->TimeIndexComponent != -1;
    }
  // --------------------------------------------------------------------------
  int GetNumberOfTimeSteps()
    {
    if(!this->IsTimeDependent())
      return 1;
    return this->Extent[7];
    }
  // --------------------------------------------------------------------------
  virtual ~AdiosVariable()
    {
    }
  // --------------------------------------------------------------------------
  void Print() const
    {
    cout << "Var " << this->Name.c_str() << endl
         << " - Extent: [ "
         << this->Extent[0] << ", " << this->Extent[1] << ", "
         << this->Extent[2] << ", " << this->Extent[3] << ", "
         << this->Extent[4] << ", " << this->Extent[5] << ", "
         << this->Extent[6] << ", " << this->Extent[7] << " ]" << endl
         << " - TimeDependant: " << this->IsTimeDependent() << endl;
    }
  // --------------------------------------------------------------------------
public:
  std::string Name;
  int GroupIndex;
  int VarIndex;
  int NumberOfElements;
  int Dimension;

  // Contain extent of the array
  uint64_t Extent[8]; // [ offsetZ, countZ, offsetY, countY, ox, cx , 0, nbTime]
  int TimeIndexComponent;
  double Range[2];
  ADIOS_DATATYPES Type;
  int TimeStepOffset;
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
  std::string AsString() const
    {
    std::string v = (char *)this->Data;
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
  std::string Name;
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
    this->HasTimeInformationInVariableNames = false;
    this->ExtentTranslator = NULL;
    this->File = NULL;
    this->Groups = NULL;
    this->FileName = fileName;
    // Since the whole extent will always be based on offset, it is always 0
    this->CurrentWholeExtent[0] = 0;
    this->CurrentWholeExtent[2] = 0;
    this->CurrentWholeExtent[4] = 0;
    }
  // --------------------------------------------------------------------------
  virtual ~AdiosFile()
    {
    if(this->ExtentTranslator)
      {
      this->ExtentTranslator->Delete();
      this->ExtentTranslator = NULL;
      }
    this->Close();
    }
  // --------------------------------------------------------------------------
  bool IsOpen() const {return this->File != NULL;}
  // --------------------------------------------------------------------------
  bool Open()
    {
    if (this->IsOpen())
      return true;

    this->HasTimeInformationInVariableNames = false;

    //cout << "=> Before Adios_fopen" << endl;
#ifdef _NOMPI
    //cout << "NO mpi for the adios reader" << endl;
    this->File = adios_fopen(this->FileName.c_str(), 0);
    if(this->ExtentTranslator)
      {
      this->ExtentTranslator->Delete();
      this->ExtentTranslator = NULL;
      }
#else
    //cout << "Use the mpi communicator for the adios reader" << endl;
    MPI_Comm *comm = NULL;
    vtkMultiProcessController *ctrl =
        vtkMultiProcessController::GetGlobalController();

    // Handle piece management
    if(!this->ExtentTranslator)
      {
      this->ExtentTranslator = vtkExtentTranslator::New();
      }
    this->ExtentTranslator->SetPiece(ctrl->GetLocalProcessId());
    this->ExtentTranslator->SetNumberOfPieces(ctrl->GetNumberOfProcesses());
    this->ExtentTranslator->SetGhostLevel(0); // FIXME !!!!

    vtkMPICommunicator *mpiComm =
        vtkMPICommunicator::SafeDownCast(ctrl->GetCommunicator());
    if(mpiComm && mpiComm->GetMPIComm())
      {
      comm = mpiComm->GetMPIComm()->GetHandle();
      }

    this->File = adios_fopen(this->FileName.c_str(), (comm) ? *comm : 0);

    if(this->File == NULL)
      {
      if(adios_errno != err_end_of_file)
        {
        cout << "The data is not ready yet. (" << adios_errmsg() << ")" << endl;
        }
      else
        {
        cout << "We reach the end of the timesteps." << endl;
        }
      //cout << "=> EXIT Adios_fopen" << endl;
      return false;
      }

#endif
    //cout << "=> After Adios_fopen" << endl;

    if (this->File == NULL)
    {
      cout << "Error opening bp file " <<  this->FileName.c_str() << ":\n"
           << adios_errmsg() << endl;
      return false;  // Throw exception
    }

    //cout << "===> Adios_fopen OK" << endl;

    // Load groups
    this->Groups = (ADIOS_GROUP **) malloc(this->File->groups_count * sizeof(ADIOS_GROUP *));
    if (this->Groups == NULL)
      {
      cout << "The file could not be opened. Not enough memory" << endl;
      return false;  // Throw exception
      }

    if(true)
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

      //cout << "===> Before Adios_gopen " << groupIdx << endl;
      this->Groups[groupIdx] = adios_gopen_byid(this->File, groupIdx);
      //cout << "===> After Adios_gopen " << groupIdx << endl;

      if (this->Groups[groupIdx] == NULL)
        {
        cout << "Error opening group " << this->File->group_namelist[groupIdx]
             << " in bp file " << this->FileName.c_str() << ":" << endl
             << adios_errmsg() << endl;
        return false; // Throw exception
        }

      // Load variables metadata
      for (int varIdx=0; varIdx < this->Groups[groupIdx]->vars_count; varIdx++)
        {
        //cout << "=====> Before adios_inq_var_byid " << varIdx << endl;
        ADIOS_VARINFO *varInfo = adios_inq_var_byid(this->Groups[groupIdx], varIdx);
        //cout << "=====> After adios_inq_var_byid " << varIdx << endl;
        //cout << "=====> Adios msg: " << adios_errmsg() << endl;

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
            //cout <<  "=====> adios create scalar " << this->Groups[groupIdx]->var_namelist[varIdx] << endl;
            AdiosData scalar(this->Groups[groupIdx]->var_namelist[varIdx], varInfo);
            this->Scalars[scalar.Name] = scalar;
            //cout <<  "=====> adios scalar ok" << endl;
            }
          else
            {
            // Variable
            // add variable to map, map id = variable path without the '/' in the beginning
            //cout <<  "=====> adios create variable " << this->Groups[groupIdx]->var_namelist[varIdx] << endl;
            AdiosVariable variable(this->Groups[groupIdx]->var_namelist[varIdx], groupIdx, varInfo);
            variable.TimeStepOffset = this->Groups[groupIdx]->timestep;
            this->Variables[variable.Name] = variable;

            // Find out real timestep values
            if(hasTimeSteps && ((realTime = this->ExtractTimeStep(variable.Name)) != -1))
              {
              this->RealTimeSteps.insert(realTime);
              }

            // Add metadata to figure out if timestep prefix is used
            this->HasTimeInformationInVariableNames =
                this->HasTimeInformationInVariableNames
                || (this->ExtractTimeStep(variable.Name) != -1);
            }
          }
        else
          {
          cout << "Skip variable: " << this->Groups[groupIdx]->var_namelist[varIdx]
              << " dimension: " << varInfo->ndim
              << " type: " << adios_type_to_string(varInfo->type) << endl;
          }
        //cout <<  "=====> adios before adios_free_varinfo" << endl;
        //adios_free_varinfo(varInfo);
        free(varInfo);
        //cout <<  "=====> adios after adios_free_varinfo" << endl;
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
      //cout << "===> Before Adios_gclose " << groupIdx << endl;
      adios_gclose(this->Groups[groupIdx]);
      //cout << "===> After Adios_gclose " << groupIdx << endl;

      this->Groups[groupIdx] = NULL;
      }

    // Make sure that this->RealTimeSteps is filled even if the variables
    // do not have any /Timestep_xxx/ prefix
    if(hasTimeSteps && this->RealTimeSteps.size() == 0)
      {
      for(int i=0;i<this->GetNumberOfTimeSteps();i++)
        {
        this->RealTimeSteps.insert(i);
        }
      }

    return true;
    }
  // --------------------------------------------------------------------------
  int ExtractTimeStep(std::string &variableName)
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
    std::string schema;
    return (this->GetStringAttribute("/schema/name", schema) && schema == "Pixie");
    }
  // --------------------------------------------------------------------------

  // The Pixie specification is complex for figuring variable location !
  // And the final choice is made on the variable dimension compare to
  // the node and cell sizes.
  // The question is, why not just check that ???
  // It is basically a compatibility check ! Which has to be done anyway,
  // and much more easy to do !

//  int GetPixieDataType(const char* variableName)
//    {
//    std::string name = variableName;
//    const std::string prefix("/Timestep_");
//    std::string::size_type index = name.find(prefix);

//    std::string head = name;
//    if(index == 0) // Remove /Timestep_xxx
//      {
//      head = name.substr(0, name.find( 1, "/" ));
//      }

//    cout << "Head: " << head.c_str() << endl;

//    // Pixie specific coordinate management
//    std::string c1Key = head + "/coords/coord1";
//    std::string c2Key = head + "/coords/coord2";
//    std::string c3Key = head + "/coords/coord3";

//    std::string c1Value;
//    std::string c2Value;
//    std::string c3Value;

//    if(this->GetStringAttribute(c1Key, c1Value) &&
//       this->GetStringAttribute(c2Key, c2Value) &&
//       this->GetStringAttribute(c3Key, c3Value) )
//      {
//      // Caution: This might be wrong, we also have to check the size of
//      //          both arrays

//      AdiosVariableMapIterator varIter;
//      varIter = this->Variables.find(variableName);
//      if(varIter == this->Variables.end())
//        {
//        cout << "No var with name " << variableName << endl;
//        return VTK_RECTILINEAR_GRID;
//        }
//      AdiosVariable requestedVar = varIter->second;
//      std::string coordVarName = head2 + c1Value;
//      varIter = this->Variables.find(coordVarName);
//      if(varIter == this->Variables.end())
//        {
//        cout << "No var with name " << coordVarName.c_str() << endl;
//        return VTK_RECTILINEAR_GRID;
//        }
//      AdiosVariable pointCoordVar = varIter->second;

//      if(pointCoordVar.NumberOfElements == requestedVar.NumberOfElements)
//        return VTK_STRUCTURED_GRID;
//      }

//    // Get the rectilinear size
//    return VTK_RECTILINEAR_GRID;
//    }
  // --------------------------------------------------------------------------
  vtkDataSet* GetPixieRectilinearGrid(int timestep)
    {
    // Make sure the file is open and metadata loaded
    this->Open();

    // Make sure we are in the Pixie case
    if(this->File == NULL || !this->IsPixieFileType())
      return NULL;

    // Retreive Rectilinear mesh for Pixie format
    const char axis[3] = {'X', 'Y', 'Z'};
    uint64_t gridSize[3] = {1, 1, 1};

    // Build coordinate arrays
    vtkFloatArray *coords[3];
    for(int i = 0; i < 3; i++)
      {
      float min, max, value, delta;
      min = max = 0;
      coords[i] = vtkFloatArray::New();
      coords[i]->SetNumberOfComponents(1);

      // Build variable name
      AdiosVariableMapIterator varIter;
      ostringstream stream;
      if(this->HasTimeInformationInVariableNames)
        {
        stream << "/Timestep_" << timestep << "/cells/" << axis[i];
        varIter = this->Variables.find(stream.str());
        }
      else
        {
        stream << "/cells/" << axis[i];
        varIter = this->Variables.find(stream.str());
        }

      // Process the variable
      if(varIter != this->Variables.end())
        {
        // Coord extents are available
        min = varIter->second.Range[0];
        max = varIter->second.Range[1];

        vtkIdType size3D[3];
        varIter->second.GetDimension3D(size3D);

        gridSize[0] = size3D[0] + 1;  // nbCell + 1 <=> nbPoints
        gridSize[1] = size3D[1] + 1;  // nbCell + 1 <=> nbPoints
        gridSize[2] = size3D[2] + 1;  // nbCell + 1 <=> nbPoints
        }
      else
        {
        cout << "No var found with name " << stream.str().c_str() << endl;
        // Use the dimension as extents
        }

      // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      // FIXME tmp code for staging that do not provide min/max values
      // Are we going to do something ???
      // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      // Make sure that max is set to a value (in stage the max won't be set)
      max = (max == 0) ? ((float)gridSize[i]) : max;

      // Deal with peace management
      this->CurrentWholeExtent[1] = gridSize[0];
      this->CurrentWholeExtent[3] = gridSize[1];
      this->CurrentWholeExtent[5] = gridSize[2];

      if(this->ExtentTranslator)
        {
        this->ExtentTranslator->SetWholeExtent(this->CurrentWholeExtent);
        this->ExtentTranslator->PieceToExtent();
        this->ExtentTranslator->GetExtent(this->CurrentPieceExtent);
        // Shift non-zero offset to make geometry continu
        for(int i = 0; i < 3; i++)
          {
          if(this->CurrentPieceExtent[i*2] > 0)
            {
            --this->CurrentPieceExtent[i*2];
            }
          }
        }
      else
        {
        // Just copy
        for(int i=0; i<6; i++)
          {
          this->CurrentPieceExtent[i] = this->CurrentWholeExtent[i];
          }
        }
      vtkIdType nbTuples = this->CurrentPieceExtent[i*2 + 1]
                           - this->CurrentPieceExtent[i*2];

      // Compute coordinates based on the size and the extent of the grid
      coords[i]->SetNumberOfTuples(nbTuples);
      delta = (max - min) / (float)(this->CurrentWholeExtent[i*2+1]);
      value = min + (delta * this->CurrentPieceExtent[i*2]);
      for(vtkIdType j = 0; j < nbTuples; j++, value += delta)
        {
        coords[i]->SetValue(j, value);
        }
      }

    // Setup the grid
    vtkRectilinearGrid *rectilinearGrid = vtkRectilinearGrid::New();
    rectilinearGrid->SetDimensions(this->CurrentPieceExtent[1]-this->CurrentPieceExtent[0],
                                   this->CurrentPieceExtent[3]-this->CurrentPieceExtent[2],
                                   this->CurrentPieceExtent[5]-this->CurrentPieceExtent[4]);

    rectilinearGrid->SetXCoordinates(coords[0]);
    coords[0]->FastDelete();

    rectilinearGrid->SetYCoordinates(coords[1]);
    coords[1]->FastDelete();

    rectilinearGrid->SetZCoordinates(coords[2]);
    coords[2]->FastDelete();

    // Build filter var name
    ostringstream varHead;
    varHead << "/Timestep_" << timestep << "/";
    std::string timestepFilter = varHead.str();
    std::string nodesFilter = "/nodes";
    std::string cellsFilter = "/cells";

    // Loop over variable to find the ones to load
    AdiosVariableMapIterator varIter = this->Variables.begin();
    for(; varIter != this->Variables.end(); varIter++)
      {
      if( ((varIter->second.Name.find(timestepFilter) == 0 ^ // Exclusive OR
            !this->HasTimeInformationInVariableNames) )
          && varIter->second.Extent[1] == (gridSize[0]-1)
          && varIter->second.Extent[3] == (gridSize[1]-1)
          && varIter->second.Extent[5] == (gridSize[2]-1)
          && varIter->second.Name.find(nodesFilter) == std::string::npos
          && varIter->second.Name.find(cellsFilter) == std::string::npos )
        {
        vtkDataArray *array = NULL;
        if(varIter->second.IsTimeDependent())
          {
          uint64_t offset[4] = { timestep + varIter->second.TimeStepOffset,
                                 this->CurrentPieceExtent[4],
                                 this->CurrentPieceExtent[2],
                                 this->CurrentPieceExtent[0] };
          uint64_t count[4] = { 1,
                                rectilinearGrid->GetDimensions()[2]-1,
                                rectilinearGrid->GetDimensions()[1]-1,
                                rectilinearGrid->GetDimensions()[0]-1 };
          array = this->ReadVariable(varIter->second, offset, count);
          }
        else
          {
          uint64_t offset[4] = { this->CurrentPieceExtent[4],
                                 this->CurrentPieceExtent[2],
                                 this->CurrentPieceExtent[0],
                                 0 };
          uint64_t count[4] = { rectilinearGrid->GetDimensions()[2]-1,
                                rectilinearGrid->GetDimensions()[1]-1,
                                rectilinearGrid->GetDimensions()[0]-1,
                                0 };
          array = this->ReadVariable(varIter->second, offset, count);
          }
        if(array)
          {
          rectilinearGrid->GetCellData()->AddArray(array);
          array->FastDelete();
          }
        }
      }
    return rectilinearGrid;
    }
  // --------------------------------------------------------------------------
  vtkDataSet* GetPixieStructuredGrid(int timestep)
    {
    // Make sure the file is open and metadata loaded
    this->Open();

    // Make sure we are in the Pixie case
    if(this->File == NULL || !this->IsPixieFileType())
      return NULL;

    // Get grid size
    std::string coordName[3];
    coordName[0] = "/nodes/X";
    coordName[1] = "/nodes/Y";
    coordName[2] = "/nodes/Z";
    if(this->HasTimeInformationInVariableNames)
      {
      // Add the /Timestep_ header for the variable name
      ostringstream stream;
      stream << "/Timestep_" << timestep;
      for(int i=0;i<3;i++)
        {
        coordName[i] = stream.str() + coordName[i];
        }
      }

    // Deal with peace management
    vtkIdType size3D[3];
    this->Variables[coordName[0].c_str()].GetDimension3D(size3D);
    this->CurrentWholeExtent[1] = size3D[0];
    this->CurrentWholeExtent[3] = size3D[1];
    this->CurrentWholeExtent[5] = size3D[2];

    if(this->ExtentTranslator)
      {
      this->ExtentTranslator->SetWholeExtent(this->CurrentWholeExtent);
      this->ExtentTranslator->PieceToExtent();
      this->ExtentTranslator->GetExtent(this->CurrentPieceExtent);
      // Shift non-zero offset to make geometry continu
      for(int i = 0; i < 3; i++)
        {
        if(this->CurrentPieceExtent[i*2] > 0)
          {
          --this->CurrentPieceExtent[i*2];
          }
        }
      }
    else
      {
      // Just copy
      for(int i=0; i<6; i++)
        {
        this->CurrentPieceExtent[i] = this->CurrentWholeExtent[i];
        }
      }

    // Manage specific data type
    vtkDataArray *rawCoords[3];
    for (int i = 0; i < 3; i++)
      {
      if(this->Variables[coordName[i].c_str()].IsTimeDependent())
        {
        uint64_t offset[4] = { timestep + this->Variables[coordName[i].c_str()].TimeStepOffset,
                               this->CurrentPieceExtent[4],
                               this->CurrentPieceExtent[2],
                               this->CurrentPieceExtent[0] };
        uint64_t count[4] = { 1,
                              this->CurrentPieceExtent[5]-this->CurrentPieceExtent[4],
                              this->CurrentPieceExtent[3]-this->CurrentPieceExtent[2],
                              this->CurrentPieceExtent[1]-this->CurrentPieceExtent[0] };
        rawCoords[i] = this->ReadVariable( this->Variables[coordName[i].c_str()],
                                           offset, count);
        }
      else
        {
        uint64_t offset[4] = { this->CurrentPieceExtent[4],
                               this->CurrentPieceExtent[2],
                               this->CurrentPieceExtent[0],
                               0 };
        uint64_t count[4] = { this->CurrentPieceExtent[5]-this->CurrentPieceExtent[4],
                              this->CurrentPieceExtent[3]-this->CurrentPieceExtent[2],
                              this->CurrentPieceExtent[1]-this->CurrentPieceExtent[0],
                              0 };

        rawCoords[i] = this->ReadVariable( this->Variables[coordName[i].c_str()],
                                           offset, count);
        }
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
      // Need data conversion
      for(vtkIdType k=0; k < 3; k++)
        {
        coords[k] = vtkFloatArray::New();
        coords[k]->SetNumberOfComponents(1);
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
    structuredGrid->SetDimensions(
        this->CurrentPieceExtent[1]-this->CurrentPieceExtent[0],
        this->CurrentPieceExtent[3]-this->CurrentPieceExtent[2],
        this->CurrentPieceExtent[5]-this->CurrentPieceExtent[4]);

    structuredGrid->SetPoints(points);
    points->FastDelete();

    // Build filter var name
    ostringstream varHead;
    varHead << "/Timestep_" << timestep << "/";
    std::string timestepFilter = varHead.str();
    std::string nodesFilter = "/nodes";
    std::string cellsFilter = "/cells";

    // Loop over variable to find the ones to load
    AdiosVariableMapIterator varIter = this->Variables.begin();
    for(; varIter != this->Variables.end(); varIter++)
      {
      if( ((varIter->second.Name.find(timestepFilter) == 0 ^ // Exclusive OR
            !this->HasTimeInformationInVariableNames) )
          && varIter->second.Extent[1] == size3D[0]
          && varIter->second.Extent[3] == size3D[1]
          && varIter->second.Extent[5] == size3D[2]
          && varIter->second.Name.find(nodesFilter) == std::string::npos
          && varIter->second.Name.find(cellsFilter) == std::string::npos )
        {
        vtkDataArray *array = NULL;
        if(varIter->second.IsTimeDependent())
          {
          uint64_t offset[4] = { timestep + varIter->second.TimeStepOffset,
                                 this->CurrentPieceExtent[4],
                                 this->CurrentPieceExtent[2],
                                 this->CurrentPieceExtent[0] };
          uint64_t count[4] = { 1,
                                structuredGrid->GetDimensions()[2],
                                structuredGrid->GetDimensions()[1],
                                structuredGrid->GetDimensions()[0] };
          array = this->ReadVariable(varIter->second, offset, count);
          }
        else
          {
          uint64_t offset[4] = { this->CurrentPieceExtent[4],
                                 this->CurrentPieceExtent[2],
                                 this->CurrentPieceExtent[0],
                                 0 };
          uint64_t count[4] = { structuredGrid->GetDimensions()[2],
                                structuredGrid->GetDimensions()[1],
                                structuredGrid->GetDimensions()[0],
                                0 };
          array = this->ReadVariable(varIter->second, offset, count);
          }
        if(array)
          {
          structuredGrid->GetPointData()->AddArray(array);
          array->FastDelete();
          }
        }
      }
    return structuredGrid;
    }
  // --------------------------------------------------------------------------
  // CAUTION: the GetXGCMesh and AddPointDataToXGCMesh methods can not be called
  // from the same instance of AdiosFile since the mesh and
  // the data are in two different files
  vtkUnstructuredGrid* GetXGCMesh()
    {
    if(!this->Open())
      return NULL;

    // Retreive mesh for XCG format
    int nbNodes, nbTriangles;
    this->GetIntegerAttribute("/nnodes", nbNodes);
    this->GetIntegerAttribute("/cell_set[0]/nbcells", nbTriangles);

    vtkPoints *points = this->ReadPoints("/coordinates/values");
    vtkUnstructuredGrid *grid = vtkUnstructuredGrid::New();
    grid->SetPoints(points);
    points->Delete();

    vtkDataArray *rawData = this->ReadVariable("/cell_set[0]/node_connect_list");
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
        std::string arrayName;
        if(!this->GetStringAttribute(labelKey.str(), arrayName))
          continue;

        ostringstream varKey;
        varKey << "/node_data[" << i << "]/values";
        vtkDataArray* array = this->ReadVariable(varKey.str().c_str());
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
  bool GetIntegerAttribute(const std::string &key, int &value)
    {
    if(!this->Open())
      return false;

    AdiosDataMapIterator iter = this->Attributes.find(key);
    if (iter == this->Attributes.end() || !iter->second.IsInt())
      return false;
    value = iter->second.AsInt();
    return true;
    }
  // --------------------------------------------------------------------------
  bool GetStringAttribute(const std::string &key, std::string &value)
    {
    if(!this->Open())
      return false;

    AdiosDataMapIterator iter = this->Attributes.find(key);
    if (iter == this->Attributes.end() || !iter->second.IsString())
      return false;
    value = iter->second.AsString();
    return true;
    }
  // --------------------------------------------------------------------------
  vtkPoints* ReadPoints(const char* name)
    {
    if(!this->Open())
      return NULL;

    AdiosVariableMapIterator iter = this->Variables.find(std::string(name));
    if(iter == this->Variables.end())
      {
      cout << "ERROR in ReadPoints: Variable " << name << " not found." << endl;
      return NULL;
      }

    // Get var
    AdiosVariable var = iter->second;

    // Read data info
    vtkIdType size3D[3];
    var.GetDimension3D(size3D);
    int nbTuples = var.NumberOfElements / var.GetNumberOfTimeSteps() / var.Dimension;
    uint64_t offsets[4] = { 0,0,0,0 };
    uint64_t counts[4] = { size3D[2],
                           size3D[1],
                           size3D[0],
                           0 };

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
      int64_t retval = adios_read_var_byid( this->Groups[var.GroupIndex],
                                             var.VarIndex, offsets, counts,
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
        int nbComp = var.Dimension;
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
  vtkDataArray* ReadVariable(const char* name)
    {
    if(!this->Open())
      return NULL;

    AdiosVariableMapIterator varIter = this->Variables.find(std::string(name));
    if( varIter == this->Variables.end() )
      {
      cout << "ERROR in ReadVariable: Variable " << name << " not found !" << endl;
      return NULL;
      }

    // Read data info
    AdiosVariable var = varIter->second;
    vtkIdType size3d[3];
    var.GetDimension3D(size3d);
    uint64_t offsets[4] = {0,0,0,0};
    uint64_t counts[4] = { size3d[2],
                           size3d[1],
                           size3d[0],
                           0};

    return this->ReadVariable(var, offsets, counts);
    }
  // --------------------------------------------------------------------------
  vtkDataArray* ReadVariable( const AdiosVariable &var,
                              uint64_t *offsets, uint64_t *counts)
    {
//    cout << "ReadVariable: " << var.Name.c_str() << endl
//        << " - offset: [" << offsets[0] << ", " << offsets[1] << ", "
//        << offsets[2] << ", " << offsets[3] << "]" << endl
//        << " - counts: [" << counts[0] << ", " << counts[1] << ", "
//        << counts[2] << ", " << counts[3] << "]" << endl;

    // Compute the array size
    int nbTuples = 1;
    uint64_t delta;
    for(int i=0; i < 4; i++)
      {
      delta = counts[i];
      if(delta != 0)
        {
        nbTuples *= delta;
        }
      }

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
    array->SetNumberOfTuples(nbTuples); // Allocate array memory

    // Create a nice array name (Just remove the xxx part of /xxx/yy/zzz)
    std::string arrayName = var.Name;
    std::string::size_type index = arrayName.find("/", 1);
    if(index != std::string::npos)
      {
      arrayName = arrayName.substr( index + 1, arrayName.size() - 1 );
      }
    array->SetName(arrayName.c_str());

    int groupIdx = var.GroupIndex;
    this->OpenGroup(groupIdx);

//    cout << "start: " << offsets[0] << " " << offsets[1] << " " << offsets[2] << " " << offsets[3] << endl;
//    cout << "count: " << counts[0] << " " << counts[1] << " " << counts[2] << " " << counts[3] << endl;
//    cout << "varIdx: " << var.VarIndex << endl;
//    cout << "array "<< arrayName.c_str() <<" of type: "<< array->GetClassName() << " with size " << array->GetNumberOfTuples() << endl;

    if( array && (adios_read_var_byid(this->Groups[groupIdx],
                                      var.VarIndex, offsets, counts,
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
    if(!this->Open())
      return 0;
    return this->File->ntimesteps;
    }
  // --------------------------------------------------------------------------
  int GetRealTimeStep(int timeStepIndex)
    {
    std::set<int>::iterator iter = this->RealTimeSteps.begin();
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
    if(!this->Open())
      return;

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
  std::string FileName;
  ADIOS_FILE *File;
  ADIOS_GROUP **Groups;

  AdiosDataMap Scalars;
  AdiosDataMap Attributes;
  AdiosVariableMap Variables;
  std::set<int> RealTimeSteps;

  vtkExtentTranslator *ExtentTranslator;
  int CurrentWholeExtent[6];
  int CurrentPieceExtent[6];
  bool HasTimeInformationInVariableNames;
};
//*****************************************************************************
class AdiosGlobal
{
public:
  static void SetReadMethodToBP()
    {
    adios_set_read_method(ADIOS_READ_METHOD_BP);
    }
  static void SetReadMethodToDART()
    {
    adios_set_read_method(ADIOS_READ_METHOD_DART);
    }

  static void SetReadMethod(int methodEnum)
    {
    //cout << "==> AdiosGlobal::SetReadMethod " << methodEnum << endl;
    switch(methodEnum)
      {
      case 0:
        adios_set_read_method(ADIOS_READ_METHOD_BP);
        break;
      case 1:
        adios_set_read_method(ADIOS_READ_METHOD_HDF5);
        break;
      case 2:
        adios_set_read_method(ADIOS_READ_METHOD_DART);
        break;
      case 3:
        adios_set_read_method(ADIOS_READ_METHOD_DIMES);
        break;
//      case 4:
//        adios_set_read_method(ADIOS_READ_METHOD_DATATAP);
//        break;
      default:
        cout << "Adios read method unknown set to " << methodEnum << endl;
        adios_set_read_method((ADIOS_READ_METHOD)methodEnum);
        break;
      }
    }

  static void SetApplicationId(int id)
    {
    //cout << "==> AdiosGlobal::SetApplicationId " << id << endl;
    globals_adios_set_application_id(id);
    }

  static void Initialize()
    {

#ifdef _NOMPI
    // Nothing to do
#else
    MPI_Comm *comm = NULL;
    vtkMultiProcessController *ctrl =
        vtkMultiProcessController::GetGlobalController();
    if(!ctrl)
      {
      cout << "No global controller set" << endl;
      return;
      }
    //cout << "==> AdiosGlobal::Initialize() " << ctrl->GetLocalProcessId() << endl;
    vtkMPICommunicator *mpiComm =
        vtkMPICommunicator::SafeDownCast(ctrl->GetCommunicator());
    if(mpiComm && mpiComm->GetMPIComm())
      {
      comm = mpiComm->GetMPIComm()->GetHandle();
      }
    if (!adios_read_init(*comm))
      {
      fprintf (stderr, "%s\n", adios_errmsg());
      }
#endif
    }

  static void Finalize()
    {
    //cout << "==> AdiosGlobal::Finalize() " << endl;
#ifdef _NOMPI
    // Nothing to do
#else
    adios_read_finalize();
#endif
    }
};

#endif
//*****************************************************************************
