/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkACosmoReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkACosmoReader.cxx

Copyright (c) 2007, 2009 Los Alamos National Security, LLC

All rights reserved.

Copyright 2007, 2009. Los Alamos National Security, LLC.
This software was produced under U.S. Government contract DE-AC52-06NA25396
for Los Alamos National Laboratory (LANL), which is operated by
Los Alamos National Security, LLC for the U.S. Department of Energy.
The U.S. Government has rights to use, reproduce, and distribute this software.
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.
If software is modified to produce derivative works, such modified software
should be clearly marked, so as not to confuse it with the version available
from LANL.

Additionally, redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following conditions
are met:
-   Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkACosmoReader.h"
#include "vtkErrorCode.h"
#include "vtkUnstructuredGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkFieldData.h"
#include "vtkPointData.h"
#include "vtkByteSwap.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkDataArray.h"
#include "vtkConfigure.h"
#include "vtkStdString.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkAdaptiveOptions.h"

#ifdef VTK_TYPE_USE_LONG_LONG
#include "vtkLongLongArray.h"
#endif

vtkStandardNewMacro(vtkACosmoReader);

#define FILE_BIG_ENDIAN 0
#define FILE_LITTLE_ENDIAN 1
#define DIMENSION 3
#define X 0
#define VX 1
#define Y 2
#define VY 3
#define Z 4
#define VZ 5
#define MASS 6
#define NUMBER_OF_VAR 3
#define BYTES_PER_DATA_MINUS_TAG 7 * sizeof(vtkTypeFloat32)
#define NUMBER_OF_FLOATS 7
#define NUMBER_OF_INTS 1

//----------------------------------------------------------------------------
vtkACosmoReader::vtkACosmoReader()
{
  this->SetNumberOfInputPorts(0);

  this->FileName               = 0;
  this->ByteOrder              = FILE_LITTLE_ENDIAN;
  this->BoxSize                = 90.140846;
  this->TagSize = 0;

  this->FileStream             = 0;
  this->pieceBounds = 0;
}

//----------------------------------------------------------------------------
vtkACosmoReader::~vtkACosmoReader()
{
  if (this->FileName)
    {
    delete [] this->FileName;
    }

  if(this->pieceBounds)
    {
      delete [] this->pieceBounds;
    }
}

//----------------------------------------------------------------------------
void vtkACosmoReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "File Name: "
     << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "Byte Order: "
     << (this->ByteOrder ? "LITTLE ENDIAN" : "BIG ENDIAN") << endl;
  os << indent << "BoxSize: " << this->BoxSize << endl;
  os << indent << "TagSize: " << (this->TagSize ? "64-bit" : "32-bit")
     << endl;
}

