/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSquirtCompressor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSquirtCompressor.h"
#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"
#include "vtkImageData.h"

vtkStandardNewMacro(vtkSquirtCompressor);
vtkCxxRevisionMacro(vtkSquirtCompressor, "1.4");
//-----------------------------------------------------------------------------
vtkSquirtCompressor::vtkSquirtCompressor()
{
  this->SquirtLevel = 0; // no compression by default.
}

//-----------------------------------------------------------------------------
vtkSquirtCompressor::~vtkSquirtCompressor()
{
}

//-----------------------------------------------------------------------------
int vtkSquirtCompressor::CompressData()
{
  vtkUnsignedCharArray* input =  this->GetInput();
  
  if (input->GetNumberOfComponents() != 4 && input->GetNumberOfComponents() != 3)
    {
    vtkErrorMacro("Squirt only works with RGBA or RGB");
    return VTK_ERROR;
    }

  int count=0;
  int index=0;
  int comp_index=0;
  int end_index;
  int compress_level = this->SquirtLevel;
  unsigned int current_color;
  unsigned char compress_masks[6][4] = {  {0xFF, 0xFF, 0xFF, 0xFF},
      {0xFE, 0xFF, 0xFE, 0xFF},
      {0xFC, 0xFE, 0xFC, 0xFF},
      {0xF8, 0xFC, 0xF8, 0xFF},
      {0xF0, 0xF8, 0xF0, 0xFF},
      {0xE0, 0xF0, 0xE0, 0xFF}};
  
  if (compress_level < 0 || compress_level > 5)
    {
    vtkErrorMacro("Squirt compression level (" << compress_level 
      << ") is out of range [0,5].");
    compress_level = 1;
    }

  // Set bitmask based on compress_level
  unsigned int compress_mask;
  // I shifted the level by one so that 0 means no compression.
  memcpy(&compress_mask, &compress_masks[compress_level], 4);

  // Access raw arrays directly
  if (input->GetNumberOfComponents() == 4)
    {
    unsigned int* _rawColorBuffer;
    unsigned int* _rawCompressedBuffer;
    int numPixels = input->GetNumberOfTuples();
    _rawColorBuffer = (unsigned int*)input->GetPointer(0);
    _rawCompressedBuffer = (unsigned int*)this->Output->WritePointer(0,numPixels*4);
    end_index = numPixels;

    // Go through color buffer and put RLE format into compressed buffer
    while((index < end_index) && (comp_index < end_index)) 
      {

      // Record color
      current_color = _rawCompressedBuffer[comp_index] =_rawColorBuffer[index];
      index++;

      // Compute Run
      while(((current_color&compress_mask) == (_rawColorBuffer[index]&compress_mask)) &&
        (index<end_index) && (count<255))
        { 
        index++; count++;   
        }

      // Record Run length
      *((unsigned char*)_rawCompressedBuffer+comp_index*4+3) =(unsigned char)count;
      comp_index++;

      count = 0;

      }
    }
  else if (input->GetNumberOfComponents() == 3)
    {
    unsigned char* _rawColorBuffer;
    unsigned int* _rawCompressedBuffer;
    int numPixels = input->GetNumberOfTuples();
    _rawColorBuffer = (unsigned char*)input->GetPointer(0);
    _rawCompressedBuffer = (unsigned int*)this->Output->WritePointer(0,numPixels*4);
    end_index = numPixels;

    // Go through color buffer and put RLE format into compressed buffer
    while((index < 3*numPixels) && (comp_index < end_index)) 
      {

      int next_color = 0;
      // Record color
      unsigned char* p = (unsigned char*)&current_color;
      *p++ = _rawColorBuffer[index];
      *p++ = _rawColorBuffer[index+1];
      *p++ = _rawColorBuffer[index+2];
      *p = 0x0;
      
      _rawCompressedBuffer[comp_index] = current_color;
      index+=3;
      
      p = (unsigned char*)&next_color;
      *p++ = _rawColorBuffer[index];
      *p++ = _rawColorBuffer[index+1];
      *p++ = _rawColorBuffer[index+2];
      *p = 0x0;
    
      // Compute Run
      while(((current_color&compress_mask) == (next_color&compress_mask)) &&
        (index < 3*numPixels) && (count<255))
        { 
        index+=3; count++;   
        if (index < 3*numPixels)
          {
          p = (unsigned char*)&next_color;
          *p++ = _rawColorBuffer[index];
          *p++ = _rawColorBuffer[index+1];
          *p++ = _rawColorBuffer[index+2];
          *p = 0x0;
          }
        }

      // Record Run length
      *((unsigned char*)_rawCompressedBuffer+comp_index*4+3) =(unsigned char)count;
      comp_index++;

      count = 0;
      }
    }

  // Back to vtk arrays :)
  this->Output->SetNumberOfComponents(4);
  this->Output->SetNumberOfTuples(comp_index);
  return VTK_OK;
}

//-----------------------------------------------------------------------------
int vtkSquirtCompressor::DecompressData()
{
  vtkUnsignedCharArray* in = this->GetInput();
  vtkUnsignedCharArray* out = this->GetOutput();
  int count=0;
  int index=0;
  unsigned int current_color;
  unsigned int* _rawColorBuffer;
  unsigned int* _rawCompressedBuffer;

  // Get compressed buffer size
  int CompSize = in->GetNumberOfTuples();

  // Access raw arrays directly
  _rawColorBuffer = (unsigned int*)out->GetPointer(0);
  _rawCompressedBuffer = (unsigned int*)in->GetPointer(0);

  // Go through compress buffer and extract RLE format into color buffer
  for(int i=0; i<CompSize; i++)
    {
    // Get color and count
    current_color = _rawCompressedBuffer[i];

    // Get run length count;
    count = *((unsigned char*)&current_color+3);

    // Fixed Alpha
    *((unsigned char*)&current_color+3) = 0xFF;

    // Set color
    _rawColorBuffer[index++] = current_color;

    // Blast color into color buffer
    for(int j=0; j< count; j++)
      _rawColorBuffer[index++] = current_color;
    }
  return VTK_OK;
}

//-----------------------------------------------------------------------------
void vtkSquirtCompressor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SquirtLevel: " << this->SquirtLevel << endl;
}
