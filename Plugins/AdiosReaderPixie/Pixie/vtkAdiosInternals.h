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

// Standard library
#include <assert.h>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <sys/stat.h>

using std::ostringstream;

// Variable containers
#include <vtkCharArray.h>
#include <vtkDataArray.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkIntArray.h>
#include <vtkLongArray.h>
#include <vtkShortArray.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnsignedIntArray.h>
#include <vtkUnsignedLongArray.h>
#include <vtkUnsignedShortArray.h>

// VTK related classes
#include <vtkCellData.h>
#include <vtkDataSet.h>
#include <vtkExtentTranslator.h>
#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>
#include <vtkStructuredGrid.h>

// MUST be before adios include
#include <vtkCommunicator.h>
#include <vtkMPI.h>
#include <vtkMPICommunicator.h>
#include <vtkMPICommunicator.h>
#include <vtkMultiProcessController.h>

extern "C" {
#include <adios_error.h>
#include <adios_read.h>
}

#ifndef vtkAdiosInternals_v2_h
#define vtkAdiosInternals_v2_h

//*****************************************************************************
class AdiosGlobal;   // Internal class used to handle adios initialization
class AdiosVariable; // Internal class used to handle adios variables meta-data
class AdiosData;     // Internal class used to handle real data (data array)
class AdiosStream;   // Internal class used to handle adios stream
//*****************************************************************************
typedef std::map<std::string, AdiosData> AdiosDataMap;
typedef AdiosDataMap::const_iterator AdiosDataMapIterator;
typedef std::map<std::string, AdiosVariable> AdiosVariableMap;
typedef AdiosVariableMap::const_iterator AdiosVariableMapIterator;
//*****************************************************************************
class AdiosGlobal
{
public:
  static bool IsInitialized(ADIOS_READ_METHOD method)
  {
    typedef std::map<ADIOS_READ_METHOD, int> InitMapType;
    typedef InitMapType::iterator InitMapIterator;
    InitMapIterator iter = AdiosGlobal::InitializationCounter.find(method);
    if (iter != AdiosGlobal::InitializationCounter.end())
    {
      return (iter->second > 0);
    }
    return false;
  }

  // Description:
  // Initialize only call once the internal adios method.
  // The adios_read_init_method will only be called once until all of the readers
  // have finalized their state.
  static void Initialize(ADIOS_READ_METHOD method, const char* parameters)
  {
    if (!AdiosGlobal::IsInitialized(method))
    {
      AdiosGlobal::InitializationCounter[method] = 1;
      AdiosGlobal::Parameters[method] = (parameters ? parameters : "");

      // Look for MPI controller if any
      MPI_Comm* comm = nullptr;
      vtkMultiProcessController* ctrl = vtkMultiProcessController::GetGlobalController();
      vtkMPICommunicator* mpiComm = vtkMPICommunicator::SafeDownCast(ctrl->GetCommunicator());
      if (mpiComm && mpiComm->GetMPIComm())
      {
        comm = mpiComm->GetMPIComm()->GetHandle();
      }
      if (!comm)
      {
        cerr << "Adios needs a valid MPI communicator." << endl;
        return;
      }

      // Properly initialize Adios method
      AdiosGlobal::MPIController[method] = comm;

      if (0 != adios_read_init_method(method, *comm, parameters))
      {
        // Error occurred
        cerr << "Adios triggered an error while trying to initialize the method: " << method << endl
             << adios_errmsg() << endl
             << "--------------------------------" << endl;
      }
    }
    else
    {
      AdiosGlobal::InitializationCounter[method]++;
    }
  }

  // Description:
  // Finalize only calling once the internal adios method. The real adios_read_finalize_method
  // will only be called once when all the vtkAdiosReader that did an Initialize
  // called a Finalize.
  static void Finalize(ADIOS_READ_METHOD method)
  {
    if (AdiosGlobal::IsInitialized(method))
    {
      AdiosGlobal::InitializationCounter[method]--;

      // Do we need to finalize the method ?
      if (AdiosGlobal::InitializationCounter[method] == 0)
      {
        AdiosGlobal::MPIController[method] = nullptr;
        adios_read_finalize_method(method);
      }
    }
  }

