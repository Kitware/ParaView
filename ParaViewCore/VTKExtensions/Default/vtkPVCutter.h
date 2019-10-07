/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCutter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVCutter
 * @brief   Slice Filter
 *
 *
 * This is a subclass of vtkCutter that allows selection of input vtkHyperTreeGrid
*/

#ifndef vtkPVCutter_h
#define vtkPVCutter_h

#include "vtkCutter.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPVCutter : public vtkCutter
{
public:
  vtkTypeMacro(vtkPVCutter, vtkCutter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPVCutter* New();

  int ProcessRequest(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  //@
  /**
   * Only used for cutting hyper tree grids. If set to true, the dual grid is used for cutting
   */
  vtkGetMacro(Dual, bool);
  vtkSetMacro(Dual, bool);
  //@}

protected:
  vtkPVCutter();
  ~vtkPVCutter() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  virtual int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  int FillInputPortInformation(int, vtkInformation* info) override;
  int FillOutputPortInformation(int, vtkInformation* info) override;

  bool Dual;

private:
  vtkPVCutter(const vtkPVCutter&) = delete;
  void operator=(const vtkPVCutter&) = delete;
};

#endif
