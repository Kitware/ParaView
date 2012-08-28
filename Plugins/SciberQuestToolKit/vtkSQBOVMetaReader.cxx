/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "vtkSQBOVMetaReader.h"

#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkRectilinearGrid.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkMultiProcessController.h"
#include "vtkPVXMLElement.h"

#include "vtkSQOOCReader.h"
#include "vtkSQOOCBOVReader.h"
#include "BOVReader.h"
#include "GDAMetaData.h"
#include "BOVTimeStepImage.h"
#include "ImageDecomp.h"
#include "RectilinearDecomp.h"
#include "Numerics.hxx"
#include "Tuple.hxx"
#include "XMLUtils.h"
#include "PrintUtils.h"
#include "SQMacros.h"
#include "postream.h"

// #include "SQWriteStringsWarningSupression.h"
#ifndef SQTK_WITHOUT_MPI
#include "SQMPICHWarningSupression.h"
#include <mpi.h>
#endif

#include <algorithm>
using std::min;
using std::max;

#include <sstream>
using std::ostringstream;

// #define vtkSQBOVMetaReaderDEBUG
// #define vtkSQBOVMetaReaderTIME

#ifdef WIN32
  #undef vtkSQBOVMetaReaderDEBUG
#endif

#if defined vtkSQBOVMetaReaderTIME
  #include "vtkSQLog.h"
#endif

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSQBOVMetaReader);

//-----------------------------------------------------------------------------
vtkSQBOVMetaReader::vtkSQBOVMetaReader()
{
  #if defined vtkSQBOVMetaReaderDEBUG
  pCerr() << "=====vtkSQBOVMetaReader::vtkSQBOVMetaReader" << endl;
  #endif

  // Initialize pipeline.
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  // Initialize variables
  this->UseCollectiveIO=HINT_DISABLED;
  this->PeriodicBC[0]=
  this->PeriodicBC[1]=
  this->PeriodicBC[2]=0;
  this->NGhosts=1;
  this->DecompDims[0]=
  this->DecompDims[1]=
  this->DecompDims[2]=1;
  this->BlockCacheSize=10;
  this->ClearCachedBlocks=1;
  this->BlockSizeGUI=0;
  this->BlockCacheSizeGUI=0;
}

//-----------------------------------------------------------------------------
vtkSQBOVMetaReader::~vtkSQBOVMetaReader()
{
  #if defined vtkSQBOVMetaReaderDEBUG
  pCerr() << "=====vtkSQBOVMetaReader::~vtkSQBOVMetaReader" << endl;
  #endif

  this->Clear();
}

//-----------------------------------------------------------------------------
void vtkSQBOVMetaReader::Clear()
{
  this->PeriodicBC[0]=
  this->PeriodicBC[1]=
  this->PeriodicBC[2]=0;
  this->NGhosts=1;
  this->DecompDims[0]=
  this->DecompDims[1]=
  this->DecompDims[2]=1;
  this->BlockCacheSize=10;
  this->ClearCachedBlocks=1;
  this->BlockSizeGUI=0;
  this->BlockCacheSizeGUI=0;

  vtkSQBOVReaderBase::Clear();
}

