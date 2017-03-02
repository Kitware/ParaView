/*=========================================================================

  Program:   ParaView
  Module:    vtkPVExtractComponent.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVExtractComponent
 * @brief   Extract a component of an attribute.
 *
 * vtkPVExtractComponent Extract a component of an attribute.
*/

#ifndef vtkPVExtractComponent_h
#define vtkPVExtractComponent_h

#include "vtkDataSetAlgorithm.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

class vtkDataSet;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPVExtractComponent : public vtkDataSetAlgorithm
{
public:
  static vtkPVExtractComponent* New();
  vtkTypeMacro(vtkPVExtractComponent, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  vtkSetMacro(InputArrayComponent, int);
  vtkGetMacro(InputArrayComponent, int);

  vtkSetStringMacro(OutputArrayName);
  vtkGetStringMacro(OutputArrayName);

protected:
  vtkPVExtractComponent();
  ~vtkPVExtractComponent();

  virtual int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  virtual int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  int InputArrayComponent;
  char* OutputArrayName;

private:
  vtkPVExtractComponent(const vtkPVExtractComponent&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVExtractComponent&) VTK_DELETE_FUNCTION;
};

#endif