//----------------------------------------------------------------------------
int vtkACosmoReader::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  // Verify that file exists
  if ( !this->FileName )
    {
    vtkErrorMacro(<< "No filename specified.");
    return 0;
    }

  // get parameters
  this->maxlevel = vtkAdaptiveOptions::GetHeight();
  this->splits = vtkAdaptiveOptions::GetDegree();

  // read meta data
  if(!this->pieceBounds)
    {
    this->SetErrorCode(vtkErrorCode::NoError);

    char* fn = new char[255 + strlen(this->FileName)];
    sprintf(fn, "%s.meta", this->FileName);
    ifstream* meta = new ifstream(fn, ios::in);

    if(meta->fail())
      {
      this->SetErrorCode(vtkErrorCode::FileNotFoundError);
      vtkErrorMacro(<< "Unable to open meta file " << fn << ".");
      delete meta;
      return 0;
      }

    int totalpieces = (int)
      ((pow((float)this->splits, (int)(this->maxlevel + 1)) - 1) /
       (this->splits - 1));
    this->pieceBounds = new float[6 * totalpieces];

    // actually read the meta data
    for(int i = 0; i < totalpieces; i = i + 1)
      {
      int piecelevel;
      int piecenumber;
      float bounds[6];

      *meta >> piecelevel;
      *meta >> piecenumber;
      *meta >> bounds[0];
      *meta >> bounds[1];
      *meta >> bounds[2];
      *meta >> bounds[3];
      *meta >> bounds[4];
      *meta >> bounds[5];

      piecelevel = (int)
        ((pow((float)this->splits, (int)piecelevel) - 1) / (this->splits - 1));
      piecenumber = (piecelevel + piecenumber) * 6;

      this->pieceBounds[piecenumber + 0] = bounds[0];
      this->pieceBounds[piecenumber + 1] = bounds[1];
      this->pieceBounds[piecenumber + 2] = bounds[2];
      this->pieceBounds[piecenumber + 3] = bounds[3];
      this->pieceBounds[piecenumber + 4] = bounds[4];
      this->pieceBounds[piecenumber + 5] = bounds[5];
      }

      delete meta;
    }

  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  /*
  // get the data object
  vtkUnstructuredGrid *outData = vtkUnstructuredGrid::SafeDownCast
    (outInfo->Get(vtkDataObject::DATA_OBJECT()));
  */

  double bounds[6];
  bounds[0] = 0.0;
  bounds[1] = this->BoxSize;
  bounds[2] = 0.0;
  bounds[3] = this->BoxSize;
  bounds[4] = 0.0;
  bounds[5] = this->BoxSize;
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX(),
               bounds, 6);

  this->Resolution = 1.0;
  if(outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION()))
    {
    this->Resolution =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION());
    }
  this->currentLevel = (vtkIdType)(this->maxlevel * this->Resolution + .5);

  this->PieceNumber = 0;
  if(outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
    {
    this->PieceNumber =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    }

  int index = (int)
    ((pow((float)this->splits, (int)this->currentLevel) - 1) /
     (this->splits - 1));
  index = (index + this->PieceNumber) * 6;

  bounds[0] = this->pieceBounds[index + 0];
  bounds[1] = this->pieceBounds[index + 1];
  bounds[2] = this->pieceBounds[index + 2];
  bounds[3] = this->pieceBounds[index + 3];
  bounds[4] = this->pieceBounds[index + 4];
  bounds[5] = this->pieceBounds[index + 5];

  outInfo->Set(vtkStreamingDemandDrivenPipeline::PIECE_BOUNDING_BOX(),
               bounds, 6);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
               -1);

  /*
  outInfo->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), -1);
  */

  return 1;
}

//----------------------------------------------------------------------------
int vtkACosmoReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the output
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  //output->Initialize();

  this->Resolution = 1.0;
  if(outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION()))
    {
    this->Resolution =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION());
    }
  this->currentLevel = (vtkIdType)(this->maxlevel * this->Resolution + .5);

  this->PieceNumber = 0;
  if(outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
    {
    this->PieceNumber =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    }

  // Read the file into the output unstructured grid
  return this->ReadFile(output);
}

