/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

#include "vtkSQLog.h"
#include "XMLUtils.h"
#include "SQMacros.h"

#include <string>

// #define SQTK_DEBUG
// #define vtkSQBinaryThresholdTIME

// ****************************************************************************
template<typename T>
void BinaryThreshold(
      T *pS,
      T *pT,
      size_t nTups,
      int nComps,
      T threshold,
      T lowVal,
      T highVal,
      int useLowVal,
      int useHighVal)
{
  for (size_t i=0; i<nTups; ++i)
    {
    T cval=T(0);         // comparison value
    T *sval=pS+i*nComps; // source value
    T *tval=pT+i*nComps; // thresholded value

    // get the comparison value
    if (nComps>1)
      {
      for (int q=0; q<nComps; ++q)
        {
        cval+=(sval[q]*sval[q]);
        }
      cval=sqrt(cval);
      }
    else
      {
      cval=sval[0];
      }

    if (useLowVal && (cval<threshold))
      {
      // reasign with low val
      for (int q=0; q<nComps; ++q)
        {
        tval[q]=lowVal;
        }
      }
    else
    if (useHighVal && (cval>=threshold))
      {
      // reasign with high val
      for (int q=0; q<nComps; ++q)
        {
        tval[q]=highVal;
        }
      }
    else
      {
      // pass through
      for (int q=0; q<nComps; ++q)
        {
        tval[q]=sval[q];
        }
      }
    }
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSQBinaryThreshold);

//-----------------------------------------------------------------------------
vtkSQBinaryThreshold::vtkSQBinaryThreshold()
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQBinaryThreshold::vtkSQBinaryThreshold" << std::endl;
  #endif

  this->Threshold=0.0;
  this->LowValue=-1.0;
  this->HighValue=1.0;
  this->UseLowValue=1;
  this->UseHighValue=1;
  this->LogLevel=0;

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkSQBinaryThreshold::~vtkSQBinaryThreshold()
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQBinaryThreshold::~vtkSQBinaryThreshold" << std::endl;
  #endif
}

//-----------------------------------------------------------------------------
int vtkSQBinaryThreshold::Initialize(vtkPVXMLElement *root)
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQBinaryThreshold::Initialize" << std::endl;
  #endif

  vtkPVXMLElement *elem=0;
  elem=GetOptionalElement(root,"vtkSQBinaryThreshold");
  if (elem==0)
    {
    return -1;
    }

  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  int globalLogLevel=log->GetGlobalLevel();
  if (this->LogLevel || globalLogLevel)
    {
    log->GetHeader()
      << "# ::vtkSQBinaryThreshold" << "\n";
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkSQBinaryThreshold::RequestData(
                vtkInformation *req,
                vtkInformationVector **inInfos,
                vtkInformationVector *outInfos)
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQBinaryThreshold::RequestData" << std::endl;
  #endif

  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  int globalLogLevel=log->GetGlobalLevel();
  if (this->LogLevel || globalLogLevel)
    {
    log->StartEvent("vtkSQBinaryThreshold::RequestData");
    }

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

  // get input
  info=inInfos[0]->GetInformationObject(0);
  vtkDataSet *in
    = dynamic_cast<vtkDataSet*>(info->Get(vtkDataObject::DATA_OBJECT()));
  if (in==0)
    {
    vtkErrorMacro("input dataset was not present.");
    return 1;
    }

  // construct the output from a shallow copy
  out->ShallowCopy(in);

  // get scalar/vector array to process
  vtkDataArray *S=this->GetInputArrayToProcess(0,inInfos);
  if (S==0)
    {
    vtkErrorMacro("Array to threshold not found.");
    }
  std::string SName=S->GetName();
  size_t nTups=(size_t)S->GetNumberOfTuples();
  int nComps=S->GetNumberOfComponents();

  // add the agyrotropy array to the output
  vtkDataArray *T=S->NewInstance();
  std::string TName;
  TName+="threshold-";
  TName+=SName;
  T->SetName(TName.c_str());
  T->SetNumberOfComponents(nComps);
  T->SetNumberOfTuples(nTups);
  out->GetPointData()->AddArray(T);
  T->Delete();

  // apply the threshold
  switch(T->GetDataType())
    {
    vtkFloatTemplateMacro(
      BinaryThreshold(
          (VTK_TT*)S->GetVoidPointer(0),
          (VTK_TT*)T->GetVoidPointer(0),
          nTups,
          nComps,
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

  if (this->LogLevel || globalLogLevel)
    {
    log->EndEvent("vtkSQBinaryThreshold::RequestData");
    }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSQBinaryThreshold::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os
   << "Threshold=" << this->Threshold << std::endl
   << "LowValue=" << this->LowValue << std::endl
   << "HighValue=" << this->HighValue << std::endl;
}
