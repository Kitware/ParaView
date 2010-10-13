/*=========================================================================

  Program:   ParaView
  Module:    vtkParallelCoordinatesRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkParallelCoordinatesRepresentation
// .SECTION Description
//

#ifndef __vtkParallelCoordinatesRepresentation_h
#define __vtkParallelCoordinatesRepresentation_h

#include "vtkChartRepresentation.h"

class vtkChartParallelCoordinates;

class VTK_EXPORT vtkParallelCoordinatesRepresentation : public vtkChartRepresentation
{
public:
  static vtkParallelCoordinatesRepresentation* New();
  vtkTypeMacro(vtkParallelCoordinatesRepresentation, vtkChartRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Provides access to the underlying VTK representation.
  vtkChartParallelCoordinates* GetChart();

  // Description:
  // Set visibility of the representation.
  virtual void SetVisibility(bool visible);

  // Description:
  // Set series visibility for the series with the given name.
  void SetSeriesVisibility(const char* name, int visible);

  // Description:
  // Set series label for the series with the given name.
  void SetLabel(const char* name, const char* label);

  void SetLineThickness(int value);
  void SetLineStyle(int value);
  void SetColor(double r, double g, double b);
  void SetOpacity(double opacity);

//BTX
protected:
  vtkParallelCoordinatesRepresentation();
  ~vtkParallelCoordinatesRepresentation();

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
  vtkParallelCoordinatesRepresentation(
      const vtkParallelCoordinatesRepresentation&); // Not implemented
  void operator=(const vtkParallelCoordinatesRepresentation&); // Not implemented
//ETX
};

#endif
