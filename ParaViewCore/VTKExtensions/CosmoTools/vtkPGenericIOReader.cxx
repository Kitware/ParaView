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
#include "vtkPGenericIOReader.h"

// VTK includes
#include "vtkCallbackCommand.h"
#include "vtkCellArray.h"
#include "vtkCellType.h"
#include "vtkCommand.h"
#include "vtkCommunicator.h"
#include "vtkDataArray.h"
#include "vtkDataArraySelection.h"
#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMPI.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStdString.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkType.h"
#include "vtkUnstructuredGrid.h"

// GenericIO includes
#include "GenericIOReader.h"
#include "GenericIOMPIReader.h"
#include "GenericIOPosixReader.h"
#include "GenericIOUtilities.h"

// C/C++ includes
#include <algorithm>
#include <cassert>
#include <cctype>
#include <set>
#include <stdexcept>
#include <vector>

// MPI
#include <mpi.h>

// Uncomment the line below to get debugging information
//#define DEBUG

//------------------------------------------------------------------------------
// Helper STL string manipulation methods

namespace vtkPGenericIOInternals {
// trim from start
std::string &ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
  return s;
}

// trim from end
std::string &rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
  return s;
}

// trim from both ends
std::string &trim(std::string &s) {
  return ltrim(rtrim(s));
}

} /* end vktPGenericIOInternals namespace */

//------------------------------------------------------------------------------
class vtkGenericIOMetaData
{
public:

  int NumberOfElements;
  std::map< std::string, gio::VariableInfo > Information;
  std::map< std::string, int >  VariableGenericIOType;
  std::map< std::string, bool > VariableStatus;
  std::map< std::string, void* > RawCache;
  MPI_Comm MPICommunicator;
  std::set< int > RanksToLoad;

  /**
   * @brief Metadata constructor.
   */
  vtkGenericIOMetaData(){};

  /**
   * @brief Destructor
   */
  ~vtkGenericIOMetaData() { this->Clear();};

  /**
   * @brief Checks if the supplied rank should load data.
   * @param r the rank in query.
   * @return status true or false.
   */
  bool LoadRank( const int r )
  {
    return( (this->RanksToLoad.find(r) != this->RanksToLoad.end()) );
  }

  /**
   * @brief Get the raw MPI communicator from a Multi-process controller.
   * @param controller the multi-process controller
   */
  void InitCommunicator(vtkMultiProcessController *controller)
  {
     assert("pre: controller is NULL!" && (controller != NULL) );

     // STEP 0: Get the communicator from the controller
     vtkCommunicator *vtkComm =  controller->GetCommunicator();
     assert("pre: VTK communicator is NULL" && (vtkComm != NULL) );

     // STEP 1: Safe downcast to an vtkMPICommunicator
     vtkMPICommunicator* vtkMPIComm = vtkMPICommunicator::SafeDownCast(vtkComm);
     assert("pre: MPI communicator is NULL" && (vtkMPIComm != NULL) );

     // STEP 2: Get the opaque VTK MPI communicator
     vtkMPICommunicatorOpaqueComm* mpiComm = vtkMPIComm->GetMPIComm();
     assert("pre: Opaque MPI communicator is NULL" && (mpiComm != NULL) );

     // STEP 3: Finally, get the MPI comm
     this->MPICommunicator = *(mpiComm->GetHandle());
  }

  /**
   * @brief Performs a quick sanity on the metadata
   * @return status false iff the metadata is somehow corrupted.
   */
  bool SanityCheck()
  {
  int N = this->Information.size();
  return( ( (this->VariableGenericIOType.size()==N) &&
             (this->VariableStatus.size()==N) &&
             (this->RawCache.size()==N)
      ) );
  }

  /**
   * @brief Checks if a variable exists
   * @param varName the name of the variable in query
   * @return status true or false
   */
  bool HasVariable(const std::string &varName)
  {
  bool status = (
   (this->Information.find(varName)!=this->Information.end()) &&
   (this->VariableGenericIOType.find(varName)!=
       this->VariableGenericIOType.end()) &&
   (this->VariableStatus.find(varName)!=this->VariableStatus.end()) &&
   (this->RawCache.find(varName)!=this->RawCache.end())
   );
  return( status );
  }

