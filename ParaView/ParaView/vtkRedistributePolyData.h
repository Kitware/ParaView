/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRedistributePolyData.h
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

// .NAME vtkRedistributePolyData - redistribute poly cells from other processes (special version to color according to processor)

#ifndef __vtkRedistributePolyData_h
#define __vtkRedistributePolyData_h

#include "vtkPolyDataToPolyDataFilter.h"

class vtkMultiProcessController;

class VTK_EXPORT vtkRedistributePolyData : public vtkPolyDataToPolyDataFilter 
{
public:
  vtkTypeMacro(vtkRedistributePolyData, vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkRedistributePolyData *New();

  // Description:
  // The filter needs a controller to determine which process it is in.
  void SetController(vtkMultiProcessController* controller);
  vtkGetObjectMacro(Controller, vtkMultiProcessController); 

  void SetColorProc(int cp) { colorProc = cp; };
  void SetColorProc() { colorProc = 1; };
  int GetColorProc() { return colorProc; };

protected:
  vtkRedistributePolyData();
  ~vtkRedistributePolyData();

//BTX
  enum {
    CELL_ID_TAG        = 10,
    POINT_COORDS_TAG   = 20,
    NUM_POINTS_TAG     = 30,
    NUM_CELLS_TAG      = 40,
    POLY_DATA_TAG      = 50,
    BOUNDS_TAG         = 60,
    CNT_SEND_TAG       = 80,
    CNT_REC_TAG        = 90,
    SEND_PROC_TAG      = 100,
    SEND_NUM_TAG       = 110,
    REC_PROC_TAG       = 120,
    REC_NUM_TAG        = 130,
    NUM_CURR_CELLS_TAG = 140,

    CELL_CNT_TAG       = 150,
    CELL_TAG           = 160,
    POINTS_SIZE_TAG    = 170,
    POINTS_TAG         = 180,

// DATA_ARRAY_SIZE_TAG     = 190,
    SCALARS_TAG        = 200,
    VECTORS_TAG        = 210,
    NORMALS_TAG        = 220,
    TCOORDS_TAG        = 230,
    TENSOR_TAG         = 240,
    FIELDDATA_TAG      = 1250
  };

  class VTK_EXPORT vtkCommSched
  {
  public:
    vtkCommSched();
    ~vtkCommSched();
    
    int SendCount;
    int ReceiveCount;
    int* SendTo;
    int* ReceiveFrom;
    vtkIdType NumberOfCells;
    vtkIdType* SendNumber;
    vtkIdType* ReceiveNumber;
    
    vtkIdType** SendCellList;
    vtkIdType* KeepCellList;
    
  private:
    vtkCommSched(const vtkCommSched&); // Not implemented
    void operator=(const vtkCommSched&); // Not implemented    
  };

//ETX

  virtual void MakeSchedule (vtkCommSched*);
  void OrderSchedule (vtkCommSched*);

  void SendCellSizes (vtkIdType, vtkIdType, vtkPolyData*, int, 
                      vtkIdType&, vtkIdType&, vtkIdType*); 
  void CopyCells (vtkIdType,vtkPolyData*, vtkPolyData*, vtkIdType*); 
  void SendCells (vtkIdType, vtkIdType, vtkPolyData*, vtkPolyData*, 
                  int, vtkIdType&, vtkIdType&, vtkIdType*); 
  void ReceiveCells (vtkIdType, vtkIdType, vtkPolyData*, int, 
                     vtkIdType, vtkIdType, vtkIdType, 
		     vtkIdType);

  void FindMemReq (vtkIdType, vtkPolyData*, vtkIdType&, vtkIdType&);

  void AllocateDataArrays (vtkDataSetAttributes*, vtkIdType*, int, int*, 
                           vtkIdType);
  void AllocateArrays (vtkDataArray*, vtkIdType);

  void CopyDataArrays(vtkDataSetAttributes* , vtkDataSetAttributes* ,
                      vtkIdType , vtkIdType*, int);

  void CopyCellBlockDataArrays(vtkDataSetAttributes* , vtkDataSetAttributes* ,
                               vtkIdType , vtkIdType* , vtkIdType, int);

  void CopyArrays (vtkDataArray*, vtkDataArray*, vtkIdType, vtkIdType*, 
                   int, int); 

  void CopyBlockArrays (vtkDataArray*, vtkDataArray*, vtkIdType, vtkIdType, 
                        int); 

  void SendDataArrays (vtkDataSetAttributes*, vtkDataSetAttributes*,
		       vtkIdType, int, vtkIdType*, int); 

  void SendCellBlockDataArrays (vtkDataSetAttributes*, vtkDataSetAttributes*,
				vtkIdType, int, vtkIdType*, vtkIdType); 

  void SendArrays (vtkDataArray*, vtkIdType, int,  vtkIdType*, int, int); 

  void SendBlockArrays (vtkDataArray*, vtkIdType, int, vtkIdType, int); 

  void ReceiveDataArrays (vtkDataSetAttributes*, vtkIdType, int, vtkIdType*, 
                          int); 

  void ReceiveArrays (vtkDataArray*, vtkIdType, int, 
                      vtkIdType*, int, int); 

  void Execute();

  void CompleteArrays (int);
  void SendCompleteArrays (int);

  vtkMultiProcessController *Controller;
  int colorProc; // Set to 1 to color data according to processor

private:
  vtkRedistributePolyData(const vtkRedistributePolyData&); // Not implemented
  void operator=(const vtkRedistributePolyData&); // Not implemented
};

#endif


