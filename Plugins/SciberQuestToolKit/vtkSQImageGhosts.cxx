/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.

*/
#include "vtkSQImageGhosts.h"

#include "CartesianExtent.h"
#include "postream.h"
#include "Numerics.hxx"
#include "GhostTransaction.h"

#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
typedef vtkStreamingDemandDrivenPipeline vtkSDDPipeline;

#include "vtkImageData.h"
#include "vtkRectilinearGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPVXMLElement.h"
#include <string>
using std::string;

// #define vtkSQImageGhostsDEBUG
// #define vtkSQImageGhostsTIME

#if defined vtkSQImageGhostsTIME
  #include "vtkSQLog.h"
#endif

vtkStandardNewMacro(vtkSQImageGhosts);

//-----------------------------------------------------------------------------
vtkSQImageGhosts::vtkSQImageGhosts()
    :
  WorldSize(1),
  WorldRank(0),
  NGhosts(0),
  Mode(CartesianExtent::DIM_MODE_3D),
  CopyAllArrays(1)
{
  #ifdef vtkSQImageGhostsDEBUG
  pCerr() << "=====vtkSQImageGhosts::vtkSQImageGhosts" << endl;
  #endif

  #ifdef SQTK_WITHOUT_MPI
  this->Comm=0;
  #else
  this->Comm=MPI_COMM_NULL;
  this->SetCommunicator(MPI_COMM_WORLD);
  #endif

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkSQImageGhosts::~vtkSQImageGhosts()
{
  #ifdef vtkSQImageGhostsDEBUG
  pCerr() << "=====vtkSQImageGhosts::~vtkSQImageGhosts" << endl;
  #endif

  #ifndef SQTK_WITHOUT_MPI
  this->SetCommunicator(MPI_COMM_NULL);
  #endif
}

//-----------------------------------------------------------------------------
int vtkSQImageGhosts::Initialize(vtkPVXMLElement *root)
{
  #if defined vtkSQImageGhostsTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  *log
    << "# ::vtkSQImageGhosts" << "\n"
    << "\n";
  #endif
  return 0;
}

//-----------------------------------------------------------------------------
void vtkSQImageGhosts::SetCommunicator(MPI_Comm comm)
{
  if (this->Comm==comm) return;

  #ifndef SQTK_WITHOUT_MPI
  int mpiOk=0;
  MPI_Initialized(&mpiOk);
  if (mpiOk)
    {
    if ((this->Comm!=comm)
      && (this->Comm!=MPI_COMM_NULL)
      && (this->Comm!=MPI_COMM_SELF))
      {
      MPI_Comm_free(&this->Comm);
      }

    if ((comm==MPI_COMM_NULL)
      ||(comm==MPI_COMM_SELF))
      {
      this->Comm=comm;
      this->WorldSize=1;
      this->WorldRank=0;
      }
    else
      {
      MPI_Comm_dup(comm,&this->Comm);
      MPI_Comm_rank(this->Comm,&this->WorldRank);
      MPI_Comm_size(this->Comm,&this->WorldSize);
      }
    }
  else
  #endif
    {
    // no mpi or not a parallel run
    this->Comm=comm;
    this->WorldSize=1;
    this->WorldRank=0;
    }
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSQImageGhosts::AddArrayToCopy(const char *name)
{
  #ifdef vtkSQImageGhostsDEBUG
  pCerr()
    << "=====vtkSQImageGhosts::ArraysToCopy" << endl
    << "name=" << name << endl;
  #endif

  if (this->ArraysToCopy.insert(name).second)
    {
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
void vtkSQImageGhosts::ClearArraysToCopy()
{
  #ifdef vtkSQImageGhostsDEBUG
  pCerr() << "=====vtkSQImageGhosts::ClearArraysToCopy" << endl;
  #endif

  if (this->ArraysToCopy.size())
    {
    this->ArraysToCopy.clear();
    this->Modified();
    }
}
//-------------------------------------------------------------------------
int vtkSQImageGhosts::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  #ifdef vtkSQImageGhostsDEBUG
  pCerr() << "=====vtkSQImageGhosts::RequestUpdateExtent" << endl;
  #endif

  // We require preceding filters to refrain from creating ghost cells.
  // In the case of structured extents we have to know the number of
  // of ghosts requested so that we can shrink the update extents.

  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int piece
    = outInfo->Get(vtkSDDPipeline::UPDATE_PIECE_NUMBER());

  int numPieces
  = outInfo->Get(vtkSDDPipeline::UPDATE_NUMBER_OF_PIECES());

  CartesianExtent updateExt;
  outInfo->Get(vtkSDDPipeline::UPDATE_EXTENT(),updateExt.GetData());


  // gather metadata here for the pending execution.
  this->NGhosts
    = outInfo->Get(vtkSDDPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  this->SetMode(
        CartesianExtent::GetDimensionMode(
              this->ProblemDomain,
              this->NGhosts));

  // shirnk the requested extent so that the reader doesn't run
  // again.
  updateExt
     = CartesianExtent::Shrink(
          updateExt,
          this->ProblemDomain,
          this->NGhosts,
          this->Mode);

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  inInfo->Set(vtkSDDPipeline::UPDATE_EXTENT(),updateExt.GetData(),6);
  inInfo->Set(vtkSDDPipeline::UPDATE_PIECE_NUMBER(), piece);
  inInfo->Set(vtkSDDPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
  inInfo->Set(vtkSDDPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  inInfo->Set(vtkSDDPipeline::EXACT_EXTENT(), 1);

  #ifdef vtkSQImageGhostsDEBUG
  pCerr()
    << "WHOLE_EXTENT=" << this->ProblemDomain << endl
    << "UPDATE_EXTENT=" << updateExt << endl
    << "Mode=" << this->Mode << endl
    << "NGhosts=" << this->NGhosts << endl;
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQImageGhosts::RequestInformation(
      vtkInformation * /*req*/,
      vtkInformationVector **inInfos,
      vtkInformationVector *outInfos)
{
  #ifdef vtkSQImageGhostsDEBUG
  pCerr() << "=====vtkSQImageGhosts::RequestInformation" << endl;
  #endif
  //this->Superclass::RequestInformation(req,inInfos,outInfos);

  vtkInformation* outInfo=outInfos->GetInformationObject(0);
  vtkInformation *inInfo=inInfos[0]->GetInformationObject(0);

  // The problem domain is unchanged. We don't provide ghosts
  // outside of the problem domain, rather we expect downstream
  // filters to shrink the problem domain and use valid cells.
  inInfo->Get(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        this->ProblemDomain.GetData());

  outInfo->Set(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        this->ProblemDomain.GetData(),
        6);

  CartesianExtent origExt;
  outInfo->Get(vtkSDDPipeline::UPDATE_EXTENT(),origExt.GetData());


  #ifdef vtkSQImageGhostsDEBUG
  pCerr()
    << "NGhosts=" << this->NGhosts << endl
    << "Mode=" << this->Mode << endl
    << "WHOLE_EXTENT(intput)=" << this->ProblemDomain << endl
    << "UPDATE_EXTENT(intput)=" << origExt << endl;
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQImageGhosts::RequestData(
    vtkInformation * /*req*/,
    vtkInformationVector **inInfoVec,
    vtkInformationVector *outInfoVec)
{
  #ifdef vtkSQImageGhostsDEBUG
  pCerr() << "=====vtkSQImageGhosts::RequestData" << endl;
  #endif
  #if defined vtkSQImageGhostsTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->StartEvent("vtkSQImageGhosts::RequestData");
  #endif

  vtkInformation *inInfo=inInfoVec[0]->GetInformationObject(0);
  vtkDataSet *inData
    = dynamic_cast<vtkDataSet*>(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation *outInfo=outInfoVec->GetInformationObject(0);
  vtkDataSet *outData
    = dynamic_cast<vtkDataSet*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  outInfo->Set(
        vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(),
        this->NGhosts);

  // Guard against empty input.
  if (!inData || !outData)
    {
    vtkErrorMacro(
      << "Empty input(" << inData << ") or output(" << outData << ") detected.");
    return 1;
    }

  // We need extent based data here.
  int isImage=inData->IsA("vtkImageData");
  int isRecti=0;//inData->IsA("vtkrectilinearGrid");
  if (!isImage && !isRecti)
    {
    vtkErrorMacro(
      << "This filter is designed for vtkImageData and subclasses."
      << "You are trying to use it with " << inData->GetClassName() << ".");
    return 1;
    }

  vtkImageData *outIm = dynamic_cast<vtkImageData*>(outData);

  // Get the input and output extents.
  CartesianExtent inPoints;
  inInfo->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        inPoints.GetData());

  CartesianExtent outPoints
     = CartesianExtent::Grow(
          inPoints,
          this->ProblemDomain,
          this->NGhosts,
          this->Mode);

  outInfo->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        outPoints.GetData(),
        6);

  outIm->SetExtent(outPoints.GetData());

  // work with cells, not points.
  CartesianExtent inCells
     = CartesianExtent::NodeToCell(inPoints,this->Mode);

  CartesianExtent outCells
     = CartesianExtent::NodeToCell(outPoints,this->Mode);

  CartesianExtent domainCells
     = CartesianExtent::NodeToCell(this->ProblemDomain,this->Mode);

  // construct the set of transactions needed to transfer the
  // ghost data from remote processes. if not a parallel build
  // or run then we need not do this
  vector<GhostTransaction> transactions;
  #ifndef SQTK_WITHOUT_MPI
  if (this->WorldSize>1)
    {
    // gather input extents
    vector<CartesianExtent> inputExts(this->WorldSize);
    int *buffer=new int [6*this->WorldSize];
    MPI_Allgather(
          inCells.GetData(),
          6,
          MPI_INT,
          buffer,
          6,
          MPI_INT,
          this->Comm);

    for (int i=0; i<this->WorldSize; ++i)
      {
      inputExts[i].Set(buffer+6*i);
      }
    delete [] buffer;

    // set up transactions
    int id=0;
    for (int i=0; i<this->WorldSize; ++i)
      {
      CartesianExtent destExt
          = CartesianExtent::Grow(
                inputExts[i],
                domainCells,
                this->NGhosts,
                this->Mode);

      for (int j=0; j<this->WorldSize; ++j, ++id)
        {
        if (i==j) continue;

        CartesianExtent srcExt = inputExts[j];
        CartesianExtent intExt = srcExt;
        intExt &= destExt;

        if (!intExt.Empty())
          {
          transactions.push_back(
                GhostTransaction(
                      j,
                      srcExt,
                      i,
                      destExt,
                      intExt,
                      id));
          }
        }
      }
    }
  #endif

  // apply transactions to point data arrays
  vtkDataSetAttributes *inPd
     = static_cast<vtkDataSetAttributes*>(inData->GetPointData());

  vtkDataSetAttributes *outPd
     = static_cast<vtkDataSetAttributes*>(outData->GetPointData());

  this->ExecuteTransactions(
        inPd,
        outPd,
        inPoints,
        outPoints,
        transactions,
        true);

  // apply transactions to cell data arrays
  vtkDataSetAttributes *inCd
    = static_cast<vtkDataSetAttributes *>(inData->GetCellData());

  vtkDataSetAttributes *outCd
    = static_cast<vtkDataSetAttributes*>(outData->GetCellData());

  this->ExecuteTransactions(
        inCd,
        outCd,
        inCells,
        outCells,
        transactions,
        false);

  #if defined vtkSQImageGhostsTIME
  log->EndEvent("vtkSQImageGhosts::RequestData");
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSQImageGhosts::ExecuteTransactions(
      vtkDataSetAttributes *inputDsa,
      vtkDataSetAttributes *outputDsa,
      CartesianExtent inputExt,
      CartesianExtent outputExt,
      vector<GhostTransaction> &transactions,
      bool pointData)
{

  size_t nOutputTups = outputExt.Size();

  // build up the output dataset attributes
  if (this->CopyAllArrays)
    {
    // all arrays
    int nArrays=inputDsa->GetNumberOfArrays();
    for (int i=0; i<nArrays; ++i)
      {
      vtkDataArray *inArray=inputDsa->GetArray(i);
      int nComps = inArray->GetNumberOfComponents();

      vtkDataArray *outArray = inArray->NewInstance();
      outArray->SetName(inArray->GetName());
      outArray->SetNumberOfComponents(nComps);
      outArray->SetNumberOfTuples(nOutputTups);
      outputDsa->AddArray(outArray);
      outArray->Delete();
      }
    }
  else
    {
    // only selected arrays
    set<string>::iterator it,begin,end;
    it=begin=this->ArraysToCopy.begin();
    end=this->ArraysToCopy.end();
    for (;it!=end; ++it)
      {
      vtkDataArray *inArray=inputDsa->GetArray((*it).c_str());
      if (inArray)
        {
        int nComps = inArray->GetNumberOfComponents();

        vtkDataArray *outArray = inArray->NewInstance();
        outArray->SetName(inArray->GetName());
        outArray->SetNumberOfComponents(nComps);
        outArray->SetNumberOfTuples(nOutputTups);
        outputDsa->AddArray(outArray);
        outArray->Delete();
        }
      }
    }

  // copy each array found in the output dataset attributes
  int nArrays = outputDsa->GetNumberOfArrays();
  for (int i=0; i<nArrays; ++i)
    {
    vtkDataArray *outArray=outputDsa->GetArray(i);
    void *pOut = outArray->GetVoidPointer(0);

    vtkDataArray *inArray=inputDsa->GetArray(outArray->GetName());
    int nComps = inArray->GetNumberOfComponents();
    void *pIn = inArray->GetVoidPointer(0);

    #ifdef vtkSQImageGhostsDEBUG
    cerr << "Copying array " << inArray->GetName() << endl;
    #endif

    // copy the valid data directly
    switch(inArray->GetDataType())
      {
      vtkTemplateMacro(
            Copy<VTK_TT>(
              inputExt.GetData(),
              outputExt.GetData(),
              (VTK_TT*)pIn,
              (VTK_TT*)pOut,
              nComps,
              this->Mode,
              USE_INPUT_BOUNDS));
      }

    // execute the transactions to copy ghosts from remote processes
    // if not a parallel build or run then we need not do this.
    #ifndef SQTK_WITHOUT_MPI
    if (this->WorldSize>1)
      {
      static int tag=0;
      vector<MPI_Request> req;
      int nTransactions = transactions.size();
      for (int j=0; j<nTransactions; ++j, ++tag)
        {
        GhostTransaction &trans = transactions[j];

        switch(inArray->GetDataType())
          {
          vtkTemplateMacro(
              trans.Execute<VTK_TT>(
                this->Comm,
                this->WorldRank,
                nComps,
                (VTK_TT*)pIn,
                (VTK_TT*)pOut,
                pointData,
                this->Mode,
                req,
                tag));
          }
        }
      MPI_Waitall(req.size(), &req[0], MPI_STATUSES_IGNORE);
      }
    #endif
    }
}

//-----------------------------------------------------------------------------
void vtkSQImageGhosts::PrintSelf(ostream& os, vtkIndent indent)
{
  #ifdef vtkSQImageGhostsDEBUG
  pCerr() << "=====vtkSQImageGhosts::PrintSelf" << endl;
  #endif

  this->Superclass::PrintSelf(os,indent);
}
