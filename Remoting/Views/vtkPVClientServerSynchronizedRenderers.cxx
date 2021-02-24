/*=========================================================================

  Program:   ParaView
  Module:    vtkPVClientServerSynchronizedRenderers.cxx

  Copyright (c) Kitware, Inc.
  Copyright (c) 2017, NVIDIA CORPORATION.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVClientServerSynchronizedRenderers.h"

#include "vtkLZ4Compressor.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPVConfig.h"
#include "vtkSquirtCompressor.h"
#include "vtkUnsignedCharArray.h"
#include "vtkZlibImageCompressor.h"
#if VTK_MODULE_ENABLE_ParaView_nvpipe
#include "vtkNvPipeCompressor.h"
#endif

#include <assert.h>
#include <sstream>

vtkStandardNewMacro(vtkPVClientServerSynchronizedRenderers);
vtkCxxSetObjectMacro(vtkPVClientServerSynchronizedRenderers, Compressor, vtkImageCompressor);
//----------------------------------------------------------------------------
vtkPVClientServerSynchronizedRenderers::vtkPVClientServerSynchronizedRenderers()
  : Compressor(nullptr)
  , LossLessCompression(true)
  , NVPipeSupport(false)
{
  this->ConfigureCompressor("vtkLZ4Compressor 0 3");
}

//----------------------------------------------------------------------------
vtkPVClientServerSynchronizedRenderers::~vtkPVClientServerSynchronizedRenderers()
{
  this->SetCompressor(nullptr);
}

//----------------------------------------------------------------------------
void vtkPVClientServerSynchronizedRenderers::MasterEndRender()
{
  // receive image from slave.
  assert(this->ParallelController->IsA("vtkSocketController") ||
    this->ParallelController->IsA("vtkCompositeMultiProcessController"));

  vtkRawImage& rawImage = this->Image;

  int header[4];
  this->ParallelController->Receive(header, 4, 1, 0x023430);
  if (header[0] > 0)
  {
    rawImage.Resize(header[1], header[2], header[3]);
    if (this->Compressor)
    {
      vtkUnsignedCharArray* data = vtkUnsignedCharArray::New();
      this->ParallelController->Receive(data, 1, 0x023430);
      this->Compressor->SetImageResolution(header[1], header[2]);
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
  assert(this->ParallelController->IsA("vtkSocketController") ||
    this->ParallelController->IsA("vtkCompositeMultiProcessController"));

  vtkRawImage& rawImage = this->CaptureRenderedImage();

  int header[4];
  header[0] = rawImage.IsValid() ? 1 : 0;
  header[1] = rawImage.GetWidth();
  header[2] = rawImage.GetHeight();
  header[3] = rawImage.IsValid() ? rawImage.GetRawPtr()->GetNumberOfComponents() : 0;

  // send the image to the client.
  this->ParallelController->Send(header, 4, 1, 0x023430);

  if (rawImage.IsValid())
  {
    if (this->Compressor)
    {
      this->Compressor->SetImageResolution(header[1], header[2]);
      this->ParallelController->Send(this->Compress(rawImage.GetRawPtr()), 1, 0x023430);
    }
    else
    {
      this->ParallelController->Send(rawImage.GetRawPtr(), 1, 0x023430);
    }
  }
}

//----------------------------------------------------------------------------
vtkUnsignedCharArray* vtkPVClientServerSynchronizedRenderers::Compress(vtkUnsignedCharArray* data)
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
void vtkPVClientServerSynchronizedRenderers::ConfigureCompressor(const char* stream)
{
  // Configure the compressor from a string. The string will
  // contain the class name of the compressor type to use,
  // follwed by a stream that the named class will restore itself
  // from.
  std::istringstream iss(stream);
  std::string className;
  iss >> className;
  // Allocate the desired compressor unless we have one in hand.
  if (this->Compressor == nullptr || !this->Compressor->IsA(className.c_str()))
  {
    vtkImageCompressor* comp = nullptr;
    if (className == "vtkSquirtCompressor")
    {
      comp = vtkSquirtCompressor::New();
    }
    else if (className == "vtkZlibImageCompressor")
    {
      comp = vtkZlibImageCompressor::New();
    }
    else if (className == "vtkLZ4Compressor")
    {
      comp = vtkLZ4Compressor::New();
    }
    else if (className == "vtkNvPipeCompressor" && this->NVPipeSupport)
    {
#if VTK_MODULE_ENABLE_ParaView_nvpipe
      comp = vtkNvPipeCompressor::New();
#else
      assert(false);
      vtkErrorMacro("NVPipe not compiled in!");
      return;
#endif
    }
    else if (className == "vtkNvPipeCompressor")
    {
      // NVPipe was requested but not available.  Fall back to LZ4.
      comp = vtkLZ4Compressor::New();
      // We also need to rewrite the stream to have the correct type.
      stream = "vtkLZ4Compressor 0 3";
    }
    else if (className == "NULL" || className.empty())
    {
      this->SetCompressor(nullptr);
      return;
    }

    if (comp == nullptr)
    {
      vtkWarningMacro("Could not create the compressor by name " << className << ".");
      return;
    }
    this->SetCompressor(comp);
    comp->Delete();
  }

  // move passed the class name and let the compressor configure itself
  // from the stream.
  const char* ok = this->Compressor->RestoreConfiguration(stream);
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