  /**
   * @brief Clears the metadata
   */
  void Clear()
  {
  this->NumberOfElements = 0;
  this->VariableGenericIOType.clear();
  this->VariableStatus.clear();
  this->Information.clear();
  this->RanksToLoad.clear();

  std::map<std::string,void*>::iterator iter;
  for( iter=this->RawCache.begin(); iter != this->RawCache.end(); ++iter)
    {
    delete [] static_cast<char*>( iter->second );
    } // END for
  this->RawCache.clear();
  }

};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkPGenericIOReader);

//------------------------------------------------------------------------------
vtkPGenericIOReader::vtkPGenericIOReader()
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

  this->MetaData  = new vtkGenericIOMetaData();
  this->MetaData->InitCommunicator( this->Controller );

  this->RequestInfoCounter = 0;
  this->RequestDataCounter = 0;

  this->RankInQuery        = 0;
  this->QueryRankNeighbors = 0;

  this->ArrayList = vtkStringArray::New();
  this->PointDataArraySelection = vtkDataArraySelection::New();
  this->SelectionObserver  = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(
      &vtkPGenericIOReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData( this );
  this->PointDataArraySelection->AddObserver(
      vtkCommand::ModifiedEvent,this->SelectionObserver);
}

//------------------------------------------------------------------------------
vtkPGenericIOReader::~vtkPGenericIOReader()
{
 if( this->Reader != NULL )
   {
   this->Reader->Close();
   delete this->Reader;
   }

 if( this->FileName != NULL )
   {
   delete [] this->FileName;
   }

 if( this->XAxisVariableName != NULL )
   {
   delete [] this->XAxisVariableName;
   }

 if( this->YAxisVariableName != NULL )
   {
   delete [] this->YAxisVariableName;
   }

 if( this->ZAxisVariableName != NULL )
   {
   delete [] this->ZAxisVariableName;
   }

 if( this->MetaData != NULL )
   {
   delete this->MetaData;
   }

 this->ArrayList->Delete();

 this->PointDataArraySelection->RemoveObserver( this->SelectionObserver );
 this->SelectionObserver->Delete();
 this->PointDataArraySelection->Delete();

 this->Controller = NULL;
}

//------------------------------------------------------------------------------
void vtkPGenericIOReader::PrintSelf(std::ostream &os, vtkIndent indent)
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
int vtkPGenericIOReader::GetNumberOfPointArrays()
{
  return( this->PointDataArraySelection->GetNumberOfArrays() );
}

//------------------------------------------------------------------------------
const char* vtkPGenericIOReader::GetPointArrayName(int i)
{
  assert("pre: array index is out-of-bounds!" &&
            (i >= 0) && (i < this->GetNumberOfPointArrays()) );
  return( this->PointDataArraySelection->GetArrayName( i ) );
}

//------------------------------------------------------------------------------
int vtkPGenericIOReader::GetPointArrayStatus(const char *name)
{
  return( this->PointDataArraySelection->ArrayIsEnabled(name)  );
}

//------------------------------------------------------------------------------
void vtkPGenericIOReader::SetPointArrayStatus(
          const char *name, int status)
{
  if( status )
    {
#ifdef DEBUG
    std::cout << "\t[INFO]: Enabling array " << name << std::endl;
    std::cout.flush();
#endif
    this->PointDataArraySelection->EnableArray( name );
    assert(this->PointDataArraySelection->ArrayIsEnabled(name));
    }
  else
    {
#ifdef DEBUG
    std::cout << "\t[INFO]: Disabling array " << name << std::endl;
    std::cout.flush();
#endif
    this->PointDataArraySelection->DisableArray( name );
    assert(!this->PointDataArraySelection->ArrayIsEnabled(name));
    }
}

