/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVCompositeUtilities.h
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
// .NAME vtkPVCompositeUtilities - Compression and composite buffers.
// .SECTION Description
// This is going  to handle buffer managment, compressing buffers
// and compositing buffer pairs. ......


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
  // We are creating this object for tiled displays initialially.
  // This would be the full size of the render window.
  // This variable is not used currently, but could be used for a minimum
  // buffer size.
  //vtkSetVector2Macro(Size, int);

  // Description:
  // Get a data array of the specified size and type.
  // This will be used to get z and color buffers.
  vtkFloatArray *NewFloatArray(int numTuples, int numComponents);
  vtkUnsignedCharArray *NewUnsignedCharArray(int numTuples, 
                                             int numComponents);

  // Description:
  // This will return a special object to hold a compressed buffer.
  vtkPVCompositeBuffer* NewCompositeBuffer(int numPixels);


  static void SendBuffer(vtkMultiProcessController* controller,
                    vtkPVCompositeBuffer* buf, int otherProc, int tag); 

  vtkPVCompositeBuffer* ReceiveNewBuffer(vtkMultiProcessController* controller,
                                         int otherProc, int tag);


  // Description:
  // I am granting access to these methods and making them static
  // So I can create a TileDisplayCompositer which uses compression.
  static void Compress(vtkFloatArray *zIn, vtkUnsignedCharArray *pIn,
                       vtkPVCompositeBuffer* outBuf);

  static void Uncompress(vtkPVCompositeBuffer* inBuf,
                         vtkUnsignedCharArray *pOut);

  static void CompositeImagePair(vtkPVCompositeBuffer* inBuf1,
                                 vtkPVCompositeBuffer* inBuf2,
                                 vtkPVCompositeBuffer* outBuf);

  static void MagnifyBuffer(vtkDataArray* in, vtkDataArray* out, 
                            int inWinSize[2], int factor);


protected:
  vtkPVCompositeUtilities();
  ~vtkPVCompositeUtilities();
  
  vtkCollection* FloatArrayCollection;
  vtkCollection* UnsignedCharArrayCollection;

private:
  vtkPVCompositeUtilities(const vtkPVCompositeUtilities&); // Not implemented
  void operator=(const vtkPVCompositeUtilities&); // Not implemented
};

#endif
