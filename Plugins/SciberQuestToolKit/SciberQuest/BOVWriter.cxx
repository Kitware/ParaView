/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "BOVWriter.h"

#include "vtkSetGet.h"
#include "vtkAlgorithm.h"
#include "vtkFloatArray.h"
#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkPointData.h"

#include "BOVTimeStepImage.h"
#include "BOVScalarImageIterator.h"
#include "BOVArrayImageIterator.h"
#include "MPIRawArrayIO.hxx"
#include "SQMacros.h"
#include "PrintUtils.h"
#include "SQVTKTemplateMacroWarningSupression.h"

#include <sstream>

#ifdef WIN32
  #define PATH_SEP "\\"
#else
  #define PATH_SEP "/"
#endif

//-----------------------------------------------------------------------------
BOVWriter::BOVWriter()
      :
  MetaData(NULL),
  ProcId(-1),
  NProcs(0)
{
  #ifdef SQTK_WITHOUT_MPI
  sqErrorMacro(
    std::cerr,
    "This class requires MPI but it was built without MPI.");
  #else
  this->Comm=MPI_COMM_NULL;
  this->Hints=MPI_INFO_NULL;

  int ok;
  MPI_Initialized(&ok);
  if (!ok)
    {
    sqErrorMacro(
      std::cerr,
      << "This class requires the MPI runtime, "
      << "you must run ParaView in client-server mode launched via mpiexec.");
    }
  #endif
}

//-----------------------------------------------------------------------------
BOVWriter::BOVWriter(const BOVWriter &other) : RefCountedPointer()
{
  *this=other;
}

//-----------------------------------------------------------------------------
BOVWriter::~BOVWriter()
{
  this->SetMetaData(NULL);
  #ifndef SQTK_WITHOUT_MPI
  this->SetCommunicator(MPI_COMM_NULL);
  this->SetHints(MPI_INFO_NULL);
  #endif
}

//-----------------------------------------------------------------------------
const BOVWriter &BOVWriter::operator=(const BOVWriter &other)
{
  if (this==&other)
    {
    return *this;
    }

  this->SetCommunicator(other.Comm);
  this->SetHints(other.Hints);
  this->SetMetaData(other.GetMetaData());

  return *this;
}

//-----------------------------------------------------------------------------
void BOVWriter::SetCommunicator(MPI_Comm comm)
{
  #ifdef SQTK_WITHOUT_MPI
  (void)comm;
  #else
  int mpiOk;
  MPI_Initialized(&mpiOk);
  if (!mpiOk)
    {
    sqErrorMacro(std::cerr,
      << "This class requires the MPI runtime, "
      << "you must run ParaView in client-server mode launched via mpiexec.");
    return;
    }

  if (this->Comm==comm) return;

  if ( this->Comm!=MPI_COMM_NULL
    && this->Comm!=MPI_COMM_WORLD
    && this->Comm!=MPI_COMM_SELF)
    {
    MPI_Comm_free(&this->Comm);
    }

  if (comm==MPI_COMM_NULL)
    {
    this->Comm=MPI_COMM_NULL;
    }
  else
    {
    MPI_Comm_dup(comm,&this->Comm);
    MPI_Comm_rank(this->Comm,&this->ProcId);
    MPI_Comm_size(this->Comm,&this->NProcs);
    }
  #endif
}

//-----------------------------------------------------------------------------
void BOVWriter::SetHints(MPI_Info hints)
{
  #ifdef SQTK_WITHOUT_MPI
  (void)hints;
  #else
  int mpiOk;
  MPI_Initialized(&mpiOk);
  if (!mpiOk)
    {
    sqErrorMacro(std::cerr,
      << "This class requires the MPI runtime, "
      << "you must run ParaView in client-server mode launched via mpiexec.");
    return;
    }

  if (this->Hints==hints) return;

  if (this->Hints!=MPI_INFO_NULL)
    {
    MPI_Info_free(&this->Hints);
    }

  if (hints==MPI_INFO_NULL)
    {
    this->Hints=MPI_INFO_NULL;
    }
  else
    {
    MPI_Info_dup(hints,&this->Hints);
    }
  #endif
}

//-----------------------------------------------------------------------------
void BOVWriter::SetMetaData(const BOVMetaData *metaData)
{
  if (this->MetaData==metaData) return;

  if (this->MetaData!=metaData)
    {
    if (this->MetaData)
      {
      delete this->MetaData;
      this->MetaData=NULL;
      }
    if (metaData)
      {
      this->MetaData=metaData->Duplicate();
      }
    }
}

