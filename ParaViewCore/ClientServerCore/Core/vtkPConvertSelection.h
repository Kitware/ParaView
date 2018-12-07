/*=========================================================================

  Program:   ParaView
  Module:    vtkPConvertSelection.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPConvertSelection
 * @brief   parallel aware vtkConvertSelection subclass.
 *
 * vtkPConvertSelection is a parallel aware vtkConvertSelection subclass.
*/

#ifndef vtkPConvertSelection_h
#define vtkPConvertSelection_h

#include "vtkConvertSelection.h"
#include "vtkPVClientServerCoreCoreModule.h" //needed for exports

class vtkMultiProcessController;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPConvertSelection : public vtkConvertSelection
{
public:
  static vtkPConvertSelection* New();
  vtkTypeMacro(vtkPConvertSelection, vtkConvertSelection);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the parallel controller.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

protected:
  vtkPConvertSelection();
  ~vtkPConvertSelection() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkMultiProcessController* Controller;

private:
  vtkPConvertSelection(const vtkPConvertSelection&) = delete;
  void operator=(const vtkPConvertSelection&) = delete;
};

#endif // vtkPConvertSelection_h
