// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkMaterialInterfaceCommBuffer_h
#define vtkMaterialInterfaceCommBuffer_h

#include "vtkPVVTKExtensionsFiltersMaterialInterfaceModule.h" //needed for exports
#include "vtkType.h"                                          // for vtkIdType

#include <vector> // for std::vector

class vtkDoubleArray;
class vtkFloatArray;

//============================================================================
// Description:
// FIFO buffer.
// Can be used to send all attributes and ids
// in a single send. UnPack in the same order that you
// packed.
class VTKPVVTKEXTENSIONSFILTERSMATERIALINTERFACE_EXPORT vtkMaterialInterfaceCommBuffer
{
public:
  // header layout
  enum
  {
    PROC_ID = 0,     // sender's id
    BUFFER_SIZE = 1, // size of buffer(bytes)
    DESCR_BASE = 2
  }; // index to first descriptor
     // descriptor is nfragments
     // in each block. Each block gets a descriptor.
  //
  vtkMaterialInterfaceCommBuffer();
  //
  ~vtkMaterialInterfaceCommBuffer();
  // Description:
  // Initialize for outgoing comm.
  void Initialize(int procId, int nBlocks, vtkIdType nBytes);
  // Description:
  // Set up for an incoming header.
  void SizeHeader(int nBlocks);
  // Description:
  // Set up for a set of incoming header.
  static void SizeHeader(std::vector<vtkMaterialInterfaceCommBuffer>& buffers, int nBlocks);
  // Description:
  // Initialize the buffer.
  void SizeBuffer(vtkIdType nBytes);
  // Description:
  // Initialize the buffer from incoming header.
  void SizeBuffer();
  // Description:
  // Free resources set to invalid state.
  void Clear();
  // Description:
  // Get size in bytes of the buffer.
  vtkIdType GetBufferSize() const { return this->Header[BUFFER_SIZE]; }
  // Description:
  // Get a pointer to the buffer
  char* GetBuffer() const { return this->Buffer; }
  // Description:
  // Get the byte that the next rerad or write will occur at
  vtkIdType GetEOD() const { return this->EOD; }
  // Description:
  // Get size in bytes of the header.
  int GetHeaderSize() const { return this->HeaderSize; }
  // Description:
  // Get a pointer to the header.
  vtkIdType* GetHeader() const { return this->Header; }
  // Description:
  // Get the total memory used.
  vtkIdType Capacity() { return this->GetHeaderSize() + this->GetBufferSize(); }
  // Description:
  // Set the number of tuples for a given block.
  void SetNumberOfTuples(int blockId, vtkIdType nFragments)
  {
    int idx = DESCR_BASE + blockId;
    this->Header[idx] = nFragments;
  }
  // Description:
  // Get the number of tuples for a given block
  vtkIdType GetNumberOfTuples(int blockId) const
  {
    int idx = DESCR_BASE + blockId;
    return this->Header[idx];
  }
  // Description:
  // Append the data to the buffer. Return the index where
  // the data was written.
  vtkIdType Pack(const double* pData, int nComps, vtkIdType nTups);
  vtkIdType Pack(const float* pData, int nComps, vtkIdType nTups);
  vtkIdType Pack(const int* pData, int nComps, vtkIdType nTups);
  vtkIdType Pack(vtkDoubleArray* da);
  vtkIdType Pack(vtkFloatArray* da);
  // Description:
  // Prepare to un pack the buffer.
  void InitUnpack() { this->EOD = 0; }
  // Description:
  // Extract the next array from the buffer
  int UnPack(double*& rData, int nComps, vtkIdType nTups, bool copyFlag);
  int UnPack(float*& rData, int nComps, vtkIdType nTups, bool copyFlag);
  int UnPack(int*& rData, int nComps, vtkIdType nTups, bool copyFlag);
  int UnPack(vtkDoubleArray* da, int nComps, vtkIdType nTups, bool copyFlag);
  int UnPack(vtkFloatArray* da, int nComps, vtkIdType nTups, bool copyFlag);
  // static void Resize(std::vector<vtkMaterialInterfaceCommBuffer> &buffers);
private:
  vtkIdType EOD;
  char* Buffer;
  int HeaderSize;
  vtkIdType* Header;
};
#endif
