/*=========================================================================

  Program:   ParaView
  Module:    vtkPVUpdateSuppressor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVUpdateSuppressor - prevents propagation of update
// .SECTION Description  I am also going to have this object manage
// flip books (geometry cache).

#ifndef __vtkPVUpdateSuppressor_h
#define __vtkPVUpdateSuppressor_h

#include "vtkDataSetSource.h"

class vtkPolyData;
class vtkUnstructuredGrid;

class VTK_EXPORT vtkPVUpdateSuppressor : public vtkDataSetSource
{
public:
  vtkTypeRevisionMacro(vtkPVUpdateSuppressor,vtkDataSetSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // These methods assume the user knows the output type,
  // a creates the output if necessary even when the input has
  // not been set yet.
  virtual vtkPolyData* GetPolyDataOutput();
  virtual vtkUnstructuredGrid* GetUnstructuredGridOutput();

  // Description:
  // Check input type and make the output the same type.
  virtual vtkDataSet* GetOutput();

  // Description:
  // Construct with user-specified implicit function.
  static vtkPVUpdateSuppressor *New();

  // Description:
  // The pipeline knows nothing about this input.  The pipeline thinks
  // that this is a source.
  void SetInput(vtkDataSet* input);
  vtkDataSet* GetInput(){return this->Input;}

  // Description:
  // Methods for saving, clearing and updating flip books.
  // Cache update will update and save cache or just use previous cache.
  // "idx" is the time index, "total" is the number of time steps.
  void RemoveAllCaches();
  void CacheUpdate(int idx, int total);

  // Description:
  // Force update on the input.
  void ForceUpdate();

  // Description:
  // Set number of pieces and piece on the data.
  // This causes the filter to ingore the request from the output.
  // It is here because the user may not have celled update on the output
  // before calling force update (it is an easy fix).
  vtkSetMacro(UpdatePiece, int);
  vtkGetMacro(UpdatePiece, int);
  vtkSetMacro(UpdateNumberOfPieces, int);
  vtkGetMacro(UpdateNumberOfPieces, int);

protected:
  vtkPVUpdateSuppressor();
  ~vtkPVUpdateSuppressor();

  void Execute();

  vtkDataSet* Input;

  int UpdatePiece;
  int UpdateNumberOfPieces;

  vtkTimeStamp UpdateTime;

  vtkDataSet** CachedGeometry;
  int CachedGeometryLength;

private:
  vtkPVUpdateSuppressor(const vtkPVUpdateSuppressor&);  // Not implemented.
  void operator=(const vtkPVUpdateSuppressor&);  // Not implemented.
};

#endif
