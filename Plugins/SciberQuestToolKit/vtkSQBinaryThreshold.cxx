/* ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "vtkSQBinaryThreshold.h"

#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkDataSet.h"
#include "vtkPointData.h"
#include "vtkDataArray.h"

#include "XMLUtils.h"
#include "SQMacros.h"

#include <string>
using std::string;

// #define vtkSQBinaryThresholdDEBUG
// #define vtkSQBinaryThresholdTIME

#if defined vtkSQBinaryThresholdTIME
  #include "vtkSQLog.h"
#endif

// ****************************************************************************
template<typename T>
void BinaryThreshold(
      T *pS,
      T *pT,
      size_t n,
      T threshold,
      T lowVal,
      T highVal,
      int useLowVal,
      int useHighVal)
{
  for (size_t i=0; i<n; ++i)
    {
    if (useLowVal && (pS[i]<threshold))
      {
      pT[i]=lowVal;
      }
    else
    if (useHighVal && (pS[i]>=threshold))
      {
      pT[i]=highVal;
      }
    }
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSQBinaryThreshold);

//-----------------------------------------------------------------------------
vtkSQBinaryThreshold::vtkSQBinaryThreshold()
{
  #if defined vtkSQBinaryThresholdDEBUG
  pCerr() << "=====vtkSQBinaryThreshold::vtkSQBinaryThreshold" << endl;
  #endif

  this->Threshold=0.0;
  this->LowValue=-1.0;
  this->HighValue=1.0;
  this->UseLowValue=1;
  this->UseHighValue=1;

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkSQBinaryThreshold::~vtkSQBinaryThreshold()
{
  #if defined vtkSQBinaryThresholdDEBUG
  pCerr() << "=====vtkSQBinaryThreshold::~vtkSQBinaryThreshold" << endl;
  #endif
}

//-----------------------------------------------------------------------------
int vtkSQBinaryThreshold::Initialize(vtkPVXMLElement *root)
{
  #if defined vtkSQBinaryThresholdDEBUG
  pCerr() << "=====vtkSQBinaryThreshold::Initialize" << endl;
  #endif

  vtkPVXMLElement *elem=0;
  elem=GetOptionalElement(root,"vtkSQBinaryThreshold");
  if (elem==0)
    {
    return -1;
    }

  #if defined vtkSQBinaryThresholdTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  *log
    << "# ::vtkSQBinaryThreshold" << "\n";
  #endif

  return 0;
}

//----------------------------------------------------------------------------
int vtkSQBinaryThreshold::RequestData(
                vtkInformation *req,
                vtkInformationVector **inInfos,
                vtkInformationVector *outInfos)
{
  #if defined vtkSQBinaryThresholdDEBUG
  pCerr() << "=====vtkSQBinaryThreshold::RequestData" << endl;
  #endif
  #if defined vtkSQBinaryThresholdTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->StartEvent("vtkSQBinaryThreshold::RequestData");
  #endif

  (void)req;

  vtkInformation *info;

  // get output
  info=outInfos->GetInformationObject(0);
  vtkDataSet *out
    = dynamic_cast<vtkDataSet*>(info->Get(vtkDataObject::DATA_OBJECT()));
  if (out==0)
    {
    vtkErrorMacro("output dataset was not present.");
    return 1;
    }

  /// get input
  info=inInfos[0]->GetInformationObject(0);
  vtkDataSet *in
    = dynamic_cast<vtkDataSet*>(info->Get(vtkDataObject::DATA_OBJECT()));
  if (in==0)
    {
    vtkErrorMacro("input dataset was not present.");
    return 1;
    }

  /// construct the output from a shallow copy
  out->ShallowCopy(in);

  /// get scalar array to process
  vtkDataArray *S=this->GetInputArrayToProcess(0,inInfos);
  if (S==0)
    {
    vtkErrorMacro("Scalar array to threshold not found.");
    }
  string SName=S->GetName();
  size_t nTups=(size_t)S->GetNumberOfTuples();

  // add the agyrotropy array to the output
  vtkDataArray *T=S->NewInstance();
  string TName;
  TName+="threshold-";
  TName+=SName;
  T->SetName(TName.c_str());
  T->SetNumberOfTuples(nTups);
  out->GetPointData()->AddArray(T);

  // compute the agyrotropy
  switch(T->GetDataType())
    {
    vtkTemplateMacro(
      BinaryThreshold(
          (VTK_TT*)S->GetVoidPointer(0),
          (VTK_TT*)T->GetVoidPointer(0),
          nTups,
          ((VTK_TT)this->Threshold),
          ((VTK_TT)this->LowValue),
          ((VTK_TT)this->HighValue),
          this->UseLowValue,
          this->UseHighValue)
      );
    default:
      vtkErrorMacro(
          << "Cannot compute threshold on type "
          << S->GetClassName());
    }

  #if defined vtkSQBinaryThresholdTIME
  log->EndEvent("vtkSQBinaryThreshold::RequestData");
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSQBinaryThreshold::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os
   << "Threshold=" << this->Threshold << endl
   << "LowValue=" << this->LowValue << endl
   << "HighValue=" << this->HighValue << endl;
}