//-----------------------------------------------------------------------------
int BOVWriter::Open(const char *fileName, char mode)
{
  if (this->MetaData==0)
    {
    sqErrorMacro(std::cerr,"No MetaData object.");
    return 0;
    }

  int ok=this->MetaData->OpenDataset(fileName,mode);
  return ok;
}

//-----------------------------------------------------------------------------
int BOVWriter::Close()
{
  int ok=this->MetaData->CloseDataset();
  return ok;
}

//-----------------------------------------------------------------------------
bool BOVWriter::IsOpen()
{
  if (this->MetaData)
    {
    return (bool)this->MetaData->IsDatasetOpen();
    }
  return false;
}

//-----------------------------------------------------------------------------
int BOVWriter::WriteMetaData()
{
  if (!this->IsOpen())
    {
    return 0;
    }

  if (this->ProcId==0)
    {
    return this->MetaData->Write();
    }

  return 1;
}

//-----------------------------------------------------------------------------
int BOVWriter::WriteScalarArray(
      const BOVScalarImageIterator &it,
      vtkDataSet *grid)
{
  vtkDataArray *array=grid->GetPointData()->GetArray(it.GetName());
  if (array==0)
    {
    sqErrorMacro(pCerr(),"Array " << it.GetName() << " not present.");
    return 0;
    }

  const CartesianExtent &domain=this->MetaData->GetDomain();
  const CartesianExtent &decomp=this->MetaData->GetDecomp();

  // write
  int ok=0;
  switch(array->GetDataType())
  {
    vtkTemplateMacro(
        ok=WriteDataArray(
          it.GetFile(),
          this->Hints,
          domain,
          decomp,
          1,
          0,
          (VTK_TT*)array->GetVoidPointer(0));
      );
  }

  return ok;
}

//-----------------------------------------------------------------------------
int BOVWriter::WriteVectorArray(
      const BOVArrayImageIterator &it,
      vtkDataSet *grid)
{
  // Memory requirements:
  // The vtk data is nComps*sizeof(component file).
  // The intermediate buffer is sizeof(component file).
  // One might use a strided mpi type for the memory view
  // but it's much slower in our tests

  vtkDataArray *array=grid->GetPointData()->GetArray(it.GetName());
  if (array==0)
    {
    sqErrorMacro(pCerr(),"Array " << it.GetName() << " not present.");
    return 0;
    }

  const CartesianExtent &domain=this->MetaData->GetDomain();
  const CartesianExtent &decomp=this->MetaData->GetDecomp();
  const size_t nCells=decomp.Size();

  const int nComps=it.GetNumberOfComponents();

  switch(array->GetDataType())
    {
    vtkTemplateMacro(

      VTK_TT *pArray=(VTK_TT*)array->GetVoidPointer(0);
      VTK_TT *buf=(VTK_TT*)malloc(nCells*sizeof(VTK_TT));

      for (int q=0; q<nComps; ++q)
        {
        // flatten
        for (size_t i=0; i<nCells; ++i)
          {
          buf[i]=pArray[nComps*i+q];
          }
        // Write qth component array
        if (!WriteDataArray(
              it.GetComponentFile(q),
              this->Hints,
              domain,
              decomp,
              1,
              0,
              buf))
          {
              sqErrorMacro(std::cerr,
                "WriteDataArray "<< it.GetName() << " component " << q << " failed.");
              free(buf);
              return 0;
          }
        }
      free(buf);
      );
    }

  return 1;
}

