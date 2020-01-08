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
#include "vtkPVVTKExtensionsFiltersRenderingModule.h" // needed for export macro

class vtkMultiProcessController;

class VTKPVVTKEXTENSIONSFILTERSRENDERING_EXPORT vtkAllToNRedistributeCompositePolyData
  : public vtkDataObjectAlgorithm
{
public:
  static vtkAllToNRedistributeCompositePolyData* New();
  vtkTypeMacro(vtkAllToNRedistributeCompositePolyData, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
  virtual vtkExecutive* CreateDefaultExecutive() override;

  virtual int RequestDataObject(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  virtual int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  virtual int FillInputPortInformation(int port, vtkInformation* info) override;

  int NumberOfProcesses;
  vtkMultiProcessController* Controller;

private:
  vtkAllToNRedistributeCompositePolyData(const vtkAllToNRedistributeCompositePolyData&) = delete;
  void operator=(const vtkAllToNRedistributeCompositePolyData&) = delete;
};

#endif
