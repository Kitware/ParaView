/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRawStridedReader2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRawStridedReader2.h"

#include "vtkAdaptiveOptions.h"
#include "vtkByteSwap.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkExtentTranslator.h"
#include "vtkGridSampler2.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMetaInfoDatabase.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"

#include <fstream>

#ifndef _WIN32
#include <sys/mman.h>
#include <errno.h>
#endif

#include "vtkAdaptiveOptions.h"

vtkStandardNewMacro(vtkRawStridedReader2);

#define MAPSIZE (1024 * 1024 * 1024)

int vtkRawStridedReader2::Read(float* data, int* uExtents)
{
  size_t ir = uExtents[1] - uExtents[0] + 1;
  size_t jr = uExtents[3] - uExtents[2] + 1;
  size_t kr = uExtents[5] - uExtents[4] + 1;

  size_t is = ir;
  size_t ijs = ir * jr;

  size_t js = (this->sWholeExtent[1] - this->sWholeExtent[0] + 1);
  size_t ks = (this->sWholeExtent[1] - this->sWholeExtent[0] + 1) *
    (this->sWholeExtent[3] - this->sWholeExtent[2] + 1);

#ifndef _WIN32
  this->SetupMap(0);
  if(this->map != MAP_FAILED) {
    for(size_t k = 0; k < kr; k++)
      {
      for(size_t j = 0; j < jr; j++)
        {
        for(size_t i = 0; i < ir; i++)
          {
          size_t di = i + j * is + k * ijs;

          size_t index = (i + uExtents[0]) +
            (j + uExtents[2]) * js + (k + uExtents[4]) * ks;

          // damn OS X Leopard bug doesn't allow mmaps bigger than 1 GB
          // so have to chunk the damn mmap into separate 1 GB segments
          this->SetupMap(index / (MAPSIZE / sizeof(float)));
          if(this->map != MAP_FAILED)
            {
            data[di] = this->map[index % (MAPSIZE / sizeof(float))];
            }
          else
            {
            fseek(this->fp, index * sizeof(float), SEEK_SET);
            fread(&(data[di]), sizeof(float), 1, this->fp);
            }
          }
        }
      }
  }
  else
  {
#endif
    for(size_t k = 0; k < kr; k++)
      {
      for(size_t j = 0; j < jr; j++)
        {
        size_t di = j * is + k * ijs;
        fseek(this->fp,
              (uExtents[0] +
               (j + uExtents[2]) * js + (k + uExtents[4]) * ks) *
              sizeof(float), SEEK_SET);
        fread(&(data[di]), sizeof(float), ir, this->fp);
        }
      }
#ifndef _WIN32
  }
#endif

  if(this->SwapBytes)
    {
    vtkByteSwap::SwapVoidRange(data, ir * jr * kr, sizeof(float));
    }

  return 1;
}

void vtkRawStridedReader2::SetupFile() {
  int height = vtkAdaptiveOptions::GetHeight();
  int degree = vtkAdaptiveOptions::GetDegree();
  int rate = vtkAdaptiveOptions::GetRate();

  int newfile = 1;

  // figure out the index
  vtkIdType stop =
    (vtkIdType)(height * (1.0 - this->Resolution) + 0.5);

  // try to set up the file
  // there may be a better way of detecting this in VTK, yes?
  // with the modified flag... basically want to keep the
  // file and mmaps open until the filename changes
  if(this->lastname) {
    if(this->lastresolution != stop ||
       strcmp(this->lastname, this->Filename)) {
#ifndef _WIN32
      this->TearDownMap();
#endif
      this->TearDownFile();
    }
    else {
      newfile = 0;
    }
  }

  this->lastresolution = stop;

  if(newfile) {
    // copy the filename
    this->lastname = new char[strlen(this->Filename) + 255];
    if(stop > 0)
      {
      sprintf(this->lastname, "%s-%d-%d-%ds/%d",
              this->Filename, height, degree, rate, (int)stop);
      }
    else
      {
      strcpy(this->lastname, this->Filename);
      }

    // open the files
    this->fp = fopen(this->lastname, "r");

    // remember the base filename
    strcpy(this->lastname, this->Filename);

    if(!this->fp) {
      delete [] this->lastname;
      this->lastname = 0;

      return;
    }

    this->fd = fileno(this->fp);
  }
}

void vtkRawStridedReader2::TearDownFile() {
  // clean up files
  if(this->fp) {
    fclose(this->fp);
  }

  if(this->lastname) {
    delete [] this->lastname;
  }

  this->lastname = 0;
  this->fp = 0;
  this->fd = -1;
}