//-----------------------------------------------------------------------------
int BOVWriter::WriteSymetricTensorArray(
      const BOVArrayImageIterator &it,
      vtkDataSet *grid)
{
  // Memory requirements:
  // The vtk data is nComps*sizeof(component file).
  // The intermediate buffer is sizeof(component file).
  // One might use a strided mpi type for the memory view
  // but it's much slower if the seive buffer is either
  // disabled or too small to hold the read.

  vtkDataArray *array=grid->GetPointData()->GetArray(it.GetName());
  if (array==0)
    {
    sqErrorMacro(pCerr(),"Array " << it.GetName() << " not present.");
    return 0;
    }

  const CartesianExtent &domain=this->MetaData->GetDomain();
  const CartesianExtent &decomp=this->MetaData->GetDecomp();
  const size_t nCells=decomp.Size();

  const int memComp[6]={0,1,2,4,5,8};

  switch(array->GetDataType())
    {
    vtkTemplateMacro(

      VTK_TT *pArray=(VTK_TT*)array->GetVoidPointer(0);
      VTK_TT *buf=(VTK_TT*)malloc(nCells*sizeof(VTK_TT));

      for (int q=0; q<6; ++q)
        {
        // flatten
        for (size_t i=0; i<nCells; ++i)
          {
          buf[i]=pArray[9*i+memComp[q]];
          }
        // Write qth component array
        if (!WriteDataArray(
              it.GetComponentFile(q),
              this->Hints,
              domain,
              decomp,
              1,
              0,
              buf))
          {
          sqErrorMacro(std::cerr,
            "WriteDataArray "<< it.GetName() << " component " << q << " failed.");
          free(buf);
          return 0;
          }
        }

      free(buf);
      );
  }

  return 1;
}

//-----------------------------------------------------------------------------
BOVTimeStepImage *BOVWriter::OpenTimeStep(int stepNo)
{
  if (!(this->MetaData && this->MetaData->IsDatasetOpen()))
    {
    sqErrorMacro(std::cerr,
      << "Cannot open a timestep because the "
      << "dataset is not open.");
    return 0;
    }

  return
    new BOVTimeStepImage(this->Comm,this->Hints,stepNo,this->GetMetaData());
}

//-----------------------------------------------------------------------------
void BOVWriter::CloseTimeStep(BOVTimeStepImage *handle)
{
  delete handle;
  handle=0;
}

//-----------------------------------------------------------------------------
int BOVWriter::WriteTimeStep(
      const BOVTimeStepImage *step,
      vtkDataSet *grid,
      vtkAlgorithm *alg)
{
  double progInc=0.70/(double)step->GetNumberOfImages();
  double prog=0.25;
  if(alg)alg->UpdateProgress(prog);

  // scalars
  BOVScalarImageIterator sIt(step);
  for (;sIt.Ok(); sIt.Next())
    {
    int ok=this->WriteScalarArray(sIt,grid);
    if (!ok)
      {
      return 0;
      }
    prog+=progInc;
    if(alg)alg->UpdateProgress(prog);
    }

  // vectors
  BOVVectorImageIterator vIt(step);
  for (;vIt.Ok(); vIt.Next())
    {
    int ok=this->WriteVectorArray(vIt,grid);
    if (!ok)
      {
      return 0;
      }
    prog+=progInc;
    if(alg)alg->UpdateProgress(prog);
    }

  // tensors
  BOVTensorImageIterator tIt(step);
  for (;tIt.Ok(); tIt.Next())
    {
    int ok=this->WriteVectorArray(tIt,grid);
    if (!ok)
      {
      return 0;
      }
    prog+=progInc;
    if(alg)alg->UpdateProgress(prog);
    }

  // symetric tensors
  BOVSymetricTensorImageIterator stIt(step);
  for (;stIt.Ok(); stIt.Next())
    {
    int ok=this->WriteSymetricTensorArray(stIt,grid);
    if (!ok)
      {
      return 0;
      }
    prog+=progInc;
    if(alg)alg->UpdateProgress(prog);
    }

  return 1;
}

//-----------------------------------------------------------------------------
void BOVWriter::PrintSelf(ostream &os)
{
  os
    << "BOVWriter: " << this << std::endl
    << "  Comm: " << this->Comm << std::endl
    << "  ProcId: " << this->ProcId << std::endl
    << "  NProcs: " << this->NProcs << std::endl;

  #ifndef SQTK_WITHOUT_MPI
  int mpiOk;
  MPI_Initialized(&mpiOk);
  if (!mpiOk)
    {
    sqErrorMacro(std::cerr,
      << "This class requires the MPI runtime, "
      << "you must run ParaView in client-server mode launched via mpiexec.");
    return;
    }

  if (this->Hints!=MPI_INFO_NULL)
    {
    os << "  Hints:" << std::endl;
    int nKeys=0;
    MPI_Info_get_nkeys(this->Hints,&nKeys);
    for (int i=0; i<nKeys; ++i)
      {
      char key[256];
      char val[256];
      int flag=0;
      MPI_Info_get_nthkey(this->Hints,i,key);
      MPI_Info_get(this->Hints,key,256,val,&flag);
      os << "    " << key << "=" << val << std::endl;
      }
    }
  #endif

  this->MetaData->Print(os);
}
