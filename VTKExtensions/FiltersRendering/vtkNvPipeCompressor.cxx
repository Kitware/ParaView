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
#include "vtkProcessModule.h"
#include "vtkSMPTools.h"
#include "vtkUnsignedCharArray.h"
#include <cinttypes>
#include <cstdio>
#include <nvpipe.h>
#include <sstream>

vtkStandardNewMacro(vtkNvPipeCompressor);

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
  assert(this->Quality <= 5);
  const uint64_t f_m =
    (5 - this->Quality + 1); // Quality setting in ParaView compressor GUI is inverted
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

  if (this->Bitrate != brate)
  {
    nvpipe_bitrate(this->Pipe, brate);
    this->Bitrate = brate;
  }

  vtkUnsignedCharArray* input = this->GetInput();
  assert(input->GetNumberOfComponents() == 4); // Expecting RGBA data.

  const int num_pixels = this->Width * this->Height;
  assert(num_pixels <= input->GetNumberOfTuples());

  const uint8_t* rgba = (const uint8_t*)input->GetPointer(0);

  size_t output_size = num_pixels * 3;
  uint8_t* obuf = static_cast<uint8_t*>(this->Output->WriteVoidPointer(0, output_size));
  if (NULL == obuf)
  {
    vtkErrorMacro("Error obtaining (allocating) output buffer");
    return VTK_ERROR;
  }

  const nvp_err_t encerr = nvpipe_encode(
    this->Pipe, rgba, num_pixels * 4, obuf, &output_size, this->Width, this->Height, NVPIPE_RGBA);
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

  vtkUnsignedCharArray* out = this->GetOutput();
  // NvPipe strips the alpha channel if it's given; output is always RGB.
  out->SetNumberOfComponents(3);
  const size_t imgsz = static_cast<size_t>(out->GetNumberOfTuples() * out->GetNumberOfComponents());
  assert(imgsz >= this->Width * this->Height * 3);
  const nvp_err_t decerr =
    nvpipe_decode(this->Pipe, strm, insz, out->GetPointer(0), this->Width, this->Height);

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
  this->Superclass::SaveConfiguration(stream);
  *stream << (int)this->Quality;
}

//-----------------------------------------------------------------------------
bool vtkNvPipeCompressor::RestoreConfiguration(vtkMultiProcessStream* stream)
{
  if (!this->Superclass::RestoreConfiguration(stream))
  {
    return false;
  }

  int qual;
  *stream >> qual;
  this->Quality = qual;
  return true;
}

//-----------------------------------------------------------------------------
const char* vtkNvPipeCompressor::SaveConfiguration()
{
  std::ostringstream oss;
  oss << vtkImageCompressor::SaveConfiguration() << " " << this->Quality;
  this->SetConfiguration(oss.str().c_str());
  return this->Configuration;
}

//-----------------------------------------------------------------------------
const char* vtkNvPipeCompressor::RestoreConfiguration(const char* stream)
{
  stream = this->Superclass::RestoreConfiguration(stream);
  if (stream == NULL)
  {
    return NULL;
  }
  std::istringstream iss(stream);
  int qual;
  iss >> qual;
  this->SetQuality(qual);

  return stream + iss.tellg();
}

//-----------------------------------------------------------------------------
void vtkNvPipeCompressor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << "using compressor: NvPipe\n";
}
