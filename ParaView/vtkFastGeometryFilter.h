/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFastGeometryFilter.h
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
// .NAME vtkFastGeometryFilter - Extracts outer (polygonal) surface.
// .SECTION Description
// vtkFastGeometryFilter is a fast version of vtkGeometry filter.
// It does not have as many options.

// .SECTION See Also
// vtkStructuredGridGeometryFilter vtkStructuredGridGeometryFilter
// vtkExtractGeometry vtkGeometryFilter.

#ifndef __vtkFastGeometryFilter_h
#define __vtkFastGeometryFilter_h

#include "vtkDataSetToPolyDataFilter.h"
#include "vtkStructuredGrid.h"
#include "vtkImageData.h"
#include "vtkUnstructuredGrid.h"

class vtkFastGeomQuad; 


class VTK_EXPORT vtkFastGeometryFilter : public vtkDataSetToPolyDataFilter
{
public:
  static vtkFastGeometryFilter *New();
  vtkTypeMacro(vtkFastGeometryFilter,vtkDataSetToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);


protected:
  vtkFastGeometryFilter();
  ~vtkFastGeometryFilter();
  vtkFastGeometryFilter(const vtkFastGeometryFilter&) {};
  void operator=(const vtkFastGeometryFilter&) {};

  void ComputeInputUpdateExtents(vtkDataObject *output);

  void Execute();
  void StructuredExecute(vtkDataSet *input, int *ext);
  void UnstructuredGridExecute();
  void ExecuteInformation();

  // Helper methods.
  void ExecuteFace(vtkDataSet *input, int maxFlag, int *ext,
                   int aAxis, int bAxis, int cAxis);

  void InitializeQuadHash(int numPoints);
  void DeleteQuadHash();
  void InsertQuadInHash(int a, int b, int c, int d, int hidden);
  void InsertTriInHash(int a, int b, int c, int hidden);
  void InitQuadHashTraversal();
  vtkFastGeomQuad *GetNextVisibleQuadFromHash();

  vtkFastGeomQuad **QuadHash;
  int QuadHashLength;
  vtkFastGeomQuad *QuadHashTraversal;
  int QuadHashTraversalIndex;

};

#endif