#ifndef _WIN32
void vtkRawStridedReader2::SetupMap(int which) {
  // make sure it's the right map
  if(which != this->chunk) {
    this->TearDownMap();

    this->chunk = which;

    // try to set up a memory map
    size_t pagesize = getpagesize();
    fseek(this->fp, 0, SEEK_END);
    size_t filesize = ftell(this->fp);
    fseek(this->fp, 0, SEEK_SET);

    filesize = filesize % pagesize > 0 ? filesize +
      + (pagesize - filesize % pagesize) : filesize;

    // damn OS X Leopard bug, it won't let mmaps bigger than 1 GB on Intel
    // so gotta chop it into 1 GB pages
    if(filesize > MAPSIZE) {
      this->mapsize = MAPSIZE;
      this->map = (float*)mmap(0, MAPSIZE, PROT_READ, MAP_SHARED,
                               fd, which * MAPSIZE);
    }
    else {
      this->mapsize = filesize;
      this->map = (float*)mmap(0, filesize, PROT_READ, MAP_SHARED,
                               fd, 0);
    }

    if(this->map == MAP_FAILED)
      {
      vtkDebugMacro
        (<< "Memory map failed: " << strerror(errno) << ".");
      this->chunk = -1;
      }
  }
}

void vtkRawStridedReader2::TearDownMap() {
  // cleanup memory maps
  if(this->map != MAP_FAILED) {
    if(munmap(this->map, this->mapsize))
      {
      vtkDebugMacro
        (<< "Memory unmap failed: " << strerror(errno) << ".");
      }
  }

  this->chunk = -1;
  this->map = (float*)MAP_FAILED;
}
#endif

//============================================================================

vtkRawStridedReader2::vtkRawStridedReader2()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->Filename = NULL;
  this->WholeExtent[0] = this->WholeExtent[2] = this->WholeExtent[4] = 0;
  this->WholeExtent[1] = this->WholeExtent[3] = this->WholeExtent[5] = 99;
  this->sWholeExtent[0] = this->sWholeExtent[2] =
    this->sWholeExtent[4] = this->WholeExtent[0];
  this->sWholeExtent[1] = this->sWholeExtent[3] =
    this->sWholeExtent[5] = this->WholeExtent[1];
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;

  this->RangeKeeper = vtkMetaInfoDatabase::New();

  this->GridSampler = vtkGridSampler2::New();
  this->Resolution = 1.0;
  this->SwapBytes = 0;

  this->fp = 0;
  this->fd = -1;
  this->lastname = 0;

#ifndef _WIN32
  this->chunk = -1;
  this->map = (float*)MAP_FAILED;
#endif
}

//----------------------------------------------------------------------------
vtkRawStridedReader2::~vtkRawStridedReader2()
{
  if (this->Filename)
    {
    delete[] this->Filename;
    }
  this->RangeKeeper->Delete();
  this->GridSampler->Delete();

#ifndef _WIN32
  this->TearDownMap();
#endif
  this->TearDownFile();
}

//----------------------------------------------------------------------------
void vtkRawStridedReader2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


//----------------------------------------------------------------------------
void vtkRawStridedReader2::SwapDataByteOrder(int i)
{
  this->SwapBytes = i;
}

//------------------------------------------------------------------------------
int vtkRawStridedReader2::CanReadFile(const char* rawfile)
{
  int height = vtkAdaptiveOptions::GetHeight();
  int degree = vtkAdaptiveOptions::GetDegree();
  int rate = vtkAdaptiveOptions::GetRate();

  int ret = 0;
  char* filename = new char[strlen(rawfile) + 255];
  sprintf(filename, "%s-%d-%d-%ds/1",
          rawfile, height, degree, rate);

  FILE *tfp = fopen(filename, "r");
  if (tfp)
    {
    ret = 1;
    fclose(tfp);
    }

  delete filename;
  return ret;
}

//----------------------------------------------------------------------------
//RequestInformation supplies global meta information
// Global Extents  (integer count range of point count in x,y,z)
// Global Origin
// Global Spacing (should be the stride value * original)

