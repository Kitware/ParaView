/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositeUtilities.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifdef VTK_USE_MPI
 #include <mpi.h>
#endif

#include "vtkCompositer.h"
#include "vtkPVCompositeUtilities.h"
#include "vtkMultiProcessController.h"
#include "vtkPVCompositeBuffer.h"
#include "vtkObjectFactory.h"
#include "vtkCollection.h"
#include "vtkFloatArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkTimerLog.h"

vtkCxxRevisionMacro(vtkPVCompositeUtilities, "1.10");
vtkStandardNewMacro(vtkPVCompositeUtilities);


// Different pixel types to template.
typedef struct {
  unsigned char r;
  unsigned char g;
  unsigned char b;
} vtkCharRGBType;

typedef struct {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
} vtkCharRGBAType;

typedef struct {
  float r;
  float g;
  float b;
  float a;
} vtkFloatRGBAType;



//-------------------------------------------------------------------------
vtkPVCompositeUtilities::vtkPVCompositeUtilities()
{
  this->FloatArrayCollection = vtkCollection::New();
  this->UnsignedCharArrayCollection = vtkCollection::New();
  this->MaximumMemoryUsage = 500000; // 500MB
  this->FloatMemoryUsage = 0;
  this->UnsignedCharMemoryUsage = 0;
}

  
//-------------------------------------------------------------------------
vtkPVCompositeUtilities::~vtkPVCompositeUtilities()
{
  if (this->FloatArrayCollection)
    {
    this->FloatArrayCollection->Delete();
    this->FloatArrayCollection = NULL;
    }
  if (this->UnsignedCharArrayCollection)
    {
    this->UnsignedCharArrayCollection->Delete();
    this->UnsignedCharArrayCollection = NULL;
    }
}


//-------------------------------------------------------------------------
vtkFloatArray* vtkPVCompositeUtilities::NewFloatArray(int numTuples,
                                                      int numComps)
{
  vtkFloatArray* best = NULL;
  int bestLength = 0;
  vtkFloatArray* a;
  int length;
  vtkObject* o;
  int target = numTuples * numComps;
  unsigned long memoryTotal = 0;

  // Search for an appropriate sized array.
  // Has to have the same number of components (all will).
  // Pick the shortest which which is big enough for the requested size.
  this->FloatArrayCollection->InitTraversal();
  while ( (o = this->FloatArrayCollection->GetNextItemAsObject()))
    {
    a = vtkFloatArray::SafeDownCast(o);
    memoryTotal += a->GetActualMemorySize();
    if (a->GetReferenceCount() == 1 && 
        a->GetNumberOfComponents() == numComps)
      {
      length = a->GetSize();
      if (length >= target)
        { // This is a viable candidate.
        if (best == NULL || bestLength > length)
          {
          best = a;
          bestLength = length;
          }
        }
      }
    }

  if (best == NULL)
    {
    // Allocate a new array.
    best = vtkFloatArray::New();
    // what should be our default size?
    best->SetNumberOfComponents(numComps);
    best->SetNumberOfTuples(numTuples);
    vtkCompositer::ResizeFloatArray(best, 
                                    numComps, numTuples);
    memoryTotal += best->GetActualMemorySize();
    this->FloatArrayCollection->AddItem(best);
    }
  else
    {
    best->Register(NULL);
    best->SetNumberOfTuples(numTuples);
    }

  // Modify to keep track of oldest.
  best->Modified();

  int removedMemory = 1;
  while (memoryTotal+this->FloatMemoryUsage > this->MaximumMemoryUsage &&
         removedMemory)
    {
    removedMemory = this->RemoveOldestUnused(this->FloatArrayCollection);
    memoryTotal = memoryTotal - removedMemory;
    }

  this->FloatMemoryUsage = memoryTotal;
  return best;
}

