/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositeUtilities.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCompositeUtilities - Compression and composite buffers.
//
// .SECTION Description
// This is going  to handle buffer management, compressing buffers
// and compositing buffer pairs. ......
//
// .SECTION See Also
// vtkPVCompositeBuffer


#ifndef __vtkPVCompositeUtilities_h
#define __vtkPVCompositeUtilities_h

#include "vtkObject.h"

class vtkCollection;
class vtkDataArray;
class vtkUnsignedCharArray;
class vtkFloatArray;
class vtkPVCompositeBuffer;
class vtkMultiProcessController;

class VTK_EXPORT vtkPVCompositeUtilities : public vtkObject
{
public:
  static vtkPVCompositeUtilities *New();
  vtkTypeRevisionMacro(vtkPVCompositeUtilities,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get a data array of the specified size and type.
  // This will be used to get z and color buffers.
  vtkFloatArray *NewFloatArray(int numTuples, int numComponents);
  vtkUnsignedCharArray *NewUnsignedCharArray(int numTuples, 
                                             int numComponents);

  // Description:
  // This will return a special object to hold a compressed buffer.
  vtkPVCompositeBuffer* NewCompositeBuffer(int numPixels);

  // Description:
  // This is used to create a buffer when not using compositing.
  vtkPVCompositeBuffer* NewCompositeBuffer(
                                     vtkUnsignedCharArray* pData,
                                     vtkFloatArray* zData);

  // Description:
  // Send a buffer in one easy call.
  static void SendBuffer(vtkMultiProcessController* controller,
                    vtkPVCompositeBuffer* buf, int otherProc, int tag); 

  // Description:
  // Receive a buffer in one easy call.  The caller is responsible
  // for deleting the returned buffer.
  vtkPVCompositeBuffer* ReceiveNewBuffer(vtkMultiProcessController* controller,
                                         int otherProc, int tag);

  // Description:
  // I am granting access to these methods and making them static
  // So I can create a TileDisplayCompositer which uses compression.
  static void Compress(vtkFloatArray *zIn, vtkUnsignedCharArray *pIn,
                       vtkPVCompositeBuffer* outBuf);

  // Description:
  // This method predicts the length of a compressed buffer.
  // It is used to get the best size array for compressed buffer.
  static int GetCompressedLength(vtkFloatArray* zIn);

  static void Uncompress(vtkPVCompositeBuffer* inBuf,
                         vtkUnsignedCharArray *pOut);

  // Description:
  // This method returns a conservative guess at the 
  // output length after two bufferes are composited.
  static int GetCompositedLength(vtkPVCompositeBuffer* b1,
                                 vtkPVCompositeBuffer* b2);

  static void CompositeImagePair(vtkPVCompositeBuffer* inBuf1,
                                 vtkPVCompositeBuffer* inBuf2,
                                 vtkPVCompositeBuffer* outBuf);

  static void MagnifyBuffer(vtkDataArray* in, vtkDataArray* out, 
                            int inWinSize[2], int factor);

  // Description:
  // The maximum amount of memory (kB) that the buffers can use.
  // If this value is set too low, then memory will be reallocated
  // as needed.
  vtkSetMacro(MaximumMemoryUsage, unsigned long);
  vtkGetMacro(MaximumMemoryUsage, unsigned long);

  // Description:
  // Access for debugging purposes.
  unsigned long GetTotalMemoryUsage();

protected:
  vtkPVCompositeUtilities();
  ~vtkPVCompositeUtilities();
  
  vtkCollection* FloatArrayCollection;
  vtkCollection* UnsignedCharArrayCollection;

  unsigned long MaximumMemoryUsage;
  unsigned long FloatMemoryUsage;
  unsigned long UnsignedCharMemoryUsage;

  // Returns memory freed up in kB.
  int RemoveOldestUnused(vtkCollection* arrayCollection);

private:
  vtkPVCompositeUtilities(const vtkPVCompositeUtilities&); // Not implemented
  void operator=(const vtkPVCompositeUtilities&); // Not implemented
};

#endif
