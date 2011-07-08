/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2008 SciberQuest Inc.
*/
#include "BOVReader.h"

#include "vtkAlgorithm.h"
#include "vtkFloatArray.h"
#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkPointData.h"
// #include "vtkMultiProcessController.h"
// #include "vtkMPICommunicator.h"

#include "BinaryStream.hxx"
#include "BOVTimeStepImage.h"
#include "BOVScalarImageIterator.h"
#include "BOVArrayImageIterator.h"
#include "CartesianDataBlockIODescriptor.h"
#include "CartesianDataBlockIODescriptorIterator.h"
#include "MPIRawArrayIO.hxx"
#include "SQMacros.h"
#include "PrintUtils.h"

#include <mpi.h>

#include <sstream>
using std::ostringstream;

#ifdef WIN32
  #define PATH_SEP "\\"
#else
  #define PATH_SEP "/"
#endif

//-----------------------------------------------------------------------------
BOVReader::BOVReader()
      :
  MetaData(NULL),
  NGhost(1),
  ProcId(-1),
  NProcs(0),
  Comm(MPI_COMM_NULL),
  Hints(MPI_INFO_NULL)
{
  int ok;
  MPI_Initialized(&ok);
  if (!ok)
    {
    sqErrorMacro(cerr,
      << "The BOVReader requires MPI. Start ParaView in"
      << " Client-Server mode using mpiexec.");
    }
}

//-----------------------------------------------------------------------------
BOVReader::BOVReader(const BOVReader &other)
{
  *this=other;
}

//-----------------------------------------------------------------------------
BOVReader::~BOVReader()
{
  this->SetMetaData(NULL);
  this->SetCommunicator(MPI_COMM_NULL);
  this->SetHints(MPI_INFO_NULL);
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

  return *this;
}

//-----------------------------------------------------------------------------
void BOVReader::SetCommunicator(MPI_Comm comm)
{
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
}

//-----------------------------------------------------------------------------
void BOVReader::SetHints(MPI_Info hints)
{
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
  if (this->MetaData==0)
    {
    sqErrorMacro(cerr,"No MetaData object.");
    return 0;
    }

  // Only one process touches the disk to avoid
  // swamping the metadata server.
  int ok=0;
  if (this->ProcId==0)
    {
    ok=this->MetaData->OpenDataset(fileName);
    if (!ok)
      {
      int nBytes=0;
      MPI_Bcast(&nBytes,1,MPI_INT,0,this->Comm);
      }
    else
      {
      BinaryStream str;
      this->MetaData->Pack(str);
      int nBytes=str.GetSize();
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
    sqErrorMacro(cerr,
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
  return
    ReadDataArray(
          it.GetFile(),
          this->Hints,
          this->MetaData->GetDomain(),
          decomp,
          1,
          0,
          pfa);
}

//-----------------------------------------------------------------------------
int BOVReader::ReadScalarArray(
      const BOVScalarImageIterator &fhit,
      const CartesianDataBlockIODescriptor *descr,
      vtkDataSet *grid)
{
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
      sqErrorMacro(cerr,
        << "ReadDataArray "<< fhit.GetName()
        << " views " << ioit
        << " failed.");
      return 0;
      }
    }

  return 1;
}

//-----------------------------------------------------------------------------
int BOVReader::ReadVectorArray(
      const BOVArrayImageIterator &it,
      vtkDataSet *grid)
{
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
    // Read qth component array
    if (!ReadDataArray(
            it.GetComponentFile(q),
            this->Hints,
            domain,
            decomp,
            1,0,
            buf))
      {
      sqErrorMacro(cerr,
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

  return 1;
}

//-----------------------------------------------------------------------------
int BOVReader::ReadVectorArray(
      const BOVArrayImageIterator &fhit,
      const CartesianDataBlockIODescriptor *descr,
      vtkDataSet *grid)
{
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
    for (ioit.Initialize(); ioit.Ok(); ioit.Next())
      {
      if (!ReadDataArray(
              fhit.GetComponentFile(q),
              this->Hints,
              ioit.GetMemView(),
              ioit.GetFileView(),
              buf))
        {
        sqErrorMacro(cerr,
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

  return 1;
}

//-----------------------------------------------------------------------------
int BOVReader::ReadSymetricTensorArray(
      const BOVArrayImageIterator &it,
      vtkDataSet *grid)
{
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
      sqErrorMacro(cerr,
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

  return 1;
}

//-----------------------------------------------------------------------------
int BOVReader::ReadSymetricTensorArray(
      const BOVArrayImageIterator &fhit,
      const CartesianDataBlockIODescriptor *descr,
      vtkDataSet *grid)
{
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
        sqErrorMacro(cerr,
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

  return 1;
}

//-----------------------------------------------------------------------------
BOVTimeStepImage *BOVReader::OpenTimeStep(int stepNo)
{
  if (!(this->MetaData && this->MetaData->IsDatasetOpen()))
    {
    sqErrorMacro(cerr,
      << "Cannot open a timestep because the "
      << "dataset is not open.");
    return 0;
    }

  return
    new BOVTimeStepImage(this->Comm,this->Hints,stepNo,this->GetMetaData());
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
  double progInc=0.70/step->GetNumberOfImages();
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

  return 1;

}

//-----------------------------------------------------------------------------
int BOVReader::ReadTimeStep(
      const BOVTimeStepImage *step,
      vtkDataSet *grid,
      vtkAlgorithm *alg)
{
  double progInc=0.70/step->GetNumberOfImages();
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

  return 1;
}

//-----------------------------------------------------------------------------
int BOVReader::ReadMetaTimeStep(int stepIdx, vtkDataSet *grid, vtkAlgorithm *alg)
{
  if (!(this->MetaData && this->MetaData->IsDatasetOpen()))
    {
    sqErrorMacro(cerr,"Cannot read because the dataset is not open.");
    return 0;
    }

  if (grid==NULL)
    {
    sqErrorMacro(cerr,"Empty output.");
    return 0;
    }

  ostringstream seriesExt;
  seriesExt << "_" << stepIdx << "." << this->MetaData->GetBrickFileExtension();
  // In a meta read we won't read the arrays, rather we'll put a place holder
  // for each in the output. Downstream array can then be selected.
  const size_t nPoints=grid->GetNumberOfPoints();
  const size_t nArrays=this->MetaData->GetNumberOfArrays();
  double progInc=0.75/nArrays;
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
      sqErrorMacro(cerr,"Bad array type for array " << arrayName << ".");
      }
    grid->GetPointData()->AddArray(array);
    array->Delete();

    prog+=progInc;
    if(alg)alg->UpdateProgress(prog);
    }

  return 1;
}

//-----------------------------------------------------------------------------
void BOVReader::PrintSelf(ostream &os)
{
  os
    << "BOVReader: " << this << endl
    << "  Comm: " << this->Comm << endl
    << "  NGhost: " << this->NGhost << endl
    << "  ProcId: " << this->ProcId << endl
    << "  NProcs: " << this->NProcs << endl;

  if (this->Hints!=MPI_INFO_NULL)
    {
    os << "  Hints:" << endl;
    int nKeys=0;
    MPI_Info_get_nkeys(this->Hints,&nKeys);
    for (int i=0; i<nKeys; ++i)
      {
      char key[256];
      char val[256];
      int flag=0;
      MPI_Info_get_nthkey(this->Hints,i,key);
      MPI_Info_get(this->Hints,key,256,val,&flag);
      os << "    " << key << "=" << val << endl;
      }
    }

  this->MetaData->Print(os);
}
