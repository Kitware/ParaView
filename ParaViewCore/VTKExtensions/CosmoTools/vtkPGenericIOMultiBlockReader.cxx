/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkPGenericIOReader.cxx

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
#include "vtkGenericIOUtilities.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkType.h"
#include "vtkUnstructuredGrid.h"

#include "GenericIOReader.h"
#include "GenericIOUtilities.h"

#include <map>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

// Uncomment the line below to get debugging information
//#define DEBUG

struct vtkPGenericIOMultiBlockReader::block_t
{
  vtkTypeUInt64 GlobalId;
  vtkTypeUInt64 NumberOfElements;
  double bounds[6];

  std::map< std::string, void* > RawCache;
  std::map< std::string, bool > VariableStatus;
};

//------------------------------------------------------------------------------
class vtkPGenericIOMultiBlockReader::vtkGenericIOMultiBlockMetaData
{
public:
  int NumberOfBlocks; // Total number of blocks across all ranks
  std::map < std::string, gio::VariableInfo > VariableInformation;
  std::map < std::string, int > VariableGenericIOType;
  std::map < int, vtkPGenericIOMultiBlockReader::block_t > Blocks;


  /**
   * @brief Metadata constructor.
   */
  vtkGenericIOMultiBlockMetaData(){}

  /**
   * @brief Destructor
   */
  ~vtkGenericIOMultiBlockMetaData() { this->Clear();}

  /**
   * @brief Checks if the supplied rank should load data.
   * @param r the rank in query.
   * @return status true or false.
   */
  bool HasBlock( const int blockId )
  {
    return (this->Blocks.find(blockId) != this->Blocks.end());
  }

  /**
   * @brief Get the raw MPI communicator from a Multi-process controller.
   * @param controller the multi-process controller
   * /
  void InitCommunicator(vtkMultiProcessController *controller)
  {
    assert("pre: controller is NULL!" && (controller != NULL) );
    this->MPICommunicator =
        vtkGenericIOUtilities::GetMPICommunicator(controller);
  } */

  /**
   * @brief Performs a quick sanity on the metadata
   * @return status false if the metadata is somehow corrupted.
   */
  bool SanityCheck()
  {
    return  (this->VariableInformation.size() == this->VariableGenericIOType.size());
  }

  /**
   * @brief Checks if a variable exists
   * @param varName the name of the variable in query
   * @return status true or false
   */
  bool HasVariable(const std::string &varName)
  {
    return (this->VariableInformation.find(varName) !=
      this->VariableInformation.end());
  }

  /**
   * @brief Clears the metadata
   */
  void Clear()
  {
    this->NumberOfBlocks = 0;
    this->VariableGenericIOType.clear();
    this->VariableInformation.clear();

    for (std::map< int, block_t >::iterator blockItr = this->Blocks.begin();
          blockItr != this->Blocks.end(); ++blockItr)
      {
      std::map< std::string, void* >::iterator varItr = blockItr->second.RawCache.begin();
      for ( ; varItr != blockItr->second.RawCache.end(); ++varItr)
        {
        if (varItr->second != NULL )
          {
          delete [] static_cast< char* >(varItr->second);
          }
        }
      }
    this->Blocks.clear();

  }
};

vtkStandardNewMacro(vtkPGenericIOMultiBlockReader)
//------------------------------------------------------------------------------
vtkPGenericIOMultiBlockReader::vtkPGenericIOMultiBlockReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->Controller        = vtkMultiProcessController::GetGlobalController();
  this->Reader            = NULL;
  this->FileName          = NULL;
  this->XAxisVariableName = NULL;
  this->YAxisVariableName = NULL;
  this->ZAxisVariableName = NULL;
  this->GenericIOType     = IOTYPEMPI;
  this->BlockAssignment   = ROUND_ROBIN;
  this->BuildMetaData     = false;

  this->SetXAxisVariableName("x");
  this->SetYAxisVariableName("y");
  this->SetZAxisVariableName("z");

  this->MetaData = new vtkGenericIOMultiBlockMetaData();
  //this->MetaData->InitCommunicator(this->Controller);

  this->ArrayList = vtkStringArray::New();
  this->PointDataArraySelection = vtkDataArraySelection::New();
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(
        &vtkPGenericIOMultiBlockReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData( this );
  this->PointDataArraySelection->AddObserver(
        vtkCommand::ModifiedEvent,this->SelectionObserver);
}

