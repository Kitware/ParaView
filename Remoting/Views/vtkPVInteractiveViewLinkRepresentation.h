// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVInteractiveViewLinkRepresentation
 * @brief   A Representation to manipulate
 * an interactive view link widget
 *
 *
 * This Representation creates and manages a custom vtkLogoRepresentation which is precented
 * to go over the edge of the viewport
 */

#ifndef vtkPVInteractiveViewLinkRepresentation_h
#define vtkPVInteractiveViewLinkRepresentation_h

#include "vtkLogoRepresentation.h"
#include "vtkRemotingViewsModule.h" // needed for export macro

class VTKREMOTINGVIEWS_EXPORT vtkPVInteractiveViewLinkRepresentation : public vtkLogoRepresentation
{
public:
  static vtkPVInteractiveViewLinkRepresentation* New();
  vtkTypeMacro(vtkPVInteractiveViewLinkRepresentation, vtkLogoRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Considering an eventPosition and current interaction state, this method will adjust
   * internal position and shape so the widget behave correctly, ie: resizable, movable, but not
   * going
   * over the edges of the render window
   */
  void WidgetInteraction(double eventPos[2]) override;

protected:
  vtkPVInteractiveViewLinkRepresentation();
  ~vtkPVInteractiveViewLinkRepresentation() override;

  /**
   * Redefining method to avoid adjustment of image
   */
  void AdjustImageSize(double o[2], double borderSize[2], double imageSize[2]) override;

private:
  vtkPVInteractiveViewLinkRepresentation(const vtkPVInteractiveViewLinkRepresentation&) = delete;
  void operator=(const vtkPVInteractiveViewLinkRepresentation&) = delete;
};

#endif
