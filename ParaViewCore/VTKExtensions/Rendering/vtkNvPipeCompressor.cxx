/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNvPipeCompressor.cxx

  Copyright (c) 2016-2017, NVIDIA CORPORATION.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkNvPipeCompressor.h"
#include "vtkDataArrayTemplate.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkUnsignedCharArray.h"
#include <cinttypes>
#include <cstdio>
#include <nvpipe.h>
#include <sstream>

vtkStandardNewMacro(vtkNvPipeCompressor);

vtkInformationKeyMacro(vtkNvPipeCompressor, PIXELS_SKIPPED, Integer);

//-----------------------------------------------------------------------------
vtkNvPipeCompressor::vtkNvPipeCompressor()
  : Quality(1)
  , Width(1920)
  , Height(1080)
  , Pipe(NULL)
  , Bitrate(0)
{
}

//-----------------------------------------------------------------------------
vtkNvPipeCompressor::~vtkNvPipeCompressor()
{
  nvpipe_destroy(this->Pipe);
  this->Pipe = NULL;
}

//-----------------------------------------------------------------------------
int vtkNvPipeCompressor::Compress()
{
  if (!(this->Input && this->Output))
  {
    vtkWarningMacro("Cannot compress: empty input or output detected.");
    return VTK_ERROR;
  }
  // We use the user's quality setting as our f_m value.  But the user setting
  // is inverted from how f_m is used, so invert it.
  assert(this->Quality <= 5);
  const uint64_t f_m = (5u - this->Quality) + 1u;
  const uint64_t fps = 30;
  const uint64_t brate = static_cast<uint64_t>(this->Width * this->Height * fps * f_m * 0.07);
  if (NULL == this->Pipe)
  {
    this->Pipe = nvpipe_create_encoder(NVPIPE_H264_NV, brate);
    if (NULL == this->Pipe)
    {
      vtkErrorMacro("Could not create NvPipe encoder.");
      return VTK_ERROR;
    }
    this->Bitrate = brate;
  }

  // We choose our bitrate based on the image size.  If the window has been
  // significantly resized, we should update the bitrate.
  if (this->Bitrate < brate / 2 || this->Bitrate > brate * 2)
  {
    nvpipe_bitrate(this->Pipe, brate);
  }

  vtkUnsignedCharArray* input = this->GetInput();
  const int num_pixels = input->GetNumberOfTuples();
  assert(num_pixels == this->Width * this->Height);
  assert(input->GetNumberOfComponents() == 4); // Expecting RGBA data.

  const size_t input_size = num_pixels * 4;
  const uint8_t* rgba = (const uint8_t*)input->GetPointer(0);

  size_t output_size = num_pixels * 3;
  uint8_t* obuf = static_cast<uint8_t*>(this->Output->WriteVoidPointer(0, output_size));
  if (NULL == obuf)
  {
    vtkErrorMacro("Error obtaining (allocating) output buffer");
    return VTK_ERROR;
  }

  // NvPipe input images must be even.  Just pretend one row of pixels does not
  // exist if the image is not appropriately sized.
  const size_t h = this->Height % 2 == 0 ? this->Height : this->Height - 1;
  vtkInformation* info = this->Output->GetInformation();
  if (this->Height % 2 == 0)
  {
    info->Set(PIXELS_SKIPPED(), 0);
  }
  else
  {
    info->Set(PIXELS_SKIPPED(), 1);
  }

  const nvp_err_t encerr = nvpipe_encode(
    this->Pipe, rgba, num_pixels * 4, obuf, &output_size, this->Width, h, NVPIPE_RGBA);
  if (NVPIPE_SUCCESS != encerr)
  {
    vtkErrorMacro("NvPipe encode error (" << (int)encerr << "): " << nvpipe_strerror(encerr));
    return VTK_ERROR;
  }

  // The encode() call modified output_size to the number of bytes that it
  // wrote into the output buffer.
  this->Output->SetNumberOfComponents(1);
  this->Output->SetNumberOfTuples(output_size);

  return VTK_OK;
}