//-------------------------------------------------------------------------
vtkUnsignedCharArray* 
vtkPVCompositeUtilities::NewUnsignedCharArray(int numTuples,
                                              int numComps)
{
  vtkUnsignedCharArray* best = NULL;
  int bestLength = 0;
  vtkUnsignedCharArray* a;
  int length;
  vtkObject* o;
  int target = numTuples * numComps;
  unsigned long memoryTotal = 0;

  // Search for an appropriate sized array.
  // Has to have the same number of components (all will).
  // Pick the shortest which which is big enough for the requested size.
  this->UnsignedCharArrayCollection->InitTraversal();
  while ( (o = this->UnsignedCharArrayCollection->GetNextItemAsObject()))
    {
    a = vtkUnsignedCharArray::SafeDownCast(o);
    memoryTotal += a->GetActualMemorySize();
    if (a->GetReferenceCount() == 1 && 
        a->GetNumberOfComponents() == numComps)
      {
      length = a->GetSize();
      if (length >= target)
        { // This is a viable candidate.
        if (best == NULL || bestLength > length)
          {
          best = a;
          bestLength = length;
          }
        }
      }
    }

  if (best == NULL)
    {
    // Allocate a new array.
    best = vtkUnsignedCharArray::New();
    // what should be our default size?
    best->SetNumberOfComponents(numComps);
    best->SetNumberOfTuples(numTuples);
    vtkCompositer::ResizeUnsignedCharArray(best, 
                                           numComps, numTuples);

    memoryTotal += best->GetActualMemorySize();
    this->UnsignedCharArrayCollection->AddItem(best);
    }
  else
    {
    best->Register(NULL);
    best->SetNumberOfTuples(numTuples);
    }

  // Modify to keep track of oldest.
  best->Modified();

  int removedMemory = 1;
  while (memoryTotal+this->UnsignedCharMemoryUsage > this->MaximumMemoryUsage &&
         removedMemory)
    {
    removedMemory = this->RemoveOldestUnused(this->UnsignedCharArrayCollection);
    memoryTotal = memoryTotal - removedMemory;
    }

  this->UnsignedCharMemoryUsage = memoryTotal;
  return best;
}

//-------------------------------------------------------------------------
int vtkPVCompositeUtilities::RemoveOldestUnused(vtkCollection* arrayCollection)
{
  vtkObject* o;
  vtkDataArray* a;
  vtkDataArray* oldest = NULL;
  int memory;

  arrayCollection->InitTraversal();
  while ( (o = arrayCollection->GetNextItemAsObject()) )
    {
    a = vtkDataArray::SafeDownCast(o);
    if (a->GetReferenceCount() == 1)
      {
      if (oldest == NULL || a->GetMTime() < oldest->GetMTime())
        {
        oldest = a;
        }
      }
    }

  if (oldest)
    {
    memory = oldest->GetActualMemorySize();
    arrayCollection->RemoveItem(oldest);
    return memory;
    }
  return 0;
}



//-------------------------------------------------------------------------
vtkPVCompositeBuffer* 
vtkPVCompositeUtilities::NewCompositeBuffer(int numPixels)
{
  vtkPVCompositeBuffer* b = vtkPVCompositeBuffer::New();
  
  // RGB
  b->PData = this->NewUnsignedCharArray(numPixels, 3);
  b->ZData = this->NewFloatArray(numPixels, 1);

  return b;
}


//-------------------------------------------------------------------------
vtkPVCompositeBuffer* 
vtkPVCompositeUtilities::NewCompositeBuffer(vtkUnsignedCharArray* pData,
                                            vtkFloatArray* zData)
{
  if (pData == NULL || zData == NULL)
    {
    vtkErrorMacro("Missing array.");
    return NULL;
    }

  vtkPVCompositeBuffer* b = vtkPVCompositeBuffer::New();
  
  // RGB
  b->PData = pData;
  b->ZData = zData;
  pData->Register(this);
  zData->Register(this);

  b->UncompressedLength = pData->GetNumberOfTuples();
  if (b->UncompressedLength != zData->GetNumberOfTuples())
    {
    vtkErrorMacro("Inconsistent number of pixels.");
    }

  return b;
}


