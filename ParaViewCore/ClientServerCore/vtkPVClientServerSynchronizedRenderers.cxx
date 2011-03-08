/*=========================================================================

  Program:   ParaView
  Module:    vtkPVClientServerSynchronizedRenderers.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVClientServerSynchronizedRenderers.h"

#include "vtkObjectFactory.h"
#include "vtkSquirtCompressor.h"
#include "vtkZlibImageCompressor.h"
#include "vtkMultiProcessController.h"
#include "vtkUnsignedCharArray.h"

#include <vtksys/ios/sstream>
#include <assert.h>

vtkStandardNewMacro(vtkPVClientServerSynchronizedRenderers);
vtkCxxSetObjectMacro(vtkPVClientServerSynchronizedRenderers, Compressor,
  vtkImageCompressor);
//----------------------------------------------------------------------------
vtkPVClientServerSynchronizedRenderers::vtkPVClientServerSynchronizedRenderers()
{
  this->Compressor = NULL;
  this->ConfigureCompressor("vtkSquirtCompressor 0 3");
  this->LossLessCompression = true;
}

//----------------------------------------------------------------------------
vtkPVClientServerSynchronizedRenderers::~vtkPVClientServerSynchronizedRenderers()
{
  this->SetCompressor(NULL);
}


//----------------------------------------------------------------------------
void vtkPVClientServerSynchronizedRenderers::MasterEndRender()
{
  // receive image from slave.
  assert(this->ParallelController->IsA("vtkSocketController"));

  vtkRawImage& rawImage = (this->ImageReductionFactor == 1)?
    this->FullImage : this->ReducedImage;

  int header[4];
  this->ParallelController->Receive(header, 4, 1, 0x023430);
  if (header[0] > 0)
    {
    rawImage.Resize(header[1], header[2], header[3]);
    if (this->Compressor)
      {
      vtkUnsignedCharArray* data = vtkUnsignedCharArray::New();
      this->ParallelController->Receive(data, 1, 0x023430);
      this->Decompress(data, rawImage.GetRawPtr());
      data->Delete();
      }
    else
      {
      this->ParallelController->Receive(rawImage.GetRawPtr(), 1, 0x023430);
      }
    rawImage.MarkValid();
    }
}

//----------------------------------------------------------------------------
void vtkPVClientServerSynchronizedRenderers::SlaveEndRender()
{
  assert(this->ParallelController->IsA("vtkSocketController"));

  vtkRawImage &rawImage = this->CaptureRenderedImage();

  int header[4];
  header[0] = rawImage.IsValid()? 1 : 0;
  header[1] = rawImage.GetWidth();
  header[2] = rawImage.GetHeight();
  header[3] = rawImage.IsValid()?
    rawImage.GetRawPtr()->GetNumberOfComponents() : 0;

  // send the image to the client.
  this->ParallelController->Send(header, 4, 1, 0x023430);
  if (rawImage.IsValid())
    {
    this->ParallelController->Send(
      this->Compress(rawImage.GetRawPtr()), 1, 0x023430);
    }
}

//----------------------------------------------------------------------------
vtkUnsignedCharArray* vtkPVClientServerSynchronizedRenderers::Compress(
  vtkUnsignedCharArray* data)
{
  if (this->Compressor)
    {
    this->Compressor->SetLossLessMode(this->LossLessCompression);
    this->Compressor->SetInput(data);
    if (this->Compressor->Compress() == 0)
      {
      vtkErrorMacro("Image compression failed!");
      return data;
      }
    return this->Compressor->GetOutput();
    }

  return data;
}

//----------------------------------------------------------------------------
void vtkPVClientServerSynchronizedRenderers::Decompress(
  vtkUnsignedCharArray* data, vtkUnsignedCharArray* outputBuffer)
{
  if (this->Compressor)
    {
    this->Compressor->SetLossLessMode(this->LossLessCompression);
    this->Compressor->SetInput(data);
    this->Compressor->SetOutput(outputBuffer);
    if (this->Compressor->Decompress() == 0)
      {
      vtkErrorMacro("Image de-compression failed!");
      }
    }
  else
    {
    vtkErrorMacro("No compressor present.");
    }
}


//----------------------------------------------------------------------------
void vtkPVClientServerSynchronizedRenderers::ConfigureCompressor(const char *stream)
{
  // cerr << this->GetClassName() << "::ConfigureCompressor " << stream << endl;

  // Configure the compressor from a string. The string will
  // contain the class name of the compressor type to use,
  // follwed by a stream that the named class will restore itself
  // from.
  vtkstd::istringstream iss(stream);
  vtkstd::string className;
  iss >> className;

  // Allocate the desired compressor unless we have one in hand.
  if (!(this->Compressor && this->Compressor->IsA(className.c_str())))
    {
    vtkImageCompressor *comp=0;
    if (className=="vtkSquirtCompressor")
      {
      comp=vtkSquirtCompressor::New();
      }
    else if (className=="vtkZlibImageCompressor")
      {
      comp=vtkZlibImageCompressor::New();
      }
    else if (className=="NULL")
      {
      this->SetCompressor(0);
      return;
      }
    if (comp==0)
      {
      vtkWarningMacro("Could not create the compressor by name " << className << ".");
      return;
      }
    this->SetCompressor(comp);
    comp->Delete();
    }
  // move passed the class name and let the compressor configure itself
  // from the stream.
  const char *ok=this->Compressor->RestoreConfiguration(stream);
  if (!ok)
    {
    vtkWarningMacro("Could not configure the compressor, invalid stream. " << stream << ".");
    }
}

//----------------------------------------------------------------------------
void vtkPVClientServerSynchronizedRenderers::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
