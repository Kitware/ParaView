/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVCompositeBuffer.h
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

protected:
  vtkPVCompositeBuffer();
  ~vtkPVCompositeBuffer();
  
//BTX
  friend class vtkPVCompositeUtilities;
//ETX

  vtkFloatArray* ZData;
  vtkUnsignedCharArray* PData;

private:
  vtkPVCompositeBuffer(const vtkPVCompositeBuffer&); // Not implemented
  void operator=(const vtkPVCompositeBuffer&); // Not implemented
};

#endif
