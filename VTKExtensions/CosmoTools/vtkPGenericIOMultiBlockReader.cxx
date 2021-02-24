/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkPGenericIOMultiBlockReader.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkPGenericIOMultiBlockReader.h"

#include "vtkCallbackCommand.h"
#include "vtkCellArray.h"
#include "vtkCellType.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataArraySelection.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkGenericIOUtilities.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationKey.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkType.h"
#include "vtkTypeUInt64Array.h"
#include "vtkUnstructuredGrid.h"

#include "GenericIOReader.h"
#include "GenericIOUtilities.h"

#include <map>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

#include <unistd.h>

// Uncomment the line below to get debugging information
//#define DEBUG

namespace
{
struct block_t
{
  vtkTypeUInt64 GlobalId;
  vtkTypeUInt64 NumberOfElements;
  double bounds[6];
  uint64_t coords[3];
  int ProcessId;

  std::map<std::string, void*> RawCache;
  std::map<std::string, bool> VariableStatus;
};

struct most_of_block_t
{
  vtkTypeUInt64 GlobalId;
  vtkTypeUInt64 NumberOfElements;
  double bounds[6];
  uint64_t coords[3];
  int ProcessId;
};
}

//------------------------------------------------------------------------------
class vtkPGenericIOMultiBlockReader::vtkGenericIOMultiBlockMetaData
{
public:
  int NumberOfBlocks;        // Total number of blocks across all ranks
  int TotalNumberOfElements; // Total number of points across all blocks
  std::map<std::string, gio::VariableInfo> VariableInformation;
  std::map<std::string, int> VariableGenericIOType;
  std::map<int, block_t> Blocks;

  /**
   * @brief Metadata constructor.
   */
  vtkGenericIOMultiBlockMetaData() {}

  /**
   * @brief Destructor
   */
  ~vtkGenericIOMultiBlockMetaData() { this->Clear(); }

  /**
   * @brief Checks if the supplied rank should load data.
   * @param r the rank in query.
   * @return status true or false.
   */
  bool HasBlock(const int blockId) { return (this->Blocks.find(blockId) != this->Blocks.end()); }

  bool BlockIsOnMyProcess(const int blockId, const int myProcessId)
  {
    return this->Blocks.find(blockId) != this->Blocks.end() &&
      this->Blocks[blockId].ProcessId == myProcessId;
  }

  /**
   * @brief Get the raw MPI communicator from a Multi-process controller.
   * @param controller the multi-process controller
   * /
  void InitCommunicator(vtkMultiProcessController *controller)
  {
    assert("pre: controller is nullptr!" && (controller != nullptr) );
    this->MPICommunicator =
        vtkGenericIOUtilities::GetMPICommunicator(controller);
  } */

  /**
   * @brief Performs a quick sanity on the metadata
   * @return status false if the metadata is somehow corrupted.
   */
  bool SanityCheck()
  {
    return (this->VariableInformation.size() == this->VariableGenericIOType.size());
  }

  /**
   * @brief Checks if a variable exists
   * @param varName the name of the variable in query
   * @return status true or false
   */
  bool HasVariable(const std::string& varName)
  {
    return (this->VariableInformation.find(varName) != this->VariableInformation.end());
  }

  /**
   * @brief Clears the metadata
   */
  void Clear()
  {
    this->NumberOfBlocks = 0;
    this->VariableGenericIOType.clear();
    this->VariableInformation.clear();

    for (std::map<int, block_t>::iterator blockItr = this->Blocks.begin();
         blockItr != this->Blocks.end(); ++blockItr)
    {
      std::map<std::string, void*>::iterator varItr = blockItr->second.RawCache.begin();
      for (; varItr != blockItr->second.RawCache.end(); ++varItr)
      {
        if (varItr->second != nullptr)
        {
          delete[] static_cast<char*>(varItr->second);
        }
      }
    }
    this->Blocks.clear();
  }
};

vtkStandardNewMacro(vtkPGenericIOMultiBlockReader);
//------------------------------------------------------------------------------
vtkPGenericIOMultiBlockReader::vtkPGenericIOMultiBlockReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->Controller = vtkMultiProcessController::GetGlobalController();
  this->Reader = nullptr;
  this->FileName = nullptr;
  this->XAxisVariableName = nullptr;
  this->YAxisVariableName = nullptr;
  this->ZAxisVariableName = nullptr;
  this->HaloIdVariableName = nullptr;
  this->GenericIOType = IOTYPEMPI;
  this->BlockAssignment = ROUND_ROBIN;
  this->BuildMetaData = false;

  this->SetXAxisVariableName("x");
  this->SetYAxisVariableName("y");
  this->SetZAxisVariableName("z");

  this->MetaData = new vtkGenericIOMultiBlockMetaData();
  // this->MetaData->InitCommunicator(this->Controller);

  this->ArrayList = vtkStringArray::New();
  this->PointDataArraySelection = vtkDataArraySelection::New();
  this->HaloList = vtkIdList::New();
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(&vtkPGenericIOMultiBlockReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);
  this->PointDataArraySelection->AddObserver(vtkCommand::ModifiedEvent, this->SelectionObserver);
}

