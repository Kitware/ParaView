/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageOutlineFilter.h
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
// .NAME vtkImageOutlineFilter - create wireframe outline for an image.
// .SECTION Description
// vtkImageOutlineFilter is different from outline filter because
// only the information needs to be updated.

#ifndef __vtkImageOutlineFilter_h
#define __vtkImageOutlineFilter_h

#include "vtkPolyDataSource.h"
#include "vtkImageData.h"
class vtkOutlineSource;
class vtkImageData;

class VTK_EXPORT vtkImageOutlineFilter : public vtkPolyDataSource
{
public:
  static vtkImageOutlineFilter *New();
  vtkTypeMacro(vtkImageOutlineFilter,vtkPolyDataSource);

  // Description:
  // Set/Get the source for the scalar data to contour.
  void SetInput(vtkImageData *input);
  vtkImageData *GetInput();
  
  // Description:
  // We do not need any of the input every (only the information).
  void ComputeInputUpdateExtents( vtkDataObject *output);

  // Description:
  // There has to be an easier way for a filter to tell its input not to update.
  void UpdateData(vtkDataObject *vtkNotUsed(output));
  
protected:
  vtkImageOutlineFilter();
  ~vtkImageOutlineFilter();
  vtkImageOutlineFilter(const vtkImageOutlineFilter&) {};
  void operator=(const vtkImageOutlineFilter&) {};

  vtkOutlineSource *OutlineSource;
  void Execute();
  void ExecuteInformation();
};

#endif


