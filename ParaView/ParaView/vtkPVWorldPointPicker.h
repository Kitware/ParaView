/*=========================================================================

  Program:   ParaView
  Module:    vtkPVWorldPointPicker.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkPVWorldPointPicker - Pciker to use with parallel compositing.
// .SECTION Description
// vtkPVWorldPointPicker uses the compositied z buffer to select the world point.

// .SECTION See Also
// vtkWorldPointPicker vtkTreeComposite

#ifndef __vtkPVWorldPointPicker_h
#define __vtkPVWorldPointPicker_h

#include "vtkWorldPointPicker.h"
#include "vtkPVTreeComposite.h"


class VTK_EXPORT vtkPVWorldPointPicker : public vtkWorldPointPicker
{
public:
  static vtkPVWorldPointPicker *New();
  vtkTypeMacro(vtkPVWorldPointPicker,vtkWorldPointPicker);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // To use compositied z buffer value, we must have access to the compositer.
  vtkSetObjectMacro(Composite, vtkPVTreeComposite);
  vtkGetObjectMacro(Composite, vtkPVTreeComposite);

  // Description:
  // A pick method that uses composited zbuffer.
  int Pick(float selectionX, float selectionY, 
	   float selectionZ, vtkRenderer *renderer);
  
protected:
  vtkPVWorldPointPicker();
  ~vtkPVWorldPointPicker();
  vtkPVWorldPointPicker(const vtkPVWorldPointPicker&) {};
  void operator=(const vtkPVWorldPointPicker&) {};

  vtkPVTreeComposite *Composite;
};

#endif