//-----------------------------------------------------------------------------
int vtkNvPipeCompressor::Decompress()
{
  if (!(this->Input && this->Output))
  {
    vtkWarningMacro("Cannot decompress empty input or output detected.");
    return VTK_ERROR;
  }
  if (this->Pipe == NULL)
  {
    this->Pipe = nvpipe_create_decoder(NVPIPE_H264_NV);
    if (this->Pipe == NULL)
    {
      vtkErrorMacro("Could not create NvPipe decoder.");
      return VTK_ERROR;
    }
  }

  const uint8_t* strm = this->Input->GetPointer(0);
  const size_t insz = this->Input->GetNumberOfTuples() * this->Input->GetNumberOfComponents();

  size_t w = this->Width;
  size_t h = this->Height % 2 == 0 ? this->Height : this->Height - 1;

  vtkUnsignedCharArray* out = this->GetOutput();
  // NvPipe strips the alpha channel if it's given; output is always RGB.
  out->SetNumberOfComponents(3);
  const size_t imgsz = static_cast<size_t>(out->GetNumberOfTuples() * out->GetNumberOfComponents());
  assert(imgsz >= w * h * 3);
  const nvp_err_t decerr = nvpipe_decode(this->Pipe, strm, insz, out->GetPointer(0), w, h);

  if (decerr != NVPIPE_SUCCESS)
  {
    vtkErrorMacro("NvPipe decode error (" << (int)decerr << "): " << nvpipe_strerror(decerr));
    return VTK_ERROR;
  }

  return VTK_OK;
}

//-----------------------------------------------------------------------------
void vtkNvPipeCompressor::SetImageResolution(int w, int h)
{
  assert(w >= 0);
  assert(h >= 0);
  if (w > 8192 || h > 8192)
  {
    vtkWarningMacro("Image size (" << w << "x" << h << ") exceeds max image "
                                                       "size for NvPipe.");
  }
  this->Width = static_cast<size_t>(w);
  this->Height = static_cast<size_t>(h);
}

//-----------------------------------------------------------------------------
void vtkNvPipeCompressor::SaveConfiguration(vtkMultiProcessStream* stream)
{
  vtkImageCompressor::SaveConfiguration(stream);
  *stream << (int)this->Width << (int)this->Height;
}

//-----------------------------------------------------------------------------
bool vtkNvPipeCompressor::RestoreConfiguration(vtkMultiProcessStream* stream)
{
  if (!vtkImageCompressor::RestoreConfiguration(stream))
  {
    return false;
  }
  int w, h;
  *stream >> w >> h;
  this->Width = w;
  this->Height = h;
  return true;
}

//-----------------------------------------------------------------------------
const char* vtkNvPipeCompressor::SaveConfiguration()
{
  std::ostringstream oss;
  oss << vtkImageCompressor::SaveConfiguration() << " " << this->Width << " " << this->Height;

  this->SetConfiguration(oss.str().c_str());

  return this->Configuration;
}

//-----------------------------------------------------------------------------
const char* vtkNvPipeCompressor::RestoreConfiguration(const char* stream)
{
  stream = vtkImageCompressor::RestoreConfiguration(stream);
  if (stream == NULL)
  {
    return NULL;
  }
  std::istringstream iss(stream);
  int w, h;
  iss >> w >> h;
  this->Width = w;
  this->Height = h;

  return stream + iss.tellg();
}

//-----------------------------------------------------------------------------
void vtkNvPipeCompressor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << "using compressor: NvPipe\n";
}

//-----------------------------------------------------------------------------
bool vtkNvPipeCompressor::Available()
{
  // Instantiate an encoder; this initializes CUDA and the NVEncode side of the
  // Video SDK, so if it succeeds we are good to go.
  nvpipe* dummy = nvpipe_create_encoder(NVPIPE_H264_NV, 1024);
  bool success = dummy != NULL;
  nvpipe_destroy(dummy);
}