//------------------------------------------------------------------------------
bool vtkPGenericIOReader::ReaderParametersChanged()
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
      if(status == true)
        {
        std::cout << "\t[INFO]: I/O strategy changed from POSIX\n";
        std::cout.flush();
        }
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
gio::GenericIOReader* vtkPGenericIOReader::GetInternalReader()
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
  this->PointDataArraySelection->DisableAllArrays();

  assert("pre: Reader should be NULL" && (this->Reader==NULL));
  gio::GenericIOReader *r = NULL;
  switch( this->GenericIOType )
    {
    case IOTYPEMPI:
#ifdef DEBUG
      std::cout << "\t[INFO]: Creating MPI Reader instance...";
#endif
      r = new gio::GenericIOMPIReader();
#ifdef DEBUG
      std::cout << "[DONE]\n";
      std::cout.flush();
#endif
      break;
    case IOTYPEPOSIX:
#ifdef DEBUG
      std::cout << "\t[INFO]: Creating Posix Reader instance...";
#endif
      r = new gio::GenericIOPosixReader();
#ifdef DEBUG
      std::cout << "[DONE]\n";
      std::cout.flush();
#endif
      break;
    default:
      r = NULL;
    } // END switch

  r->SetCommunicator( this->MetaData->MPICommunicator );
  r->SetFileName(this->FileName);

  if( this->BlockAssignment == ROUND_ROBIN)
    {
#ifdef DEBUG
      std::cout << "\t[INFO]: Using Round-Robin block assignment...";
#endif
    r->SetBlockAssignmentStrategy(gio::RR_BLOCK_ASSIGNMENT);
#ifdef DEBUG
      std::cout << "[DONE]\n";
      std::cout.flush();
#endif
    }
  else if( this->BlockAssignment == RCB )
    {
#ifdef DEBUG
      std::cout << "\t[INFO]: Using RCB block assignment...";
#endif
    r->SetBlockAssignmentStrategy(gio::RCB_BLOCK_ASSIGNMENT);
#ifdef DEBUG
      std::cout << "[DONE]\n";
      std::cout.flush();
#endif
    }
  return( r );
}

//------------------------------------------------------------------------------
void vtkPGenericIOReader::LoadMetaData()
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

#ifdef DEBUG
    std::cout << "\t[INFO]: Reading header to build metadata!\n";
    std::cout << "\t[INFO]: Filename: " << this->FileName << std::endl;
    std::cout.flush();
#endif
  this->Reader->OpenAndReadHeader();
  this->MetaData->NumberOfElements = this->Reader->GetNumberOfElements();

  this->ArrayList->SetNumberOfValues(0);

  for(int i=0; i < this->Reader->GetNumberOfVariablesInFile(); ++i)
    {
    std::string vname = this->Reader->GetVariableName( i );

    this->ArrayList->InsertNextValue( vname.c_str() );

    this->MetaData->Information[vname] =
        this->Reader->GetFileVariableInfo(i);

    this->MetaData->VariableGenericIOType[vname]=
        gio::GenericIOUtilities::DetectVariablePrimitiveType(
              this->MetaData->Information[vname] );

    this->MetaData->VariableStatus[vname]= false;
    this->MetaData->RawCache[vname]      = NULL;

    this->PointDataArraySelection->AddArray(vname.c_str());
    this->PointDataArraySelection->DisableArray( vname.c_str() );
    } // END for all variables in the file

  this->BuildMetaData = false; /* signal that the metadata is build */

  assert("pre: metadata is corrupt!" && (this->MetaData->SanityCheck()));
}