//-------------------------------------------------------------------------
void vtkPVCompositeUtilities::SendBuffer(vtkMultiProcessController* controller,
                                         vtkPVCompositeBuffer* buf, 
                                         int otherProc, int tag) 
{
  int lengths[2];
  lengths[0] = buf->PData->GetNumberOfTuples();
  lengths[1] = buf->UncompressedLength;

  //cout << "Send " << otherProc << ", " << tag << endl;

  controller->Send(lengths, 2, otherProc, tag);
  controller->Send(buf->ZData->GetPointer(0), lengths[0], otherProc, tag*2);
  controller->Send(buf->PData->GetPointer(0), lengths[0]*3, otherProc, tag*2);
}


//-------------------------------------------------------------------------
vtkPVCompositeBuffer* vtkPVCompositeUtilities::ReceiveNewBuffer(
                                      vtkMultiProcessController* controller,
                                      int otherProc, int tag)
{
  int lengths[2];
  vtkPVCompositeBuffer *buf;

  //cout << "Recv " << otherProc << ", " << tag << endl;

  controller->Receive(lengths, 2, otherProc, tag);
  buf = this->NewCompositeBuffer(lengths[0]);
  controller->Receive(buf->ZData->GetPointer(0), lengths[0], otherProc, tag*2);
  controller->Receive(buf->PData->GetPointer(0), lengths[0]*3, otherProc, tag*2);
  buf->UncompressedLength = lengths[1];

  return buf;
}



//-------------------------------------------------------------------------
int vtkPVCompositeUtilities::GetCompressedLength(vtkFloatArray *zArray)
{
  int numPixels;
  float* zRun;
  float* zIn;
  float* endZ;
  int length = 0;

  numPixels = zArray->GetNumberOfTuples();
  zIn = zArray->GetPointer(0);
  // Do not go past the last pixel (zbuf check/correct)
  endZ = zIn+numPixels-1;

  if (*zIn < 0.0 || *zIn > 1.0)
    {
    *zIn = 1.0;
    } 
  while (zIn < endZ)
    {
    ++length;
    zRun = zIn;
    // Skip over compressed runs.
    while (*zIn == 1.0 && zIn < endZ)
      {
      ++zIn;
      if (*zIn < 0.0 || *zIn > 1.0)
        {
        *zIn = 1.0;
        } 
      }

    if (zRun == zIn)
      { 
      //*zIn++;
      zIn++;
      if (*zIn < 0.0 || *zIn > 1.0)
        {
        *zIn = 1.0;
        } 
      }
    }
  // 1 more for last pixel.
  ++length;

  zIn = zArray->GetPointer(0);

  // This uncovered a bug with the NVidia drivers.
  // Getting the buffere of a sub window messed up the zbuffer.
  //fprintf(stdout, "Compress (%.1f,%.1f,%.1f,%.1f) %d, %d)\n",
  //        zIn[0],zIn[1],zIn[2],zIn[3],zArray->GetNumberOfTuples(), length);

  return length;
}

//-------------------------------------------------------------------------
// Compress background pixels with runlength encoding.
// z values above 1.0 mean: Repeat background for that many pixels.
// We could easily compress inplace, but it works out better for buffer 
// managment if we do not.  zIn == zOut is allowed....
template <class P>
int vtkPVCompositeUtilitiesCompress(float *zIn, P *pIn, float *zOut, P *pOut,
                                  int numPixels)
{
  float* endZ;
  int length = 0;
  int compressCount;

  // Do not go past the last pixel (zbuf check/correct)
  endZ = zIn+numPixels-1;

  if (*zIn < 0.0 || *zIn > 1.0)
    {
    *zIn = 1.0;
    } 
  while (zIn < endZ)
    {
    ++length;
    // Always copy the first pixel value.
    *pOut++ = *pIn++;
    // Find the length of any compressed run.
    compressCount = 0;
    while (*zIn == 1.0 && zIn < endZ)
      {
      ++compressCount;
      ++zIn;
      if (*zIn < 0.0 || *zIn > 1.0)
        {
        *zIn = 1.0;
        } 
      }
 
    if (compressCount > 0)
      { // Only compress runs of 2 or more.
      // Move the pixel pointer past compressed region.
      pIn += (compressCount-1);
      // Set the special z value.
      *zOut++ = (float)(compressCount);
      }
    else
      { 
      *zOut++ = *zIn++;
      if (*zIn < 0.0 || *zIn > 1.0)
        {
        *zIn = 1.0;
        } 
      }
    }
  // Put the last pixel in.
  *pOut = *pIn;
  *zOut = *zIn;

  return length;
}

