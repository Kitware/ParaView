/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVisibilityPrioritizer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkVisibilityPrioritizer.h"

#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformationExecutivePortKey.h"
#include "vtkMath.h"
#include "vtkBoundingBox.h"
#include "vtkExtractSelectedFrustum.h"

vtkStandardNewMacro(vtkVisibilityPrioritizer);

#define DEBUGPRINT_PRIORITY(arg) ;

//----------------------------------------------------------------------------
vtkVisibilityPrioritizer::vtkVisibilityPrioritizer()
{
  this->CameraState = new double[9];
  const double caminit[9] = {0.0,0.0,-1.0, 0.0,1.0,0.0, 0.0,0.0,0.0};
  memcpy(this->CameraState, caminit, 9*sizeof(double));
  this->Frustum = new double[32];
  const double frustinit[32] = {
    0.0, 0.0, 0.0, 1.0,
    0.0, 0.0, 1.0, 1.0,
    0.0, 1.0, 0.0, 1.0,
    0.0, 1.0, 1.0, 1.0,
    1.0, 0.0, 0.0, 1.0,
    1.0, 0.0, 1.0, 1.0,
    1.0, 1.0, 0.0, 1.0,
    1.0, 1.0, 1.0, 1.0};
  memcpy(this->Frustum, frustinit, 32*sizeof(double));
  this->FrustumTester = vtkExtractSelectedFrustum::New();
  this->GetInformation()->Set(vtkAlgorithm::PRESERVES_DATASET(), 1);

  //if estimate was perfect, backfaces would be <0.0, but since dealing with curves
  //and a single normal we err on side of visibility to reduce popping artifacts
  this->BackFaceFactor = -0.75;
}

//----------------------------------------------------------------------------
vtkVisibilityPrioritizer::~vtkVisibilityPrioritizer()
{
  this->FrustumTester->Delete();
  delete[] this->CameraState;
  delete[] this->Frustum;
}

//----------------------------------------------------------------------------
void vtkVisibilityPrioritizer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkVisibilityPrioritizer::ProcessRequest(vtkInformation* request,
                                          vtkInformationVector** inputVector,
                                          vtkInformationVector* outputVector)
{
  if(request->Has(vtkStreamingDemandDrivenPipeline::
                  REQUEST_UPDATE_EXTENT_INFORMATION()))
    {
    return this->RequestUpdateExtentInformation
      (request, inputVector, outputVector);
    }
  return this->Superclass::ProcessRequest(request, inputVector,
                                          outputVector);
}

//----------------------------------------------------------------------------
int vtkVisibilityPrioritizer::RequestUpdateExtentInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  DEBUGPRINT_PRIORITY
    (
     cerr << "VS(" << this << ") RUEI" << endl;
     );

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 1;
    }

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
    {
    return 1;
    }

  //get incoming priority
  double inPriority = 1.0;
  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::PRIORITY()))
    {
    inPriority = inInfo->Get(vtkStreamingDemandDrivenPipeline::
                          PRIORITY());
    }
  DEBUGPRINT_PRIORITY
    (cerr << "VS(" << this << ") In Priority is " << inPriority << endl;);
  if (!inPriority)
    {
    return 1;
    }
  double outPriority = inPriority;

  double pbbox[6];
  vtkExecutive* executive;
  int port;
  vtkExecutive::PRODUCER()->Get(inInfo,executive,port);
  vtkStreamingDemandDrivenPipeline *sddp =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(
      executive);
  if (sddp)
    {
    //get bounding box of current piece in world coordinates

    DEBUGPRINT_PRIORITY
      (cerr << "VS(" << this << ") Asking for bounds " << endl;);
    sddp->GetPieceBoundingBox(port, pbbox);

    double viewPriority = this->CalculatePriority(pbbox);
    outPriority = inPriority * viewPriority;
    }

  outInfo->Set(vtkStreamingDemandDrivenPipeline::PRIORITY(), outPriority);

  return 1;
}