//-----------------------------------------------------------------------------
int vtkSQBOVMetaReader::Initialize(
      vtkPVXMLElement *root,
      const char *fileName,
      vector<string> &arrays)
{
  #if defined vtkSQBOVReaderDEBUG
  pCerr() << "=====vtkSQBOVReader::Initialize" << endl;
  #endif

  vtkPVXMLElement *elem=GetOptionalElement(root,"vtkSQBOVMetaReader");
  if (elem==0)
    {
    return -1;
    }

  int decomp_dims[3]={0,0,0};
  GetOptionalAttribute<int,3>(elem,"decomp_dims",decomp_dims);
  if (decomp_dims[0]>0)
    {
    this->SetDecompDims(decomp_dims);
    }

  int block_cache_size=0;
  GetOptionalAttribute<int,1>(elem,"block_cache_size",&block_cache_size);
  if (block_cache_size>0)
    {
    this->SetBlockCacheSize(block_cache_size);
    }

  int periodic_bc[3]={0,0,0};
  GetOptionalAttribute<int,3>(elem,"periodic_bc",periodic_bc);
  this->SetPeriodicBC(periodic_bc);

  int n_ghosts=1;
  GetOptionalAttribute<int,1>(elem,"n_ghosts",&n_ghosts);
  if (n_ghosts>1)
    {
    this->SetNumberOfGhostCells(n_ghosts);
    }

  int clear_cache=1;
  GetOptionalAttribute<int,1>(elem,"clear_cache",&clear_cache);
  if (clear_cache==0)
    {
    this->SetClearCachedBlocks(0);
    }

  this->SetUseCollectiveIO(vtkSQBOVMetaReader::HINT_DISABLED);

  #if defined vtkSQBOVReaderTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  *log
    << "# ::vtkSQBOVMetaReader" << "\n"
    << "decomp_dims=" << Tuple<int>(decomp_dims,3) << "\n"
    << "block_cache_size=" << blok_cache_size << "\n"
    << "periodic_bc=" << Tuple<int>(periodic_bc,3) << "\n"
    << "n_ghosts=" << n_ghosts << "\n"
    << "clear_cache=" << clear_cache << "\n"
    << "\n";
  #endif

  return vtkSQBOVReaderBase::Initialize(root,fileName,arrays);
}

