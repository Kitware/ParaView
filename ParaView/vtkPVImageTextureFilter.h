/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImageTextureFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkPVImageTextureFilter - Creates polydata and texture.
// .SECTION Description
// vtkPVImageTextureFilter creates polydata and a texture image to 
// Display a volume.  It uses assignement to determine what to display.
// It only works for a 2D image for now, but in the future it could be extended 
// to display the faces of a volume.

#ifndef __vtkPVImageTextureFilter_h
#define __vtkPVImageTextureFilter_h

#include "vtkSource.h"
#include "vtkPVAssignment.h"
class vtkOutlineSource;
class vtkImageData;
class vtkPolyData;
class vtkImageClip;
class vtkPlaneSource;

class VTK_EXPORT vtkPVImageTextureFilter : public vtkSource
{
public:
  static vtkPVImageTextureFilter *New();
  vtkTypeMacro(vtkPVImageTextureFilter,vtkSource);

  // Description:
  // Set/Get the image input.
  void SetInput(vtkImageData *input);
  vtkImageData *GetInput();
  
  // Description:
  // The main output is poly data.
  vtkPolyData *GetGeometryOutput();
  vtkPolyData *GetOutput() {return this->GetGeometryOutput();}
  void SetGeometryOutput(vtkPolyData *pd);
  void SetOutput(vtkPolyData *pd) {this->SetGeometryOutput(pd);}
  
  // Description:
  // Texture output is a second output the contains the texture map
  // to use with the poly data output.
  void SetTextureOutput(vtkImageData *out);
  vtkImageData *GetTextureOutput();
  
  
  
  // Description:
  // Select which portion of the input we need.
  void ComputeInputUpdateExtents( vtkDataObject *output);

  // Description:
  // Assignment to a process is encoded in this object.
  vtkSetObjectMacro(Assignment, vtkPVAssignment);
  vtkGetObjectMacro(Assignment, vtkPVAssignment);
  
protected:
  vtkPVImageTextureFilter();
  ~vtkPVImageTextureFilter();
  vtkPVImageTextureFilter(const vtkPVImageTextureFilter&) {};
  void operator=(const vtkPVImageTextureFilter&) {};
  
  void Execute();

  vtkPVAssignment *Assignment;
  
  vtkImageData *IntermediateImage;
  vtkImageClip *Clip;
  vtkPlaneSource *PlaneSource;
  
  int PlaneAxis;
  
  int Extent[6];
};

#endif


