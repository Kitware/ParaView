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

#include "vtkLZ4Compressor.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkSquirtCompressor.h"
#include "vtkUnsignedCharArray.h"
#include "vtkZlibImageCompressor.h"

#include <assert.h>
#include <sstream>

vtkStandardNewMacro(vtkPVClientServerSynchronizedRenderers);
vtkCxxSetObjectMacro(vtkPVClientServerSynchronizedRenderers, Compressor, vtkImageCompressor);
//----------------------------------------------------------------------------
vtkPVClientServerSynchronizedRenderers::vtkPVClientServerSynchronizedRenderers()
{
  this->Compressor = NULL;
  this->ConfigureCompressor("vtkLZ4Compressor 0 3");
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
  assert(this->ParallelController->IsA("vtkSocketController") ||
    this->ParallelController->IsA("vtkCompositeMultiProcessController"));

  vtkRawImage& rawImage = (this->ImageReductionFactor == 1) ? this->FullImage : this->ReducedImage;

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
void vtkPVClientServerSynchronizedRenderers::SlaveStartRender()
{
  this->Superclass::SlaveStartRender();

#ifdef VTKGL2
  // In client-server mode, we want all the server ranks to simply render using
  // a black background. That makes it easier to blend the image we obtain from
  // the server rank on top of the background rendered locally on the client.
  // vtkPVSynchronizedRenderer only creates
  // vtkPVClientServerSynchronizedRenderers in client-server mode that support
  // image delivery to client (i.e. not in tile-display, or cave mode).
  // Hence, we know it won't affect server ranks that do display the rendered
  // result. (Fixes BUG #0015961).
  this->Renderer->SetBackground(0, 0, 0);
  this->Renderer->SetGradientBackground(false);
  this->Renderer->SetTexturedBackground(false);
#endif
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
    this->ParallelController->Send(this->Compress(rawImage.GetRawPtr()), 1, 0x023430);
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
  // cerr << this->GetClassName() << "::ConfigureCompressor " << stream << endl;

  // Configure the compressor from a string. The string will
  // contain the class name of the compressor type to use,
  // follwed by a stream that the named class will restore itself
  // from.
  std::istringstream iss(stream);
  std::string className;
  iss >> className;

  // Allocate the desired compressor unless we have one in hand.
  if (!(this->Compressor && this->Compressor->IsA(className.c_str())))
  {
    vtkImageCompressor* comp = 0;
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
    else if (className == "NULL" || className.empty())
    {
      this->SetCompressor(0);
      return;
    }
    if (comp == 0)
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
void vtkPVClientServerSynchronizedRenderers::PushImageToScreen()
{
  // This trick allows us to not clear the color buffer before pasting back
  // the image from the server. This makes it possible to preserve any
  // annotations rendered esp. vtkGridAxes3DActor which renders in multiple
  // layers.
  // This is not the most elegant solution. We should rethink if
  // vtkSynchronizedRenderers::PushImageToScreen() should clear the screen by
  // default -- I can argue not.
  int layer = this->Renderer->GetLayer();
  int prev = this->Renderer->GetPreserveColorBuffer();
  if (layer == 0)
  {
    this->Renderer->SetPreserveColorBuffer(1);
  }
  this->Superclass::PushImageToScreen();
  this->Renderer->SetPreserveColorBuffer(prev);
}

//----------------------------------------------------------------------------
void vtkPVClientServerSynchronizedRenderers::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