//-------------------------------------------------------------------------
// Compress background pixels with runlength encoding.
// z values above 1.0 mean: Repeat background for that many pixels.
// We could easily compress inplace, but it works out better for buffer 
// managment if we do not.  zIn == zOut is allowed....
void vtkPVCompositeUtilities::Compress(vtkFloatArray *zIn, vtkUnsignedCharArray *pIn,
                                       vtkPVCompositeBuffer* outBuf)
{
  float* pzf1 = zIn->GetPointer(0);
  float* pzf2 = outBuf->ZData->GetPointer(0);
  void*  ppv1 = pIn->GetVoidPointer(0);
  void*  ppv2 = outBuf->PData->GetVoidPointer(0);
  int totalPixels = zIn->GetNumberOfTuples();
  int length;
  
  vtkTimerLog::MarkStartEvent("Compress");

  // Store for later use.
  outBuf->UncompressedLength = zIn->GetNumberOfTuples();

  // This is just a complex switch statment 
  // to call the correct templated function.
  if (pIn->GetDataType() == VTK_UNSIGNED_CHAR) 
    {
    if (pIn->GetNumberOfComponents() == 3) 
      {
      length = vtkPVCompositeUtilitiesCompress(
        pzf1, reinterpret_cast<vtkCharRGBType*>(ppv1),
        pzf2, reinterpret_cast<vtkCharRGBType*>(ppv2),
        totalPixels);
      }
    else if (pIn->GetNumberOfComponents() == 4) 
      {
      length = vtkPVCompositeUtilitiesCompress(
        pzf1, reinterpret_cast<vtkCharRGBAType*>(ppv1),
        pzf2, reinterpret_cast<vtkCharRGBAType*>(ppv2),
        totalPixels);
      }
    else 
      {
      vtkGenericWarningMacro("Pixels have unexpected number of components.");
      return;
      }
    }
  //else if (pIn->GetDataType() == VTK_FLOAT && 
  //         pIn->GetNumberOfComponents() == 4) 
  //  {
  //  length = vtkPVCompositeUtilitiesCompress(
  //    pzf1, reinterpret_cast<vtkFloatRGBAType*>(ppv1),
  //    pzf2, reinterpret_cast<vtkFloatRGBAType*>(ppv2),
  //    totalPixels);
  //  }
  else
    {
    vtkGenericWarningMacro("Unexpected pixel type.");
    return;
    }

  // Sanity check
  if (outBuf->ZData->GetSize() < length)
    {
    vtkGenericWarningMacro("Buffer too small.");
    }

  outBuf->ZData->SetNumberOfTuples(length);
  outBuf->PData->SetNumberOfTuples(length);

  vtkTimerLog::MarkEndEvent("Compress");
}

//-------------------------------------------------------------------------
//  z values above 1.0 mean: Repeat background for that many pixels.
// Assume that the array has enough allocated space for the uncompressed.
// In place/reverse order.
template <class P>
void vtkPVCompositeUtilitiesUncompress(float *zIn, P *pIn, P *pOut, int lengthIn)
{
  float* endZ;
  int count;
  P background;
  
  endZ = zIn + lengthIn;

  while (zIn < endZ)
    {
    // Expand any compressed data.
    if (*zIn > 1.0)
      {
      background = *pIn++;
      count = (int)(*zIn++);
      while (count-- > 0)
        {
        *pOut++ = background;
        //*zOut++ = 1.0;
        }
      }
    else
      {
      *pOut++ = *pIn++;
      //*zOut++ = *zIn++;
      ++zIn;
      }
    }
}

