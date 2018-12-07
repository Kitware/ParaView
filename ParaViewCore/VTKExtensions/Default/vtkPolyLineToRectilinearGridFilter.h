/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyLineToRectilinearGridFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPolyLineToRectilinearGridFilter
 * @brief   filter that converts an input
 * poly data with a single polyline to a 1-D regular rectilinear grid.
 *
 * vtkPolyLineToRectilinearGridFilter converts an input polydata with single
 * polyline to a 1-D regular rectilinear grid. The output has additional point
 * data indicating the arc-length for each point. Note that the Xcoordinates
 * of the output are not related to those of the input. The input point
 * coordinates themselves are added as point data in the output.
*/

#ifndef vtkPolyLineToRectilinearGridFilter_h
#define vtkPolyLineToRectilinearGridFilter_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkRectilinearGridAlgorithm.h"

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPolyLineToRectilinearGridFilter
  : public vtkRectilinearGridAlgorithm
{
public:
  static vtkPolyLineToRectilinearGridFilter* New();
  vtkTypeMacro(vtkPolyLineToRectilinearGridFilter, vtkRectilinearGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPolyLineToRectilinearGridFilter();
  ~vtkPolyLineToRectilinearGridFilter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkPolyLineToRectilinearGridFilter(const vtkPolyLineToRectilinearGridFilter&) = delete;
  void operator=(const vtkPolyLineToRectilinearGridFilter&) = delete;
};

#endif