//------------------------------------------------------------------------------
void vtkPGenericIOReader::LoadRawVariableData(std::string varName)
{
#ifdef DEBUG
  std::cout << "[INFO]: Loading variable: " << varName << std::endl;
  std::cout.flush();
#endif

  // Sanity check
  assert("pre: no variable in metadata with given name" &&
          this->MetaData->HasVariable(varName));

  if(this->MetaData->VariableStatus[varName])
    {
    // variable has already been loaded
#ifdef DEBUG
    std::cout << "\t[INFO]: Variable appears to be already loaded!\n";
    std::cout.flush();
#endif
    return;
    }

  this->MetaData->RawCache[varName]=
  gio::GenericIOUtilities::AllocateVariableArray(
    this->MetaData->Information[varName],this->MetaData->NumberOfElements);

  this->Reader->AddVariable(
    this->MetaData->Information[varName],this->MetaData->RawCache[varName]);

  this->MetaData->VariableStatus[varName] = true;

#ifdef DEBUG
  std::cout << "\t[INFO]: Variable [" << varName << "] is now loaded!\n";
  std::cout.flush();
#endif
}

//------------------------------------------------------------------------------
void vtkPGenericIOReader::LoadRawData()
{
  assert("pre: metadata is corrupt!" && (this->MetaData->SanityCheck()));

  std::string xaxis = std::string(this->XAxisVariableName);
  xaxis = vtkPGenericIOInternals::trim(xaxis);

  std::string yaxis = std::string(this->YAxisVariableName);
  yaxis = vtkPGenericIOInternals::trim(yaxis);

  std::string zaxis = std::string(this->ZAxisVariableName);
  zaxis = vtkPGenericIOInternals::trim(zaxis);

  this->LoadRawVariableData( xaxis );
  this->LoadRawVariableData( yaxis );
  this->LoadRawVariableData( zaxis );

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
      this->LoadRawVariableData( varName );
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

  this->Reader->ReadData();

#ifdef DEBUG
  std::cout << "[DONE]\n";
  std::cout.flush();
#endif
}

//------------------------------------------------------------------------------
void vtkPGenericIOReader::GetPointFromRawData(
        std::string xName, std::string yName, std::string zName,
        vtkIdType idx, double pnt[3])
{
  std::string name[3];
  name[0] = xName;
  name[1] = yName;
  name[2] = zName;

  int type = -1;
  for( int i=0; i < 3; ++i )
    {
    type = this->MetaData->VariableGenericIOType[ name[i] ];
    void *rawBuffer = this->MetaData->RawCache[ name[i] ];
    assert("pre: raw buffer is NULL!" && (rawBuffer != NULL) );

    switch( type )
      {
      case gio::GENERIC_IO_INT32_TYPE:
        {
        int32_t *dataPtr = static_cast<int32_t*>(rawBuffer);
        pnt[i] = static_cast<double>(dataPtr[idx]);
        }
        break;
      case gio::GENERIC_IO_INT64_TYPE:
        {
        int64_t *dataPtr = static_cast<int64_t*>(rawBuffer);
        pnt[i] = static_cast<double>(dataPtr[idx]);
        }
        break;
      case gio::GENERIC_IO_UINT32_TYPE:
        {
        uint32_t *dataPtr = static_cast<uint32_t*>(rawBuffer);
        pnt[i] = static_cast<double>(dataPtr[idx]);
        }
        break;
      case gio::GENERIC_IO_UINT64_TYPE:
        {
        uint64_t *dataPtr = static_cast<uint64_t*>(rawBuffer);
        pnt[i] = static_cast<double>(dataPtr[idx]);
        }
        break;
      case gio::GENERIC_IO_DOUBLE_TYPE:
        {
        double *dataPtr = static_cast<double*>(rawBuffer);
        pnt[i] = dataPtr[idx];
        }
        break;
      case gio::GENERIC_IO_FLOAT_TYPE:
        {
        float *dataPtr = static_cast<float*>(rawBuffer);
        pnt[i] = static_cast<double>(dataPtr[idx]);
        }
        break;
      default:
        assert("pre: Undefined GENERIC IO type: " && true);
      } // END switch

    } // END for all dimensions
}