//------------------------------------------------------------------------------
vtkPGenericIOMultiBlockReader::~vtkPGenericIOMultiBlockReader()
{
  if (this->Reader != NULL)
    {
    this->Reader->Close();
    delete this->Reader;
    }

  vtkGenericIOUtilities::SafeDeleteString(this->FileName);
  vtkGenericIOUtilities::SafeDeleteString(this->XAxisVariableName);
  vtkGenericIOUtilities::SafeDeleteString(this->YAxisVariableName);
  vtkGenericIOUtilities::SafeDeleteString(this->ZAxisVariableName);

  if (this->MetaData != NULL)
    {
    delete this->MetaData;
    }

  this->ArrayList->Delete();
  this->PointDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->SelectionObserver->Delete();
  this->PointDataArraySelection->Delete();

  this->Controller = NULL;
}

//------------------------------------------------------------------------------
void vtkPGenericIOMultiBlockReader::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "FileName: " << this->FileName << endl;
  os << indent << "x-axis: " << this->XAxisVariableName << endl;
  os << indent << "y-axis: " << this->YAxisVariableName << endl;
  os << indent << "z-axis: " << this->ZAxisVariableName << endl;
  os << indent << "GenericIOType: " << this->GenericIOType << endl;
  os << indent << "BlockAssignment: " << this->BlockAssignment << endl;
  os << indent << "ArrayList: " << endl;
  this->ArrayList->PrintSelf(os,indent.GetNextIndent());
  os << indent << "PointDataSelection: " << endl;
  this->PointDataArraySelection->PrintSelf(os,indent.GetNextIndent());
  if( Controller != NULL )
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
  assert("pre: array index is out-of-bounds!" &&
    (i >= 0) && (i < this->GetNumberOfPointArrays()));
  return this->PointDataArraySelection->GetArrayName(i);
}

//------------------------------------------------------------------------------
int vtkPGenericIOMultiBlockReader::GetPointArrayStatus(const char* name)
{
  assert(this->PointDataArraySelection->ArrayExists(name));
  return this->PointDataArraySelection->ArrayIsEnabled(name);
}

