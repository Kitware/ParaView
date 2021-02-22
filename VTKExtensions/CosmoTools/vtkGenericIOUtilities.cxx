#include "vtkGenericIOUtilities.h"

// VTK includes
#include "vtkDataArray.h"
#include "vtkMPI.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkMultiProcessController.h"

// GenericIO includes
#include "GenericIOMPIReader.h"
#include "GenericIOMPIWriter.h"
#include "GenericIOPosixReader.h"

// C/C++ includes
#include <cassert>

// MPI
#include <vtk_mpi.h>

namespace vtkGenericIOUtilities
{

//==============================================================================
MPI_Comm GetMPICommunicator(vtkMultiProcessController* mpc)
{
  assert("pre: nullptr multiprocess controller!" && (mpc != nullptr));
  MPI_Comm comm = MPI_COMM_NULL;

  // STEP 0: Get the communicator from the controller
  vtkCommunicator* vtkComm = mpc->GetCommunicator();
  assert("pre: VTK communicator is nullptr" && (vtkComm != nullptr));

  // STEP 1: Safe downcast to an vtkMPICommunicator
  vtkMPICommunicator* vtkMPIComm = vtkMPICommunicator::SafeDownCast(vtkComm);
  assert("pre: MPI communicator is nullptr" && (vtkMPIComm != nullptr));

  // STEP 2: Get the opaque VTK MPI communicator
  vtkMPICommunicatorOpaqueComm* mpiComm = vtkMPIComm->GetMPIComm();
  assert("pre: Opaque MPI communicator is nullptr" && (mpiComm != nullptr));

  // STEP 3: Finally, get the MPI comm
  comm = *(mpiComm->GetHandle());

  return (comm);
}

//==============================================================================
vtkDataArray* GetVtkDataArray(std::string name, int type, void* rawBuffer, int N)
{
  assert("pre: cannot read from nullptr buffer!" && (rawBuffer != nullptr));
  vtkDataArray* dataArray = nullptr;
  size_t dataSize = 0;

  switch (type)
  {
    case gio::GENERIC_IO_INT32_TYPE:
      dataArray = vtkDataArray::CreateDataArray(VTK_TYPE_INT32);
      dataSize = sizeof(vtkTypeInt32);
      break;
    case gio::GENERIC_IO_INT64_TYPE:
      dataArray = vtkDataArray::CreateDataArray(VTK_TYPE_INT64);
      // i.e., don't run this on windows
      assert(sizeof(vtkTypeInt64) == sizeof(uint64_t));
      dataSize = sizeof(vtkTypeInt64);
      break;
    case gio::GENERIC_IO_UINT32_TYPE:
      dataArray = vtkDataArray::CreateDataArray(VTK_TYPE_UINT32);
      dataSize = sizeof(vtkTypeUInt32);
      break;
    case gio::GENERIC_IO_UINT64_TYPE:
      dataArray = vtkDataArray::CreateDataArray(VTK_TYPE_UINT64);
      // i.e., don't run this on windows
      assert(sizeof(vtkTypeUInt64) == sizeof(uint64_t));
      dataSize = sizeof(vtkTypeUInt64);
      break;
    case gio::GENERIC_IO_DOUBLE_TYPE:
      dataArray = vtkDataArray::CreateDataArray(VTK_DOUBLE);
      dataSize = sizeof(double);
      break;
    case gio::GENERIC_IO_FLOAT_TYPE:
      dataArray = vtkDataArray::CreateDataArray(VTK_FLOAT);
      dataSize = sizeof(float);
      break;
    default:
      dataSize = 0;
      return nullptr;
  } // END switch

  assert("pre: nullptr data array!" && (dataArray != nullptr));

  dataArray->SetNumberOfComponents(1);
  dataArray->SetNumberOfTuples(N);
  dataArray->SetName(name.c_str());
  if (N > 0)
  {
    void* dataBuffer = dataArray->GetVoidPointer(0);
    assert("pre: encountered nullptr data buffer!" && (dataBuffer != nullptr));
    memcpy(dataBuffer, rawBuffer, N * dataSize);
  }
  return (dataArray);
}

//==============================================================================
double GetDoubleFromRawBuffer(const int type, void* buffer, vtkIdType buffer_idx)
{
  assert("pre: cannot read from nullptr buffer!" && (buffer != nullptr));

  double dataItem = 0.0;

  switch (type)
  {
    case gio::GENERIC_IO_INT32_TYPE:
    {
      int32_t* dataPtr = static_cast<int32_t*>(buffer);
      dataItem = static_cast<double>(dataPtr[buffer_idx]);
    }
    break;
    case gio::GENERIC_IO_INT64_TYPE:
    {
      int64_t* dataPtr = static_cast<int64_t*>(buffer);
      dataItem = static_cast<double>(dataPtr[buffer_idx]);
    }
    break;
    case gio::GENERIC_IO_UINT32_TYPE:
    {
      uint32_t* dataPtr = static_cast<uint32_t*>(buffer);
      dataItem = static_cast<double>(dataPtr[buffer_idx]);
    }
    break;
    case gio::GENERIC_IO_UINT64_TYPE:
    {
      uint64_t* dataPtr = static_cast<uint64_t*>(buffer);
      dataItem = static_cast<double>(dataPtr[buffer_idx]);
    }
    break;
    case gio::GENERIC_IO_DOUBLE_TYPE:
    {
      double* dataPtr = static_cast<double*>(buffer);
      dataItem = dataPtr[buffer_idx];
    }
    break;
    case gio::GENERIC_IO_FLOAT_TYPE:
    {
      float* dataPtr = static_cast<float*>(buffer);
      dataItem = static_cast<double>(dataPtr[buffer_idx]);
    }
    break;
    default:
      assert("pre: Undefined GENERIC IO type: " && true);
  } // END switch

  return (dataItem);
}

//==============================================================================
vtkIdType GetIdFromRawBuffer(const int type, void* buffer, vtkIdType buffer_idx)
{
  assert("pre: cannot read from nullptr buffer!" && (buffer != nullptr));

  vtkIdType dataItem = 0;

  switch (type)
  {
    case gio::GENERIC_IO_INT32_TYPE:
    {
      int32_t* dataPtr = static_cast<int32_t*>(buffer);
      dataItem = static_cast<vtkIdType>(dataPtr[buffer_idx]);
    }
    break;
    case gio::GENERIC_IO_INT64_TYPE:
    {
      int64_t* dataPtr = static_cast<int64_t*>(buffer);
      dataItem = static_cast<vtkIdType>(dataPtr[buffer_idx]);
    }
    break;
    case gio::GENERIC_IO_UINT32_TYPE:
    {
      uint32_t* dataPtr = static_cast<uint32_t*>(buffer);
      dataItem = static_cast<vtkIdType>(dataPtr[buffer_idx]);
    }
    break;
    case gio::GENERIC_IO_UINT64_TYPE:
    {
      uint64_t* dataPtr = static_cast<uint64_t*>(buffer);
      dataItem = static_cast<vtkIdType>(dataPtr[buffer_idx]);
    }
    break;
    case gio::GENERIC_IO_DOUBLE_TYPE:
    {
      double* dataPtr = static_cast<double*>(buffer);
      dataItem = static_cast<vtkIdType>(dataPtr[buffer_idx]);
    }
    break;
    case gio::GENERIC_IO_FLOAT_TYPE:
    {
      float* dataPtr = static_cast<float*>(buffer);
      dataItem = static_cast<vtkIdType>(dataPtr[buffer_idx]);
    }
    break;
    default:
      assert("pre: Undefined GENERIC IO type: " && true);
  } // END switch

  return (dataItem);
}

//==============================================================================
gio::GenericIOReader* GetReader(
  MPI_Comm comm, bool posix, int distribution, const std::string& fileName)
{
  gio::GenericIOReader* reader = nullptr;
  if (posix)
  {
    reader = new gio::GenericIOPosixReader();
  }
  else
  {
    reader = new gio::GenericIOMPIReader();
  }
  assert("pre: reader is nullptr!" && (reader != nullptr));

  reader->SetCommunicator(comm);
  reader->SetBlockAssignmentStrategy(distribution);
  reader->SetFileName(fileName);
  return (reader);
}

//==============================================================================
gio::GenericIOWriter* GetWriter(MPI_Comm comm, const std::string& fileName)
{
  gio::GenericIOWriter* writer = new gio::GenericIOMPIWriter(comm);
  writer->SetFileName(fileName);
  return writer;
}
}
