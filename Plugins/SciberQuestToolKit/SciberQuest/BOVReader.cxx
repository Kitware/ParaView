/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "BOVReader.h"

#include "vtkAlgorithm.h"
#include "vtkFloatArray.h"
#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkPointData.h"

#include "BinaryStream.hxx"
#include "BOVTimeStepImage.h"
#include "BOVScalarImageIterator.h"
#include "BOVArrayImageIterator.h"
#include "CartesianDataBlockIODescriptor.h"
#include "CartesianDataBlockIODescriptorIterator.h"
#include "MPIRawArrayIO.hxx"
#include "SQMacros.h"
#include "PrintUtils.h"

#include <sstream>

#ifdef WIN32
  #define PATH_SEP "\\"
#else
  #define PATH_SEP "/"
#endif

//#define BOVReaderTIME

#ifdef WIN32
  // these are only usefull in terminals
  #undef BOVReaderTIME
  #undef BOVReaderDEBUG
#endif

#if defined BOVReaderTIME
  #include "vtkSQLog.h"
#endif

//-----------------------------------------------------------------------------
BOVReader::BOVReader()
      :
  MetaData(NULL),
  NGhost(1),
  ProcId(-1),
  NProcs(0),
  VectorProjection(VECTOR_PROJECT_NONE)
{
  #ifdef SQTK_WITHOUT_MPI
  // don't report this error since the reader get's constructed
  // as part of PV's format selection process.
  //sqErrorMacro(
  //    std::cerr,
  //    << "This class requires MPI however it was built without MPI.");
  #else
  this->Comm=MPI_COMM_NULL;
  this->Hints=MPI_INFO_NULL;

  int mpiOk;
  MPI_Initialized(&mpiOk);
  if (!mpiOk)
    {
    sqWarningMacro(
      std::cerr,
      << "This class requires the MPI runtime, "
      << "you must run ParaView in client-server mode launched via mpiexec.");
    }
  #endif
}

//-----------------------------------------------------------------------------
BOVReader::BOVReader(const BOVReader &other) : RefCountedPointer()
{
  *this=other;
}

//-----------------------------------------------------------------------------
BOVReader::~BOVReader()
{
  this->SetMetaData(NULL);
  #ifndef SQTK_WITHOUT_MPI
  this->SetCommunicator(MPI_COMM_NULL);
  this->SetHints(MPI_INFO_NULL);
  #endif
}

//-----------------------------------------------------------------------------
const BOVReader &BOVReader::operator=(const BOVReader &other)
{
  if (this==&other)
    {
    return *this;
    }

  this->SetCommunicator(other.Comm);
  this->SetHints(other.Hints);
  this->SetMetaData(other.GetMetaData());
  this->NGhost=other.NGhost;
  this->VectorProjection=other.VectorProjection;

  return *this;
}

