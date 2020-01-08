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
 * @class   vtkPVSingleOutputExtractSelection
 *
 * vtkPVSingleOutputExtractSelection extends to vtkPVExtractSelection to simply
 * hide the second output-port. This is the filter used in ParaView GUI.
*/

#ifndef vtkPVSingleOutputExtractSelection_h
#define vtkPVSingleOutputExtractSelection_h

#include "vtkPVExtractSelection.h"

class VTKPVVTKEXTENSIONSEXTRACTION_EXPORT vtkPVSingleOutputExtractSelection
  : public vtkPVExtractSelection
{
public:
  static vtkPVSingleOutputExtractSelection* New();
  vtkTypeMacro(vtkPVSingleOutputExtractSelection, vtkPVExtractSelection);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPVSingleOutputExtractSelection();
  ~vtkPVSingleOutputExtractSelection() override;

private:
  vtkPVSingleOutputExtractSelection(const vtkPVSingleOutputExtractSelection&) = delete;
  void operator=(const vtkPVSingleOutputExtractSelection&) = delete;
};

#endif
