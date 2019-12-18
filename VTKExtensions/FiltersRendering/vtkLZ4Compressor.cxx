/*=========================================================================

  Program:   ParaView
  Module:    vtkLZ4Compressor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLZ4Compressor.h"

#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"

#include "vtk_lz4.h"
#include <cassert>
#include <sstream>

vtkStandardNewMacro(vtkLZ4Compressor);
//----------------------------------------------------------------------------
vtkLZ4Compressor::vtkLZ4Compressor()
  : Quality(3)
{
}

//----------------------------------------------------------------------------
vtkLZ4Compressor::~vtkLZ4Compressor()
{
}

//----------------------------------------------------------------------------
int vtkLZ4Compressor::Compress()
{
  if (!(this->Input && this->Output))
  {
    vtkWarningMacro("Cannot compress, empty input or output detected.");
    return VTK_ERROR;
  }

  unsigned char compress_masks[6][4] = { { 0xFF, 0xFF, 0xFF, 0xFF }, { 0xFE, 0xFF, 0xFE, 0xFE },
    { 0xFC, 0xFE, 0xFC, 0xFC }, { 0xF8, 0xFC, 0xF8, 0xF8 }, { 0xF0, 0xF8, 0xF0, 0xF0 },
    { 0xE0, 0xF0, 0xE0, 0xE0 } };

  int compress_level = this->LossLessMode ? 0 : this->Quality;
  assert(compress_level >= 0 && compress_level <= 5);

  // Set bitmask based on compress_level
  unsigned int compress_mask;
  // I shifted the level by one so that 0 means no compression.
  memcpy(&compress_mask, &compress_masks[compress_level], 4);

  vtkUnsignedCharArray* input = this->Input;
  int inputSize = input->GetNumberOfTuples() * input->GetNumberOfComponents();

  if (this->Quality > 0 && input->GetNumberOfComponents() == 4)
  {
    this->TemporaryBuffer->SetNumberOfComponents(input->GetNumberOfComponents());
    this->TemporaryBuffer->SetNumberOfTuples(input->GetNumberOfTuples());
    const unsigned int* in = reinterpret_cast<const unsigned int*>(input->GetPointer(0));
    unsigned int* out = reinterpret_cast<unsigned int*>(this->TemporaryBuffer->GetPointer(0));
    for (vtkIdType cc = 0, max = this->Input->GetNumberOfTuples(); cc < max; ++cc)
    {
      out[cc] = in[cc] & compress_mask;
    }
    input = this->TemporaryBuffer.Get();
  }

  int maxOutputSize = LZ4_compressBound(inputSize);
  int compressedSize = LZ4_compress_fast(reinterpret_cast<const char*>(input->GetPointer(0)),
    reinterpret_cast<char*>(this->Output->WritePointer(0, maxOutputSize)), inputSize, maxOutputSize,
    16);
  this->Output->SetNumberOfTuples(compressedSize);
  return compressedSize > 0 ? VTK_OK : VTK_ERROR;
}

//----------------------------------------------------------------------------
int vtkLZ4Compressor::Decompress()
{
  if (!(this->Input && this->Output))
  {
    vtkWarningMacro("Cannot decompress, empty input or output detected.");
    return VTK_ERROR;
  }

  int maxDecompressedSize =
    this->Output->GetNumberOfComponents() * this->Output->GetNumberOfTuples();
  int decompressedSize =
    LZ4_decompress_safe(reinterpret_cast<const char*>(this->Input->GetPointer(0)),
      reinterpret_cast<char*>(this->Output->GetPointer(0)), this->Input->GetNumberOfTuples(),
      maxDecompressedSize);

  //  // We use LZ4_decompress_safe for now since there seems to be some bug
  //  // in LZ4_decompress_fast which is causing segfaults on Windows.
  //  int decompressedSize = LZ4_decompress_fast(
  //    reinterpret_cast<const char*>(this->Input->GetPointer(0)),
  //    reinterpret_cast<char*>(this->Output->GetPointer(0)),
  //    maxDecompressedSize);
  return decompressedSize > 0 ? VTK_OK : VTK_ERROR;
}

//-----------------------------------------------------------------------------
void vtkLZ4Compressor::SaveConfiguration(vtkMultiProcessStream* stream)
{
  this->Superclass::SaveConfiguration(stream);
  *stream << this->Quality;
}

//-----------------------------------------------------------------------------
bool vtkLZ4Compressor::RestoreConfiguration(vtkMultiProcessStream* stream)
{
  if (this->Superclass::RestoreConfiguration(stream))
  {
    int quality;
    *stream >> quality;
    this->SetQuality(quality);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
const char* vtkLZ4Compressor::SaveConfiguration()
{
  std::ostringstream oss;
  oss << this->Superclass::SaveConfiguration() << " " << this->Quality;
  this->SetConfiguration(oss.str().c_str());
  return this->Configuration;
}

//-----------------------------------------------------------------------------
const char* vtkLZ4Compressor::RestoreConfiguration(const char* stream)
{
  stream = this->Superclass::RestoreConfiguration(stream);
  if (stream)
  {
    std::istringstream iss(stream);
    int quality;
    iss >> quality;
    this->SetQuality(quality);
    return stream + iss.tellg();
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkLZ4Compressor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  cout << "Quality: " << this->Quality << endl;
}
