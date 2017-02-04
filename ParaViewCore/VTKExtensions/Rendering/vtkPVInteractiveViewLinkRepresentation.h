/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInteractiveViewLinkRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkPVInteractiveViewLinkRepresentation
  : public vtkLogoRepresentation
{
public:
  static vtkPVInteractiveViewLinkRepresentation* New();
  vtkTypeMacro(vtkPVInteractiveViewLinkRepresentation, vtkLogoRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Considering an eventPosition and current interaction state, this method will adjust
   * internal position and shape so the widget behave correctly, ie: resizable, movable, but not
   * going
   * over the edges of the render window
   */
  virtual void WidgetInteraction(double eventPos[2]) VTK_OVERRIDE;

protected:
  vtkPVInteractiveViewLinkRepresentation();
  ~vtkPVInteractiveViewLinkRepresentation();

  /**
   * Redefining method to avoid adjustment of image
   */
  virtual void AdjustImageSize(double o[2], double borderSize[2], double imageSize[2]) VTK_OVERRIDE;

private:
  vtkPVInteractiveViewLinkRepresentation(
    const vtkPVInteractiveViewLinkRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVInteractiveViewLinkRepresentation&) VTK_DELETE_FUNCTION;
};

#endif
