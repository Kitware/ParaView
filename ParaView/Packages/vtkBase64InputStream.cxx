/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBase64InputStream.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBase64InputStream.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkBase64InputStream, "1.2");
vtkStandardNewMacro(vtkBase64InputStream);

//----------------------------------------------------------------------------
static unsigned char vtkBase64InputStreamDecode(unsigned char c);

//----------------------------------------------------------------------------
vtkBase64InputStream::vtkBase64InputStream()
{
  this->BufferLength = 0;
}

//----------------------------------------------------------------------------
vtkBase64InputStream::~vtkBase64InputStream()
{
}

//----------------------------------------------------------------------------
void vtkBase64InputStream::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkBase64InputStream::StartReading()
{
  this->Superclass::StartReading();
  this->BufferLength = 0;
}

//----------------------------------------------------------------------------
void vtkBase64InputStream::EndReading()
{
}

//----------------------------------------------------------------------------
int vtkBase64InputStream::Seek(unsigned long offset)
{
  unsigned long triplet = offset/3;
  int skipLength = offset%3;
  
  // Seek to the beginning of the encoded triplet containing the
  // offset.
  if(!this->Stream->seekg(this->StreamStartPosition+(triplet*4)))
    {
    return 0;
    }
  
  // Decode the first triplet if it is paritally skipped.
  if(skipLength == 0)
    {
    this->BufferLength = 0;
    }
  else if(skipLength == 1)
    {
    unsigned char c;
    this->BufferLength =
      this->DecodeTriplet(c, this->Buffer[0], this->Buffer[1]) - 1;
    }
  else
    {
    unsigned char c[2];
    this->BufferLength =
      this->DecodeTriplet(c[0], c[1], this->Buffer[0]) - 2;
    }
  
  // A DecodeTriplet call may have failed to read the stream.
  // If so, the current buffer length will be negative.
  return ((this->BufferLength >= 0) ? 1:0);
}

//----------------------------------------------------------------------------
unsigned long vtkBase64InputStream::Read(unsigned char* data,
                                         unsigned long length)
{
  unsigned char* out = data;
  unsigned char* end = data + length;
  
  // If the previous read ended before filling buffer, don't read
  // more.
  if(this->BufferLength < 0) { return 0; }
  
  // Use leftover characters from a previous decode.
  if((out != end) && (this->BufferLength == 2))
    {
    *out++ = this->Buffer[0];
    this->Buffer[0] = this->Buffer[1];
    this->BufferLength = 1;
    }
  if((out != end) && (this->BufferLength == 1))
    {
    *out++ = this->Buffer[0];
    this->BufferLength = 0;
    }
  
  // Decode all complete triplets.
  while((end - out) >= 3)
    {
    int len = this->DecodeTriplet(out[0], out[1], out[2]);
    out += len;
    if(len < 3)
      {
      this->BufferLength = len-3;
      return (out-data);
      }
    }
  
  // Decode the last triplet and save leftover characters in the buffer.
  if((end - out) == 2)
    {
    int len = this->DecodeTriplet(out[0], out[1], this->Buffer[0]);
    this->BufferLength = len-2;
    if(len > 2) { out += 2; }
    else { out += len; }
    }
  else if((end - out) == 1)
    {
    int len = this->DecodeTriplet(out[0], this->Buffer[0], this->Buffer[1]);
    this->BufferLength = len-1;
    if(len > 1) { out += 1; }
    else { out += len; }
    }
  
  return (out-data);  
}

//----------------------------------------------------------------------------
inline int vtkBase64InputStream::DecodeTriplet(unsigned char& c0,
                                               unsigned char& c1,
                                               unsigned char& c2)
{
  unsigned char in[4];
  unsigned char d[4];
  
  // Read the 4 bytes encoding this triplet from the stream.
  this->Stream->read(in, 4);
  if(this->Stream->gcount() < 4) { return 0; }
  d[0] = vtkBase64InputStreamDecode(in[0]);
  d[1] = vtkBase64InputStreamDecode(in[1]);
  d[2] = vtkBase64InputStreamDecode(in[2]);
  d[3] = vtkBase64InputStreamDecode(in[3]);
  
  // Make sure all characters were valid.
  if((d[0] == 0xFF) || (d[1] == 0xFF) || (d[2] == 0xFF) || (d[3] == 0xFF))
    { return 0; }
  
  // Decode the 3 bytes.
  c0 = ((d[0] << 2) & 0xFC) | ((d[1] >> 4) & 0x03);
  c1 = ((d[1] << 4) & 0xF0) | ((d[2] >> 2) & 0x0F);
  c2 = ((d[2] << 6) & 0xC0) | ((d[3] >> 0) & 0x3F);
  
  // Return the number of bytes actually decoded.
  if(in[2] == '=') { return 1; }
  else if(in[3] == '=') { return 2; }
  else { return 3; }
}

//----------------------------------------------------------------------------
static const unsigned char vtkBase64InputStreamDecodeTable[256] =
{
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0x3E,0xFF,0xFF,0xFF,0x3F,
  0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,
  0x3C,0x3D,0xFF,0xFF,0xFF,0x00,0xFF,0xFF,
  0xFF,0x00,0x01,0x02,0x03,0x04,0x05,0x06,
  0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,
  0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,
  0x17,0x18,0x19,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,
  0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,
  0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x30,
  0x31,0x32,0x33,0xFF,0xFF,0xFF,0xFF,0xFF,
  //-------------------------------------
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
};

  
//----------------------------------------------------------------------------
unsigned char vtkBase64InputStreamDecode(unsigned char c)
{ 
  // Decode table lookup function.
  return vtkBase64InputStreamDecodeTable[c]; 
}