//------------------------------------------------------------------------------
void vtkPGenericIOMultiBlockReader::SetPointArrayStatus(
  const char* name, int status)
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
bool vtkPGenericIOMultiBlockReader::ReaderParametersChanged()
{
  assert("pre: internal reader is NULL!" && (this->Reader != NULL) );

  if(this->Reader->GetFileName() != std::string(this->FileName) )
    {
#ifdef DEBUG
    std::cout << "\t[INFO]: File name has changed!\n";
    std::cout.flush();
#endif
    return true;
    }

  bool status = false;
  switch(this->Reader->GetIOStrategy())
    {
    case gio::GenericIOBase::FileIOMPI:
      status = (this->GenericIOType!=IOTYPEMPI)? true : false;
#ifdef DEBUG
      if(status == true)
        {
        std::cout << "\t[INFO]: I/O strategy changed from MPI\n";
        std::cout.flush();
        }
#endif
      break;
    case gio::GenericIOBase::FileIOPOSIX:
      status = (this->GenericIOType!=IOTYPEPOSIX)? true : false;
#ifdef DEBUG
      if(status == true)
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

  if( status == true )
    {
    /* short-circuit here */
    return( status );
    }

  switch(this->Reader->GetBlockAssignmentStrategy())
    {
    case gio::RR_BLOCK_ASSIGNMENT:
      status = (this->BlockAssignment!=ROUND_ROBIN)? true : false;
#ifdef DEBUG
      if(status == true)
        {
        std::cout << "\t[INFO]: I/O block assignment changed to Round-Robin\n";
        std::cout.flush();
        }
#endif
      break;
    case gio::RCB_BLOCK_ASSIGNMENT:
      status = (this->BlockAssignment!=RCB)? true : false;
#ifdef DEBUG
      if(status == true)
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

  return( status );
}

//------------------------------------------------------------------------------
gio::GenericIOReader* vtkPGenericIOMultiBlockReader::GetInternalReader()
{
  if( this->Reader != NULL )
    {
    if( this->ReaderParametersChanged() )
      {
#ifdef DEBUG
      std::cout << "\t[INFO]: Deleting Reader instance...\n";
      std::cout.flush();
#endif
      this->Reader->Close();
      delete this->Reader;
      this->Reader=NULL;
      } // END if the reader parameters
    else
      {
      return( this->Reader );
      }
    } // END if the reader is not NULL

  this->BuildMetaData = true; // signal to re-build metadata

  assert("pre: Reader should be NULL" && (this->Reader==NULL));
  gio::GenericIOReader *r = NULL;
  bool posix              = (this->GenericIOType==IOTYPEMPI)? false : true;
  int distribution        = (this->BlockAssignment==RCB)?
                                gio::RCB_BLOCK_ASSIGNMENT :
                                gio::RR_BLOCK_ASSIGNMENT;

  r = vtkGenericIOUtilities::GetReader(
        vtkGenericIOUtilities::GetMPICommunicator(this->Controller),
        posix,distribution,std::string(this->FileName));
  assert("post: Internal GenericIO reader should not be NULL!" && (r!=NULL) );

  return( r );
}

//------------------------------------------------------------------------------
void vtkPGenericIOMultiBlockReader::LoadMetaData()
{
  if( !this->BuildMetaData )
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

  // load variable information
  for (int i = 0; i < this->Reader->GetNumberOfVariablesInFile(); ++i)
    {
    std::string vname = this->Reader->GetVariableName(i);
    this->ArrayList->InsertNextValue( vname.c_str() );
    this->PointDataArraySelection->AddArray( vname.c_str() );
    this->PointDataArraySelection->DisableArray( vname.c_str() );

    this->MetaData->VariableInformation[vname]=
      this->Reader->GetFileVariableInfo(i);
    this->MetaData->VariableGenericIOType[vname]=
      gio::GenericIOUtilities::DetectVariablePrimitiveType(
        this->MetaData->VariableInformation[vname]);
    } // end for all variables in file

  // load block information
  this->MetaData->NumberOfBlocks = this->Reader->GetTotalNumberOfBlocks();
  block_t myBlock;
  double min[3];
  double max[3];
  for (int i = 0; i < this->Reader->GetNumberOfBlockHeaders(); ++i)
    {
    gio::RankHeader block = this->Reader->GetBlockHeader(i);
    assert("pre: loading duplicate block in metadata!" &&
      !this->MetaData->HasBlock(block.GlobalRank));

    myBlock.GlobalId = block.GlobalRank;
    myBlock.NumberOfElements = block.NElems;

    for (int var = 0; var < this->Reader->GetNumberOfVariablesInFile(); ++var)
      {
      std::string vname = this->Reader->GetVariableName(var);
      myBlock.RawCache[vname] = NULL;
      myBlock.VariableStatus[vname] = false;
      }

    this->Reader->GetBlockBounds(i,min,max);
    myBlock.bounds[0] = min[0];
    myBlock.bounds[1] = max[0];
    myBlock.bounds[2] = min[1];
    myBlock.bounds[3] = max[1];
    myBlock.bounds[4] = min[2];
    myBlock.bounds[5] = max[2];

    this->MetaData->Blocks[ block.GlobalRank ] = myBlock;
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
  assert("pre: Reader is NULL" && (this->Reader != NULL));
  assert("pre: metadata is NULL" && (this->MetaData != NULL));
  assert("pre: metadata is corrupt" && (this->MetaData->SanityCheck()));
  assert("pre: block is owned by another process" &&
    this->MetaData->HasBlock(blockId));
  assert("pre: metadata does not have requested variable" &&
    this->MetaData->HasVariable(varName));

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

  dataBlock.RawCache[varName] =
    gio::GenericIOUtilities::AllocateVariableArray(
      this->MetaData->VariableInformation[varName],dataBlock.NumberOfElements);

  this->Reader->AddVariable(
    this->MetaData->VariableInformation[varName],dataBlock.RawCache[varName]);

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
  assert("pre: Reader is NULL" && (this->Reader != NULL));
  assert("pre: metadata is NULL" && (this->MetaData != NULL));
  assert("pre: metadata is corrupt!" && (this->MetaData->SanityCheck()));
  assert("pre: block is not owned by this process!" &&
    this->MetaData->HasBlock(blockId));

  // This method is called for every block, so we must clear any previously
  // registered variables
  this->Reader->ClearVariables();

  std::string xaxis = std::string(this->XAxisVariableName);
  xaxis = vtkGenericIOUtilities::trim(xaxis);

  std::string yaxis = std::string(this->YAxisVariableName);
  yaxis = vtkGenericIOUtilities::trim(yaxis);

  std::string zaxis = std::string(this->ZAxisVariableName);
  zaxis = vtkGenericIOUtilities::trim(zaxis);

  this->LoadRawVariableDataForBlock( xaxis, blockId );
  this->LoadRawVariableDataForBlock( yaxis, blockId );
  this->LoadRawVariableDataForBlock( zaxis, blockId );

#ifdef DEBUG
  std::cout << "\t==========\n";
  std::cout << "\tNUMBER OF ARRAYS: "
      << this->PointDataArraySelection->GetNumberOfArrays() << std::endl;
  std::cout.flush();
#endif

  int arrayIdx = 0;
  for(;arrayIdx < this->PointDataArraySelection->GetNumberOfArrays(); ++arrayIdx)
    {
    const char *name = this->PointDataArraySelection->GetArrayName(arrayIdx);
#ifdef DEBUG
    std::cout << "\tARRAY " << name << " is ";
#endif
    if( this->PointDataArraySelection->ArrayIsEnabled(name) )
      {
#ifdef DEBUG
      std::cout << "ENABLED\n";
      std::cout.flush();
#endif
      std::string varName = std::string( name );
      this->LoadRawVariableDataForBlock( varName, blockId );
      } // END if the array is enabled
    else
      {
#ifdef DEBUG
      std::cout << "DISBLED\n";
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
void vtkPGenericIOMultiBlockReader::GetPointFromRawData(
        int xType, void* xBuffer, int yType, void* yBuffer, int zType, void* zBuffer,
        vtkIdType idx, double pnt[3])
{
  void* buffer[3] = {xBuffer, yBuffer, zBuffer};

  int type[3] = {xType, yType, zType};

  for( int i=0; i < 3; ++i )
    {
//    type = this->MetaData->VariableGenericIOType[ name[i] ];
//    void *rawBuffer = this->MetaData->Blocks[ blockId ].RawCache[ name[i] ];
    assert("pre: raw buffer is NULL!" && (buffer[i] != NULL) );

    pnt[i] = vtkGenericIOUtilities::GetDoubleFromRawBuffer(type[i],buffer[i],idx);
    } // END for all dimensions
}

//------------------------------------------------------------------------------
void vtkPGenericIOMultiBlockReader::LoadCoordinatesForBlock(vtkUnstructuredGrid *grid,
                                                    int blockId)
{
  assert("pre: metadata is NULL!" && (this->MetaData != NULL));
  assert("pre: grid is NULL!" && (grid != NULL) );
  assert("pre: block is not owned by this process!" &&
    this->MetaData->HasBlock(blockId));

  std::string xaxis = std::string(this->XAxisVariableName);
  xaxis = vtkGenericIOUtilities::trim(xaxis);

  std::string yaxis = std::string(this->YAxisVariableName);
  yaxis = vtkGenericIOUtilities::trim(yaxis);

  std::string zaxis = std::string(this->ZAxisVariableName);
  zaxis = vtkGenericIOUtilities::trim(zaxis);

  if( !this->MetaData->HasVariable(xaxis) ||
       !this->MetaData->HasVariable(yaxis) ||
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

  vtkSmartPointer< vtkCellArray > cells =
      vtkSmartPointer< vtkCellArray >::New();
  cells->Allocate(cells->EstimateSize(nparticles,1));

  vtkSmartPointer< vtkPoints > pnts =
      vtkSmartPointer< vtkPoints >::New();
  pnts->SetDataTypeToDouble();
  pnts->SetNumberOfPoints( nparticles );


  double pnt[3];
  vtkIdType idx = 0;
  for( ;idx < nparticles; ++idx)
    {
    this->GetPointFromRawData(xType, xBuffer, yType, yBuffer, zType, zBuffer,
                              idx, pnt);
    pnts->SetPoint(idx,pnt);
    cells->InsertNextCell(1,&idx);
    } // END for all points

  grid->SetPoints(pnts);

  grid->SetCells(VTK_VERTEX,cells);

  grid->Squeeze();
}

//------------------------------------------------------------------------------
void vtkPGenericIOMultiBlockReader::LoadDataArraysForBlock(vtkUnstructuredGrid *grid,
                                             int blockId)
{
  assert("pre: metadata is NULL!" && (this->MetaData != NULL));
  assert("pre: grid is NULL!" && (grid != NULL) );
  assert("pre: block is not owned by this process!" &&
    this->MetaData->HasBlock(blockId));

  block_t& dataBlock = this->MetaData->Blocks[blockId];

  vtkTypeUInt64 N = grid->GetNumberOfPoints();
  assert("pre: # points in dataset different from points in block" &&
    (N == dataBlock.NumberOfElements));
  vtkPointData* PD = grid->GetPointData();

  int arrayIdx = 0;
  for(;arrayIdx < this->PointDataArraySelection->GetNumberOfArrays(); ++arrayIdx)
    {
    const char *name = this->PointDataArraySelection->GetArrayName( arrayIdx );
    if( this->PointDataArraySelection->ArrayIsEnabled(name) )
      {
      std::string varName( name );
      vtkDataArray *dataArray =
          vtkGenericIOUtilities::GetVtkDataArray(
              varName,
              this->MetaData->VariableGenericIOType[ varName ],
              dataBlock.RawCache[ varName ],
              dataBlock.NumberOfElements
              );

      PD->AddArray( dataArray );
      dataArray->Delete();
      } // END if the array is enabled
    } // END for all arrays

}

//------------------------------------------------------------------------------
vtkUnstructuredGrid* vtkPGenericIOMultiBlockReader::LoadBlock(int blockId)
{
  // Sanity Check
  assert("pre: metadata is null" && (this->MetaData != NULL));
  assert("pre: block is not owned by this process!" &&
    this->MetaData->HasBlock(blockId));
  // STEP 1: Load raw data
  this->LoadRawDataForBlock(blockId);

  vtkUnstructuredGrid* grid = vtkUnstructuredGrid::New();

  // STEP 2: Load coordinates
  this->LoadCoordinatesForBlock(grid,blockId);

  // STEP 3: Load data
  this->LoadDataArraysForBlock(grid,blockId);

  return grid;
}

//------------------------------------------------------------------------------
void vtkPGenericIOMultiBlockReader::SelectionModifiedCallback(
    vtkObject *caller, unsigned long eid, void *clientdata, void *calldata)
{
  assert("pre: clientdata is NULL!" && (clientdata != NULL) );
  static_cast<vtkPGenericIOMultiBlockReader*>(clientdata)->Modified();
}

//------------------------------------------------------------------------------
int vtkPGenericIOMultiBlockReader::RequestInformation(
      vtkInformation *vtkNotUsed(request),
      vtkInformationVector **vtkNotUsed(inputVector),
      vtkInformationVector *outputVector)
{
#ifdef DEBUG
  std::cout << "[INFO]: Call RequestInformation()\n";
  std::cout.flush();
#endif

//  ++this->RequestInfoCounter;

  // tell the pipeline that this dataset is distributed
  outputVector->GetInformationObject(0)->Set(
      CAN_HANDLE_PIECE_REQUEST(),
      1
      );
  outputVector->GetInformationObject(0)->Set(
      vtkDataObject::DATA_NUMBER_OF_PIECES(),
      this->Controller->GetNumberOfProcesses()
      );

  this->Reader = this->GetInternalReader();
  assert("pre: internal reader is NULL!" && (this->Reader != NULL) );

  this->LoadMetaData();

  vtkSmartPointer< vtkMultiBlockDataSet > outline =
    vtkSmartPointer< vtkMultiBlockDataSet >::New();
  outline->SetNumberOfBlocks(this->MetaData->NumberOfBlocks);
  for (int i = 0; i < this->MetaData->NumberOfBlocks; ++i)
    {
    outline->SetBlock(i,NULL);
    if (this->MetaData->HasBlock(i))
      {
      vtkInformation* blockInfo = outline->GetMetaData(i);
      assert("pre: block info is NULL!" && (blockInfo != NULL));
      blockInfo->Set(
        vtkStreamingDemandDrivenPipeline::BOUNDS(),
        this->MetaData->Blocks[i].bounds,6
      );
      }
    }
  outputVector->GetInformationObject(0)->Set(
    vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA(), outline);
  return 1;
}

//------------------------------------------------------------------------------
int vtkPGenericIOMultiBlockReader::RequestData(
    vtkInformation *vtkNotUsed(request),
    vtkInformationVector **vtkNotUsed(inputVector),
    vtkInformationVector *outputVector)
{
#ifdef DEBUG
  std::cout << "[INFO]: Call RequestData()\n";
  std::cout.flush();
#endif

  // STEP 0: get the output grid
  vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outputVector,0);
  assert("pre: output dataset is NULL!" && (output != NULL) );

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  output->SetNumberOfBlocks(this->MetaData->NumberOfBlocks);

  if (outInfo->Has(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES()))
    {
    int size = outInfo->Length(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES());
    int* ids = outInfo->Get(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES());
    for (int i = 0; i < size; ++i)
      {
      int blockId = ids[i];
      if (this->MetaData->HasBlock(blockId))
        {
        vtkSmartPointer< vtkUnstructuredGrid > grid =
          vtkSmartPointer< vtkUnstructuredGrid >::Take(
            this->LoadBlock(blockId));
        output->SetBlock(blockId,grid);
        }
      }
    }
  else
    {
    std::map< int, block_t >::iterator blockItr = this->MetaData->Blocks.begin();
    for ( ; blockItr != this->MetaData->Blocks.end(); ++blockItr)
      {
      vtkSmartPointer< vtkUnstructuredGrid > grid =
          vtkSmartPointer< vtkUnstructuredGrid >::Take(
            this->LoadBlock(blockItr->first));

      output->SetBlock(blockItr->first,grid);
      }
    }
  //MPI_Barrier(this->MetaData->MPICommunicator);

  return 1;
}