//-------------------------------------------------------------------------
// Compress background pixels with runlength encoding.
// z values above 1.0 mean: Repeat background for that many pixels.
// We could easily compress inplace, but it works out better for buffer 
// managment if we do not.  zIn == zOut is allowed....
void vtkPVCompositeUtilities::Uncompress(vtkPVCompositeBuffer *inBuf,
                                         vtkUnsignedCharArray *pOut)
{
  float* pzf1 = inBuf->ZData->GetPointer(0);
  void*  ppv1 = inBuf->PData->GetVoidPointer(0);
  void*  ppv2 = pOut->GetVoidPointer(0);
  int lengthIn = inBuf->ZData->GetNumberOfTuples();

  // Sanity check
  if (pOut->GetSize() < inBuf->UncompressedLength*3)
    {
    vtkGenericWarningMacro("Buffer too small.");
    }

  vtkTimerLog::MarkStartEvent("Uncompress");

  // This is just a complex switch statment 
  // to call the correct templated function.
  if (inBuf->PData->GetDataType() == VTK_UNSIGNED_CHAR) 
    {
    if (inBuf->PData->GetNumberOfComponents() == 3) 
      {
      vtkPVCompositeUtilitiesUncompress(pzf1, 
                                      reinterpret_cast<vtkCharRGBType*>(ppv1),
                                      reinterpret_cast<vtkCharRGBType*>(ppv2),
                                      lengthIn);
      }
    else if (inBuf->PData->GetNumberOfComponents() == 4) 
      {
      vtkPVCompositeUtilitiesUncompress(pzf1, 
                                      reinterpret_cast<vtkCharRGBAType*>(ppv1),
                                      reinterpret_cast<vtkCharRGBAType*>(ppv2),
                                      lengthIn);
      }
    else 
      {
      vtkGenericWarningMacro("Pixels have unexpected number of components.");
      return;
      }
    }
  else if (inBuf->PData->GetDataType() == VTK_FLOAT && 
           inBuf->PData->GetNumberOfComponents() == 4) 
    {
    vtkPVCompositeUtilitiesUncompress(pzf1, 
                                    reinterpret_cast<vtkFloatRGBAType*>(ppv1),
                                    reinterpret_cast<vtkFloatRGBAType*>(ppv2),
                                    lengthIn);
    }
  else
    {
    vtkGenericWarningMacro("Unexpected pixel type.");
    return;
    }

  vtkTimerLog::MarkEndEvent("Uncompress");
}