//------------------------------------------------------------------------------
void vtkPGenericIOReader::LoadCoordinates(vtkUnstructuredGrid *grid)
{
  assert("pre: grid is NULL!" && (grid != NULL) );

  if( this->QueryRankNeighbors && (this->BlockAssignment==RCB) &&
      !this->MetaData->LoadRank(this->Controller->GetLocalProcessId()))
    {
    return;
    }

  std::string xaxis = std::string(this->XAxisVariableName);
  xaxis = vtkPGenericIOInternals::trim(xaxis);

  std::string yaxis = std::string(this->YAxisVariableName);
  yaxis = vtkPGenericIOInternals::trim(yaxis);

  std::string zaxis = std::string(this->ZAxisVariableName);
  zaxis = vtkPGenericIOInternals::trim(zaxis);

  if( !this->MetaData->HasVariable(xaxis) ||
       !this->MetaData->HasVariable(yaxis) ||
       !this->MetaData->HasVariable(zaxis))
    {
    vtkErrorMacro(<< "Don't have one or more coordinate arrays!\n");
    return;
    }

  vtkCellArray *cells = vtkCellArray::New();
  cells->Allocate(cells->EstimateSize(this->MetaData->NumberOfElements,1));

  vtkPoints *pnts = vtkPoints::New();
  pnts->SetDataTypeToDouble();
  pnts->SetNumberOfPoints( this->MetaData->NumberOfElements );


  int nparticles = this->MetaData->NumberOfElements;
  double pnt[3];
  vtkIdType idx = 0;
  for( ;idx < nparticles; ++idx)
    {
    this->GetPointFromRawData(xaxis,yaxis,zaxis, idx, pnt);
    pnts->SetPoint(idx,pnt);
    cells->InsertNextCell(1,&idx);
    } // END for all points

  grid->SetPoints(pnts);
  pnts->Delete();

  grid->SetCells(VTK_VERTEX,cells);
  cells->Delete();

  grid->Squeeze();
}

//------------------------------------------------------------------------------
vtkDataArray* vtkPGenericIOReader::GetVtkDataArray(std::string name)
{
  if( !this->MetaData->HasVariable(name) )
    {
    vtkErrorMacro(<< "Invalid variable name: " << name );
    return NULL;
    }

  vtkDataArray *dataArray = NULL;
  size_t dataSize = 0;
  void *rawBuffer = this->MetaData->RawCache[ name ];
  assert("pre: rawBuffer should not be NULL!" && (rawBuffer != NULL) );
  int type = this->MetaData->VariableGenericIOType[ name ];
  switch( type )
    {
    case gio::GENERIC_IO_INT32_TYPE:
        #ifdef DEBUG
          std::cout << "\t[INFO]: Get VTK_INT array for " << name;
          std::cout << std::endl;
          std::cout.flush();
        #endif
       dataArray = vtkDataArray::CreateDataArray(VTK_INT);
       dataSize = sizeof(int32_t);
       break;
     case gio::GENERIC_IO_INT64_TYPE:
       #ifdef DEBUG
        std::cout << "\t[INFO]: Get VTK_LONG_LONG array for " << name;
        std::cout << std::endl;
        std::cout.flush();
       #endif
       dataArray = vtkDataArray::CreateDataArray(VTK_LONG_LONG);
       // i.e., don't run this on windows
       assert(sizeof(long long)==sizeof(int64_t));
       dataSize = sizeof(int64_t);
       break;
     case gio::GENERIC_IO_UINT32_TYPE:
       #ifdef DEBUG
        std::cout << "\t[INFO]: Get VTK_UNSIGNED_INT array for " << name;
        std::cout << std::endl;
        std::cout.flush();
       #endif
       dataArray = vtkDataArray::CreateDataArray(VTK_UNSIGNED_INT);
       dataSize = sizeof(uint32_t);
       break;
     case gio::GENERIC_IO_UINT64_TYPE:
       #ifdef DEBUG
        std::cout << "\t[INFO]: Get VTK_UNSIGNED_LONG_LONG array for " << name;
        std::cout << std::endl;
        std::cout.flush();
       #endif
       dataArray = vtkDataArray::CreateDataArray(VTK_UNSIGNED_LONG_LONG);
       // i.e., don't run this on windows
       assert(sizeof(unsigned long long) == sizeof(uint64_t) );
       dataSize = sizeof(uint64_t);
       break;
     case gio::GENERIC_IO_DOUBLE_TYPE:
       #ifdef DEBUG
        std::cout << "\t[INFO]: Get VTK_DOUBLE array for " << name;
        std::cout << std::endl;
        std::cout.flush();
       #endif
       dataArray = vtkDataArray::CreateDataArray(VTK_DOUBLE);
       dataSize = sizeof(double);
       break;
     case gio::GENERIC_IO_FLOAT_TYPE:
       #ifdef DEBUG
        std::cout << "\t[INFO]: Get VTK_FLOAT array for " << name;
        std::cout << std::endl;
        std::cout.flush();
       #endif
       dataArray = vtkDataArray::CreateDataArray(VTK_FLOAT);
       dataSize = sizeof(float);
       break;
     default:
       vtkErrorMacro(<<"Undefined GENERIC_IO_TYPE");
       dataSize = 0;
       return NULL;
    } // END switch

  dataArray->SetNumberOfComponents(1);
  dataArray->SetNumberOfTuples( this->MetaData->NumberOfElements );
  dataArray->SetName(name.c_str());
  void *dataBuffer = dataArray->GetVoidPointer(0);
  assert("pre: Encountered NULL data buffer!" && (dataBuffer != NULL) );
  memcpy(dataBuffer,rawBuffer,
       this->MetaData->NumberOfElements*sizeof(int32_t));

  return( dataArray );
}
//------------------------------------------------------------------------------
void vtkPGenericIOReader::LoadData(vtkUnstructuredGrid *grid)
{
  assert("pre: grid is NULL!" && (grid != NULL) );

  if( this->QueryRankNeighbors && (this->BlockAssignment==RCB) &&
      !this->MetaData->LoadRank(this->Controller->GetLocalProcessId()))
    {
    return;
    }

  vtkPointData *PD = grid->GetPointData();
  int arrayIdx = 0;
  for(;arrayIdx < this->PointDataArraySelection->GetNumberOfArrays(); ++arrayIdx)
    {
    const char *name = this->PointDataArraySelection->GetArrayName( arrayIdx );
    if( this->PointDataArraySelection->ArrayIsEnabled(name) )
      {
      std::string varName = std::string( name );
      vtkDataArray *dataArray = this->GetVtkDataArray( varName );
      PD->AddArray( dataArray );
      dataArray->Delete();
      } // END if the array is enabled
    } // END for all arrays

}