//-----------------------------------------------------------------------------
void BOVReader::SetCommunicator(MPI_Comm comm)
{
  #ifndef SQTK_WITHOUT_MPI
  if (this->Comm==comm) return;

  int mpiOk=0;
  MPI_Initialized(&mpiOk);
  if (!mpiOk)
    {
    sqErrorMacro(
      std::cerr,
      << "This class requires the MPI runtime, "
      << "you must run ParaView in client-server mode launched via mpiexec.");
    return;
    }

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
void BOVReader::SetHints(MPI_Info hints)
{
  #ifdef SQTK_WITHOUT_MPI
  (void)hints;
  #else
  if (this->Hints==hints) return;

  int mpiOk=0;
  MPI_Initialized(&mpiOk);
  if (!mpiOk)
    {
    sqErrorMacro(
      std::cerr,
      << "This class requires the MPI runtime, "
      << "you must run ParaView in client-server mode launched via mpiexec.");
    return;
    }

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
void BOVReader::SetMetaData(const BOVMetaData *metaData)
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
int BOVReader::Close()
{
  return this->MetaData && this->MetaData->CloseDataset();
}

//-----------------------------------------------------------------------------
int BOVReader::Open(const char *fileName)
{
  #if defined BOVReaderTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->StartEvent("BOVReader::Open");
  #endif

  int ok=0;

  #ifdef SQTK_WITHOUT_MPI
  (void)fileName;
  #else
  int mpiOk=0;
  MPI_Initialized(&mpiOk);
  if (!mpiOk)
    {
    sqErrorMacro(
      std::cerr,
      << "This class requires the MPI runtime, "
      << "you must run ParaView in client-server mode launched via mpiexec.");
    return 0;
    }

  if (this->MetaData==0)
    {
    sqErrorMacro(std::cerr,"No MetaData object.");
    return 0;
    }

  // Only one process touches the disk to avoid
  // swamping the metadata server.
  if (this->ProcId==0)
    {
    ok=this->MetaData->OpenDataset(fileName,'r');
    if (!ok)
      {
      int nBytes=0;
      MPI_Bcast(&nBytes,1,MPI_INT,0,this->Comm);
      }
    else
      {
      BinaryStream str;
      this->MetaData->Pack(str);
      int nBytes=(int)str.GetSize();
      MPI_Bcast(&nBytes,1,MPI_INT,0,this->Comm);
      MPI_Bcast(str.GetData(),nBytes,MPI_CHAR,0,this->Comm);
      }
    }
  // other processes are initialized from a stream
  // broadcast by the root.
  else
    {
    int nBytes;
    MPI_Bcast(&nBytes,1,MPI_INT,0,this->Comm);
    if (nBytes>0)
      {
      ok=1;
      BinaryStream str;
      str.Resize(nBytes);
      MPI_Bcast(str.GetData(),nBytes,MPI_CHAR,0,this->Comm);
      this->MetaData->UnPack(str);
      }
    }
  #endif

  #if defined BOVReaderTIME
  log->EndEvent("BOVReader::Open");
  #endif

  return ok;
}

//-----------------------------------------------------------------------------
bool BOVReader::IsOpen()
{
  if (this->MetaData)
    {
    return (bool)this->MetaData->IsDatasetOpen();
    }
  return false;
}

//-----------------------------------------------------------------------------
vtkDataSet *BOVReader::GetDataSet()
{
  if (this->MetaData->DataSetTypeIsImage())
    {
    return vtkImageData::New();
    }
  else
  if (this->MetaData->DataSetTypeIsRectilinear())
    {
    return vtkRectilinearGrid::New();
    }
  else
  if (this->MetaData->DataSetTypeIsStructured())
    {
    return vtkStructuredGrid::New();
    }
  else
    {
    sqErrorMacro(std::cerr,
      << "Unsupported dataset type \""
      << this->MetaData->GetDataSetType()
      <<  "\".");
    return 0;
    }
}

//-----------------------------------------------------------------------------
int BOVReader::ReadScalarArray(
      const BOVScalarImageIterator &it,
      vtkDataSet *grid)
{
  #if defined BOVReaderTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->StartEvent("BOVReader::ReadScalarArray");
  #endif

  const CartesianExtent &decomp=this->MetaData->GetDecomp();
  const size_t nCells=decomp.Size();

  // Create a VTK array and insert it into the point data.
  vtkFloatArray *fa=vtkFloatArray::New();
  fa->SetNumberOfComponents(1);
  fa->SetNumberOfTuples(nCells); // dual grid
  fa->SetName(it.GetName());
  grid->GetPointData()->AddArray(fa);
  fa->Delete();
  float *pfa=fa->GetPointer(0);

  // read
  int ok=ReadDataArray(
          it.GetFile(),
          this->Hints,
          this->MetaData->GetDomain(),
          decomp,
          1,
          0,
          pfa);

  #if defined BOVReaderTIME
  log->EndEvent("BOVReader::ReadScalarArray");
  #endif

  return ok;
}

//-----------------------------------------------------------------------------
int BOVReader::ReadScalarArray(
      const BOVScalarImageIterator &fhit,
      const CartesianDataBlockIODescriptor *descr,
      vtkDataSet *grid)
{
  #if defined BOVReaderTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->StartEvent("BOVReader::ReadScalarArray");
  #endif

  const CartesianExtent &memExt=descr->GetMemExtent();
  size_t nPts=memExt.Size();

  vtkFloatArray *scal=vtkFloatArray::New();
  scal->SetNumberOfComponents(1);
  scal->SetNumberOfTuples(nPts);
  scal->SetName(fhit.GetName());
  grid->GetPointData()->AddArray(scal);
  scal->Delete();
  float *pScal=scal->GetPointer(0);

  CartesianDataBlockIODescriptorIterator ioit(descr);
  for (; ioit.Ok(); ioit.Next())
    {
    if (!ReadDataArray(
            fhit.GetFile(),
            this->Hints,
            ioit.GetMemView(),
            ioit.GetFileView(),
            pScal))
      {
      sqErrorMacro(std::cerr,
        << "ReadDataArray "<< fhit.GetName()
        << " views " << ioit
        << " failed.");
      return 0;
      }
    }

  #if defined BOVReaderTIME
  log->EndEvent("BOVReader::ReadScalarArray");
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int BOVReader::ReadVectorArray(
      const BOVArrayImageIterator &it,
      vtkDataSet *grid)
{
  #if defined BOVReaderTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->StartEvent("BOVReader::ReadVectorArray");
  #endif

  // Memory requirements:
  // The vtk data is nComps*sizeof(component file).
  // The intermediate buffer is sizeof(component file).
  // One might use a strided mpi type for the memory view
  // but it's much slower if the seive buffer is either
  // disabled or too small to hold the read.

  const CartesianExtent &domain=this->MetaData->GetDomain();
  const CartesianExtent &decomp=this->MetaData->GetDecomp();
  const size_t nCells=decomp.Size();

  const int nComps=it.GetNumberOfComponents();

  // Create a VTK array.
  vtkFloatArray *fa=vtkFloatArray::New();
  fa->SetNumberOfComponents(nComps);
  fa->SetNumberOfTuples(nCells); // dual grid
  fa->SetName(it.GetName());
  grid->GetPointData()->AddArray(fa);
  fa->Delete();
  float *pfa=fa->GetPointer(0);

  float *buf=static_cast<float*>(malloc(nCells*sizeof(float)));

  for (int q=0; q<nComps; ++q)
    {
    // if a projection is requested then we zero out
    // the out-of-plane component and skip the I/O/.
    int p=1<<q;
    if (p&this->VectorProjection)
      {
      for (size_t i=0; i<nCells; ++i)
        {
        pfa[nComps*i+q]=0.0;
        }
      continue;
      }

    // Read qth component array
    if (!ReadDataArray(
            it.GetComponentFile(q),
            this->Hints,
            domain,
            decomp,
            1,0,
            buf))
      {
      sqErrorMacro(std::cerr,
        "ReadDataArray "<< it.GetName() << " component " << q << " failed.");
      free(buf);
      return 0;
      }
    // unpack from the read buffer into the vtk array
    for (size_t i=0; i<nCells; ++i)
      {
      pfa[nComps*i+q]=buf[i];
      }
    }

  free(buf);

  #if defined BOVReaderTIME
  log->EndEvent("BOVReader::ReadVectorArray");
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int BOVReader::ReadVectorArray(
      const BOVArrayImageIterator &fhit,
      const CartesianDataBlockIODescriptor *descr,
      vtkDataSet *grid)
{
  #if defined BOVReaderTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->StartEvent("BOVReader::ReadVectorArray");
  #endif

  const CartesianExtent &memExt=descr->GetMemExtent();
  size_t nPts=memExt.Size();

  float *buf=(float*)malloc(nPts*sizeof(float));

  const int nComps=fhit.GetNumberOfComponents();

  vtkFloatArray *vec=vtkFloatArray::New();
  vec->SetNumberOfComponents(nComps);
  vec->SetNumberOfTuples(nPts);
  vec->SetName(fhit.GetName());
  grid->GetPointData()->AddArray(vec);
  vec->Delete();
  float *pVec=vec->GetPointer(0);

  CartesianDataBlockIODescriptorIterator ioit(descr);

  for (int q=0; q<nComps; ++q)
    {
    // if a projection is requested then we zero out
    // the out-of-plane component and skip the I/O/.
    int p=1<<q;
    if (p&this->VectorProjection)
      {
      for (size_t i=0; i<nPts; ++i)
        {
        pVec[nComps*i+q]=0.0;
        }
      continue;
      }

    // read the qth component
    for (ioit.Initialize(); ioit.Ok(); ioit.Next())
      {
      if (!ReadDataArray(
              fhit.GetComponentFile(q),
              this->Hints,
              ioit.GetMemView(),
              ioit.GetFileView(),
              buf))
        {
        sqErrorMacro(std::cerr,
          << "ReadDataArray "<< fhit.GetName()
          << " component " << q
          << " views " << ioit
          << " failed.");
        free(buf);
        return 0;
        }
      }

    for (size_t i=0; i<nPts; ++i)
      {
      pVec[nComps*i+q]=buf[i];
      }
    }

  free(buf);

  #if defined BOVReaderTIME
  log->EndEvent("BOVReader::ReadVectorArray");
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int BOVReader::ReadSymetricTensorArray(
      const BOVArrayImageIterator &it,
      vtkDataSet *grid)
{
  #if defined BOVReaderTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->StartEvent("BOVReader::SymetricTensorArray");
  #endif

  // Memory requirements:
  // The vtk data is nComps*sizeof(component file).
  // The intermediate buffer is sizeof(component file).
  // One might use a strided mpi type for the memory view
  // but it's much slower if the seive buffer is either
  // disabled or too small to hold the read.

  const CartesianExtent &domain=this->MetaData->GetDomain();
  const CartesianExtent &decomp=this->MetaData->GetDecomp();
  const size_t nCells=decomp.Size();

  // Create a VTK array.
  vtkFloatArray *fa=vtkFloatArray::New();
  fa->SetNumberOfComponents(9);
  fa->SetNumberOfTuples(nCells); // dual grid
  fa->SetName(it.GetName());
  grid->GetPointData()->AddArray(fa);
  fa->Delete();
  float *pfa=fa->GetPointer(0);

  float *buf=static_cast<float*>(malloc(nCells*sizeof(float)));

  // maps file component to memory component
  const int memComp[6]={0,1,2,4,5,8};

  for (int q=0; q<6; ++q)
    {
    // Read qth component array
    if (!ReadDataArray(
            it.GetComponentFile(q),
            this->Hints,
            domain,
            decomp,
            1,0,
            buf))
      {
      sqErrorMacro(std::cerr,
        "ReadDataArray "<< it.GetName() << " component " << q << " failed.");
      free(buf);
      return 0;
      }
    // unpack from the read buffer into the vtk array
    for (size_t i=0; i<nCells; ++i)
      {
      pfa[9*i+memComp[q]]=buf[i];
      }
    }

  free(buf);

  // fill in the symetric components
  const int srcComp[3]={1,2,5};
  const int desComp[3]={3,6,7};
  for (int q=0; q<3; ++q)
    {
    for (size_t i=0; i<nCells; ++i)
      {
      pfa[9*i+desComp[q]]=pfa[9*i+srcComp[q]];
      }
    }

  #if defined BOVReaderTIME
  log->EndEvent("BOVReader::ReadSymetricTensorArray");
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int BOVReader::ReadSymetricTensorArray(
      const BOVArrayImageIterator &fhit,
      const CartesianDataBlockIODescriptor *descr,
      vtkDataSet *grid)
{
  #if defined BOVReaderTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->StartEvent("BOVReader::SymetricTensorArray");
  #endif

  const CartesianExtent &memExt=descr->GetMemExtent();
  size_t nPts=memExt.Size();

  float *buf=(float*)malloc(nPts*sizeof(float));

  vtkFloatArray *vec=vtkFloatArray::New();
  vec->SetNumberOfComponents(9);
  vec->SetNumberOfTuples(nPts);
  vec->SetName(fhit.GetName());
  grid->GetPointData()->AddArray(vec);
  vec->Delete();
  float *pVec=vec->GetPointer(0);

  CartesianDataBlockIODescriptorIterator ioit(descr);

  // maps file component to memory component
  int memComp[6]={0,1,2,4,5,8};

  for (int q=0; q<6; ++q)
    {
    for (ioit.Initialize(); ioit.Ok(); ioit.Next())
      {
      if (!ReadDataArray(
              fhit.GetComponentFile(q),
              this->Hints,
              ioit.GetMemView(),
              ioit.GetFileView(),
              buf))
        {
        sqErrorMacro(std::cerr,
          << "ReadDataArray "<< fhit.GetName()
          << " component " << q
          << " views " << ioit
          << " failed.");
        free(buf);
        return 0;
        }
      }

    for (size_t i=0; i<nPts; ++i)
      {
      pVec[9*i+memComp[q]]=buf[i];
      }
    }

  free(buf);

  // fill in the symetric components
  int srcComp[3]={1,2,5};
  int desComp[3]={3,6,7};
  for (int q=0; q<3; ++q)
    {
    for (size_t i=0; i<nPts; ++i)
      {
      pVec[9*i+desComp[q]]=pVec[9*i+srcComp[q]];
      }
    }

  #if defined BOVReaderTIME
  log->EndEvent("BOVReader::ReadSymetricTensorArray");
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
BOVTimeStepImage *BOVReader::OpenTimeStep(int stepNo)
{
  #if defined BOVReaderTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->StartEvent("BOVReader::OpenTimeStep");
  #endif

  if (!(this->MetaData && this->MetaData->IsDatasetOpen()))
    {
    sqErrorMacro(std::cerr,
      << "Cannot open a timestep because the "
      << "dataset is not open.");
    return 0;
    }

  BOVTimeStepImage *image
    = new BOVTimeStepImage(this->Comm,this->Hints,stepNo,this->GetMetaData());

  #if defined BOVReaderTIME
  log->EndEvent("BOVReader::OpenTimeStep");
  #endif

  return image;
}

//-----------------------------------------------------------------------------
void BOVReader::CloseTimeStep(BOVTimeStepImage *handle)
{
  delete handle;
  handle=0;
}

//-----------------------------------------------------------------------------
int BOVReader::ReadTimeStep(
      const BOVTimeStepImage *step,
      const CartesianDataBlockIODescriptor *descr,
      vtkDataSet *grid,
      vtkAlgorithm *alg)
{
  #if defined BOVReaderTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->StartEvent("BOVReader::ReadTimeStep");
  #endif

  double progInc=0.70/(double)step->GetNumberOfImages();
  double prog=0.25;
  if(alg)alg->UpdateProgress(prog);

  // scalars
  BOVScalarImageIterator sIt(step);
  for (;sIt.Ok(); sIt.Next())
    {
    int ok=this->ReadScalarArray(sIt,descr,grid);
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
    int ok=this->ReadVectorArray(vIt,descr,grid);
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
    int ok=this->ReadVectorArray(tIt,descr,grid);
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
    int ok=this->ReadSymetricTensorArray(stIt,descr,grid);
    if (!ok)
      {
      return 0;
      }
    prog+=progInc;
    if(alg)alg->UpdateProgress(prog);
    }

  #if defined BOVReaderTIME
  log->EndEvent("BOVReader::ReadTimeStep");
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int BOVReader::ReadTimeStep(
      const BOVTimeStepImage *step,
      vtkDataSet *grid,
      vtkAlgorithm *alg)
{
  #if defined BOVReaderTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->StartEvent("BOVReader::ReadTimeStep");
  #endif

  double progInc=0.70/(double)step->GetNumberOfImages();
  double prog=0.25;
  if(alg)alg->UpdateProgress(prog);

  // scalars
  BOVScalarImageIterator sIt(step);
  for (;sIt.Ok(); sIt.Next())
    {
    int ok=this->ReadScalarArray(sIt,grid);
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
    int ok=this->ReadVectorArray(vIt,grid);
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
    int ok=this->ReadVectorArray(tIt,grid);
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
    int ok=this->ReadSymetricTensorArray(stIt,grid);
    if (!ok)
      {
      return 0;
      }
    prog+=progInc;
    if(alg)alg->UpdateProgress(prog);
    }

  #if defined BOVReaderTIME
  log->EndEvent("BOVReader::ReadTimeStep");
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int BOVReader::ReadMetaTimeStep(int stepIdx, vtkDataSet *grid, vtkAlgorithm *alg)
{
  #if defined BOVReaderTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->StartEvent("BOVReader::ReadMetaTimeStep");
  #endif

  if (!(this->MetaData && this->MetaData->IsDatasetOpen()))
    {
    sqErrorMacro(std::cerr,"Cannot read because the dataset is not open.");
    return 0;
    }

  if (grid==NULL)
    {
    sqErrorMacro(std::cerr,"Empty output.");
    return 0;
    }

  std::ostringstream seriesExt;
  seriesExt << "_" << stepIdx << "." << this->MetaData->GetBrickFileExtension();
  // In a meta read we won't read the arrays, rather we'll put a place holder
  // for each in the output. Downstream array can then be selected.
  const size_t nPoints=grid->GetNumberOfPoints();
  const size_t nArrays=this->MetaData->GetNumberOfArrays();
  double progInc=0.75/(double)nArrays;
  double prog=0.25;
  if(alg)alg->UpdateProgress(prog);
  for (size_t i=0; i<nArrays; ++i)
    {
    const char *arrayName=this->MetaData->GetArrayName(i);
    // skip inactive arrays
    if (!this->MetaData->IsArrayActive(arrayName))
      {
      prog+=progInc;
      if(alg)alg->UpdateProgress(prog);
      continue;
      }
    vtkFloatArray *array=vtkFloatArray::New();
    array->SetName(arrayName);
    // scalar
    if (this->MetaData->IsArrayScalar(arrayName))
      {
      array->SetNumberOfTuples(nPoints);
      array->FillComponent(0,-5.5);
      }
    // vector
    else
    if (this->MetaData->IsArrayVector(arrayName))
      {
      array->SetNumberOfComponents(3);
      array->SetNumberOfTuples(nPoints);
      array->FillComponent(0,-5.5);
      array->FillComponent(1,-5.5);
      array->FillComponent(2,-5.5);
      }
    // tensor
    else
    if (this->MetaData->IsArrayTensor(arrayName)
      ||this->MetaData->IsArraySymetricTensor(arrayName))
      {
      array->SetNumberOfComponents(9);
      array->SetNumberOfTuples(nPoints);
      array->FillComponent(0,-5.5);
      array->FillComponent(1,-5.5);
      array->FillComponent(2,-5.5);
      array->FillComponent(3,-5.5);
      array->FillComponent(4,-5.5);
      array->FillComponent(5,-5.5);
      array->FillComponent(6,-5.5);
      array->FillComponent(7,-5.5);
      array->FillComponent(8,-5.5);
      }
    // other ?
    else
      {
      sqErrorMacro(std::cerr,"Bad array type for array " << arrayName << ".");
      }
    grid->GetPointData()->AddArray(array);
    array->Delete();

    prog+=progInc;
    if(alg)alg->UpdateProgress(prog);
    }

  #if defined BOVReaderTIME
  log->EndEvent("BOVReader::ReadMetaTimeStep");
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
void BOVReader::PrintSelf(ostream &os)
{
  os
    << "BOVReader: " << this << std::endl
    << "  Comm: " << this->Comm << std::endl
    << "  NGhost: " << this->NGhost << std::endl
    << "  ProcId: " << this->ProcId << std::endl
    << "  NProcs: " << this->NProcs << std::endl
    << "  VectorProjection: " << this->VectorProjection << std::endl;

  #ifndef SQTK_WITHOUT_MPI
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
