/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamedSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStreamedMandelbrot.h"

#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkExtentTranslator.h"
#include "vtkGridSampler1.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkImageData.h"
#include "vtkMetaInfoDatabase.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#define DEBUGPRINT(arg) ;
#define DEBUGPRINT_RESOLUTION(arg) ;
#define DEBUGPRINT_METAINFORMATION(arg) ;

vtkStandardNewMacro(vtkStreamedMandelbrot);

//----------------------------------------------------------------------------
vtkStreamedMandelbrot::vtkStreamedMandelbrot()
{
  this->GridSampler = vtkGridSampler1::New();
  this->RangeKeeper = vtkMetaInfoDatabase::New();
  this->Resolution = 1.0;
  this->SI = 1;
  this->SJ = 1;
  this->SK = 1;
}

//----------------------------------------------------------------------------
vtkStreamedMandelbrot::~vtkStreamedMandelbrot()
{
  this->GridSampler->Delete();
  this->RangeKeeper->Delete();
}

//----------------------------------------------------------------------------
void vtkStreamedMandelbrot::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
int vtkStreamedMandelbrot::RequestInformation (
  vtkInformation *request,
  vtkInformationVector** inputVector,
  vtkInformationVector *outputVector)
{
  int ret =
    this->Superclass::RequestInformation(request, inputVector, outputVector);

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  double *Spacing;
  Spacing = outInfo->Get(vtkDataObject::SPACING());

  int *wholeExtent;
  wholeExtent = outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

  int sWholeExtent[6];
  sWholeExtent[0] = wholeExtent[0];
  sWholeExtent[1] = wholeExtent[1];
  sWholeExtent[2] = wholeExtent[2];
  sWholeExtent[3] = wholeExtent[3];
  sWholeExtent[4] = wholeExtent[4];
  sWholeExtent[5] = wholeExtent[5];

  double sSpacing[3];
  sSpacing[0] = Spacing[0];
  sSpacing[1] = Spacing[1];
  sSpacing[2] = Spacing[2];

  this->Resolution = 1.0;

  DEBUGPRINT_RESOLUTION(
  cerr << "PRE GRID\t";
  {for (int i = 0; i < 3; i++) cerr << Spacing[i] << " ";}
  {for (int i = 0; i < 6; i++) cerr << wholeExtent[i] << " ";}
  cerr << endl;
  );

  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION()))
    {
    double rRes =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION());
    int strides[3];
    double aRes;
    int pathLen;
    int *splitPath;

    this->GridSampler->SetWholeExtent(sWholeExtent);
    vtkIntArray *ia = this->GridSampler->GetSplitPath();
    pathLen = ia->GetNumberOfTuples();
    splitPath = ia->GetPointer(0);
    DEBUGPRINT_RESOLUTION(
    cerr << "pathlen = " << pathLen << endl;
    cerr << "SP = " << splitPath << endl;
    for (int i = 0; i <40 && i < pathLen; i++)
      {
      cerr << splitPath[i] << " ";
      }
    cerr << endl;
    );
    //save split path in translator
    //vtkImageData *outData = vtkImageData::SafeDownCast(
    //  outInfo->Get(vtkDataObject::DATA_OBJECT()));
    vtkExtentTranslator *et = vtkExtentTranslator::SafeDownCast(
      outInfo->Get(vtkStreamingDemandDrivenPipeline::EXTENT_TRANSLATOR()));
    et->SetSplitPath(pathLen, splitPath);

    this->GridSampler->SetSpacing(sSpacing);
    this->GridSampler->ComputeAtResolution(rRes);

    this->GridSampler->GetStridedExtent(sWholeExtent);
    this->GridSampler->GetStridedSpacing(sSpacing);
    this->GridSampler->GetStrides(strides);
    aRes = this->GridSampler->GetStridedResolution();

    DEBUGPRINT_RESOLUTION(
    cerr << "PST GRID\t";
    {for (int i = 0; i < 3; i++) cerr << sSpacing[i] << " ";}
    {for (int i = 0; i < 6; i++) cerr << sWholeExtent[i] << " ";}
    cerr << endl;
    );

    outInfo->Set
      (vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), sWholeExtent, 6);
    outInfo->Set(vtkDataObject::SPACING(), sSpacing, 3);

    this->Resolution = aRes;
    this->SI = strides[0];
    this->SJ = strides[1];
    this->SK = strides[2];
    DEBUGPRINT_RESOLUTION(
    cerr << "RI SET STRIDE ";
    {for (int i = 0; i < 3; i++) cerr << strides[i] << " ";}
    cerr << endl;
    );
  }

  double *Origin;
  Origin = outInfo->Get(vtkDataObject::ORIGIN());

  double bounds[6];
  bounds[0] = Origin[0] + sSpacing[0] * sWholeExtent[0];
  bounds[1] = Origin[0] + sSpacing[0] * sWholeExtent[1];
  bounds[2] = Origin[1] + sSpacing[1] * sWholeExtent[2];
  bounds[3] = Origin[1] + sSpacing[1] * sWholeExtent[3];
  bounds[4] = Origin[2] + sSpacing[2] * sWholeExtent[4];
  bounds[5] = Origin[2] + sSpacing[2] * sWholeExtent[5];
  DEBUGPRINT_RESOLUTION(
  cerr << "RI SET BOUNDS ";
  {for (int i = 0; i < 6; i++) cerr << bounds[i] << " ";}
  cerr << endl;
                        );
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX(),
               bounds, 6);

  return ret;

}