//------------------------------------------------------------------------------
void vtkPGenericIOReader::FindRankNeighbors()
{
  if( !this->QueryRankNeighbors || this->BlockAssignment != RCB)
    {
    return;
    }

  this->MetaData->RanksToLoad.clear();

  // Sanity checks
  assert("pre: rank in query is out-of bounds!" &&
        (this->RankInQuery >= 0) &&
        (this->RankInQuery < this->Controller->GetNumberOfProcesses()) );
  assert("pre: reader should not be NULL!" && (this->Reader!=NULL) );
  assert("pre: block assignment is not RCB!" &&
    (this->Reader->GetBlockAssignmentStrategy()==gio::RCB_BLOCK_ASSIGNMENT) );

  std::vector<int> neiRanks;
  if( this->Controller->GetLocalProcessId() == this->RankInQuery )
    {
#ifdef DEBUG
    std::cout << "[INFO]: Loading neighbors for process P[";
    std::cout << this->Controller->GetLocalProcessId();
    std::cout << "]\n";
    std::cout.flush();
#endif

    int numNeis = this->Reader->GetNumberOfRankNeighbors();
    std::vector< gio::RankNeighbor > rankNeighbors;
    rankNeighbors.resize(numNeis);
    this->Reader->GetRankNeighbors( &rankNeighbors[0] );

    neiRanks.resize(numNeis);
    for(int nei=0; nei < numNeis; ++nei)
      {
#ifdef DEBUG
     std::cout << "\t Neighboring rank P[";
     std::cout << rankNeighbors[ nei ].RankID;
     std::cout << "] Orientation {";
     std::cout << gio::NEIGHBOR_ORIENTATION[rankNeighbors[nei].Orient[0]];
     std::cout << ", ";
     std::cout << gio::NEIGHBOR_ORIENTATION[rankNeighbors[nei].Orient[1]];
     std::cout << ", ";
     std::cout << gio::NEIGHBOR_ORIENTATION[rankNeighbors[nei].Orient[2]];
     std::cout << "}\n";
     std::cout.flush();
#endif
      neiRanks[ nei ] = rankNeighbors[ nei ].RankID;
      } // END for all neis
    rankNeighbors.clear();

    MPI_Bcast(&numNeis,1,MPI_INT,this->RankInQuery,this->MetaData->MPICommunicator);
    MPI_Bcast(&neiRanks[0],numNeis,MPI_INT,
              this->RankInQuery,this->MetaData->MPICommunicator);
    } // END if
  else
    {
    int numNeis = -1;
    MPI_Bcast(&numNeis,1,MPI_INT,this->RankInQuery,this->MetaData->MPICommunicator);
    neiRanks.resize(numNeis);
    MPI_Bcast(&neiRanks[0],numNeis,MPI_INT,
              this->RankInQuery,this->MetaData->MPICommunicator);
    } // END else

  this->MetaData->RanksToLoad.insert( this->RankInQuery );
  for(unsigned int i=0; i < neiRanks.size(); ++i )
    {
    this->MetaData->RanksToLoad.insert( neiRanks[i] );
    }
  neiRanks.clear();
}

