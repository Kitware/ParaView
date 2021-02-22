/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMaterialInterfaceCommBuffer.h"
#include "vtkMaterialInterfaceUtilities.h"

using std::vector;

//----------------------------------------------------------------------------
vtkMaterialInterfaceCommBuffer::vtkMaterialInterfaceCommBuffer()
{
  // header
  this->HeaderSize = 0;
  this->Header = nullptr;
  // buffer
  this->EOD = 0;
  this->Buffer = nullptr;
}
//----------------------------------------------------------------------------
vtkMaterialInterfaceCommBuffer::~vtkMaterialInterfaceCommBuffer()
{
  this->Clear();
}
//----------------------------------------------------------------------------
void vtkMaterialInterfaceCommBuffer::Clear()
{
  // buffer
  this->EOD = 0;
  CheckAndReleaseArrayPointer(this->Buffer);
  // header
  this->HeaderSize = 0;
  CheckAndReleaseArrayPointer(this->Header);
}
//----------------------------------------------------------------------------
void vtkMaterialInterfaceCommBuffer::Initialize(int procId, int nBlocks, vtkIdType nBytes)
{
  // header
  this->HeaderSize = DESCR_BASE + nBlocks;
  this->Header = new vtkIdType[this->HeaderSize];
  memset(this->Header, 0, this->HeaderSize * sizeof(vtkIdType));
  this->Header[PROC_ID] = procId;
  // buffer
  CheckAndReleaseArrayPointer(this->Buffer);
  this->Buffer = new char[nBytes];
  this->Header[BUFFER_SIZE] = nBytes;
  this->EOD = 0;
}
//----------------------------------------------------------------------------
void vtkMaterialInterfaceCommBuffer::SizeHeader(int nBlocks)
{
  this->Clear();
  // header
  this->HeaderSize = DESCR_BASE + nBlocks;
  this->Header = new vtkIdType[this->HeaderSize];
  memset(this->Header, 0, this->HeaderSize * sizeof(vtkIdType));
}
//----------------------------------------------------------------------------
void vtkMaterialInterfaceCommBuffer::SizeBuffer(vtkIdType nBytes)
{
  assert("Header must be allocated before buffer is sized." && this->Header != 0);
  // buffer
  CheckAndReleaseArrayPointer(this->Buffer);
  this->Buffer = new char[nBytes];
  this->Header[BUFFER_SIZE] = nBytes;
  this->EOD = 0;
}
//----------------------------------------------------------------------------
void vtkMaterialInterfaceCommBuffer::SizeBuffer()
{
  assert("Header must be allocated before buffer is sized." && this->Header != 0);
  // buffer
  CheckAndReleaseArrayPointer(this->Buffer);
  this->Buffer = new char[this->Header[BUFFER_SIZE]];
  this->EOD = 0;
}
//----------------------------------------------------------------------------
// Append data array to the buffer. Returns the byte index where
// the array was written.
vtkIdType vtkMaterialInterfaceCommBuffer::Pack(vtkDoubleArray* da)
{
  return this->Pack(da->GetPointer(0), da->GetNumberOfComponents(), da->GetNumberOfTuples());
}
//----------------------------------------------------------------------------
// Append data array to the buffer. Returns the byte index where
// the array was written.
vtkIdType vtkMaterialInterfaceCommBuffer::Pack(vtkFloatArray* da)
{
  return this->Pack(da->GetPointer(0), da->GetNumberOfComponents(), da->GetNumberOfTuples());
}
//----------------------------------------------------------------------------
// Append data to the buffer. Returns the byte index
// where the array was written to.
vtkIdType vtkMaterialInterfaceCommBuffer::Pack(
  const double* pData, const int nComps, const vtkIdType nTups)
{
  vtkIdType byteIdx = this->EOD;

  double* pBuffer = reinterpret_cast<double*>(this->Buffer + this->EOD);
  // pack
  for (vtkIdType i = 0; i < nTups; ++i)
  {
    for (int q = 0; q < nComps; ++q)
    {
      pBuffer[q] = pData[q];
    }
    pBuffer += nComps;
    pData += nComps;
  }
  // update next pack location
  this->EOD += nComps * nTups * sizeof(double);

  return byteIdx;
}
//----------------------------------------------------------------------------
// Append data to the buffer. Returns the byte index
// where the array was written to.
vtkIdType vtkMaterialInterfaceCommBuffer::Pack(
  const float* pData, const int nComps, const vtkIdType nTups)
{
  vtkIdType byteIdx = this->EOD;

  float* pBuffer = reinterpret_cast<float*>(this->Buffer + this->EOD);
  // pack
  for (vtkIdType i = 0; i < nTups; ++i)
  {
    for (int q = 0; q < nComps; ++q)
    {
      pBuffer[q] = pData[q];
    }
    pBuffer += nComps;
    pData += nComps;
  }
  // update next pack location
  this->EOD += nComps * nTups * sizeof(float);

  return byteIdx;
}
//----------------------------------------------------------------------------
// Append data to the buffer. Returns the byte index
// where the array was written to.
vtkIdType vtkMaterialInterfaceCommBuffer::Pack(
  const int* pData, const int nComps, const vtkIdType nTups)
{
  vtkIdType byteIdx = this->EOD;

  int* pBuffer = reinterpret_cast<int*>(this->Buffer + this->EOD);
  // pack
  for (vtkIdType i = 0; i < nTups; ++i)
  {
    for (int q = 0; q < nComps; ++q)
    {
      pBuffer[q] = pData[q];
    }
    pBuffer += nComps;
    pData += nComps;
  }
  // update next pack location
  this->EOD += nComps * nTups * sizeof(int);

  return byteIdx;
}
//----------------------------------------------------------------------------
// Pull data from the current location in the buffer
// into a float array. Before the unpack the array
// is (re)sized.
int vtkMaterialInterfaceCommBuffer::UnPack(
  vtkFloatArray* da, const int nComps, const vtkIdType nTups, const bool copyFlag)
{
  int ret = 0;
  float* pData = nullptr;
  if (copyFlag)
  {
    da->SetNumberOfComponents(nComps);
    da->SetNumberOfTuples(nTups);
    pData = da->GetPointer(0);
    // copy into the buffer
    ret = this->UnPack(pData, nComps, nTups, copyFlag);
  }
  else
  {
    da->SetNumberOfComponents(nComps);
    // get a pointer to the buffer
    ret = this->UnPack(pData, nComps, nTups, copyFlag);
    vtkIdType arraySize = nComps * nTups;
    da->SetArray(pData, arraySize, 1);
  }
  return ret;
}
//----------------------------------------------------------------------------
// Pull data from the current location in the buffer
// into a double array. Before the unpack the array
// is (re)sized.
int vtkMaterialInterfaceCommBuffer::UnPack(
  vtkDoubleArray* da, const int nComps, const vtkIdType nTups, const bool copyFlag)
{
  int ret = 0;
  double* pData = nullptr;
  if (copyFlag)
  {
    da->SetNumberOfComponents(nComps);
    da->SetNumberOfTuples(nTups);
    pData = da->GetPointer(0);
    // copy into the buffer
    ret = this->UnPack(pData, nComps, nTups, copyFlag);
  }
  else
  {
    da->SetNumberOfComponents(nComps);
    // get a pointer to the buffer
    ret = this->UnPack(pData, nComps, nTups, copyFlag);
    vtkIdType arraySize = nComps * nTups;
    da->SetArray(pData, arraySize, 1);
  }
  return ret;
}