  // Description:
  // Return the parameter that have been used to initialize the adios read method
  static const char* GetMethodParameters(ADIOS_READ_METHOD method)
  {
    return AdiosGlobal::Parameters[method].c_str();
  }

  // Description:
  // Return the MPI controller that was used for the given method
  static MPI_Comm* GetMethodMPIController(ADIOS_READ_METHOD method)
  {
    return AdiosGlobal::MPIController[method];
  }

private:
  static std::map<ADIOS_READ_METHOD, std::string> Parameters;
  static std::map<ADIOS_READ_METHOD, int> InitializationCounter;
  static std::map<ADIOS_READ_METHOD, MPI_Comm*> MPIController;
};
// ----------------------------------------------------------------------------
std::map<ADIOS_READ_METHOD, std::string> AdiosGlobal::Parameters;
std::map<ADIOS_READ_METHOD, int> AdiosGlobal::InitializationCounter;
std::map<ADIOS_READ_METHOD, MPI_Comm*> AdiosGlobal::MPIController;
//*****************************************************************************
class AdiosVariable
{
public:
  // Needed for map management
  AdiosVariable();
  // Description:
  // Copy variable meta-data into our internal object
  AdiosVariable(const char* name, ADIOS_VARINFO* varInfo);
  ~AdiosVariable() {}

  // Description:
  // Return the variable index
  int GetId() const { return this->Id; }

  // Description:
  // Return variable name
  const char* GetName() const { return this->Name.c_str(); }

  // Description:
  // Return the variable dimension 1D, 2D or 3D
  int GetDimension() const { return this->Dimension; }

  // Description:
  // Return size for each of the 3 dimensions
  const uint64_t* GetSize() const { return this->Size; }
  void GetSize(uint64_t size[3]) const
  {
    for (int i = 0; i < 3; ++i)
    {
      size[i] = this->Size[i];
    }
  };

  // Description:
  // Return Adios variable type
  ADIOS_DATATYPES GetType() const { return this->Type; }

  // Description:
  // Return Adios variable type as a user friendly name
  const char* GetTypeAsString() const { return adios_type_to_string(this->Type); }

  // Description:
  // Return the number of elements needed to store the values of that variable
  vtkIdType GetNumberOfElements() const { return this->NumberOfElements; }

  // Description:
  // Return true if the current variable can be handle by VTK
  // Internally we test if the dimension is <= 3
  bool IsValid() const { return this->Dimension <= 3; }

protected:
  friend class AdiosPixie;

  std::string Name;           // Array Name
  int Id;                     // Index of variable in current step
  int Dimension;              // 1D, 2D, 3D
  uint64_t Size[3];           // Size of the Array in each dimension (dim <= 3)
  ADIOS_DATATYPES Type;       // Data type from Adios point of view
  vtkIdType NumberOfElements; // Total number of values in the variable
};
// ----------------------------------------------------------------------------
AdiosVariable::AdiosVariable()
{
  this->Name = "Empty";
  this->Id = -1;
  this->Type = adios_double;
  this->Dimension = -1;
  this->Size[0] = this->Size[1] = this->Size[2] = 0;
  this->NumberOfElements = 0;
}
// ----------------------------------------------------------------------------
AdiosVariable::AdiosVariable(const char* name, ADIOS_VARINFO* varInfo)
{
  this->Name = name;
  this->Id = varInfo->varid;
  this->Type = varInfo->type;
  this->Dimension = varInfo->ndim;
  this->Size[0] = this->Size[1] = this->Size[2] = 0;
  this->NumberOfElements = 1;

  // Fill Dimension Array if valid variable for VTK
  for (int i = 0; i < this->Dimension && this->Dimension <= 3; ++i)
  {
    this->Size[i] = varInfo->dims[i];
    this->NumberOfElements *= this->Size[i];
  }
}
//*****************************************************************************
class AdiosData
{
public:
  // Set of Constructors
  AdiosData() { this->Initialize("Undefined", adios_unknown, nullptr); }
  AdiosData(const char* name, ADIOS_VARINFO* varInfo)
  {
    this->Initialize(name, varInfo->type, varInfo->value);
  }
  AdiosData(const char* name, ADIOS_DATATYPES type, void* data)
  {
    this->Initialize(name, type, data);
  }

