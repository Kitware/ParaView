/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInhibitPoints.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkInhibitPoints - selectively filter points
// .SECTION Description
// vtkInhibitPoints is a filter that passes through points and point attributes 
// from input dataset. (Other geometry is not passed through.) It uses the point
// vectors to inhibit neighboring points.  It uses three values to define
// the neighborhood of a vector to inhibit: "ForwardScale" in direction of vector,
// "BackwardScale" behind vector, and "LateralScale" which is perpendicular distance.
// Distance of inhibition is scaled by the vector.  This filter also inhibits
// points whose vector magnitudes are smaller than "MagnitudeThreshold".

#ifndef __vtkInhibitPoints_h
#define __vtkInhibitPoints_h

#include "vtkDataSetToPolyDataFilter.h"

class VTK_EXPORT vtkInhibitPoints : public vtkDataSetToPolyDataFilter
{
public:
  static vtkInhibitPoints *New();
  vtkTypeMacro(vtkInhibitPoints,vtkDataSetToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Defines the neighborhood around a vector to supress.
  vtkSetMacro(ForwardScale,float);
  vtkGetMacro(ForwardScale,float);
  vtkSetMacro(BackwardScale,float);
  vtkGetMacro(BackwardScale,float);
  vtkSetMacro(LateralScale,float);
  vtkGetMacro(LateralScale,float);

  // Description:
  // An easy way to change all of the scales with one values.
  vtkSetMacro(Scale,float);
  vtkGetMacro(Scale,float);

  // Description:
  // Points with vector magnitudes smaller than this are not passed.
  vtkSetMacro(MagnitudeThreshold,float);
  vtkGetMacro(MagnitudeThreshold,float);

  // Description:
  // Generate output polydata vertices as well as points. A useful
  // convenience method because vertices are drawn (they are topology) while
  // points are not (they are geometry). By default this method is off.
  vtkSetMacro(GenerateVertices,int);
  vtkGetMacro(GenerateVertices,int);
  vtkBooleanMacro(GenerateVertices,int);

protected:
  vtkInhibitPoints();
  ~vtkInhibitPoints() {};
  vtkInhibitPoints(const vtkInhibitPoints&) {};
  void operator=(const vtkInhibitPoints&) {};

  void Execute();

  float ForwardScale;
  float BackwardScale;
  float LateralScale;
  float Scale;

  float MagnitudeThreshold;

  int GenerateVertices; //generate polydata verts
};

#endif


