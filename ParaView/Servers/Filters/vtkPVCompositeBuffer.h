/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositeBuffer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCompositeBuffer - Holds compressed color and zbuffers.
// .SECTION Description
// This object holds a compressed z and color buffer.
// Right now, this is just a float and char array, but the abstrcation
// allows us to try more interesting strategies in the future.
// This class works in close collaboration with vtkPVCompositeUtilities.
// Only vtkPVCompositeUtilities should contruct and destruct buffers.


#ifndef __vtkPVCompositeBuffer_h
#define __vtkPVCompositeBuffer_h

#include "vtkObject.h"

class vtkUnsignedCharArray;
class vtkFloatArray;

class VTK_EXPORT vtkPVCompositeBuffer : public vtkObject
{
public:
  static vtkPVCompositeBuffer *New();
  vtkTypeRevisionMacro(vtkPVCompositeBuffer,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Access to the color buffer.  Used when the buffer has not been compressed.
  vtkUnsignedCharArray* GetPData();

protected:
  vtkPVCompositeBuffer();
  ~vtkPVCompositeBuffer();
  
//BTX
  friend class vtkPVCompositeUtilities;
//ETX

  int UncompressedLength;

  vtkFloatArray* ZData;
  vtkUnsignedCharArray* PData;

private:
  vtkPVCompositeBuffer(const vtkPVCompositeBuffer&); // Not implemented
  void operator=(const vtkPVCompositeBuffer&); // Not implemented
};

#endif