//-------------------------------------------------------------------------
// Can handle compositing compressed buffers.
// z values above 1.0 mean: Repeat background for that many pixels.
template <class P>
int vtkPVCompositeUtilitiesCompositePair(float *z1, P *p1, float *z2, P *p2,
                                       float *zOut, P *pOut, int length1)
{
  float* startZOut = zOut;
  float* endZ1;
  // These counts keep track of the length of compressed runs.
  // Value -1 means pointer is not on a compression run.
  // Value 0 means pointer is on a used up compression run.
  int cCount1 = 0;
  int cCount2 = 0;
  int cCount3;
  int length3;
  
  // This is for the end test.
  // We are assuming that the uncompressed buffer length of 1 and 2 
  // are the same.
  endZ1 = z1 + length1;

  while(z1 != endZ1) 
    {
    // Initialize a new state if necessary.
    if (cCount1 == 0 && *z1 > 1.0)
      { // Detect a new run in buffer 1.
      cCount1 = (int)(*z1);
      }
    if (cCount2 == 0 && *z2 > 1.0)
      { // Detect a new run in buffer 2.
      cCount2 = (int)(*z2);
      }
       
    // Case 1: Neither buffer is compressed.
    // We could keep the length of uncompressed runs ...
    if (cCount1 == 0 && cCount2 == 0)
      {
      // Loop through buffers doing standard compositing.
      while (*z1 <= 1.0 && *z2 <= 1.0 && z1 != endZ1)
        {
        if (*z1 < *z2)
          {
          *zOut++ = *z1++;
          ++z2;
          *pOut++ = *p1++;
          ++p2;
          }
        else
          {            
          *zOut++ = *z2++;
          ++z1;
          *pOut++ = *p2++;
          ++p1;
          }
        }
      // Let the next iteration determine the new state (counts).
      }
    else if (cCount1 > 0 && cCount2 > 0)
      { // segment where both are compressed
      // Pick the smaller compressed run an duplicate in output.
      cCount3 = (cCount1 < cCount2) ? cCount1 : cCount2;
      cCount2 -= cCount3;
      cCount1 -= cCount3;
      // Set the output pixel.
      *zOut++ = (float)(cCount3);
      // either pixel will do.
      *pOut++ = *p1;
      if (cCount1 == 0)
        {
        ++z1;
        ++p1;
        }
      if (cCount2 == 0)
        {
        ++z2;
        ++p2;
        }
      }
    else if (cCount1 > 0 && cCount2 == 0)
      { //1 is in a compressed run but 2 is not.
      // Copy from 2 until we hit a compressed region, 
      // or we run out of the 1 compressed run.
      while (cCount1 && *z2 <= 1.0)
        {
        *zOut++ = *z2++;
        *pOut++ = *p2++;
        --cCount1;
        }
      if (cCount1 == 0)
        {
        ++z1;
        ++p1;
        }
      }
    else if (cCount1 == 0 && cCount2 > 0)
      { //2 is in a compressed run but 1 is not.
      // Copy from 1 until we hit a compressed region, 
      // or we run out of the 2 compressed run.
      while (cCount2 && *z1 <= 1.0)
        {
        *zOut++ = *z1++;
        *pOut++ = *p1++;
        --cCount2;
        }
      if (cCount2 == 0)
        {
        ++z2;
        ++p2;
        }
      } // end case if.
    } // while not finished (process cases).
  // Here is a scary way to determine the length of the new buffer.
  length3 = zOut - startZOut;

  return length3;
}
         
//-------------------------------------------------------------------------
// Can handle compositing compressed buffers.
// z values above 1.0 mean: Repeat background for that many pixels.
void vtkPVCompositeUtilities::CompositeImagePair(vtkPVCompositeBuffer* inBuf1,
                                               vtkPVCompositeBuffer* inBuf2,
                                               vtkPVCompositeBuffer* outBuf)
{
  float* z1 = inBuf1->ZData->GetPointer(0);
  float* z2 = inBuf2->ZData->GetPointer(0);
  float* z3 = outBuf->ZData->GetPointer(0);
  void*  p1 = inBuf1->PData->GetVoidPointer(0);
  void*  p2 = inBuf2->PData->GetVoidPointer(0);
  void*  p3 = outBuf->PData->GetVoidPointer(0);
  int length1 = inBuf1->ZData->GetNumberOfTuples();
  int l3;

  // Transfer the uncomposited length.
  if (inBuf1->UncompressedLength != inBuf2->UncompressedLength)
    {
    vtkGenericWarningMacro("Compositing buffers of different sizes.");
    }
  outBuf->UncompressedLength = inBuf1->UncompressedLength;
  
  //vtkTimerLog::MarkStartEvent("Composite Image Pair");

  // This is just a complex switch statment 
  // to call the correct templated function.
  if (inBuf1->PData->GetDataType() == VTK_UNSIGNED_CHAR) 
    {
    if (inBuf1->PData->GetNumberOfComponents() == 3) 
      {
      l3 = vtkPVCompositeUtilitiesCompositePair(
        z1, reinterpret_cast<vtkCharRGBType*>(p1),
        z2, reinterpret_cast<vtkCharRGBType*>(p2),
        z3, reinterpret_cast<vtkCharRGBType*>(p3),
                                              length1);
      }
    else if (inBuf1->PData->GetNumberOfComponents() == 4) 
      {
      l3 = vtkPVCompositeUtilitiesCompositePair(
        z1, reinterpret_cast<vtkCharRGBAType*>(p1),
        z2, reinterpret_cast<vtkCharRGBAType*>(p2),
        z3, reinterpret_cast<vtkCharRGBAType*>(p3),
        length1);
      }
    else 
      {
      vtkGenericWarningMacro("Pixels have unexpected number of components.");
      return;
      }
    }
  else if (inBuf1->PData->GetDataType() == VTK_FLOAT && 
           inBuf1->PData->GetNumberOfComponents() == 4) 
    {
    l3 = vtkPVCompositeUtilitiesCompositePair(
      z1, reinterpret_cast<vtkFloatRGBAType*>(p1),
      z2, reinterpret_cast<vtkFloatRGBAType*>(p2),
      z3, reinterpret_cast<vtkFloatRGBAType*>(p3),
      length1);
    }
  else
    {
    vtkGenericWarningMacro("Unexpected pixel type.");
    return;
    }

  // Sanity check
  if (outBuf->ZData->GetSize() < l3)
    {
    vtkGenericWarningMacro("Buffer too small.");
    }

  outBuf->ZData->SetNumberOfTuples(l3); // length3 not 13
  outBuf->PData->SetNumberOfTuples(l3);

  //vtkTimerLog::MarkEndEvent("Composite Image Pair");
}

