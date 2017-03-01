/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAllToNRedistributeCompositePolyData
 *
 * vtkAllToNRedistributePolyData extension that is composite data aware.
*/

#ifndef vtkAllToNRedistributeCompositePolyData_h
#define vtkAllToNRedistributeCompositePolyData_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class vtkMultiProcessController;

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkAllToNRedistributeCompositePolyData
  : public vtkDataObjectAlgorithm
{
public:
  static vtkAllToNRedistributeCompositePolyData* New();
  vtkTypeMacro(vtkAllToNRedistributeCompositePolyData, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * The filter needs a controller to determine which process it is in.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

  vtkSetMacro(NumberOfProcesses, int);
  vtkGetMacro(NumberOfProcesses, int);

protected:
  vtkAllToNRedistributeCompositePolyData();
  ~vtkAllToNRedistributeCompositePolyData();

  /**
   * Create a default executive.
   * If the DefaultExecutivePrototype is set, a copy of it is created
   * in CreateDefaultExecutive() using NewInstance().
   * Otherwise, vtkStreamingDemandDrivenPipeline is created.
   */
  virtual vtkExecutive* CreateDefaultExecutive() VTK_OVERRIDE;

  virtual int RequestDataObject(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;
  virtual int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;
  virtual int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  int NumberOfProcesses;
  vtkMultiProcessController* Controller;

private:
  vtkAllToNRedistributeCompositePolyData(
    const vtkAllToNRedistributeCompositePolyData&) VTK_DELETE_FUNCTION;
  void operator=(const vtkAllToNRedistributeCompositePolyData&) VTK_DELETE_FUNCTION;
};

#endif
