/*=========================================================================

  Program:   ParaView
  Module:    vtkPVContextInteractorStyle.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVContextInteractorStyle
 * @brief extends vtkContextInteractorStyle to fire start/end interaction events.
 *
 * vtkPVContextInteractorStyle simply extends vtkContextInteractorStyle to fire
 * vtkCommand::StartInteractionEvent and vtkCommand::EndInteractionEvent to mark
 * the beginning and end of the interaction consistent with
 * vtkPVInteractorStyle (used for render views).
 */
#ifndef vtkPVContextInteractorStyle_h
#define vtkPVContextInteractorStyle_h

#include "vtkContextInteractorStyle.h"
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVContextInteractorStyle
  : public vtkContextInteractorStyle
{
public:
  static vtkPVContextInteractorStyle* New();
  vtkTypeMacro(vtkPVContextInteractorStyle, vtkContextInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  void OnLeftButtonDown() VTK_OVERRIDE;
  void OnLeftButtonUp() VTK_OVERRIDE;
  void OnMiddleButtonDown() VTK_OVERRIDE;
  void OnMiddleButtonUp() VTK_OVERRIDE;
  void OnRightButtonDown() VTK_OVERRIDE;
  void OnRightButtonUp() VTK_OVERRIDE;
  void OnMouseWheelForward() VTK_OVERRIDE;
  void OnMouseWheelBackward() VTK_OVERRIDE;

protected:
  vtkPVContextInteractorStyle();
  ~vtkPVContextInteractorStyle();

private:
  vtkPVContextInteractorStyle(const vtkPVContextInteractorStyle&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVContextInteractorStyle&) VTK_DELETE_FUNCTION;
};

#endif