//-------------------------------------------------------------------------
int vtkPVCompositeUtilities::GetCompositedLength(vtkPVCompositeBuffer* b1,
                                                 vtkPVCompositeBuffer* b2)
{
  int total;

  if (b1->UncompressedLength < 0 || b2->UncompressedLength < 0)
    {
    vtkGenericWarningMacro("Buffers uncompressed length has not been set.");
    }
  if (b1->UncompressedLength != b2->UncompressedLength)
    {
    vtkGenericWarningMacro("Buffers have different lengths.");
    }

  total = b1->ZData->GetNumberOfTuples() + b2->ZData->GetNumberOfTuples();
  if (total > b1->UncompressedLength)
    {
    total = b1->UncompressedLength;
    }

  return total;
}

//----------------------------------------------------------------------------
// We change this to work backwards so we can make it inplace. !!!!!!!     
void vtkPVCompositeUtilities::MagnifyBuffer(vtkDataArray* localP, 
                                            vtkDataArray* magP,
                                            int inWinSize[2],
                                            int factor)
{
  float *rowp, *subp;
  float *pp1;
  float *pp2;
  int   x, y, xi, yi;
  int   xInDim, yInDim;
  // Local increments for input.
  int   pInIncY; 
  float *newLocalPData;
  int numComp = localP->GetNumberOfComponents();
  
  xInDim = inWinSize[0];
  yInDim = inWinSize[1];

  // Sanity check
  if (magP->GetSize() < xInDim*yInDim*3)
    {
    vtkGenericWarningMacro("Buffer too small.");
    }

  newLocalPData = reinterpret_cast<float*>(magP->GetVoidPointer(0));
  float* localPdata = reinterpret_cast<float*>(localP->GetVoidPointer(0));

  if (localP->GetDataType() == VTK_UNSIGNED_CHAR)
    {
    if (numComp == 4)
      {
      // Get the last pixel.
      rowp = localPdata;
      pp2 = newLocalPData;
      for (y = 0; y < yInDim; y++)
        {
        // Duplicate the row rowp N times.
        for (yi = 0; yi < factor; ++yi)
          {
          pp1 = rowp;
          for (x = 0; x < xInDim; x++)
            {
            // Duplicate the pixel p11 N times.
            for (xi = 0; xi < factor; ++xi)
              {
              *pp2++ = *pp1;
              }
            ++pp1;
            }
          }
        rowp += xInDim;
        }
      }
    else if (numComp == 3)
      { // RGB char pixel data.
      // Get the last pixel.
      pInIncY = numComp * xInDim;
      unsigned char* crowp = reinterpret_cast<unsigned char*>(localPdata);
      unsigned char* cpp2 = reinterpret_cast<unsigned char*>(newLocalPData);
      unsigned char *cpp1, *csubp;
      for (y = 0; y < yInDim; y++)
        {
        // Duplicate the row rowp N times.
        for (yi = 0; yi < factor; ++yi)
          {
          cpp1 = crowp;
          for (x = 0; x < xInDim; x++)
            {
            // Duplicate the pixel p11 N times.
            for (xi = 0; xi < factor; ++xi)
              {
              csubp = cpp1;
              *cpp2++ = *csubp++;
              *cpp2++ = *csubp++;
              *cpp2++ = *csubp;
              }
            cpp1 += numComp;
            }
          }
        crowp += pInIncY;
        }
      }
    }
  else
    {
    // Get the last pixel.
    pInIncY = numComp * xInDim;
    rowp = localPdata;
    pp2 = newLocalPData;
    for (y = 0; y < yInDim; y++)
      {
      // Duplicate the row rowp N times.
      for (yi = 0; yi < factor; ++yi)
        {
        pp1 = rowp;
        for (x = 0; x < xInDim; x++)
          {
          // Duplicate the pixel p11 N times.
          for (xi = 0; xi < factor; ++xi)
            {
            subp = pp1;
            if (numComp == 4)
              {
              *pp2++ = *subp++;
              }
            *pp2++ = *subp++;
            *pp2++ = *subp++;
            *pp2++ = *subp;
            }
          pp1 += numComp;
          }
        }
      rowp += pInIncY;
      }
    }
  
}
  