//----------------------------------------------------------------------------
// Extract data from the buffer. Copy flag indicates whether to
// copy or set pointer to buffer.
int vtkMaterialInterfaceCommBuffer::UnPack(
  float*& rData, const int nComps, const vtkIdType nTups, const bool copyFlag)
{
  // locate
  float* pBuffer = reinterpret_cast<float*>(this->Buffer + this->EOD);

  // unpack
  // copy from buffer
  if (copyFlag)
  {
    float* pData = rData;
    for (vtkIdType i = 0; i < nTups; ++i)
    {
      for (int q = 0; q < nComps; ++q)
      {
        pData[q] = pBuffer[q];
      }
      pBuffer += nComps;
      pData += nComps;
    }
  }
  // point into buffer
  else
  {
    rData = pBuffer;
  }
  // update next read location
  this->EOD += nComps * nTups * sizeof(float);

  return 1;
}

//----------------------------------------------------------------------------
// Extract data from the buffer. Copy flag indicates whether to
// copy or set pointer to buffer.
int vtkMaterialInterfaceCommBuffer::UnPack(
  double*& rData, const int nComps, const vtkIdType nTups, const bool copyFlag)
{
  // locate
  double* pBuffer = reinterpret_cast<double*>(this->Buffer + this->EOD);

  // unpack
  // copy from buffer
  if (copyFlag)
  {
    double* pData = rData;
    for (vtkIdType i = 0; i < nTups; ++i)
    {
      for (int q = 0; q < nComps; ++q)
      {
        pData[q] = pBuffer[q];
      }
      pBuffer += nComps;
      pData += nComps;
    }
  }
  // point into buffer
  else
  {
    rData = pBuffer;
  }
  // update next read location
  this->EOD += nComps * nTups * sizeof(double);

  return 1;
}
//----------------------------------------------------------------------------
// Extract data from the buffer. Copy flag indicates whether to
// copy or set poi9nter to buffer.
int vtkMaterialInterfaceCommBuffer::UnPack(
  int*& rData, const int nComps, const vtkIdType nTups, const bool copyFlag)
{
  // locate
  int* pBuffer = reinterpret_cast<int*>(this->Buffer + this->EOD);

  // unpack
  // copy from buffer
  if (copyFlag)
  {
    int* pData = rData;
    for (vtkIdType i = 0; i < nTups; ++i)
    {
      for (int q = 0; q < nComps; ++q)
      {
        pData[q] = pBuffer[q];
      }
      pBuffer += nComps;
      pData += nComps;
    }
  }
  // point into buffer
  else
  {
    rData = pBuffer;
  }
  // update next read location
  this->EOD += nComps * nTups * sizeof(int);

  return 1;
}
//----------------------------------------------------------------------------
// Set the header size for a vector of buffers.
void vtkMaterialInterfaceCommBuffer::SizeHeader(
  vector<vtkMaterialInterfaceCommBuffer>& buffers, int nBlocks)
{
  size_t nBuffers = buffers.size();
  for (size_t bufferId = 0; bufferId < nBuffers; ++bufferId)
  {
    buffers[bufferId].SizeHeader(nBlocks);
  }
}
//----------------------------------------------------------------------------
ostream& operator<<(ostream& sout, const vtkMaterialInterfaceCommBuffer& fcb)
{
  int hs = fcb.GetHeaderSize();
  sout << "Header size:" << hs << endl;
  int bs = fcb.GetBufferSize();
  sout << "Buffer size:" << bs << endl;
  sout << "EOD:" << fcb.GetEOD() << endl;
  sout << "Header:{";
  const vtkIdType* header = fcb.GetHeader();
  for (int i = 0; i < hs; ++i)
  {
    sout << header[i] << ",";
  }
  sout << (char)0x08 << "}" << endl;
  sout << "Buffer:{";
  const int* buffer = reinterpret_cast<const int*>(fcb.GetBuffer());
  bs /= sizeof(int);
  for (int i = 0; i < bs; ++i)
  {
    sout << buffer[i] << ",";
  }
  sout << (char)0x08 << "}" << endl;

  return sout;
}