//----------------------------------------------------------------------------
int vtkACosmoReader::ReadFile(vtkUnstructuredGrid *output)
{
  this->SetErrorCode(vtkErrorCode::NoError);

  char* fn = new char[255 + strlen(this->FileName)];
  sprintf(fn, "%s-%lu-%d",
          this->FileName,
          (long unsigned)this->currentLevel,
          this->PieceNumber);

#if defined(_WIN32) && !defined(__CYGWIN__)
  this->FileStream = new ifstream(fn, ios::in | ios::binary);
#else
  this->FileStream = new ifstream(fn, ios::in);
#endif

  // File exists and can be opened
  if (this->FileStream->fail())
    {
    this->SetErrorCode(vtkErrorCode::FileNotFoundError);
    delete this->FileStream;
    this->FileStream = NULL;
    vtkErrorMacro(<< "Specified filename " << fn << " not found");
    delete [] fn;
    return 0;
    }
  delete [] fn;

  this->FileStream->seekg(0L, ios::end);
  size_t fileLength = this->FileStream->tellg();

  size_t tagBytes;
  if(this->TagSize)
    {
    tagBytes = sizeof(vtkTypeInt64);
    }
  else
    {
    tagBytes = sizeof(vtkTypeInt32);
    }

  // Divide by number of components per record
  vtkIdType numberOfParticles =
    fileLength / (BYTES_PER_DATA_MINUS_TAG + tagBytes);

  // Create the arrays to hold location and field data
  vtkPoints *points       = vtkPoints::New();
  points->SetDataTypeToFloat();
  vtkFloatArray *velocity = vtkFloatArray::New();
  //vtkFloatArray *mass     = vtkFloatArray::New();
  vtkDataArray *tag;
  if(this->TagSize)
    {
    if(sizeof(long) == sizeof(vtkTypeInt64))
      {
      tag = vtkLongArray::New();
      }
    else if(sizeof(int) == sizeof(vtkTypeInt64))
      {
      tag = vtkIntArray::New();
      }
#ifdef VTK_TYPE_USE_LONG_LONG
    else if(sizeof(long long) == sizeof(vtkTypeInt64))
      {
      tag = vtkLongLongArray::New();
      }
#endif
    else
      {
      vtkErrorMacro("Unable to match 64-bit int type to a compiler type. " <<
                    "Going to use long array to store tag data. " <<
                    "Might truncate data.");
      tag = vtkLongArray::New();
      }
    }
  else
    {
    if(sizeof(int) == sizeof(vtkTypeInt32))
      {
      tag = vtkIntArray::New();
      }
    else if(sizeof(long) == sizeof(vtkTypeInt32))
      {
      tag = vtkLongArray::New();
      }
#ifdef VTK_TYPE_USE_LONG_LONG
    else if(sizeof(long long) == sizeof(vtkTypeInt32))
      {
      tag = vtkLongLongArray::New();
      }
#endif
    else
      {
      vtkErrorMacro("Unable to match 32-bit int type to a compiler type. " <<
                    "Going to use int array to store tag data. " <<
                    "Might truncate data.");
      tag = vtkIntArray::New();
      }
    }

  // Allocate space in the unstructured grid for all nodes
  output->Allocate(numberOfParticles);
  output->SetPoints(points);

  velocity->SetName("velocity");
  velocity->SetNumberOfComponents(DIMENSION);
  velocity->SetNumberOfTuples(numberOfParticles);
  output->GetPointData()->AddArray(velocity);

  tag->SetName("tag");
  tag->SetNumberOfComponents(1);
  tag->SetNumberOfTuples(numberOfParticles);
  output->GetPointData()->AddArray(tag);

  /*
  mass->SetName("mass");
  mass->SetNumberOfComponents(1);
  mass->SetNumberOfTuples(numberOfParticles);
  output->GetPointData()->AddArray(mass);
  */

  // record elements
  vtkTypeFloat32 fBlock[NUMBER_OF_FLOATS]; // x,xvel,y,yvel,z,zvel,mass
  char *iBlock = new char[NUMBER_OF_INTS * tagBytes];

  // Loop to read all particle data
  this->FileStream->seekg(0, ios::beg);
  //vtkIdType chunksize = numberOfParticles / 100;

  for (vtkIdType i = 0; i < numberOfParticles; i = i + 1)
    {

    /*
    // update progress
    if (i % chunksize == 0)
      {
      double progress = (double)i / (double)(numberOfParticles);

      this->UpdateProgress(progress);
      }
    */

    // seek and read
      /*
    size_t position = i * (BYTES_PER_DATA_MINUS_TAG + tagBytes);
    this->FileStream->seekg(position, ios::beg);
      */

    // Read the floating point part of the data
    this->FileStream->read((char*)fBlock,
                           NUMBER_OF_FLOATS * sizeof(vtkTypeFloat32));

    size_t returnValue = this->FileStream->gcount();
    if (returnValue != NUMBER_OF_FLOATS * sizeof(vtkTypeFloat32))
      {
      vtkErrorMacro(<< "Only read "
                    << returnValue << " bytes when reading floats.");
      this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
      continue;
      }

    // Read the integer part of the data
    this->FileStream->read(iBlock, NUMBER_OF_INTS * tagBytes);
    returnValue = this->FileStream->gcount();
    if (returnValue != NUMBER_OF_INTS * tagBytes)
      {
      vtkErrorMacro(<< "Only read "
                    << returnValue << " bytes when reading ints.");
      this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
      continue;
      }

    // swap if necessary
#ifdef VTK_WORDS_BIG_ENDIAN
    if(this->ByteOrder == FILE_LITTLE_ENDIAN)
      {
      vtkByteSwap::SwapVoidRange(fBlock, NUMBER_OF_FLOATS,
                                 (int)sizeof(vtkTypeFloat32));
      vtkByteSwap::SwapVoidRange(iBlock, NUMBER_OF_INTS, (int)tagBytes);
      }
#else
    if(this->ByteOrder == FILE_BIG_ENDIAN)
      {
      vtkByteSwap::SwapVoidRange(fBlock, NUMBER_OF_FLOATS,
                                 (int)sizeof(vtkTypeFloat32));
      vtkByteSwap::SwapVoidRange(iBlock, NUMBER_OF_INTS, (int)tagBytes);
      }
#endif

    // Wraparound
    fBlock[X] = fBlock[X] < 0.0 ? this->BoxSize + fBlock[X] :
      (fBlock[X] > this->BoxSize ? fBlock[X] - this->BoxSize : fBlock[X]);
    fBlock[Y] = fBlock[Y] < 0.0 ? this->BoxSize + fBlock[Y] :
      (fBlock[Y] > this->BoxSize ? fBlock[Y] - this->BoxSize : fBlock[Y]);
    fBlock[Z] = fBlock[Z] < 0.0 ? this->BoxSize + fBlock[Z] :
      (fBlock[Z] > this->BoxSize ? fBlock[Z] - this->BoxSize : fBlock[Z]);

    // Insert the location into the point array
    vtkIdType vtkPointID =
      points->InsertNextPoint(fBlock[X], fBlock[Y], fBlock[Z]);
    output->InsertNextCell(1, 1, &vtkPointID);

    velocity->SetComponent(vtkPointID, 0, fBlock[VX]);
    velocity->SetComponent(vtkPointID, 1, fBlock[VY]);
    velocity->SetComponent(vtkPointID, 2, fBlock[VZ]);

    //mass->SetComponent(vtkPointID, 0, fBlock[MASS]);

    double value;
    if(this->TagSize)
      {
      value = *((vtkTypeInt64*)iBlock);
      }
    else
      {
      value = *((vtkTypeInt32*)iBlock);
      }
    tag->SetComponent(vtkPointID, 0, value);

    } // end loop over PositionRange

  // Clean up internal storage
  delete[] iBlock;
  velocity->Delete();
  //mass->Delete();
  tag->Delete();
  points->Delete();
  output->Squeeze();

  // Close the file stream just read
  delete this->FileStream;
  this->FileStream = 0;

  return 1;
}

