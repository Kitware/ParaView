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
// .NAME vtkPVExtractComponent - Extract a component of an attribute.
// .SECTION Description
// vtkPVExtractComponent Extract a component of an attribute.

#ifndef vtkPVExtractComponent_h
#define vtkPVExtractComponent_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkDataSetAlgorithm.h"

class vtkDataSet;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPVExtractComponent : public vtkDataSetAlgorithm
{
public:
  static vtkPVExtractComponent *New();
  vtkTypeMacro(vtkPVExtractComponent,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkSetMacro(InputArrayComponent, int);
  vtkGetMacro(InputArrayComponent, int);

  vtkSetStringMacro(OutputArrayName);
  vtkGetStringMacro(OutputArrayName);

protected:
  vtkPVExtractComponent();
  ~vtkPVExtractComponent();

  virtual int RequestData(vtkInformation*,
                          vtkInformationVector**,
                          vtkInformationVector*);

  virtual int FillInputPortInformation(int port, vtkInformation* info);

  int InputArrayComponent;
  char* OutputArrayName;

private:
  vtkPVExtractComponent(const vtkPVExtractComponent&);  // Not implemented.
  void operator=(const vtkPVExtractComponent&);  // Not implemented.
};

#endif