//------------------------------------------------------------------------------
vtkPGenericIOMultiBlockReader::~vtkPGenericIOMultiBlockReader()
{
  if (this->Reader != nullptr)
  {
    this->Reader->Close();
    delete this->Reader;
  }

  vtkGenericIOUtilities::SafeDeleteString(this->FileName);
  vtkGenericIOUtilities::SafeDeleteString(this->XAxisVariableName);
  vtkGenericIOUtilities::SafeDeleteString(this->YAxisVariableName);
  vtkGenericIOUtilities::SafeDeleteString(this->ZAxisVariableName);
  vtkGenericIOUtilities::SafeDeleteString(this->HaloIdVariableName);

  if (this->MetaData != nullptr)
  {
    delete this->MetaData;
  }

  this->ArrayList->Delete();
  this->PointDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->HaloList->Delete();
  this->SelectionObserver->Delete();
  this->PointDataArraySelection->Delete();

  this->Controller = nullptr;
}

//------------------------------------------------------------------------------
void vtkPGenericIOMultiBlockReader::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << this->FileName << endl;
  os << indent << "x-axis: " << this->XAxisVariableName << endl;
  os << indent << "y-axis: " << this->YAxisVariableName << endl;
  os << indent << "z-axis: " << this->ZAxisVariableName << endl;
  os << indent << "GenericIOType: " << this->GenericIOType << endl;
  os << indent << "BlockAssignment: " << this->BlockAssignment << endl;
  os << indent << "ArrayList: " << endl;
  this->ArrayList->PrintSelf(os, indent.GetNextIndent());
  os << indent << "PointDataSelection: " << endl;
  this->PointDataArraySelection->PrintSelf(os, indent.GetNextIndent());
  if (Controller != nullptr)
  {
    os << indent << "Controller: " << this->Controller << endl;
  }
  else
  {
    os << indent << "Controller: (null)" << endl;
  }
}

//------------------------------------------------------------------------------
int vtkPGenericIOMultiBlockReader::GetNumberOfPointArrays()
{
  return this->PointDataArraySelection->GetNumberOfArrays();
}

//------------------------------------------------------------------------------
const char* vtkPGenericIOMultiBlockReader::GetPointArrayName(int i)
{
  assert("pre: array index is out-of-bounds!" && (i >= 0) && (i < this->GetNumberOfPointArrays()));
  return this->PointDataArraySelection->GetArrayName(i);
}

//------------------------------------------------------------------------------
int vtkPGenericIOMultiBlockReader::GetPointArrayStatus(const char* name)
{
  assert(this->PointDataArraySelection->ArrayExists(name));
  return this->PointDataArraySelection->ArrayIsEnabled(name);
}