//----------------------------------------------------------------------------
void vtkPGenericIOReader::SelectionModifiedCallback(
    vtkObject*, unsigned long, void* clientdata, void*)
{
  assert("pre: clientdata is NULL!" && (clientdata != NULL) );
  static_cast<vtkPGenericIOReader*>(clientdata)->Modified();
}

//------------------------------------------------------------------------------
int vtkPGenericIOReader::RequestInformation(
      vtkInformation *vtkNotUsed(request),
      vtkInformationVector **vtkNotUsed(inputVector),
      vtkInformationVector *outputVector)
{
#ifdef DEBUG
  std::cout << "[INFO]: Call RequestInformation()\n";
  std::cout.flush();
#endif

  ++this->RequestInfoCounter;

  // tell the pipeline that this dataset is distributed
  outputVector->GetInformationObject(0)->Set(
      vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
      this->Controller->GetNumberOfProcesses()
      );
  outputVector->GetInformationObject(0)->Set(
      vtkDataObject::DATA_NUMBER_OF_PIECES(),
      this->Controller->GetNumberOfProcesses()
      );

  this->Reader = this->GetInternalReader();
  assert("pre: internal reader is NULL!" && (this->Reader != NULL) );

  this->LoadMetaData();
  return 1;
}

//------------------------------------------------------------------------------
int vtkPGenericIOReader::RequestData(
    vtkInformation *vtkNotUsed(request),
    vtkInformationVector **vtkNotUsed(inputVector),
    vtkInformationVector *outputVector)
{
#ifdef DEBUG
  std::cout << "[INFO]: Call RequestData()\n";
  std::cout.flush();
#endif

  // STEP 0: get the output grid
  vtkInformation *outInfo     = outputVector->GetInformationObject(0);
  vtkUnstructuredGrid *output =
      vtkUnstructuredGrid::SafeDownCast(
          outInfo->Get(vtkDataObject::DATA_OBJECT()));
  assert("pre: output grid is NULL!" && (output != NULL) );

  // STEP 1: Load raw data
  this->LoadRawData();

  // STEP 2: See if we should only show
  this->FindRankNeighbors();
  MPI_Barrier(this->MetaData->MPICommunicator);

  // STEP 3: Load coordinates
  this->LoadCoordinates(output);
  MPI_Barrier(this->MetaData->MPICommunicator);

  // STEP 4: Load data
  this->LoadData(output);
  MPI_Barrier(this->MetaData->MPICommunicator);

  // STEP 5: Clear variables
  this->Reader->ClearVariables();
  return 1;
}
