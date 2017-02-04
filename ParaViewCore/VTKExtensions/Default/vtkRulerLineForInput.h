/*=========================================================================

  Program:   ParaView
  Module:    vtkRulerLineForInput.h

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  Copyright 2013 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.

=========================================================================*/
/**
 * @class   vtkRulerLineForInput
 *
 *
 * This filter produces a line along a side of the bounding box of its input.
 * Which coordinate axis this line is parallel to is configurable via SetAxis.
 * This class is designed to work with the vtkRulerLineRepresentation to show
 * a ruler along the input data.
 *
 * .SEE vtkRulerLineForInput
*/

#ifndef vtkRulerLineForInput_h
#define vtkRulerLineForInput_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkPolyDataAlgorithm.h"

class vtkMultiProcessController;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkRulerLineForInput : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkRulerLineForInput, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkRulerLineForInput* New();

  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  vtkSetClampMacro(Axis, int, 0, 2);
  vtkGetMacro(Axis, int);

protected:
  vtkRulerLineForInput();
  ~vtkRulerLineForInput();

  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;
  int RequestInformation(vtkInformation* request, vtkInformationVector** inVectors,
    vtkInformationVector* outVector) VTK_OVERRIDE;
  int RequestData(vtkInformation* request, vtkInformationVector** inVectors,
    vtkInformationVector* outVector) VTK_OVERRIDE;

private:
  vtkMultiProcessController* Controller;
  int Axis;

  vtkRulerLineForInput(const vtkRulerLineForInput&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRulerLineForInput&) VTK_DELETE_FUNCTION;
};

#endif