//------------------------------------------------------------------------------
double vtkVisibilityPrioritizer::CalculatePriority(double *pbbox, double *pnorm)
{
  double outPriority = 1.0;

  if (pbbox[0] <= pbbox[1] &&
      pbbox[2] <= pbbox[3] &&
      pbbox[4] <= pbbox[5])
    {
    if (pnorm != NULL)
      {
      double n = sqrt(pnorm[0]*pnorm[0] + pnorm[1]*pnorm[1] + pnorm[2]*pnorm[2]);
      pnorm[0] = pnorm[0]/n;
      pnorm[1] = pnorm[1]/n;
      pnorm[2] = pnorm[2]/n;
      double gaze[3];
      gaze[0] = this->CameraState[6] - this->CameraState[0];
      gaze[1] = this->CameraState[7] - this->CameraState[1];
      gaze[2] = this->CameraState[8] - this->CameraState[2];
      n = sqrt(gaze[0]*gaze[0] + gaze[1]*gaze[1] + gaze[2]*gaze[2]);
      gaze[0] = gaze[0]/n;
      gaze[1] = gaze[1]/n;
      gaze[2] = gaze[2]/n;
      //cerr << "gaze " << gaze[0] << "," << gaze[1] << "," << gaze[2] << endl;
      double dotprod = gaze[0]*pnorm[0] + gaze[1]*pnorm[1] + gaze[2]*pnorm[2];
      if (dotprod < this->BackFaceFactor)
        {
        //cerr << "reject" << endl;
        outPriority = 0.0;
        return outPriority;
        }
      else
        {
        //cerr << "accept" << endl;
        }
      }

    //use the frustum extraction filter to reject pieces that do not
    //intersect the view frustum
    if (!this->FrustumTester->OverallBoundsTest(pbbox))
      {
      outPriority = 0.0;
      }
    else
      {
      //for those that are not rejected, compute a priority from the bounds
      //such that pieces nearest to camera eye have highest priority 1 and
      //those furthest away have lowest 0.

      vtkBoundingBox box(pbbox);
      double nearestptonbox[3];
      for (int i=0; i<3; i++)
        {
        double p = this->CameraState[i];
        if (pbbox[i*2] > p)
          {
          p = pbbox[i*2];
          }
        if (pbbox[i*2+1] < p)
          {
          p = pbbox[i*2+1];
          }
        nearestptonbox[i] = p;
        }
      double dbox=sqrt
        (vtkMath::Distance2BetweenPoints(&this->CameraState[0],
                                         nearestptonbox));

      //ordering must only use information about current piece, so we
      //normalize by dividing by distance to far corner or frustum
      const double *farlowerleftcorner = &this->Frustum[1*4];
      double dfar=sqrt
        (vtkMath::Distance2BetweenPoints
         (&this->CameraState[0], farlowerleftcorner));

      double dist = 1.0-dbox/dfar;
      if (dist < 0.0)
        {
        DEBUGPRINT_PRIORITY
          (cerr << "VS(" << this << ") reject too far" << endl;);
        dist = 0.0;
        }
      if (dist > 1.0)
        {
        DEBUGPRINT_PRIORITY
          (cerr << "VS(" << this << ") reject too near" << endl;);
        dist = 0.0;
        }

      outPriority = dist;
      }
    }
  return outPriority;
}

//----------------------------------------------------------------------------
int vtkVisibilityPrioritizer::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  output->ShallowCopy(input);
  return 1;
}

//-----------------------------------------------------------------------------
void vtkVisibilityPrioritizer::SetFrustum(double *frustum)
{
  int i;
  for (i=0; i<32; i++)
    {
    if ( frustum[i] != this->Frustum[i] )
      {
      break;
      }
    }
  if ( i < 32 )
    {
    for (i=0; i<32; i++)
      {
      this->Frustum[i] = frustum[i];
      }
    DEBUGPRINT_PRIORITY
      (
       cerr << "FRUST" << endl;
       for (i=0; i<8; i++)
         {
         cerr << this->Frustum[i*4+0] << ","
              << this->Frustum[i*4+1] << ","
              << this->Frustum[i*4+2] << endl;
         }
       );

    this->FrustumTester->CreateFrustum(frustum);

    //No! camera changes are low priority, should NOT cause
    //reexecution of pipeline or invalidate cache filter
    //this->Modified();
    }
}

//-----------------------------------------------------------------------------
void vtkVisibilityPrioritizer::SetCameraState(double *cameraState)
{
  int i;
  for (i=0; i<9; i++)
    {
    if ( cameraState[i] != this->CameraState[i] )
      {
      break;
      }
    }
  if ( i < 9 )
    {
    for (i=0; i<9; i++)
      {
      this->CameraState[i] = cameraState[i];
      }
    DEBUGPRINT_PRIORITY
      (
       cerr
       << "EYE" << this->CameraState[0]
       << "," << this->CameraState[1]
       << "," << this->CameraState[2]
       << endl;
       );
    }
}
