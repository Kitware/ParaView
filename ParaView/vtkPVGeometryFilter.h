/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVGeometryFilter.h
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
// .NAME vtkPVGeometryFilter - Geometry filter with a couple other options.
// .SECTION Description
// This allows an outline of an image data set as one mode.

#ifndef __vtkPVGeometryFilter_h
#define __vtkPVGeometryFilter_h

#include "vtkDataSetSurfaceFilter.h"
#include "vtkImageData.h"


#define VTK_PV_SURFACE 0
#define VTK_PV_IMAGE_OUTLINE 1



class VTK_EXPORT vtkPVGeometryFilter : public vtkDataSetSurfaceFilter
{
public:
  static vtkPVGeometryFilter *New();
  vtkTypeMacro(vtkPVGeometryFilter,vtkDataSetSurfaceFilter);

  // Description:
  // We do not need any of the input every (only the information).
  void ComputeInputUpdateExtents( vtkDataObject *output);

  // Description:
  // There has to be an easier way for a filter to tell its input not to update.
  void UpdateData(vtkDataObject *output);

  // Description:
  vtkSetMacro(Mode,int);
  vtkGetMacro(Mode,int);  
  void SetModeToSurface() {this->SetMode(VTK_PV_SURFACE);}
  void SetModeToImageOutline() {this->SetMode(VTK_PV_IMAGE_OUTLINE);}
  
protected:
  vtkPVGeometryFilter();
  ~vtkPVGeometryFilter();
  vtkPVGeometryFilter(const vtkPVGeometryFilter&) {};
  void operator=(const vtkPVGeometryFilter&) {};

  void Execute();
  void ExecuteInformation();
  
  int Mode;
};

#endif