//----------------------------------------------------------------------------
unsigned long vtkPVCompositeUtilities::GetTotalMemoryUsage()
{
  unsigned long         arrayMemory;
  unsigned long         totalMemory = 0;
  vtkFloatArray*        floatArray;
  vtkUnsignedCharArray* ucharArray;

  this->FloatArrayCollection->InitTraversal();
  while( (floatArray = (vtkFloatArray*)(this->FloatArrayCollection->GetNextItemAsObject())) )
    {
    arrayMemory = floatArray->GetActualMemorySize();
    totalMemory += arrayMemory;
    }

  this->UnsignedCharArrayCollection->InitTraversal();
  while( (ucharArray = (vtkUnsignedCharArray*)(this->UnsignedCharArrayCollection->GetNextItemAsObject())) )
    {
    arrayMemory = ucharArray->GetActualMemorySize();
    totalMemory += arrayMemory;
    }

  return totalMemory;
}


//----------------------------------------------------------------------------
void vtkPVCompositeUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);


  unsigned long         arrayMemory;
  unsigned long         totalMemory = 0;
  vtkFloatArray*        floatArray;
  vtkUnsignedCharArray* ucharArray;

  this->FloatArrayCollection->InitTraversal();
  while( (floatArray = (vtkFloatArray*)(this->FloatArrayCollection->GetNextItemAsObject())) )
    {
    arrayMemory = floatArray->GetActualMemorySize();
    os << indent << "Float Array: " << arrayMemory << " kB\n";
    totalMemory += arrayMemory;
    }

  this->UnsignedCharArrayCollection->InitTraversal();
  while( (ucharArray = (vtkUnsignedCharArray*)(this->UnsignedCharArrayCollection->GetNextItemAsObject())) )
    {
    arrayMemory = ucharArray->GetActualMemorySize();
    os << indent << "Unsigned Char Array: " << arrayMemory << " kB\n";
    totalMemory += arrayMemory;
    }

  os << "Total Memory Usage: " << totalMemory << " kB \n";
  os << "Maximum Memory Usage: " << this->MaximumMemoryUsage << " kB \n";
}



