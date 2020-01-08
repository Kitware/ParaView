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
#include "vtkRemotingViewsModule.h" //needed for exports

class VTKREMOTINGVIEWS_EXPORT vtkPVContextInteractorStyle : public vtkContextInteractorStyle
{
public:
  static vtkPVContextInteractorStyle* New();
  vtkTypeMacro(vtkPVContextInteractorStyle, vtkContextInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void OnLeftButtonDown() override;
  void OnLeftButtonUp() override;
  void OnMiddleButtonDown() override;
  void OnMiddleButtonUp() override;
  void OnRightButtonDown() override;
  void OnRightButtonUp() override;
  void OnMouseWheelForward() override;
  void OnMouseWheelBackward() override;

protected:
  vtkPVContextInteractorStyle();
  ~vtkPVContextInteractorStyle() override;

private:
  vtkPVContextInteractorStyle(const vtkPVContextInteractorStyle&) = delete;
  void operator=(const vtkPVContextInteractorStyle&) = delete;
};

#endif