//-----------------------------------------------------------------------------
void vtkSQBOVMetaReader::SetPeriodicBC(int *flags)
{
  if ( (this->PeriodicBC[0]==flags[0])
    && (this->PeriodicBC[1]==flags[1])
    && (this->PeriodicBC[2]==flags[2]) )
    {
    return;
    }

  this->PeriodicBC[0]=flags[0];
  this->PeriodicBC[1]=flags[1];
  this->PeriodicBC[2]=flags[2];

  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSQBOVMetaReader::SetXHasPeriodicBC(int flag)
{
  if (this->PeriodicBC[0]==flag)
    {
    return;
    }

  this->PeriodicBC[0]=flag;

  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSQBOVMetaReader::SetYHasPeriodicBC(int flag)
{
  if (this->PeriodicBC[1]==flag)
    {
    return;
    }

  this->PeriodicBC[1]=flag;

  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSQBOVMetaReader::SetZHasPeriodicBC(int flag)
{
  if (this->PeriodicBC[2]==flag)
    {
    return;
    }

  this->PeriodicBC[2]=flag;

  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkSQBOVMetaReader::RequestInformation(
  vtkInformation *req,
  vtkInformationVector **inInfos,
  vtkInformationVector* outInfos)
{
  #if defined vtkSQBOVMetaReaderDEBUG
  pCerr() << "=====vtkSQBOVMetaReader::RequestInformation" << endl;
  #endif

  (void)req;
  (void)inInfos;

  if (!this->Reader->IsOpen())
    {
    vtkWarningMacro("No file open, cannot process RequestInformation!");
    return 1;
    }

  vtkInformation *info=outInfos->GetInformationObject(0);

  // In a meta read we need to trick the pipeline into
  // displaying the dataset bounds, in a single cell per
  // process. This keeps the memory footprint minimal.
  // We still need to provide the actual meta data for
  // use downstream, including a file name so that filters
  // may use it to read the actual data, we do so using
  // keys provided having the same name as the original.

  // Set the extent of the data.
  // The point extent given PV by the meta reader will always start from
  // 0 and range to nProcs and 1. This will neccesitate a false origin
  // and spacing as well.
  int wholeExtent[6]={0,this->WorldSize,0,1,0,1};

  info->Set(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        wholeExtent,
        6);

  // req->Append(
  //     vtkExecutive::KEYS_TO_COPY(),
  //     vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

  if (this->Reader->DataSetTypeIsImage())
    {
    // Adjust PV's keys for the false subsetting extents.
    double X0[3];
    this->Reader->GetMetaData()->GetOrigin(X0);

    double dX[3];
    this->Reader->GetMetaData()->GetSpacing(dX);

    X0[0]=X0[0]+this->Subset[0]*dX[0];
    X0[1]=X0[1]+this->Subset[2]*dX[1];
    X0[2]=X0[2]+this->Subset[4]*dX[2];

    // Adjust grid spacing for our single cell per process. We are using the dual
    // grid so we are subtracting 1 to get the number of cells.
    int nCells[3]={
      this->Subset[1]-this->Subset[0],
      this->Subset[3]-this->Subset[2],
      this->Subset[5]-this->Subset[4]};

    dX[0]=dX[0]*((double)nCells[0])/((double)this->WorldSize);
    dX[1]=dX[1]*((double)nCells[1]);
    dX[2]=dX[2]*((double)nCells[2]);

    // Pass values into the pipeline.
    info->Set(vtkDataObject::ORIGIN(),X0,3);
    // req->Append(vtkExecutive::KEYS_TO_COPY(),vtkDataObject::ORIGIN());

    info->Set(vtkDataObject::SPACING(),dX,3);
    // req->Append(vtkExecutive::KEYS_TO_COPY(),vtkDataObject::SPACING());
    }

  return vtkSQBOVReaderBase::RequestInformation(req,inInfos,outInfos);
}

//-----------------------------------------------------------------------------
int vtkSQBOVMetaReader::RequestData(
        vtkInformation *req,
        vtkInformationVector **inInfos,
        vtkInformationVector *outInfos)
{
  #if defined vtkSQBOVMetaReaderDEBUG
  pCerr() << "=====vtkSQBOVMetaReader::RequestData" << endl;
  #endif
  #if defined vtkSQBOVMetaReaderTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->StartEvent("vtkSQBOVMetaReader::RequestData");
  #endif

  (void)inInfos;

  vtkInformation *info=outInfos->GetInformationObject(0);

  // Get the output dataset.
  vtkDataSet *output
    = dynamic_cast<vtkDataSet *>(info->Get(vtkDataObject::DATA_OBJECT()));
  if (output==NULL)
    {
    vtkErrorMacro("Filter data has not been configured correctly.");
    return 1;
    }

  // get the step id of requested time.
  int stepId=this->GetTimeStepId(info,output->GetInformation());

  // The subset is the what the user selected in the GUI. This is what will
  // be loaded in aggregate across the entire run.
  BOVMetaData *md=this->Reader->GetMetaData();
  CartesianExtent subset=md->GetSubset();

  #if defined vtkSQBOVMetaReaderDEBUG
  if (this->WorldRank==0)
    {
    pCerr()
      << "subset=" << subset
      << " size=" << subset.Size()*sizeof(float)
      << endl;
    }
  #endif
  // shift to the dual grid
  subset.NodeToCell();

  // this is a hack to accomodate 2D grids.
  for (int q=0; q<3; ++q)
    {
    int qq=2*q;
    if (subset[qq+1]<subset[qq])
      {
      subset[qq+1]=subset[qq];
      }
    }

  // ParaView sends the update extent to inform us of the domain decomposition.
  // The decomp is what will be loaded by this process.
  CartesianExtent decomp;
  //int decomp[6];
  info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),decomp.GetData());

  // Set the region to be read.
  md->SetDecomp(decomp);

  #if defined vtkSQBOVMetaReaderDEBUG
  pCerr() << "decomp=" << decomp << endl;
  #endif

  // Construct MPI File hints for the reader.
  this->SetMPIFileHints();


  // pass the boundary condition flags
  info->Set(vtkSQOOCReader::PERIODIC_BC(),this->PeriodicBC,3);
  req->Append(vtkExecutive::KEYS_TO_COPY(), vtkSQOOCReader::PERIODIC_BC());

  CartesianDecomp *ddecomp;

  // The file extents describe the data as it is on the disk.
  CartesianExtent fileExt=md->GetDomain();

  // shift to dual grid
  fileExt.NodeToCell();

  // this is a hack to accomodate 2D grids.
  for (int q=0; q<3; ++q)
    {
    int qq=2*q;
    if (fileExt[qq+1]<fileExt[qq])
      {
      fileExt[qq+1]=fileExt[qq];
      }
    }

  // This reader can read vtkImageData, vtkRectilinearGrid
  // and vtkStructuredData

  if (this->Reader->DataSetTypeIsImage())
    {
    /// Image data

    // Pull origin and spacing we stored during RequestInformation pass.
    double dX[3];
    info->Get(vtkDataObject::SPACING(),dX);
    double X0[3];
    info->Get(vtkDataObject::ORIGIN(),X0);

    // dimensions of the dummy output
    int nPoints[3];
    nPoints[0]=this->WorldSize+1;
    nPoints[1]=2;
    nPoints[2]=2;

    // Configure the output.
    vtkImageData *idds=dynamic_cast<vtkImageData*>(output);
    idds->SetDimensions(nPoints);
    idds->SetOrigin(X0);
    idds->SetSpacing(dX);
    idds->SetExtent(decomp.GetData());

    // Store the bounds of the requested subset.
    double subsetBounds[6];
    subsetBounds[0]=X0[0];
    subsetBounds[1]=X0[0]+dX[0]*((double)this->WorldSize);
    subsetBounds[2]=X0[1];
    subsetBounds[3]=X0[1]+dX[1];
    subsetBounds[4]=X0[2];
    subsetBounds[5]=X0[2]+dX[2];
    info->Set(vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX(),subsetBounds,6);
    req->Append(
        vtkExecutive::KEYS_TO_COPY(),
        vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX());

    // Setup the user defined domain decomposition over the subset. This
    // decomposition is used to fine tune the I/O performance of out-of-core
    // filters.
    double *subsetX0=md->GetOrigin();
    double *subsetDX=md->GetSpacing();

    ImageDecomp *iddecomp=ImageDecomp::New();
    iddecomp->SetFileExtent(fileExt);
    iddecomp->SetExtent(subset);
    iddecomp->SetOrigin(subsetX0);
    iddecomp->SetSpacing(subsetDX);
    iddecomp->SetNumberOfGhostCells(this->NGhosts);
    iddecomp->ComputeDimensionMode();
    iddecomp->ComputeBounds();
    iddecomp->SetDecompDims(this->DecompDims);
    iddecomp->SetPeriodicBC(this->PeriodicBC);
    int ok=iddecomp->DecomposeDomain();
    if (!ok)
      {
      vtkErrorMacro("Failed to decompose domain.");
      output->Initialize();
      return 1;
      }
    ddecomp=iddecomp;
    }
  else
  if (this->Reader->DataSetTypeIsRectilinear())
    {
    /// Rectilinear grid
    // Store the bounds of the requested subset.
    double subsetBounds[6]={
      md->GetCoordinate(0)->GetPointer()[subset[0]],
      md->GetCoordinate(0)->GetPointer()[subset[1]+1],
      md->GetCoordinate(1)->GetPointer()[subset[2]],
      md->GetCoordinate(1)->GetPointer()[subset[3]+1],
      md->GetCoordinate(2)->GetPointer()[subset[4]],
      md->GetCoordinate(2)->GetPointer()[subset[5]+1]};
    info->Set(vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX(),subsetBounds,6);
    req->Append(
        vtkExecutive::KEYS_TO_COPY(),
        vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX());

    // Store the bounds of the requested subset.
    int nCells[3];
    subset.Size(nCells);

    int nLocal=nCells[0]/this->WorldSize;
    int nLarge=nCells[0]%this->WorldSize;

    int ilo;
    int ihi;

    if (this->WorldRank<nLarge)
      {
      ilo=subset[0]+this->WorldRank*(nLocal+1);
      ihi=ilo+nLocal+1;
      }
    else
      {
      ilo=subset[0]+this->WorldRank*nLocal+nLarge;
      ihi=ilo+nLocal;
      }

    // Configure the output.
    vtkRectilinearGrid *rgds=dynamic_cast<vtkRectilinearGrid*>(output);

    vtkFloatArray *fa;
    float *pFa;
    fa=vtkFloatArray::New();
    fa->SetNumberOfTuples(2);
    pFa=fa->GetPointer(0);
    pFa[0]=md->GetCoordinate(0)->GetPointer()[ilo];
    pFa[1]=md->GetCoordinate(0)->GetPointer()[ihi];
    rgds->SetXCoordinates(fa);
    fa->Delete();

    fa=vtkFloatArray::New();
    fa->SetNumberOfTuples(2);
    pFa=fa->GetPointer(0);
    pFa[0]=((float)subsetBounds[2]);
    pFa[1]=((float)subsetBounds[3]);
    rgds->SetYCoordinates(fa);
    fa->Delete();

    fa=vtkFloatArray::New();
    fa->SetNumberOfTuples(2);
    pFa=fa->GetPointer(0);
    pFa[0]=((float)subsetBounds[4]);
    pFa[1]=((float)subsetBounds[5]);
    rgds->SetZCoordinates(fa);
    fa->Delete();

    rgds->SetExtent(decomp.GetData());

    // Setup the user defined domain decomposition over the subset. This
    // decomposition is used to fine tune the I/O performance of out-of-core
    // filters.
    RectilinearDecomp *rddecomp=RectilinearDecomp::New();
    rddecomp->SetFileExtent(fileExt);
    rddecomp->SetExtent(subset);
    rddecomp->SetDecompDims(this->DecompDims);
    rddecomp->SetPeriodicBC(this->PeriodicBC);
    rddecomp->SetNumberOfGhostCells(this->NGhosts);
    rddecomp->SetCoordinate(0,md->GetCoordinate(0));
    rddecomp->SetCoordinate(1,md->GetCoordinate(1));
    rddecomp->SetCoordinate(2,md->GetCoordinate(2));
    int ok=rddecomp->DecomposeDomain();
    if (!ok)
      {
      vtkErrorMacro("Failed to decompose domain.");
      output->Initialize();
      return 1;
      }
    ddecomp=rddecomp;
    }
  else
  if (this->Reader->DataSetTypeIsStructured())
    {
    /// Structured data
    vtkErrorMacro("vtkStructuredData is not implemented yet.");
    return 1;
    }
  else
    {
    /// unrecognized dataset type
    vtkErrorMacro(
      << "Error: invalid dataset type \""
      << md->GetDataSetType() << "\".");
    }

  // Pseduo read puts place holders for the selected arrays into
  // the output so they can be selected in downstream filters' GUIs.
  int ok;
  ok=this->Reader->ReadMetaTimeStep(stepId,output,this);
  if (!ok)
    {
    vtkErrorMacro("Read failed." << endl << *md);
    output->Initialize();
    return 1;
    }

  // Put a reader into the pipeline, downstream filters can
  // the read on demand.
  vtkSQOOCBOVReader *OOCReader=vtkSQOOCBOVReader::New();
  OOCReader->SetReader(this->Reader);
  OOCReader->SetTimeIndex(stepId);
  OOCReader->SetDomainDecomp(ddecomp);
  OOCReader->SetBlockCacheSize(this->BlockCacheSize);
  OOCReader->SetCloseClearsCachedBlocks(this->ClearCachedBlocks);
  OOCReader->InitializeBlockCache();
  info->Set(vtkSQOOCReader::READER(),OOCReader);
  OOCReader->Delete();
  ddecomp->Delete();
  req->Append(vtkExecutive::KEYS_TO_COPY(),vtkSQOOCReader::READER());

  // Give implementation classes a chance to store specialized keys
  // into the pipeline.
  md->PushPipelineInformation(req, info);

  #if defined vtkSQBOVMetaReaderDEBUG
  this->Reader->PrintSelf(pCerr());
  output->Print(pCerr());
  #endif

  #if defined vtkSQBOVMetaReaderTIME
  log->EndEvent("vtkSQBOVMetaReader::RequestData");
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSQBOVMetaReader::PrintSelf(ostream& os, vtkIndent indent)
{
  os << "vtkSQBOVMetaReader" << endl;
  // TODO
  this->Superclass::PrintSelf(os,indent);
  os << endl;
}
