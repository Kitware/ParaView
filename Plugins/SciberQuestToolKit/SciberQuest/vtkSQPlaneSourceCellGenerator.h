/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef vtkSQPlaneSourceCellGenerator_h
#define vtkSQPlaneSourceCellGenerator_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkSQCellGenerator.h"
#include "vtkCellType.h" // for VTK_POLYGON

/// A plane source that provide data on demand
/**
Given a cell id the source provides corresponding
cell points and indexes (globally unique ids).
*/
class VTKSCIBERQUEST_EXPORT vtkSQPlaneSourceCellGenerator : public vtkSQCellGenerator
{
public:
  static vtkSQPlaneSourceCellGenerator *New();
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeMacro(vtkSQPlaneSourceCellGenerator, vtkSQCellGenerator);

  /**
  Return the total number of cells available.
  */
  virtual vtkIdType GetNumberOfCells()
    {
    return this->Resolution[0]*this->Resolution[1];
    }

  /**
  Return the cell type of the cell at id.
  */
  virtual int GetCellType(vtkIdType){ return VTK_POLYGON; }

  /**
  Return the number of points required for the named
  cell. For homogeneous datasets its always the same.
  */
  virtual int GetNumberOfCellPoints(vtkIdType){ return 4;}

  /**
  Copy the points from a cell into the provided buffer,
  buffer is expected to be large enough. Return the number
  of points coppied.
  */
  virtual int GetCellPoints(vtkIdType cid, float *pts);

  /**
  Copy the point's indexes into the provided bufffer,
  buffer is expected to be large enough. Return the
  number of points coppied. The index is unique across
  all processes but is not the same as the point id
  in a VTK dataset.
  */
  virtual int GetCellPointIndexes(vtkIdType cid, vtkIdType *idx);

  /**
  Copy the texture coordinates from a cell into the provided
  buffer. buffer is expected to be large enough. return the
  number of tcoords coppied.
  */
  int GetCellTextureCoordinates(vtkIdType cid, float *pts);

  /**
  Set/Get plane cell resolution.
  */
  void SetResolution(int *r);
  void SetResolution(int rx, int ry);
  vtkGetVector2Macro(Resolution,int);
  /**
  Set/Get plane coordinates, origin,point1,point2.
  */
  void SetOrigin(double *o);
  void SetOrigin(double x, double y, double z);
  vtkGetVector3Macro(Origin,double);

  void SetPoint1(double *o);
  void SetPoint1(double x, double y, double z);
  vtkGetVector3Macro(Point1,double);

  void SetPoint2(double *o);
  void SetPoint2(double x, double y, double z);
  vtkGetVector3Macro(Point2,double);

  /**
  Call this after any change to coordinates and resolution.
  */
  void ComputeDeltas();

protected:
  vtkSQPlaneSourceCellGenerator();
  virtual ~vtkSQPlaneSourceCellGenerator(){};
private:
  vtkSQPlaneSourceCellGenerator(const vtkSQPlaneSourceCellGenerator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSQPlaneSourceCellGenerator&) VTK_DELETE_FUNCTION;

private:
  int Resolution[3];
  double Origin[3];
  double Point1[3];
  double Point2[3];
  double Dx[3];
  double Dy[3];
};

#endif