int vtkACosmoReader::ProcessRequest(vtkInformation *request,
                                    vtkInformationVector **inputVector,
                                    vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  this->Resolution = 1.0;
  if(outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION()))
    {
    this->Resolution =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION());
    }
  this->currentLevel = (vtkIdType)(this->maxlevel * this->Resolution + .5);

  this->PieceNumber = 0;
  if(outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
    {
    this->PieceNumber =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    }

  int index = (int)
    ((pow((float)this->splits, (int)this->currentLevel) - 1) /
     (this->splits - 1));
  index = (index + this->PieceNumber) * 6;

  double bounds[6];
  if(this->pieceBounds)
    {
    bounds[0] = this->pieceBounds[index + 0];
    bounds[1] = this->pieceBounds[index + 1];
    bounds[2] = this->pieceBounds[index + 2];
    bounds[3] = this->pieceBounds[index + 3];
    bounds[4] = this->pieceBounds[index + 4];
    bounds[5] = this->pieceBounds[index + 5];
    }
  else
    {
    bounds[0] = 0.0;
    bounds[1] = this->BoxSize;
    bounds[2] = 0.0;
    bounds[3] = this->BoxSize;
    bounds[4] = 0.0;
    bounds[5] = this->BoxSize;
    }

  outInfo->Set(vtkStreamingDemandDrivenPipeline::PIECE_BOUNDING_BOX(),
               bounds, 6);

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}