//----------------------------------------------------------------------------
int vtkStreamedMandelbrot::RequestData
(
 vtkInformation* vtkNotUsed(request),
 vtkInformationVector** vtkNotUsed(inputVector),
 vtkInformationVector* outputVector)
{
  // get the output
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *data = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION()))
    {
    this->Resolution = outInfo->Get
      (vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION());
    }

  // We need to allocate our own scalars since we are overriding
  // the superclasses "Execute()" method.
  int *ext = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  data->SetExtent(ext);
  data->AllocateScalars(outInfo);

  int a0, a1, a2;
  float *ptr;
  int min0, max0;
  int idx0, idx1, idx2;
  vtkIdType inc0, inc1, inc2;
  double *origin, *sample;
  double p[4];
  unsigned long count = 0;
  unsigned long target;

  // Name the array appropriately.
  data->GetPointData()->GetScalars()->SetName("Iterations");

  if (data->GetNumberOfPoints() <= 0)
    {
    return 1;
    }

  // Copy origin into pixel
  for (idx0 = 0; idx0 < 4; ++idx0)
    {
    p[idx0] = this->OriginCX[idx0];
    }

  ptr = static_cast<float *>(data->GetScalarPointerForExtent(ext));

  vtkDebugMacro("Generating Extent: " << ext[0] << " -> " << ext[1] << ", "
                << ext[2] << " -> " << ext[3]);

  // Get min and max of axis 0 because it is the innermost loop.
  min0 = ext[0];
  max0 = ext[1];
  data->GetContinuousIncrements(ext, inc0, inc1, inc2);

  target = static_cast<unsigned long>(
    (ext[5]-ext[4]+1)*(ext[3]-ext[2]+1)/50.0);
  target++;

  a0 = this->ProjectionAxes[0];
  a1 = this->ProjectionAxes[1];
  a2 = this->ProjectionAxes[2];
  origin = this->OriginCX;
  sample = this->SampleCX;

  if (a0<0 || a1<0 || a2<0 || a0>3 || a1>3 || a2>3)
    {
    vtkErrorMacro("Bad projection axis");
    return 0;
    }
  for (idx2 = ext[4]; idx2 <= ext[5]; ++idx2)
    {
    p[a2] = static_cast<double>(origin[a2]) +
      static_cast<double>(idx2)*(sample[a2]*this->SubsampleRate*this->SK);
    for (idx1 = ext[2]; !this->AbortExecute && idx1 <= ext[3]; ++idx1)
      {
      if (!(count%target))
        {
        this->UpdateProgress(static_cast<double>(count)/
                             (50.0*static_cast<double>(target)));
        }
      count++;
      p[a1] = static_cast<double>(origin[a1]) +
        static_cast<double>(idx1)*(sample[a1]*this->SubsampleRate*this->SJ);
      for (idx0 = min0; idx0 <= max0; ++idx0)
        {
        p[a0] = static_cast<double>(origin[a0]) +
          static_cast<double>(idx0)*(sample[a0]*this->SubsampleRate*this->SI);

        *ptr = static_cast<float>(this->EvaluateSet(p));

        ++ptr;
        // inc0 is 0
        }
      ptr += inc1;
      }
    ptr += inc2;
    }

  vtkInformation *dInfo = data->GetInformation();
  dInfo->Set(vtkDataObject::DATA_RESOLUTION(), this->Resolution);

  double range[2];
  data->GetPointData()->GetScalars()->GetRange(range);
  int P = outInfo->Get
    (vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int NP = outInfo->Get
    (vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  //double RES = outInfo->Get
  //  (vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION());
  //cerr << "RD " << P << "/" << NP << "@" << RES << endl;

  DEBUGPRINT_METAINFORMATION
    (
     cerr << "SMS(" << this << ") Calculate range "
     << range[0] << ".." << range[1] << " for "
     << P << "/" << NP << " @ " << this->Resolution << endl;
     );
  this->RangeKeeper->Insert(P, NP, ext, this->Resolution,
                            0, "Iterations", 0,
                            range);

  return 1;
}

//----------------------------------------------------------------------------
int vtkStreamedMandelbrot::ProcessRequest(vtkInformation *request,
                                      vtkInformationVector **inputVector,
                                      vtkInformationVector *outputVector)
{
  DEBUGPRINT(
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  int P = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int NP = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  double res = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION());
  cerr << "SMS(" << this << ") PR " << P << "/" << NP << "@" << res << "->"
     << this->SI << " " << this->SJ << " " << this->SK << endl;
  );

  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    DEBUGPRINT
      (cerr << "SMS(" << this << ") RDO ============================" << endl;);
    }

  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    DEBUGPRINT
      (cerr << "SMS(" << this << ") RI =============================" << endl;);
    }

  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    DEBUGPRINT
      (cerr << "SMS(" << this << ") RUE ============================" << endl;);
    }

  if(request->Has
     (vtkStreamingDemandDrivenPipeline::REQUEST_RESOLUTION_PROPAGATE()))
    {
    DEBUGPRINT_METAINFORMATION
      (cerr << "SMS(" << this << ") RRP ============================" << endl;);
    }

  if(request->Has
     (vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT_INFORMATION()))
    {
    DEBUGPRINT_METAINFORMATION(
    cerr << "SMS(" << this << ") RUEI ===============================" << endl;
    );
    //create meta information for this piece
    double *origin;
    double *spacing;
    int *ext;
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    origin = outInfo->Get(vtkDataObject::ORIGIN());
    spacing = outInfo->Get(vtkDataObject::SPACING());
    ext = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
    int P = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    int NP = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
    double bounds[6];
    bounds[0] = origin[0] + spacing[0] * ext[0];
    bounds[1] = origin[0] + spacing[0] * ext[1];
    bounds[2] = origin[1] + spacing[1] * ext[2];
    bounds[3] = origin[1] + spacing[1] * ext[3];
    bounds[4] = origin[2] + spacing[2] * ext[4];
    bounds[5] = origin[2] + spacing[2] * ext[5];
    outInfo->Set
      (vtkStreamingDemandDrivenPipeline::PIECE_BOUNDING_BOX(), bounds, 6);
    int ic = (ext[1]-ext[0]);
    if (ic < 1)
      {
      ic = 1;
      }
    int jc = (ext[3]-ext[2]);
    if (jc < 1)
      {
      jc = 1;
      }
    int kc = (ext[5]-ext[4]);
    if (kc < 1)
      {
      kc = 1;
      }
    outInfo->Set
      (vtkStreamingDemandDrivenPipeline::ORIGINAL_NUMBER_OF_CELLS(), ic*jc*kc);

/*
    cerr << P << "/" << NP << "\t";
    {for (int i = 0; i < 3; i++) cerr << spacing[i] << " ";}
    cerr << "\t";
    {for (int i = 0; i < 6; i++) cerr << ext[i] << " ";}
    cerr << "\t";
    {for (int i = 0; i < 6; i++) cerr << bounds[i] << " ";}
    cerr << endl;
*/
    vtkInformationVector *miv = outInfo->Get(vtkDataObject::POINT_DATA_VECTOR());
    vtkInformation *fInfo = miv->GetInformationObject(0);
    if (!fInfo)
      {
      fInfo = vtkInformation::New();
      miv->SetInformationObject(0, fInfo);
      fInfo->Delete();
      }
    const char *name = "Iterations";
    double range[2];
    range[0] = 0;
    range[1] = -1;
    if (this->RangeKeeper->Search(P, NP, ext,
                                  0, name, 0,
                                  range))
      {
      DEBUGPRINT_METAINFORMATION
        (
         cerr << "Range for " << name << " "
         << P << "/" << NP << " "
         << ext[0] << "," << ext[1] << ","
         << ext[2] << "," << ext[3] << ","
         << ext[4] << "," << ext[5] << " is "
         << range[0] << " .. " << range[1] << endl;
         );
      fInfo->Set(vtkDataObject::FIELD_ARRAY_NAME(), name);
      fInfo->Set(vtkDataObject::PIECE_FIELD_RANGE(), range, 2);
      }
    else
      {
      DEBUGPRINT_METAINFORMATION(
      cerr << "No range for " << name << " "
           << ext[0] << "," << ext[1] << ","
           << ext[2] << "," << ext[3] << ","
           << ext[4] << "," << ext[5] << " yet" << endl;
                                 );
      fInfo->Remove(vtkDataObject::FIELD_ARRAY_NAME());
      fInfo->Remove(vtkDataObject::PIECE_FIELD_RANGE());
      }
    }

  //This is overridden just to intercept requests for debugging purposes.
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    DEBUGPRINT
      (cerr << "SMS(" << this << ") RD =============================" << endl;);

    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    int updateExtent[6];
    int wholeExtent[6];
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      updateExtent);
    outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
      wholeExtent);
    double res = this->Resolution;
    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION()))
      {
      res = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION());
      }
    bool match = true;
    for (int i = 0; i< 6; i++)
      {
        if (updateExtent[i] != wholeExtent[i])
          {
          match = false;
          }
      }
    if (match && (res == 1.0))
      {
      vtkErrorMacro("Whole extent requested, streaming is not working.");
      }
    }

  int rc = this->Superclass::ProcessRequest(request, inputVector, outputVector);
  return rc;
}
