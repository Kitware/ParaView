/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRedistributePoly.h
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

/*======================================================================
// This software and ancillary information known as vtk_ext (and
// herein called "SOFTWARE") is made available under the terms
// described below.  The SOFTWARE has been approved for release with
// associated LA_CC Number 99-44, granted by Los Alamos National
// Laboratory in July 1999.
//
// Unless otherwise indicated, this SOFTWARE has been authored by an
// employee or employees of the University of California, operator of
// the Los Alamos National Laboratory under Contract No. W-7405-ENG-36
// with the United States Department of Energy.
//
// The United States Government has rights to use, reproduce, and
// distribute this SOFTWARE.  The public may copy, distribute, prepare
// derivative works and publicly display this SOFTWARE without charge,
// provided that this Notice and any statement of authorship are
// reproduced on all copies.
//
// Neither the U. S. Government, the University of California, nor the
// Advanced Computing Laboratory makes any warranty, either express or
// implied, nor assumes any liability or responsibility for the use of
// this SOFTWARE.
//
// If SOFTWARE is modified to produce derivative works, such modified
// SOFTWARE should be clearly marked, so as not to confuse it with the
// version available from Los Alamos National Laboratory.
======================================================================*/

// .NAME vtkRedistributePoly - redistribute poly cells from other processes
//                        (special version to color according to processor)

#ifndef __vtkRedistributePoly_h
#define __vtkRedistributePoly_h

#include "vtkPolyDataToPolyDataFilter.h"
#include "vtkMultiProcessController.h"

#define VTK_CELL_ID_TAG      10
#define VTK_POINT_COORDS_TAG 20
#define VTK_NUM_POINTS_TAG   30
#define VTK_NUM_CELLS_TAG    40
#define VTK_POLY_DATA_TAG    50
#define VTK_BOUNDS_TAG       60
#define VTK_CNT_SEND_TAG            80
#define VTK_CNT_REC_TAG             90
#define VTK_SEND_PROC_TAG          100
#define VTK_SEND_NUM_TAG           110
#define VTK_REC_PROC_TAG           120
#define VTK_REC_NUM_TAG            130
#define VTK_NUM_CURR_CELLS_TAG     140

#define VTK_CELL_CNT_TAG     150
#define VTK_CELL_TAG         160
#define VTK_POINTS_SIZE_TAG  170
#define VTK_POINTS_TAG       180

//#define VTK_DATA_ARRAY_SIZE_TAG     190
#define VTK_SCALARS_TAG             200
#define VTK_VECTORS_TAG             210
#define VTK_NORMALS_TAG             220
#define VTK_TCOORDS_TAG             230
#define VTK_TENSOR_TAG              240
#define VTK_FIELDDATA_TAG         1250

#include "vtkCommSched.h"

//*******************************************************************

class VTK_EXPORT vtkRedistributePoly : public vtkPolyDataToPolyDataFilter 
{
public:
  vtkTypeMacro(vtkRedistributePoly, vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  static vtkRedistributePoly *New();

  // Description:
  // The filter needs a controller to determine which process it is in.
  void SetController(vtkMultiProcessController* controller);
  vtkGetObjectMacro(Controller, vtkMultiProcessController); 

  void SetColorProc(int cp){colorProc = cp;};
  void SetColorProc(){colorProc = 1;};
  int GetColorProc(){return colorProc;};

protected:
  vtkRedistributePoly();
  ~vtkRedistributePoly();
  
  virtual void MakeSchedule (vtkCommSched&);
  void OrderSchedule (vtkCommSched&);

  void SendCellSizes (const vtkIdType, const vtkIdType, vtkPolyData*, const int, 
                      vtkIdType&, vtkIdType&, vtkIdType*); 
  void CopyCells (const vtkIdType,vtkPolyData*, vtkPolyData*, vtkIdType*); 
  void SendCells (const vtkIdType, const vtkIdType, vtkPolyData*, vtkPolyData*, 
                  const int, vtkIdType&, vtkIdType&, vtkIdType*); 
  void ReceiveCells (const vtkIdType, const vtkIdType, vtkPolyData*, const int, 
                     const vtkIdType, const vtkIdType, const vtkIdType, const vtkIdType);

  void FindMemReq (const vtkIdType, vtkPolyData*, vtkIdType&, vtkIdType&);

  void AllocateDataArrays (vtkDataSetAttributes*, vtkIdType*, const int, int*, 
                           const vtkIdType);
  void AllocateArrays (vtkDataArray*, const vtkIdType);

  void CopyDataArrays(vtkDataSetAttributes* , vtkDataSetAttributes* ,
                      const vtkIdType , vtkIdType*, const int);

  void CopyCellBlockDataArrays(vtkDataSetAttributes* , vtkDataSetAttributes* ,
                               const vtkIdType , vtkIdType* , const vtkIdType, 
                               const int);

  void CopyArrays (vtkDataArray*, vtkDataArray*, const vtkIdType, vtkIdType*, 
                   const int, const int); 

  void CopyBlockArrays (vtkDataArray*, vtkDataArray*, const vtkIdType, const vtkIdType, 
                        const int); 

  void SendDataArrays (vtkDataSetAttributes*, vtkDataSetAttributes*,
                       const vtkIdType, const int, vtkIdType*, const int); 

  void SendCellBlockDataArrays (vtkDataSetAttributes*, vtkDataSetAttributes*,
                       const vtkIdType, const int, vtkIdType*, const vtkIdType); 

  void SendArrays (vtkDataArray*, const vtkIdType, const int, 
                   vtkIdType*, const int, const int); 

  void SendBlockArrays (vtkDataArray*, const vtkIdType, const int, 
                        const vtkIdType, const int); 

  void ReceiveDataArrays (vtkDataSetAttributes*, const vtkIdType, const int, vtkIdType*, 
                          const int); 

  void ReceiveArrays (vtkDataArray*, const vtkIdType, const int, 
                      vtkIdType*, const int, const int); 

  void Execute();

  void CompleteArrays (const int);
  void SendCompleteArrays (const int);

  vtkMultiProcessController *Controller;
  //vtkPointLocator *Locator;
  int colorProc; // Set to 1 to color data according to processor
private:
  vtkRedistributePoly(const vtkRedistributePoly&); // Not implemented
  void operator=(const vtkRedistributePoly&); // Not implemented
};

//****************************************************************

#endif