  // Destructors
  virtual ~AdiosData()
  {
    if (this->Data)
    {
      free(this->Data);
      this->Data = nullptr;
    }
  }

  // Type testing
  bool IsInt() const { return this->Type == adios_integer; }
  bool IsFloat() const { return this->Type == adios_real; }
  bool IsDouble() const { return this->Type == adios_double; }
  bool IsString() const { return this->Type == adios_string; }

  // Convert Scalar value
  int AsInt() const;
  float AsFloat() const;
  double AsDouble() const;
  std::string AsString() const;

  // Override equal operator
  AdiosData& operator=(const AdiosData& other)
  {
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

private:
  // Internal full constructor
  void Initialize(const char* name, ADIOS_DATATYPES type, void* data);

  std::string Name;
  size_t Size;
  void* Data;
  ADIOS_DATATYPES Type;
};
// ----------------------------------------------------------------------------
void AdiosData::Initialize(const char* name, ADIOS_DATATYPES type, void* data)
{
  this->Name = name;
  this->Type = type;
  this->Data = nullptr;
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
// ----------------------------------------------------------------------------
int AdiosData::AsInt() const
{
  int v;
  memcpy(&v, this->Data, sizeof(int));
  return v;
}
// ----------------------------------------------------------------------------
float AdiosData::AsFloat() const
{
  float v;
  memcpy(&v, this->Data, sizeof(float));
  return v;
}
// ----------------------------------------------------------------------------
double AdiosData::AsDouble() const
{
  double v;
  memcpy(&v, this->Data, sizeof(double));
  return v;
}
// ----------------------------------------------------------------------------
std::string AdiosData::AsString() const
{
  std::string v = (char*)this->Data;
  return v;
}
//*****************************************************************************
class AdiosStream
{
public:
  AdiosStream(const char* fileName, ADIOS_READ_METHOD method, const char* parameters);
  ~AdiosStream();

  // Description:
  // Return true if the stream is currently Open
  bool IsOpen() const { return this->File != nullptr; }

  // Description:
  // Open the stream and load the meta-data
  bool Open();

  // Description:
  // Each step forward should update the MetaData.
  void UpdateMetaData();

  // Description:
  // Close the data stream and release the Adios related resources
  bool Close();

  // Description:
  // Try to read an integer attribute.
  // Return true if success.
  bool ReadIntegerAttribute(const std::string& key, int& value);

  // Description:
  // Try to read a String attribute.
  // Return true if success.
  bool ReadStringAttribute(const std::string& key, std::string& value);

  // Description:
  // Create a vtkDataArray and schedule its data read if possible, otherwise
  // return nullptr;
  vtkDataArray* ReadDataArray(
    const std::string& key, const ADIOS_SELECTION* selection, vtkIdType resultSize);

  // Description:
  // Print meta-data for debug purpose
  void PrintInfo();

  // Description:
  // Return the FileName that was provided at build time
  const char* GetFileName() { return this->FileName.c_str(); }

  // Description:
  // try to move to the next step
  bool NextStep();

protected:
  friend class AdiosPixie;

  std::string FileName;
  std::string Parameters;
  ADIOS_FILE* File;
  ADIOS_READ_METHOD Method;
  AdiosDataMap Scalars;
  AdiosDataMap Attributes;
  AdiosVariableMap MetaData;
  int CurrentStep;
  int LastAvailableStep;

  vtkExtentTranslator* ExtentTranslator;
};
// ----------------------------------------------------------------------------
AdiosStream::AdiosStream(const char* fileName, ADIOS_READ_METHOD method, const char* parameters)
{
  this->Parameters = parameters;
  this->FileName = fileName;
  this->Method = method;
  this->File = nullptr;
  this->CurrentStep = -1;
  this->LastAvailableStep = -1;
  this->ExtentTranslator = nullptr;
}
// ----------------------------------------------------------------------------
AdiosStream::~AdiosStream()
{
  if (this->ExtentTranslator)
  {
    this->ExtentTranslator->Delete();
    this->ExtentTranslator = nullptr;
  }
  this->Close();
}
// ----------------------------------------------------------------------------
bool AdiosStream::Open()
{
  if (this->IsOpen())
  {
    return true;
  }

  // Make sure the method is properly initialized
  AdiosGlobal::Initialize(this->Method, this->Parameters.c_str());

  // Create the Adios file object
  float timeout_msec = 0.0; // 0.0s

  //  cout << "1: adios_read_open_stream(\"" << this->FileName.c_str() << "\", "
  //       << this->Method << ", "
  //       << AdiosGlobal::GetMethodMPIController(this->Method)
  //       << ", ADIOS_LOCKMODE_CURRENT, "
  //       << timeout_msec << ")" << endl;

  this->File = adios_read_open_stream(this->FileName.c_str(), this->Method,
    *AdiosGlobal::GetMethodMPIController(this->Method), ADIOS_LOCKMODE_CURRENT, timeout_msec);

  // Allow to try again for 1 minute if an error occurred
  timeout_msec = 5.0; // Set timeout to 5 seconds
  for (int i = 0; (i < 12) && (adios_errno == err_file_not_found); ++i)
  {
    //    cout << (i+2) <<": adios_read_open_stream(\""
    //         << this->FileName.c_str() << "\", "
    //         << this->Method << ", "
    //         << AdiosGlobal::GetMethodMPIController(this->Method)
    //         << ", ADIOS_LOCKMODE_CURRENT, "
    //         << timeout_msec << ")" << endl;
    cerr << "Wait on stream " << adios_errmsg() << endl;
    sleep(1);
    this->File = adios_read_open_stream(this->FileName.c_str(), this->Method,
      *AdiosGlobal::GetMethodMPIController(this->Method), ADIOS_LOCKMODE_CURRENT, timeout_msec);
  }

  // Make sure the stream is still around
  if (adios_errno == err_end_of_stream)
  {
    // stream has been gone before we tried to open
    cerr << "The stream has terminated before open: " << adios_errmsg() << endl;
  }
  else if (this->File == nullptr)
  {
    // some other error happened
    cerr << "Error while trying to open the stream: " << adios_errmsg() << endl;
  }

  if (!this->File)
  {
    // We failed, make sure we finalize the method initialization
    AdiosGlobal::Finalize(this->Method);
    return false;
  }

  // Everything is good, need to load the Meta-data
  this->UpdateMetaData();

  return true;
}
// ----------------------------------------------------------------------------
void AdiosStream::UpdateMetaData()
{
  this->Scalars.clear();
  this->Attributes.clear();
  this->MetaData.clear();

  // -> Load variables
  for (int varIdx = 0; varIdx < this->File->nvars; ++varIdx)
  {
    // Load meta-data
    ADIOS_VARINFO* varInfo = adios_inq_var_byid(this->File, varIdx);
    if (varInfo == nullptr)
    {
      cerr << "Error opening variable " << this->File->var_namelist[varIdx] << " of bp file "
           << this->FileName.c_str() << ":" << endl
           << adios_errmsg() << endl;
      continue;
    }

    // Skip unwanted variable
    AdiosVariable newVar(this->File->var_namelist[varIdx], varInfo);
    if (!newVar.IsValid())
    {
      cerr << "Skip variable " << newVar.GetName() << " - Dimension: " << newVar.GetDimension()
           << " - Type: " << newVar.GetTypeAsString() << endl;
      free(varInfo);
      continue;
    }

    // Fill scalar value or array meta-data
    if (newVar.GetDimension() == 0)
    {
      // Only store data that have real value
      if (varInfo->value)
      {
        AdiosData scalarData(newVar.GetName(), varInfo);
        this->Scalars[newVar.GetName()] = scalarData;
      }
      else
      {
        //        cerr << "The field " << newVar.GetName()
        //             << " has no value while its dimension is " << newVar.GetDimension()
        //             << endl;
      }
    }
    else
    {
      this->MetaData[newVar.GetName()] = newVar;
    }

    // free adios variable
    free(varInfo);
  }

  // -> Load attributes
  for (int attrIdx = 0; attrIdx < this->File->nattrs; ++attrIdx)
  {
    int size = 0;
    void* data = nullptr;
    ADIOS_DATATYPES attributeType;
    const char* name = this->File->attr_namelist[attrIdx];

    if (adios_get_attr_byid(this->File, attrIdx, &attributeType, &size, &data))
    {
      cerr << "Failed to get attribute " << name << endl;
      continue;
    }

    AdiosData attribute(name, attributeType, data);
    this->Attributes[name] = attribute;

    free(data);
  }

  // Setup time info
  this->CurrentStep = this->File->current_step;
  this->LastAvailableStep = this->File->last_step;
}

// ----------------------------------------------------------------------------
bool AdiosStream::Close()
{
  bool success = true;
  if (this->File)
  {
    success = (0 == adios_read_close(this->File));
    this->File = nullptr;
    AdiosGlobal::Finalize(this->Method);
  }

  if (!success)
  {
    cerr << "Error while trying to close the stream: " << adios_errmsg() << endl;
  }

  return success;
}
// ----------------------------------------------------------------------------
bool AdiosStream::ReadIntegerAttribute(const std::string& key, int& value)
{
  // Make sure the file is successfully open
  if (!this->Open())
  {
    return false;
  }

  // Search for the attribute and make sure it's an integer
  AdiosDataMapIterator iter = this->Attributes.find(key);
  if (iter != this->Attributes.end() && iter->second.IsInt())
  {
    value = iter->second.AsInt();
    return true;
  }

  // Failure
  return false;
}
// ----------------------------------------------------------------------------
bool AdiosStream::ReadStringAttribute(const std::string& key, std::string& value)
{
  // Make sure the file is successfully open
  if (!this->Open())
  {
    return false;
  }

  // Search for the attribute and make sure it's an integer
  AdiosDataMapIterator iter = this->Attributes.find(key);
  if (iter != this->Attributes.end() && iter->second.IsString())
  {
    value = iter->second.AsString();
    return true;
  }

  // Failure
  return false;
}
// ----------------------------------------------------------------------------
vtkDataArray* AdiosStream::ReadDataArray(
  const std::string& key, const ADIOS_SELECTION* selection, vtkIdType resultSize)
{
  // Find the variable if possible
  AdiosVariableMapIterator iter = this->MetaData.find(key);
  if (iter == this->MetaData.end())
  {
    return nullptr;
  }
  AdiosVariable var = iter->second;

  // Create data array based on its type
  vtkDataArray* array = nullptr;
  switch (var.GetType())
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

    case adios_long_double:    // 16 bytes
    case adios_complex:        //  8 bytes
    case adios_double_complex: // 16 bytes
    default:
      cerr << "ERROR: Invalid data type" << endl;
      return nullptr;
      break;
  }

  // Init array meta-data
  array->SetName(var.GetName());
  array->SetNumberOfComponents(1);
  array->SetNumberOfTuples(resultSize);

  // Schedule the read...
  adios_schedule_read_byid(this->File, selection, var.GetId(), 0, 1, array->GetVoidPointer(0));

  // Return the array (which is not properly filled)
  return array;
}
// ----------------------------------------------------------------------------
void AdiosStream::PrintInfo()
{
  if (!this->Open())
  {
    return;
  }

  cout << "CurrentStep: " << this->CurrentStep << endl;
  cout << "LastStep: " << this->LastAvailableStep << endl;

  cout << "Attributes:" << endl;
  AdiosDataMapIterator iter = this->Attributes.begin();
  for (; iter != this->Attributes.end(); iter++)
  {
    cout << " - " << iter->first << endl;
  }

  cout << "Variables:" << endl;
  AdiosVariableMapIterator iter2 = this->MetaData.begin();
  for (; iter2 != this->MetaData.end(); iter2++)
  {
    cout << " - " << iter2->first << " - ndim: " << iter2->second.GetDimension()
         << " - dims: " << iter2->second.GetSize()[0] << "x" << iter2->second.GetSize()[1] << "x"
         << iter2->second.GetSize()[2] << endl;
  }
}
// ----------------------------------------------------------------------------
bool AdiosStream::NextStep()
{
  if (!this->IsOpen() || (this->CurrentStep == this->LastAvailableStep))
  {
    return false;
  }

  if (0 == adios_advance_step(this->File, 0, 0.0))
  {
    this->UpdateMetaData();
  }
  else
  {
    cerr << "Error while trying to move forward: " << adios_errmsg() << endl;
  }

  this->CurrentStep = this->File->current_step;
  this->LastAvailableStep = this->File->last_step;

  return (this->CurrentStep <= this->LastAvailableStep);
}
//*****************************************************************************
class AdiosPixie
{
public:
  static vtkStructuredGrid* NewPixieStructuredGrid(vtkExtentTranslator* extentTranslator,
    AdiosStream* adiosStreamFile, bool loadAllCompatibleFields)
  {
    // Find the dimension of the grid
    vtkStructuredGrid* grid = nullptr;
    vtkDataArray* xCoord = nullptr;
    vtkDataArray* yCoord = nullptr;
    vtkDataArray* zCoord = nullptr;
    int wholeExtent[6] = { 0, 0, 0, 0, 0, 0 };
    int pieceExtent[6] = { 0, 0, 0, 0, 0, 0 };
    uint64_t gridSize[3] = { 0, 0, 0 };
    uint64_t offset[3] = { 0, 0, 0 };
    uint64_t countCells[3] = { 0, 0, 0 };
    uint64_t countPoints[3] = { 0, 0, 0 };
    AdiosVariableMapIterator varIter;
    ADIOS_SELECTION* selectionCells = nullptr;
    ADIOS_SELECTION* selectionPoints = nullptr;

    // Retrieve grid dimension
    std::string nodeVarName = "/nodes/Z"; // Use Z to overcome a bug
    varIter = adiosStreamFile->MetaData.find(nodeVarName);
    if (varIter != adiosStreamFile->MetaData.end())
    {
      varIter->second.GetSize(gridSize);
    }

    // Make sure a grid can be built
    if (gridSize[0] == 0 || gridSize[1] == 0 || gridSize[2] == 0)
    {
      return nullptr;
    }

    // Update extent
    wholeExtent[0] = wholeExtent[2] = wholeExtent[4] = 0;
    wholeExtent[1] = gridSize[0];
    wholeExtent[3] = gridSize[1];
    wholeExtent[5] = gridSize[2];
    extentTranslator->SetWholeExtent(wholeExtent);
    extentTranslator->PieceToExtent();
    extentTranslator->GetExtent(pieceExtent);
    // Shift inner higher end offset to make geometry continu
    for (int i = 0; i < 3; i++)
    {
      if (pieceExtent[i * 2 + 1] < wholeExtent[i * 2 + 1])
      {
        pieceExtent[i * 2 + 1]++;
      }
    }

    // Create selections
    offset[0] = pieceExtent[4];
    offset[1] = pieceExtent[2];
    offset[2] = pieceExtent[0];

    countPoints[0] = pieceExtent[5] - pieceExtent[4];
    countPoints[1] = pieceExtent[3] - pieceExtent[2];
    countPoints[2] = pieceExtent[1] - pieceExtent[0];
    vtkIdType nbPoints = countPoints[0] * countPoints[1] * countPoints[2];

    countCells[0] = countPoints[0] - 1;
    countCells[1] = countPoints[1] - 1;
    countCells[2] = countPoints[2] - 1;
    vtkIdType nbCells = countCells[0] * countCells[1] * countCells[2];

    selectionPoints = adios_selection_boundingbox(3, offset, countPoints);
    selectionCells = adios_selection_boundingbox(3, offset, countCells);

    // Load coordinates data (Will need to release memory after)
    xCoord = adiosStreamFile->ReadDataArray("/nodes/X", selectionPoints, nbPoints);
    yCoord = adiosStreamFile->ReadDataArray("/nodes/Y", selectionPoints, nbPoints);
    zCoord = adiosStreamFile->ReadDataArray("/nodes/Z", selectionPoints, nbPoints);

    // Make sure a grid can still be built
    if (xCoord == nullptr || yCoord == nullptr || zCoord == nullptr)
    {
      return nullptr;
    }

    // Create data structure
    grid = vtkStructuredGrid::New();
    grid->SetDimensions(countPoints[2], countPoints[1], countPoints[0]);
    vtkNew<vtkPoints> points;
    points->SetNumberOfPoints(xCoord->GetNumberOfTuples());
    grid->SetPoints(points.GetPointer());

    // Schedule extra array if needed
    if (loadAllCompatibleFields)
    {
      std::string nodesFilter = "/nodes";
      std::string cellsFilter = "/cells";
      AdiosVariableMapIterator varIter = adiosStreamFile->MetaData.begin();
      for (; varIter != adiosStreamFile->MetaData.end(); varIter++)
      {
        if (varIter->second.GetSize()[0] == gridSize[0] &&
          varIter->second.GetSize()[1] == gridSize[1] &&
          varIter->second.GetSize()[2] == gridSize[2] &&
          varIter->second.Name.find(nodesFilter) == std::string::npos &&
          varIter->second.Name.find(cellsFilter) == std::string::npos)
        {
          vtkDataArray* array =
            adiosStreamFile->ReadDataArray(varIter->second.Name, selectionPoints, nbPoints);
          if (array)
          {
            grid->GetPointData()->AddArray(array);
            array->FastDelete();
          }
        }
        else if (varIter->second.GetSize()[0] == (gridSize[0] - 1) &&
          varIter->second.GetSize()[1] == (gridSize[1] - 1) &&
          varIter->second.GetSize()[2] == (gridSize[2] - 1) &&
          varIter->second.Name.find(nodesFilter) == std::string::npos &&
          varIter->second.Name.find(cellsFilter) == std::string::npos)
        {
          vtkDataArray* array =
            adiosStreamFile->ReadDataArray(varIter->second.Name, selectionCells, nbCells);
          if (array)
          {
            grid->GetCellData()->AddArray(array);
            array->FastDelete();
          }
        }
      }
    }

    // Load the data and release Adios step
    adios_perform_reads(adiosStreamFile->File, 1); // 1: We block
    adios_release_step(adiosStreamFile->File);
    adios_selection_delete(selectionPoints);
    adios_selection_delete(selectionCells);

    // Update min/max data array
    for (int i = 0; i < grid->GetPointData()->GetNumberOfArrays(); ++i)
    {
      grid->GetPointData()->GetArray(i)->Modified();
    }
    for (int i = 0; i < grid->GetCellData()->GetNumberOfArrays(); ++i)
    {
      grid->GetCellData()->GetArray(i)->Modified();
    }

    // Update points coordinates
    for (vtkIdType index = 0; index < points->GetNumberOfPoints(); ++index)
    {
      points->SetPoint(
        index, xCoord->GetTuple1(index), yCoord->GetTuple1(index), zCoord->GetTuple1(index));
    }

    // Free coordinates tmp memory
    xCoord->Delete();
    yCoord->Delete();
    zCoord->Delete();

    // Return the created grid
    return grid;
  }
  // ----------------------------------------------------------------------------
  static vtkImageData* NewPixieImageData(vtkExtentTranslator* extentTranslator,
    AdiosStream* adiosStreamFile, bool loadAllCompatibleFields)
  {
    // Find the dimension of the grid
    vtkImageData* grid = nullptr;
    int wholeExtent[6] = { 0, 0, 0, 0, 0, 0 };
    int pieceExtent[6] = { 0, 0, 0, 0, 0, 0 };
    uint64_t gridSize[3] = { 0, 0, 0 };
    uint64_t offset[3] = { 0, 0, 0 };
    uint64_t countCells[3] = { 0, 0, 0 };
    uint64_t countPoints[3] = { 0, 0, 0 };
    AdiosVariableMapIterator varIter;
    ADIOS_SELECTION* selectionCells = nullptr;
    ADIOS_SELECTION* selectionPoints = nullptr;

    // Retrieve grid dimension
    std::string nodeVarName = "/cells/Z"; // Use Z to overcome a bug
    varIter = adiosStreamFile->MetaData.find(nodeVarName);
    if (varIter != adiosStreamFile->MetaData.end())
    {
      varIter->second.GetSize(gridSize);
    }

    // Make sure a grid can be built
    if (gridSize[0] == 0 || gridSize[1] == 0 || gridSize[2] == 0)
    {
      return nullptr;
    }

    // Update extent
    wholeExtent[0] = wholeExtent[2] = wholeExtent[4] = 0;
    wholeExtent[1] = gridSize[2];
    wholeExtent[3] = gridSize[1];
    wholeExtent[5] = gridSize[0];
    extentTranslator->SetWholeExtent(wholeExtent);
    extentTranslator->PieceToExtent();
    extentTranslator->GetExtent(pieceExtent);

    // Create selections
    offset[0] = pieceExtent[4];
    offset[1] = pieceExtent[2];
    offset[2] = pieceExtent[0];

    countCells[0] = pieceExtent[5] - pieceExtent[4];
    countCells[1] = pieceExtent[3] - pieceExtent[2];
    countCells[2] = pieceExtent[1] - pieceExtent[0];
    vtkIdType nbCells = countCells[0] * countCells[1] * countCells[2];

    countPoints[0] = countCells[0] + 1;
    countPoints[1] = countCells[1] + 1;
    countPoints[2] = countCells[2] + 1;
    vtkIdType nbPoints = countPoints[0] * countPoints[1] * countPoints[2];

    selectionPoints = adios_selection_boundingbox(3, offset, countPoints);
    selectionCells = adios_selection_boundingbox(3, offset, countCells);

    //    cout << "Cell selection: " << endl
    //         << " - Offset: [" <<  offset[0]
    //         << ", "<<  offset[0]
    //         << ", "<<  offset[0] << "]" << endl
    //         << " - Count:  [" <<  countCells[0]
    //         << ", "<<  countCells[0]
    //         << ", "<<  countCells[0] << "]" << endl;

    // Create data structure
    grid = vtkImageData::New();
    grid->SetSpacing(1, 1, 1);
    grid->SetExtent(pieceExtent);

    // Schedule extra array if needed
    if (loadAllCompatibleFields)
    {
      std::string nodesFilter = "/nodes";
      std::string cellsFilter = "/cells";
      AdiosVariableMapIterator varIter = adiosStreamFile->MetaData.begin();
      for (; varIter != adiosStreamFile->MetaData.end(); varIter++)
      {
        if (varIter->second.GetSize()[0] == (gridSize[0] + 1) &&
          varIter->second.GetSize()[1] == (gridSize[1] + 1) &&
          varIter->second.GetSize()[2] == (gridSize[2] + 1) &&
          varIter->second.Name.find(nodesFilter) == std::string::npos &&
          varIter->second.Name.find(cellsFilter) == std::string::npos)
        {
          vtkDataArray* array =
            adiosStreamFile->ReadDataArray(varIter->second.Name, selectionPoints, nbPoints);
          if (array)
          {
            grid->GetPointData()->AddArray(array);
            array->FastDelete();
          }
        }
        else if (varIter->second.GetSize()[0] == gridSize[0] &&
          varIter->second.GetSize()[1] == gridSize[1] &&
          varIter->second.GetSize()[2] == gridSize[2] &&
          varIter->second.Name.find(nodesFilter) == std::string::npos &&
          varIter->second.Name.find(cellsFilter) == std::string::npos)
        {
          //          cout << "id: " << varIter->second.GetId()
          //               << ", name: " << varIter->second.GetName()
          //               << ", size: " << varIter->second.GetSize()[0] << "x"
          //               << varIter->second.GetSize()[1] << "x"
          //               << varIter->second.GetSize()[2]
          //               << endl;
          vtkDataArray* array =
            adiosStreamFile->ReadDataArray(varIter->second.Name, selectionCells, nbCells);
          if (array)
          {
            grid->GetCellData()->AddArray(array);
            array->FastDelete();
          }
        }
      }
    }

    // Load the data and release Adios step
    adios_perform_reads(adiosStreamFile->File, 1); // 1: We block
    adios_release_step(adiosStreamFile->File);
    adios_selection_delete(selectionPoints);
    adios_selection_delete(selectionCells);

    // Update min/max data array
    for (int i = 0; i < grid->GetPointData()->GetNumberOfArrays(); ++i)
    {
      grid->GetPointData()->GetArray(i)->Modified();
    }
    for (int i = 0; i < grid->GetCellData()->GetNumberOfArrays(); ++i)
    {
      grid->GetCellData()->GetArray(i)->Modified();
    }

    // Return the created grid
    return grid;
  }
};
#endif
