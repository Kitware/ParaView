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

#include "vtkSelectionNode.h"
#include "vtkSelectionOperator.h"
#include "vtkWeakPointer.h"

class vtkSelectionNode;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPythonSelector : public vtkSelectionOperator
{
public:
  static vtkPythonSelector* New();
  vtkTypeMacro(vtkPythonSelector, vtkSelectionOperator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Initialize(vtkSelectionNode* node) override;

  bool ComputeSelectedElements(vtkDataObject* input, vtkSignedCharArray* elementInside) override;

protected:
  vtkPythonSelector();
  ~vtkPythonSelector() override;

  vtkWeakPointer<vtkSelectionNode> Node;

private:
  vtkPythonSelector(const vtkPythonSelector&) = delete;
  void operator=(const vtkPythonSelector&) = delete;
};

#endif
