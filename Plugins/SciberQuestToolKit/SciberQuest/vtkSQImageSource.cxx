/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "vtkSQImageSource.h"

#include "CartesianExtent.h"
#include "postream.h"
#include "Numerics.hxx"
#include "GhostTransaction.h"
#include "vtkSQLog.h"

#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
typedef vtkStreamingDemandDrivenPipeline vtkSDDPipeline;

#include "vtkImageData.h"
#include "vtkRectilinearGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkPVXMLElement.h"
#include <string>

// #define SQTK_DEBUG
// #define vtkSQImageSourceTIME



vtkStandardNewMacro(vtkSQImageSource);

//-----------------------------------------------------------------------------
vtkSQImageSource::vtkSQImageSource()
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQImageSource::vtkSQImageSource" << std::endl;
  #endif

  this->Extent[0]=0;
  this->Extent[1]=1;
  this->Extent[2]=0;
  this->Extent[3]=1;
  this->Extent[4]=0;
  this->Extent[5]=1;

  this->Origin[0]=0.0;
  this->Origin[1]=0.0;
  this->Origin[2]=0.0;

  this->Spacing[0]=1.0;
  this->Spacing[1]=1.0;
  this->Spacing[2]=1.0;

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkSQImageSource::~vtkSQImageSource()
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQImageSource::~vtkSQImageSource" << std::endl;
  #endif
}

//-----------------------------------------------------------------------------
int vtkSQImageSource::Initialize(vtkPVXMLElement *root)
{
  #if defined vtkSQImageSourceTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  *log
    << "# ::vtkSQImageSource" << "\n"
    << "\n";
  #endif
  (void)root;
  vtkErrorMacro("Initialize not yet implemented!!!");
  return 0;
}

//-----------------------------------------------------------------------------
void vtkSQImageSource::SetIExtent(int ilo, int ihi)
{
  int *s=this->Extent;
  this->SetExtent(ilo,ihi,s[2],s[3],s[4],s[5]);
}

//-----------------------------------------------------------------------------
void vtkSQImageSource::SetJExtent(int jlo, int jhi)
{
  int *s=this->Extent;
  this->SetExtent(s[0],s[1],jlo,jhi,s[4],s[5]);
}

//-----------------------------------------------------------------------------
void vtkSQImageSource::SetKExtent(int klo, int khi)
{
  int *s=this->Extent;
  this->SetExtent(s[0],s[1],s[2],s[3],klo,khi);
}

//-----------------------------------------------------------------------------
int vtkSQImageSource::RequestInformation(
      vtkInformation *req,
      vtkInformationVector **inInfos,
      vtkInformationVector *outInfos)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQImageSource::RequestInformation" << std::endl;
  #endif

  (void)req;
  (void)inInfos;

  vtkInformation* outInfo=outInfos->GetInformationObject(0);

  outInfo->Set(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        this->Extent,
        6);

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQImageSource::RequestData(
    vtkInformation *req,
    vtkInformationVector **inInfoVec,
    vtkInformationVector *outInfoVec)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQImageSource::RequestData" << std::endl;
  #endif
  #if defined vtkSQImageSourceTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->StartEvent("vtkSQImageSource::RequestData");
  #endif

  (void)req;
  (void)inInfoVec;

  vtkInformation *outInfo=outInfoVec->GetInformationObject(0);
  vtkDataSet *outData
    = dynamic_cast<vtkDataSet*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Guard against empty input.
  vtkImageData *outIm = dynamic_cast<vtkImageData*>(outData);
  if (!outIm)
    {
    vtkErrorMacro("Empty output detected.");
    return 1;
    }

  // build an empty image data with the requested  origin, spacing
  // and extents.
  int updateExtent[6];
  outInfo->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        updateExtent);

  outIm->SetExtent(updateExtent);
  outIm->SetOrigin(this->Origin);
  outIm->SetSpacing(this->Spacing);

  #if defined vtkSQImageSourceTIME
  log->EndEvent("vtkSQImageSource::RequestData");
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSQImageSource::PrintSelf(ostream& os, vtkIndent indent)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQImageSource::PrintSelf" << std::endl;
  #endif

  this->Superclass::PrintSelf(os,indent);
}
