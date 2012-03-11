/*=========================================================================

  Program:   ParaView
  Module:    vtkPVParallelCoordinatesRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVParallelCoordinatesRepresentation
// .SECTION Description
//

#ifndef __vtkPVParallelCoordinatesRepresentation_h
#define __vtkPVParallelCoordinatesRepresentation_h

#include "vtkChartRepresentation.h"

class vtkChartParallelCoordinates;

class VTK_EXPORT vtkPVParallelCoordinatesRepresentation : public vtkChartRepresentation
{
public:
  static vtkPVParallelCoordinatesRepresentation* New();
  vtkTypeMacro(vtkPVParallelCoordinatesRepresentation, vtkChartRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Provides access to the underlying VTK representation.
  vtkChartParallelCoordinates* GetChart();

  // Description:
  // Set visibility of the representation.
  virtual void SetVisibility(bool visible);

  void SetLineThickness(int value);
  void SetLineStyle(int value);
  void SetColor(double r, double g, double b);
  void SetOpacity(double opacity);

//BTX
protected:
  vtkPVParallelCoordinatesRepresentation();
  ~vtkPVParallelCoordinatesRepresentation();

  virtual int RequestData(vtkInformation*,
    vtkInformationVector**, vtkInformationVector*);

  virtual bool AddToView(vtkView* view);

  // Description:
  // Removes the representation to the view.  This is called from
  // vtkView::RemoveRepresentation().  Subclasses should override this method.
  // Returns true if the removal succeeds.
  virtual bool RemoveFromView(vtkView* view);

  //vtkChartRepresentation* SelectionRepresentation;

private:
  vtkPVParallelCoordinatesRepresentation(
      const vtkPVParallelCoordinatesRepresentation&); // Not implemented
  void operator=(const vtkPVParallelCoordinatesRepresentation&); // Not implemented

  // Helper to determine if the number of columns changed.
  bool NumberOfColumnsChanged();
  vtkIdType NumberOfColumns;
//ETX
};

#endif
