/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCTHFragmentCommBuffer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkCTHFragmentCommBuffer_h
#define __vtkCTHFragmentCommBuffer_h

//#include<iostream>
#include<vtkstd/vector>
#include<vtkType.h>

class vtkDoubleArray;
// class vtkCTHFragmentCommBuffer;
// ostream &operator<<(ostream &sout,const vtkCTHFragmentCommBuffer &fcb);

//============================================================================
// Description:
// FIFO buffer.
// Can be used to send all attributes and ids
// in a single send. UnPack in the same order that you
// packed.
class vtkCTHFragmentCommBuffer
{
  public:
    // header layout
    enum {PROC_ID=0,    // sender's id
          BUFFER_SIZE=1,// size of buffer(bytes)
          DESCR_BASE=2}; // index to first descriptor
    //
    vtkCTHFragmentCommBuffer();
    //
    ~vtkCTHFragmentCommBuffer();
    // Description:
    // Initialize for outgoing comm.
    void Initialize(int procId,int nBlocks,int nBytes);
    // Description:
    // Set up for an incoming header.
    void SizeHeader(int nBlocks);
    // Description:
    // Set up for a set of incoming header.
    static void SizeHeader(
            vtkstd::vector<vtkCTHFragmentCommBuffer> &buffers,
            int nBlocks);
    // Description:
    // Initialize the buffer.
    void SizeBuffer(int nBytes);
    // Description:
    // Initialize the buffer from incoming header.
    void SizeBuffer();
    // Description:
    // Free resources set to invalid state.
    void Clear();
    // Description:
    // Get size in bytes of the buffer.
    int GetBufferSize() const{ return this->Header[BUFFER_SIZE]; }
    // Description:
    // Get a pointer to the buffer
    char *GetBuffer() const { return this->Buffer; }
    // Description:
    // Get the byte that the next rerad or write will occur at
    int GetEOD() const { return this->EOD; }
    // Description:
    // Get size in bytes of the header.
    int GetHeaderSize() const{ return this->HeaderSize; }
    // Description:
    // Get a pointer to the header.
    vtkIdType *GetHeader() const { return this->Header; }

    // Description:
    // Set the number of fragments for a given block.
    void SetNumberOfFragments(
            int blockId,
            vtkIdType nFragments)
    {
      int idx=DESCR_BASE+blockId;
      this->Header[idx]=nFragments;
    }
    // Description:
    // Get the number of fragments for a given block
    int GetNumberOfFragments(int blockId) const
    {
      int idx=DESCR_BASE+blockId;
      return this->Header[idx];
    }
    // Description:
    // Append the data to the buffer. Return the index where
    // the data was written.
    int Pack(const double *pData,const int nComps,const int nTups);
    int Pack(const int *pData,const int nComps,const int nTups);
    int Pack(vtkDoubleArray *da);
    // Description:
    // Prepare to un pack th buffer.
    void InitUnpack(){ this->EOD=0; }
    // Description:
    // Extract the next array from the buffer
    int UnPack(double *&rData,const int nComps,const int nTups,const bool copyFlag);
    int UnPack(int *&rData,const int nComps,const int nTups,const bool copyFlag);
    int UnPack(vtkDoubleArray *da,const int nComps,const int nTups,const bool copyFlag);
    static void Resize(vtkstd::vector<vtkCTHFragmentCommBuffer> &buffers);
  private:
    int BufferSize;
    int EOD;
    char *Buffer;
    int HeaderSize;
    vtkIdType *Header;
};

#endif
