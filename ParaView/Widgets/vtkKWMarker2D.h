/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWMarker2D.h
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
// .NAME vtkKWMarker2D - a 2D cursor annotation
// .SECTION Description
// The vtkKWMarker2D class defines a 2D cursor that can be positioned
// anywhere in the view.


#ifndef __vtkKWMarker2D_h
#define __vtkKWMarker2D_h

#include "vtkActor2D.h"

class vtkRenderer;

class VTK_EXPORT vtkKWMarker2D : public vtkActor2D
{
public:
  static vtkKWMarker2D* New();
  vtkTypeRevisionMacro(vtkKWMarker2D,vtkActor2D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the position of the marker.
  void SetPosition(float x, float y);

  // Description:
  // Set color.
  void SetColor(float r, float g, float b);

  // Description:
  // Set renderer.
  void SetRenderer(vtkRenderer* ren) { this->Renderer = ren; }

  // Description:
  // Set visibiliti of the marker.
  void SetVisibility(int i);
  
protected:
  vtkKWMarker2D();
  ~vtkKWMarker2D();

  vtkRenderer* Renderer;

private:
  vtkKWMarker2D(const vtkKWMarker2D&); // Not implemented
  void operator=(const vtkKWMarker2D&); // Not implemented
};

#endif
