/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRunTimeContour.h
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
// .NAME vtkRunTimeContour - contour data from run-time simulation
// .SECTION Description
// This is an example of reading in data written to a file during a simulation
// and contouring this data.  It is a simple example of a "pipeline macro" --
// performing multiple operations in a single filter.

#ifndef __vtkRunTimeContour_h
#define __vtkRunTimeContour_h

#include "vtkPolyDataSource.h"
#include "vtkStructuredPointsReader.h"
#include "vtkSingleContourFilter.h"

class VTK_EXPORT vtkRunTimeContour : public vtkPolyDataSource 
{
public:
  vtkTypeMacro(vtkRunTimeContour, vtkPolyDataSource);

  // Description:
  // Construct with default filename and contour value.
  static vtkRunTimeContour *New();

  // Description:
  // Set/Get the contour value.
  void SetContourValue(float value);
  float GetContourValue();
  
  // Description:
  // Get the scalar range of the data.
  vtkGetVector2Macro(Range, float);
  
protected:
  vtkRunTimeContour();
  ~vtkRunTimeContour();
  vtkRunTimeContour(const vtkRunTimeContour&) {};
  void operator=(const vtkRunTimeContour&) {};

  void Execute();

  vtkStructuredPointsReader *Reader;
  vtkSingleContourFilter *Contour;
  float Range[2];
};

#endif