int vtkRawStridedReader2::RequestInformation
(
 vtkInformation* vtkNotUsed(request),
 vtkInformationVector** vtkNotUsed(inputVector),
 vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkDataObject::ORIGIN(), this->Origin, 3);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               this->WholeExtent, 6);

  outInfo->Set(vtkDataObject::SPACING(), this->Spacing, 3);

  vtkImageData *outData = vtkImageData::SafeDownCast
    (outInfo->Get(vtkDataObject::DATA_OBJECT()));

  this->sWholeExtent[0] = this->WholeExtent[0];
  this->sWholeExtent[1] = this->WholeExtent[1];
  this->sWholeExtent[2] = this->WholeExtent[2];
  this->sWholeExtent[3] = this->WholeExtent[3];
  this->sWholeExtent[4] = this->WholeExtent[4];
  this->sWholeExtent[5] = this->WholeExtent[5];

  this->Resolution = 1.0;

  this->sSpacing[0] = this->Spacing[0];
  this->sSpacing[1] = this->Spacing[1];
  this->sSpacing[2] = this->Spacing[2];

  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION()))
    {
    double rRes =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION());

    int pathLen;
    int *splitPath;

    // get the dimension splits
    this->GridSampler->SetWholeExtent(sWholeExtent);
    vtkIntArray *ia = this->GridSampler->GetSplitPath();
    pathLen = ia->GetNumberOfTuples();
    splitPath = ia->GetPointer(0);

    //save split path in translator
    vtkExtentTranslator *et = outData->GetExtentTranslator();
    et->SetSplitPath(pathLen, splitPath);

    // set the parameters
    this->GridSampler->SetSpacing(sSpacing);
    this->GridSampler->ComputeAtResolution(rRes);

    // get the strides
    this->GridSampler->GetStridedExtent(this->sWholeExtent);
    this->GridSampler->GetStridedSpacing(this->sSpacing);
    this->Resolution = this->GridSampler->GetStridedResolution();

    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                 this->sWholeExtent, 6);
    outInfo->Set(vtkDataObject::SPACING(), this->sSpacing, 3);
  }

  double bounds[6];
  bounds[0] = this->Origin[0] + this->sSpacing[0] * this->sWholeExtent[0];
  bounds[1] = this->Origin[0] + this->sSpacing[0] * this->sWholeExtent[1];
  bounds[2] = this->Origin[1] + this->sSpacing[1] * this->sWholeExtent[2];
  bounds[3] = this->Origin[1] + this->sSpacing[1] * this->sWholeExtent[3];
  bounds[4] = this->Origin[2] + this->sSpacing[2] * this->sWholeExtent[4];
  bounds[5] = this->Origin[2] + this->sSpacing[2] * this->sWholeExtent[5];

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX(),
               bounds, 6);

  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_FLOAT, 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkRawStridedReader2::RequestData(
    vtkInformation* vtkNotUsed(request),
    vtkInformationVector** vtkNotUsed(inputVector),
    vtkInformationVector* outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *outData = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!outData)
    {
    vtkErrorMacro(<< "Wrong output type.");
    return 0;
    }

  if (!this->Filename)
    {
    vtkErrorMacro(<< "Must specify filename.");
    return 0;
    }

  outData->Initialize();

  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION()))
    {
    this->Resolution =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION());
    }

  /*
  vtkInformation *dInfo = outData->GetInformation();
  dInfo->Set(vtkDataObject::DATA_RESOLUTION(), this->Resolution);
  */

  //prepping to produce real data and thus allocate real amounts of space
  int *uext = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  outData->SetExtent(uext);

  //TODO: We need to be able to have user definable data type
  //TODO: also multiple arrays/multiple scalarComponents
  outData->AllocateScalars();
  outData->GetPointData()->GetScalars()->SetName("point_scalars");

  // file stuff
  this->SetupFile();

  if(!this->fp)
    {
    vtkErrorMacro(<< "Could not open file " << this->Filename << ".");
    return 0;
    }

  if(!this->Read((float*)outData->GetScalarPointer(), uext))
    {
    vtkErrorMacro(<< "Read failure.");
    return 0;
    }

  double range[2];
  outData->GetPointData()->GetScalars()->GetRange(range);
  int P = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int NP = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  this->RangeKeeper->Insert(P, NP, uext, this->Resolution,
                            0, NULL, 0,
                            range);
  return 1;
}

//----------------------------------------------------------------------------
int vtkRawStridedReader2::ProcessRequest(vtkInformation *request,
                   vtkInformationVector **inputVector,
                   vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  int P = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int NP = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  //create meta information for this piece
  double* origin = outInfo->Get(vtkDataObject::ORIGIN());
  double* spacing = outInfo->Get(vtkDataObject::SPACING());
  int *ext =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());

  if(ext && origin && spacing)
    {
    double bounds[6];
    bounds[0] = origin[0] + spacing[0] * ext[0];
    bounds[1] = origin[0] + spacing[0] * ext[1];
    bounds[2] = origin[1] + spacing[1] * ext[2];
    bounds[3] = origin[1] + spacing[1] * ext[3];
    bounds[4] = origin[2] + spacing[2] * ext[4];
    bounds[5] = origin[2] + spacing[2] * ext[5];
    outInfo->Set
      (vtkStreamingDemandDrivenPipeline::PIECE_BOUNDING_BOX(), bounds, 6);
    }

  double range[2];
  if (this->RangeKeeper->Search(P, NP, ext,
                                0, NULL, 0,
                                range))
    {
    vtkInformation *fInfo =
      vtkDataObject::GetActiveFieldInformation
      (outInfo, vtkDataObject::FIELD_ASSOCIATION_POINTS,
       vtkDataSetAttributes::SCALARS);
    if (fInfo)
      {
      fInfo->Set(vtkDataObject::PIECE_FIELD_RANGE(), range, 2);
      }
    }

  int rc = this->Superclass::ProcessRequest(request, inputVector, outputVector);
  return rc;
}
