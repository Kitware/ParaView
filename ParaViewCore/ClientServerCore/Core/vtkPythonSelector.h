/*=========================================================================

  Program:   ParaView
  Module:    vtkPythonSelector.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPythonSelector
 * @brief Select cells/points using numpy expressions
 */

#ifndef vtkPythonSelector_h
#define vtkPythonSelector_h

#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
#include "vtkSelector.h"

class vtkSelectionNode;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPythonSelector : public vtkSelector
{
public:
  static vtkPythonSelector* New();
  vtkTypeMacro(vtkPythonSelector, vtkSelector);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Overridden to delegate the selection to the Python expression.
   */
  bool ComputeSelectedElements(vtkDataObject* input, vtkDataObject* output) override;

protected:
  vtkPythonSelector();
  ~vtkPythonSelector() override;

  /**
   * Implementing this is required by the superclass.
   */
  virtual bool ComputeSelectedElementsForBlock(vtkDataObject* vtkNotUsed(input),
    vtkSignedCharArray* vtkNotUsed(insidednessArray), unsigned int vtkNotUsed(compositeIndex),
    unsigned int vtkNotUsed(amrLevel), unsigned int vtkNotUsed(amrIndex)) override
  {
    return false;
  }

private:
  vtkPythonSelector(const vtkPythonSelector&) = delete;
  void operator=(const vtkPythonSelector&) = delete;
};

#endif