//------------------------------------------------------------------------------
void vtkPGenericIOMultiBlockReader::SetPointArrayStatus(const char* name, int status)
{
  assert(this->PointDataArraySelection->ArrayExists(name));
  if (status)
  {
    this->PointDataArraySelection->EnableArray(name);
  }
  else
  {
    this->PointDataArraySelection->DisableArray(name);
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkPGenericIOMultiBlockReader::GetRequestedHaloId(vtkIdType i)
{
  assert("pre: array index out of bounds" && (i >= 0 && this->HaloList->GetNumberOfIds() > i));
  return this->HaloList->GetId(i);
}

//------------------------------------------------------------------------------
vtkIdType vtkPGenericIOMultiBlockReader::GetNumberOfRequestedHaloIds()
{
  return this->HaloList->GetNumberOfIds();
}

//------------------------------------------------------------------------------
void vtkPGenericIOMultiBlockReader::SetNumberOfRequestedHaloIds(vtkIdType numIds)
{
  this->HaloList->SetNumberOfIds(numIds);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkPGenericIOMultiBlockReader::AddRequestedHaloId(vtkIdType haloId)
{
  this->SetRequestedHaloId(this->GetNumberOfRequestedHaloIds(), haloId);
}

//------------------------------------------------------------------------------
void vtkPGenericIOMultiBlockReader::ClearRequestedHaloIds()
{
  this->HaloList->Reset();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkPGenericIOMultiBlockReader::SetRequestedHaloId(vtkIdType i, vtkIdType haloId)
{
  *this->HaloList->WritePointer(i, 1) = haloId;
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkPGenericIOMultiBlockReader::ReaderParametersChanged()
{
  assert("pre: internal reader is nullptr!" && (this->Reader != nullptr));

  if (this->Reader->GetFileName() != std::string(this->FileName))
  {
#ifdef DEBUG
    std::cout << "\t[INFO]: File name has changed!\n";
    std::cout.flush();
#endif
    return true;
  }

  bool status = false;
  switch (this->Reader->GetIOStrategy())
  {
    case gio::GenericIOBase::FileIOMPI:
      status = (this->GenericIOType != IOTYPEMPI) ? true : false;
#ifdef DEBUG
      if (status == true)
      {
        std::cout << "\t[INFO]: I/O strategy changed from MPI\n";
        std::cout.flush();
      }
#endif
      break;
    case gio::GenericIOBase::FileIOPOSIX:
      status = (this->GenericIOType != IOTYPEPOSIX) ? true : false;
#ifdef DEBUG
      if (status == true)
      {
        std::cout << "\t[INFO]: I/O strategy changed from POSIX\n";
        std::cout.flush();
      }
#endif
      break;
    default:
      assert("pre: Code should not reach here!" && false);
      throw std::runtime_error("Invalid I/O strategy!\n");
  } // END switch on I/O strategy

  if (status == true)
  {
    /* short-circuit here */
    return (status);
  }

  switch (this->Reader->GetBlockAssignmentStrategy())
  {
    case gio::RR_BLOCK_ASSIGNMENT:
      status = (this->BlockAssignment != ROUND_ROBIN) ? true : false;
#ifdef DEBUG
      if (status == true)
      {
        std::cout << "\t[INFO]: I/O block assignment changed to Round-Robin\n";
        std::cout.flush();
      }
#endif
      break;
    case gio::RCB_BLOCK_ASSIGNMENT:
      status = (this->BlockAssignment != RCB) ? true : false;
#ifdef DEBUG
      if (status == true)
      {
        std::cout << "\t[INFO]: I/O block assignment changed to RCB\n";
        std::cout.flush();
      }
#endif
      break;
    default:
      assert("pre: Code should not reach here!" && false);
      throw std::runtime_error("Invalid BlockAssignment strategy!\n");
  } // END switch on BlockAssignment

  return (status);
}

//------------------------------------------------------------------------------
gio::GenericIOReader* vtkPGenericIOMultiBlockReader::GetInternalReader()
{
  if (this->Reader != nullptr)
  {
    if (this->ReaderParametersChanged())
    {
#ifdef DEBUG
      std::cout << "\t[INFO]: Deleting Reader instance...\n";
      std::cout.flush();
#endif
      this->Reader->Close();
      delete this->Reader;
      this->Reader = nullptr;
    } // END if the reader parameters
    else
    {
      return (this->Reader);
    }
  } // END if the reader is not nullptr

  this->BuildMetaData = true; // signal to re-build metadata

  assert("pre: Reader should be nullptr" && (this->Reader == nullptr));
  gio::GenericIOReader* r = nullptr;
  bool posix = (this->GenericIOType == IOTYPEMPI) ? false : true;
  int distribution =
    (this->BlockAssignment == RCB) ? gio::RCB_BLOCK_ASSIGNMENT : gio::RR_BLOCK_ASSIGNMENT;

  r = vtkGenericIOUtilities::GetReader(vtkGenericIOUtilities::GetMPICommunicator(this->Controller),
    posix, distribution, std::string(this->FileName));
  assert("post: Internal GenericIO reader should not be nullptr!" && (r != nullptr));

  return (r);
}

//------------------------------------------------------------------------------
void vtkPGenericIOMultiBlockReader::LoadMetaData()
{
  if (!this->BuildMetaData)
  {
#ifdef DEBUG
    std::cout << "\t[INFO]: No need to update metadata!\n";
    std::cout.flush();
#endif
    return;
  }

  this->MetaData->Clear();
  this->PointDataArraySelection->RemoveAllArrays();
  this->ArrayList->SetNumberOfValues(0);

#ifdef DEBUG
  std::cout << "\t[INFO]: Reading header to build metadata!\n";
  std::cout << "\t[INFO]: Filename: " << this->FileName << std::endl;
  std::cout.flush();
#endif
  this->Reader->OpenAndReadHeader();
  this->MetaData->TotalNumberOfElements = this->Reader->GetNumberOfElements();

  // load variable information
  for (int i = 0; i < this->Reader->GetNumberOfVariablesInFile(); ++i)
  {
    std::string vname = this->Reader->GetVariableName(i);
    this->ArrayList->InsertNextValue(vname.c_str());
    this->PointDataArraySelection->AddArray(vname.c_str());
    this->PointDataArraySelection->DisableArray(vname.c_str());

    this->MetaData->VariableInformation[vname] = this->Reader->GetFileVariableInfo(i);
    this->MetaData->VariableGenericIOType[vname] =
      gio::GenericIOUtilities::DetectVariablePrimitiveType(
        this->MetaData->VariableInformation[vname]);
  } // end for all variables in file

  // load block information
  this->MetaData->NumberOfBlocks = this->Reader->GetTotalNumberOfBlocks();

  std::vector<most_of_block_t> localBlocks;
  localBlocks.resize(this->Reader->GetNumberOfAssignedBlocks());
  double min[3];
  double max[3];
  int myProcessId = this->Controller->GetLocalProcessId();
  for (int i = 0; i < this->Reader->GetNumberOfBlockHeaders(); ++i)
  {
    most_of_block_t& myBlock = localBlocks[i];
    gio::RankHeader block = this->Reader->GetBlockHeader(i);
    assert(
      "pre: loading duplicate block in metadata!" && !this->MetaData->HasBlock(block.GlobalRank));

    myBlock.GlobalId = block.GlobalRank;
    myBlock.NumberOfElements = block.NElems;

    if (this->Reader->IsSpatiallyDecomposed())
    {
      this->Reader->GetBlockBounds(i, min, max);
      this->Reader->GetBlockCoords(i, myBlock.coords);
      myBlock.bounds[0] = min[0];
      myBlock.bounds[1] = max[0];
      myBlock.bounds[2] = min[1];
      myBlock.bounds[3] = max[1];
      myBlock.bounds[4] = min[2];
      myBlock.bounds[5] = max[2];
    }
    else
    {
      for (int j = 0; j < 6; ++j)
      {
        myBlock.bounds[j] = 0;
      }
      myBlock.coords[0] = myBlock.coords[1] = myBlock.coords[2] = 0;
    }
    myBlock.ProcessId = myProcessId;
  }
  std::vector<most_of_block_t> allBlocks;
  allBlocks.resize(this->Reader->GetTotalNumberOfBlocks());
  assert(allBlocks.size() == localBlocks.size() * this->Controller->GetNumberOfProcesses());
  this->Controller->AllGather((const char*)&localBlocks[0], (char*)&allBlocks[0],
    localBlocks.size() * sizeof(most_of_block_t));

  for (unsigned i = 0; i < allBlocks.size(); ++i)
  {
    most_of_block_t& blockData = allBlocks[i];
    block_t block;
    block.GlobalId = blockData.GlobalId;
    block.NumberOfElements = blockData.NumberOfElements;
    block.ProcessId = blockData.ProcessId;
    block.coords[0] = blockData.coords[0];
    block.coords[1] = blockData.coords[1];
    block.coords[2] = blockData.coords[2];
    block.bounds[0] = blockData.bounds[0];
    block.bounds[1] = blockData.bounds[1];
    block.bounds[2] = blockData.bounds[2];
    block.bounds[3] = blockData.bounds[3];
    block.bounds[4] = blockData.bounds[4];
    block.bounds[5] = blockData.bounds[5];
    if (block.ProcessId == myProcessId)
    {
      for (int var = 0; var < this->Reader->GetNumberOfVariablesInFile(); ++var)
      {
        std::string vname = this->Reader->GetVariableName(var);
        block.RawCache[vname] = nullptr;
        block.VariableStatus[vname] = false;
      }
    }
    this->MetaData->Blocks[block.GlobalId] = block;
  }

  this->BuildMetaData = false;
}

//------------------------------------------------------------------------------
void vtkPGenericIOMultiBlockReader::LoadRawVariableDataForBlock(
  const std::string& varName, int blockId)
{
#ifdef DEBUG
  std::cout << "[INFO]: Loading variable: " << varName << std::endl;
  std::cout.flush();
#endif
  assert("pre: Reader is nullptr" && (this->Reader != nullptr));
  assert("pre: metadata is nullptr" && (this->MetaData != nullptr));
  assert("pre: metadata is corrupt" && (this->MetaData->SanityCheck()));
  assert("pre: block is owned by another process" && this->MetaData->HasBlock(blockId));
  assert("pre: metadata does not have requested variable" && this->MetaData->HasVariable(varName));

  block_t& dataBlock = this->MetaData->Blocks[blockId];

  assert("pre: no variable by this name!" &&
    (dataBlock.RawCache.find(varName) != dataBlock.RawCache.end()) &&
    (dataBlock.VariableStatus.find(varName) != dataBlock.VariableStatus.end()));

  if (dataBlock.VariableStatus[varName])
  {
#ifdef DEBUG
    std::cout << "\t[INFO]: Variable appears to be already loaded "
              << "for BLOCK=" << blockId << std::endl;
    std::cout.flush();
#endif
    return;
  }

  dataBlock.RawCache[varName] = gio::GenericIOUtilities::AllocateVariableArray(
    this->MetaData->VariableInformation[varName], dataBlock.NumberOfElements);

  this->Reader->AddVariable(
    this->MetaData->VariableInformation[varName], dataBlock.RawCache[varName]);

  dataBlock.VariableStatus[varName] = true;

#ifdef DEBUG
  std::cout << "\t[INFO]: Variable [" << varName << "] is now loaded "
            << "for BLOCK=" << blockId << std::endl;
  std::cout.flush();
#endif
}

//------------------------------------------------------------------------------
void vtkPGenericIOMultiBlockReader::LoadRawDataForBlock(int blockId)
{
  assert("pre: Reader is nullptr" && (this->Reader != nullptr));
  assert("pre: metadata is nullptr" && (this->MetaData != nullptr));
  assert("pre: metadata is corrupt!" && (this->MetaData->SanityCheck()));
  assert("pre: block is not owned by this process!" && this->MetaData->HasBlock(blockId));

  // This method is called for every block, so we must clear any previously
  // registered variables
  this->Reader->ClearVariables();

  std::string xaxis = std::string(this->XAxisVariableName);
  xaxis = vtkGenericIOUtilities::trim(xaxis);

  std::string yaxis = std::string(this->YAxisVariableName);
  yaxis = vtkGenericIOUtilities::trim(yaxis);

  std::string zaxis = std::string(this->ZAxisVariableName);
  zaxis = vtkGenericIOUtilities::trim(zaxis);

  this->LoadRawVariableDataForBlock(xaxis, blockId);
  this->LoadRawVariableDataForBlock(yaxis, blockId);
  this->LoadRawVariableDataForBlock(zaxis, blockId);

  if (this->HaloList->GetNumberOfIds() > 0)
  {
    std::string haloIds = std::string(this->HaloIdVariableName);
    haloIds = vtkGenericIOUtilities::trim(haloIds);
    this->LoadRawVariableDataForBlock(haloIds, blockId);
  }

#ifdef DEBUG
  std::cout << "\t==========\n";
  std::cout << "\tNUMBER OF ARRAYS: " << this->PointDataArraySelection->GetNumberOfArrays()
            << std::endl;
  std::cout.flush();
#endif

  int arrayIdx = 0;
  for (; arrayIdx < this->PointDataArraySelection->GetNumberOfArrays(); ++arrayIdx)
  {
    const char* name = this->PointDataArraySelection->GetArrayName(arrayIdx);
#ifdef DEBUG
    std::cout << "\tARRAY " << name << " is ";
#endif
    if (this->PointDataArraySelection->ArrayIsEnabled(name))
    {
#ifdef DEBUG
      std::cout << "ENABLED\n";
      std::cout.flush();
#endif
      std::string varName = std::string(name);
      this->LoadRawVariableDataForBlock(varName, blockId);
    } // END if the array is enabled
    else
    {
#ifdef DEBUG
      std::cout << "DISABLED\n";
      std::cout.flush();
#endif
    }
  } // END for all arrays

#ifdef DEBUG
  std::cout << "\t[INFO]: Reading data...";
#endif

  this->Reader->ReadBlock(blockId);

#ifdef DEBUG
  std::cout << "[DONE]\n";
  std::cout.flush();
#endif
}

//------------------------------------------------------------------------------
void vtkPGenericIOMultiBlockReader::GetPointFromRawData(int xType, void* xBuffer, int yType,
  void* yBuffer, int zType, void* zBuffer, vtkIdType idx, double pnt[3])
{
  void* buffer[3] = { xBuffer, yBuffer, zBuffer };

  int type[3] = { xType, yType, zType };

  for (int i = 0; i < 3; ++i)
  {
    //    type = this->MetaData->VariableGenericIOType[ name[i] ];
    //    void *rawBuffer = this->MetaData->Blocks[ blockId ].RawCache[ name[i] ];
    assert("pre: raw buffer is nullptr!" && (buffer[i] != nullptr));

    pnt[i] = vtkGenericIOUtilities::GetDoubleFromRawBuffer(type[i], buffer[i], idx);
  } // END for all dimensions
}

//------------------------------------------------------------------------------
void vtkPGenericIOMultiBlockReader::LoadCoordinatesForBlock(
  vtkUnstructuredGrid* grid, std::set<vtkIdType>& pointsInSelectedHalos, int blockId)
{
  assert("pre: metadata is nullptr!" && (this->MetaData != nullptr));
  assert("pre: grid is nullptr!" && (grid != nullptr));
  assert("pre: block is not owned by this process!" && this->MetaData->HasBlock(blockId));

  std::string xaxis = std::string(this->XAxisVariableName);
  xaxis = vtkGenericIOUtilities::trim(xaxis);

  std::string yaxis = std::string(this->YAxisVariableName);
  yaxis = vtkGenericIOUtilities::trim(yaxis);

  std::string zaxis = std::string(this->ZAxisVariableName);
  zaxis = vtkGenericIOUtilities::trim(zaxis);

  if (!this->MetaData->HasVariable(xaxis) || !this->MetaData->HasVariable(yaxis) ||
    !this->MetaData->HasVariable(zaxis))
  {
    vtkErrorMacro(<< "Don't have one or more coordinate arrays!\n");
    return;
  }
  block_t& dataBlock = this->MetaData->Blocks[blockId];

  int xType = this->MetaData->VariableGenericIOType[xaxis];
  void* xBuffer = dataBlock.RawCache[xaxis];
  int yType = this->MetaData->VariableGenericIOType[yaxis];
  void* yBuffer = dataBlock.RawCache[yaxis];
  int zType = this->MetaData->VariableGenericIOType[zaxis];
  void* zBuffer = dataBlock.RawCache[zaxis];

  int nparticles = dataBlock.NumberOfElements;

  vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
  cells->Allocate(cells->EstimateSize(nparticles, 1));

  vtkSmartPointer<vtkPoints> pnts = vtkSmartPointer<vtkPoints>::New();
  pnts->SetDataTypeToDouble();
  pnts->SetNumberOfPoints(nparticles);

  double pnt[3];
  vtkIdType idx = 0;
  if (this->HaloList->GetNumberOfIds() == 0)
  {
    for (; idx < nparticles; ++idx)
    {
      this->GetPointFromRawData(xType, xBuffer, yType, yBuffer, zType, zBuffer, idx, pnt);
      pnts->SetPoint(idx, pnt);
      cells->InsertNextCell(1, &idx);
    } // END for all points
  }
  else
  {
    std::string haloVarName = std::string(this->HaloIdVariableName);
    haloVarName = vtkGenericIOUtilities::trim(haloVarName);
    int haloType = this->MetaData->VariableGenericIOType[haloVarName];
    void* haloBuffer = dataBlock.RawCache[haloVarName];
    for (vtkIdType i = 0; idx < nparticles; ++idx)
    {
      vtkIdType haloId = vtkGenericIOUtilities::GetIdFromRawBuffer(haloType, haloBuffer, idx);
      bool isInRequestedHalo = false;
      for (vtkIdType j = 0; j < this->GetNumberOfRequestedHaloIds(); ++j)
      {
        if (haloId == this->HaloList->GetId(j))
        {
          isInRequestedHalo = true;
          pointsInSelectedHalos.insert(idx);
          break;
        }
      }
      if (isInRequestedHalo)
      {
        this->GetPointFromRawData(xType, xBuffer, yType, yBuffer, zType, zBuffer, idx, pnt);
        pnts->SetPoint(i, pnt);
        cells->InsertNextCell(1, &i);
        ++i;
      }
    }
    pnts->SetNumberOfPoints(pointsInSelectedHalos.size());
  }

  grid->SetPoints(pnts);

  grid->SetCells(VTK_VERTEX, cells);

  grid->Squeeze();
}

namespace
{
template <typename T>
void GetOnlyDataInHalo(
  vtkDataArray* allData, vtkDataArray* haloData, std::set<vtkIdType> pointsInHalo)
{
  T* data = (T*)allData->GetVoidPointer(0);
  T* filteredData = (T*)haloData->GetVoidPointer(0);
  vtkIdType i = 0;
  for (std::set<vtkIdType>::iterator itr = pointsInHalo.begin(); itr != pointsInHalo.end(); ++itr)
  {
    filteredData[i++] = data[*itr];
  }
}
}

//------------------------------------------------------------------------------
void vtkPGenericIOMultiBlockReader::LoadDataArraysForBlock(
  vtkUnstructuredGrid* grid, const std::set<vtkIdType>& pointsInSelectedHalos, int blockId)
{
  assert("pre: metadata is nullptr!" && (this->MetaData != nullptr));
  assert("pre: grid is nullptr!" && (grid != nullptr));
  assert("pre: block is not owned by this process!" && this->MetaData->HasBlock(blockId));

  block_t& dataBlock = this->MetaData->Blocks[blockId];

  //  assert("pre: # points in dataset different from points in block" &&
  //    (static_cast<uint64_t>(grid->GetNumberOfPoints()) == dataBlock.NumberOfElements));
  vtkPointData* PD = grid->GetPointData();

  int arrayIdx = 0;
  for (; arrayIdx < this->PointDataArraySelection->GetNumberOfArrays(); ++arrayIdx)
  {
    const char* name = this->PointDataArraySelection->GetArrayName(arrayIdx);
    if (this->PointDataArraySelection->ArrayIsEnabled(name))
    {
      std::string varName(name);
      vtkSmartPointer<vtkDataArray> dataArray;
      dataArray.TakeReference(vtkGenericIOUtilities::GetVtkDataArray(varName,
        this->MetaData->VariableGenericIOType[varName], dataBlock.RawCache[varName],
        dataBlock.NumberOfElements));
      if (this->HaloList->GetNumberOfIds() != 0)
      {
        vtkSmartPointer<vtkDataArray> onlyDataInHalo;
        onlyDataInHalo.TakeReference(dataArray->NewInstance());
        onlyDataInHalo->SetNumberOfTuples(grid->GetNumberOfPoints());
        onlyDataInHalo->SetName(dataArray->GetName());
        switch (dataArray->GetDataType())
        {
          vtkTemplateMacro(
            GetOnlyDataInHalo<VTK_TT>(dataArray, onlyDataInHalo, pointsInSelectedHalos));
        }
        dataArray = onlyDataInHalo;
      }

      PD->AddArray(dataArray);
    } // END if the array is enabled
  }   // END for all arrays
}

//------------------------------------------------------------------------------
vtkUnstructuredGrid* vtkPGenericIOMultiBlockReader::LoadBlock(int blockId)
{
  // Sanity Check
  assert("pre: metadata is nullptr" && (this->MetaData != nullptr));
  assert("pre: block is not owned by this process!" && this->MetaData->HasBlock(blockId));
  // STEP 1: Load raw data
  this->LoadRawDataForBlock(blockId);

  vtkUnstructuredGrid* grid = vtkUnstructuredGrid::New();
  std::set<vtkIdType> pointsInSelectedHalos;

  // STEP 2: Load coordinates
  this->LoadCoordinatesForBlock(grid, pointsInSelectedHalos, blockId);

  // STEP 3: Load data
  this->LoadDataArraysForBlock(grid, pointsInSelectedHalos, blockId);

  if (this->Reader->IsSpatiallyDecomposed())
  {
    vtkSmartPointer<vtkTypeUInt64Array> coords = vtkSmartPointer<vtkTypeUInt64Array>::New();
    coords->SetNumberOfComponents(3);
    coords->SetNumberOfTuples(1);
    coords->SetName("genericio_block_coords");
    coords->SetTypedTuple(0, (vtkTypeUInt64*)this->MetaData->Blocks[blockId].coords);
    grid->GetFieldData()->AddArray(coords);
  }

  return grid;
}

//------------------------------------------------------------------------------
void vtkPGenericIOMultiBlockReader::SelectionModifiedCallback(vtkObject* vtkNotUsed(caller),
  unsigned long vtkNotUsed(eid), void* clientdata, void* vtkNotUsed(calldata))
{
  assert("pre: clientdata is nullptr!" && (clientdata != nullptr));
  static_cast<vtkPGenericIOMultiBlockReader*>(clientdata)->Modified();
}

//------------------------------------------------------------------------------
int vtkPGenericIOMultiBlockReader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
#ifdef DEBUG
  std::cout << "[INFO]: Call RequestInformation()\n";
  std::cout.flush();
#endif

  //  ++this->RequestInfoCounter;

  // tell the pipeline that this dataset is distributed
  outputVector->GetInformationObject(0)->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  outputVector->GetInformationObject(0)->Set(
    vtkDataObject::DATA_NUMBER_OF_PIECES(), this->Controller->GetNumberOfProcesses());

  this->Reader = this->GetInternalReader();
  assert("pre: internal reader is nullptr!" && (this->Reader != nullptr));

  this->LoadMetaData();

  vtkSmartPointer<vtkMultiBlockDataSet> outline = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  outline->SetNumberOfBlocks(this->MetaData->NumberOfBlocks);
  int myProcessId = this->Controller->GetLocalProcessId();
  for (int i = 0; i < this->MetaData->NumberOfBlocks; ++i)
  {
    outline->SetBlock(i, nullptr);
    if (this->MetaData->HasBlock(i))
    {
      vtkInformation* blockInfo = outline->GetMetaData(i);
      assert("pre: block info is nullptr!" && (blockInfo != nullptr));
      // bounds are meaningless if this is false
      if (this->Reader->IsSpatiallyDecomposed())
      {
        blockInfo->Set(
          vtkStreamingDemandDrivenPipeline::BOUNDS(), this->MetaData->Blocks[i].bounds, 6);
      }
      blockInfo->Set(vtkCompositeDataPipeline::BLOCK_AMOUNT_OF_DETAIL(),
        this->MetaData->Blocks[i].NumberOfElements);
      blockInfo->Set(vtkCompositeDataSet::CURRENT_PROCESS_CAN_LOAD_BLOCK(),
        this->MetaData->Blocks[i].ProcessId == myProcessId ? 1 : 0);
    }
  }
  outline->GetInformation()->Set(
    vtkCompositeDataPipeline::BLOCK_AMOUNT_OF_DETAIL(), this->MetaData->TotalNumberOfElements);
  outputVector->GetInformationObject(0)->Set(
    vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA(), outline);
  return 1;
}

//------------------------------------------------------------------------------
int vtkPGenericIOMultiBlockReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
#ifdef DEBUG
  std::cout << "[INFO]: Call RequestData()\n";
  std::cout.flush();
#endif

  // STEP 0: get the output grid
  vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outputVector, 0);
  assert("pre: output dataset is nullptr!" && (output != nullptr));

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  output->SetNumberOfBlocks(this->MetaData->NumberOfBlocks);

  // Get the global dimensions and physical origin & scale from
  // the genericio file and add them to the dataset
  uint64_t tmpDims[3];
  double tmpDouble[3];
  vtkSmartPointer<vtkTypeUInt64Array> dims = vtkSmartPointer<vtkTypeUInt64Array>::New();
  vtkSmartPointer<vtkDoubleArray> origin = vtkSmartPointer<vtkDoubleArray>::New();
  vtkSmartPointer<vtkDoubleArray> scale = vtkSmartPointer<vtkDoubleArray>::New();
  dims->SetNumberOfComponents(3);
  dims->SetName("genericio_global_dimensions");
  origin->SetNumberOfComponents(3);
  origin->SetName("genericio_phys_origin");
  scale->SetNumberOfComponents(3);
  scale->SetName("genericio_phys_scale");

  this->Reader->GetGlobalDimensions(tmpDims);
  dims->InsertNextTypedTuple((vtkTypeUInt64*)tmpDims);
  this->Reader->GetPhysOrigin(tmpDouble);
  origin->InsertNextTypedTuple(tmpDouble);
  this->Reader->GetPhysScale(tmpDouble);
  scale->InsertNextTypedTuple(tmpDouble);

  output->GetFieldData()->AddArray(origin);
  output->GetFieldData()->AddArray(scale);
  output->GetFieldData()->AddArray(dims);

  int myProcessId = this->Controller->GetLocalProcessId();

  if (outInfo->Has(vtkCompositeDataPipeline::LOAD_REQUESTED_BLOCKS()))
  {
    int size = outInfo->Length(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES());
    int* ids = outInfo->Get(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES());
    for (int i = 0; i < size; ++i)
    {
      int blockId = ids[i];
      if (this->MetaData->BlockIsOnMyProcess(blockId, myProcessId))
      {
        vtkSmartPointer<vtkUnstructuredGrid> grid =
          vtkSmartPointer<vtkUnstructuredGrid>::Take(this->LoadBlock(blockId));
        output->SetBlock(blockId, grid);
      }
    }
  }
  else
  {
    std::map<int, block_t>::iterator blockItr = this->MetaData->Blocks.begin();
    for (; blockItr != this->MetaData->Blocks.end(); ++blockItr)
    {
      if (blockItr->second.ProcessId == myProcessId)
      {
        vtkSmartPointer<vtkUnstructuredGrid> grid =
          vtkSmartPointer<vtkUnstructuredGrid>::Take(this->LoadBlock(blockItr->first));
        output->SetBlock(blockItr->first, grid);
      }
    }
  }

  return 1;
}
