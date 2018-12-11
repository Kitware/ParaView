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
 * This filter produces a line along a side of the bounding box of its input
 * for either an axis-aligned bounding box or an object-oriented bounding box.
 * Use SetAxis to choose to which coordinate axis this line is parallel.
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
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkRulerLineForInput* New();

  enum class AxisType
  {
    X = 0,
    Y,
    Z,
    OrientedBoundingBoxMajorAxis,
    OrientedBoundingBoxMediumAxis,
    OrientedBoundingBoxMinorAxis
  };

  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  vtkSetClampMacro(Axis, int, 0, 5);
  vtkGetMacro(Axis, int);

protected:
  vtkRulerLineForInput();
  ~vtkRulerLineForInput() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestInformation(vtkInformation* request, vtkInformationVector** inVectors,
    vtkInformationVector* outVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inVectors,
    vtkInformationVector* outVector) override;

private:
  vtkMultiProcessController* Controller;
  int Axis;

  vtkRulerLineForInput(const vtkRulerLineForInput&) = delete;
  void operator=(const vtkRulerLineForInput&) = delete;
};

#endif
