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
 * @class   vtkViewLayout
 * @brief   used by vtkSMViewLayoutProxy.
 *
 * vtkViewLayout is the server-side object corresponding to
 * vtkSMViewLayoutProxy. vtkSMViewLayoutProxy corresponds to a single layout
 * of views. In ParaView UI, this corresponds to a single tab. In tile-display
 * mode, the server-side only has "1 tab", in a manner of speaking. Thus, we
 * need to ensure that the server shows the views laid out in the active tab.
 * This class helps vtkSMViewLayoutProxy do that.
 * `vtkSMViewLayoutProxy:ShowViewsOnTileDisplay` calls methods on this object on
 * the server side to activate it.
*/

#ifndef vtkViewLayout_h
#define vtkViewLayout_h

#include "vtkObject.h"
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkViewLayout : public vtkObject
{
public:
  static vtkViewLayout* New();
  vtkTypeMacro(vtkViewLayout, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  void ResetTileDisplay();
  void ShowOnTileDisplay(unsigned int);

protected:
  vtkViewLayout();
  ~vtkViewLayout();

private:
  vtkViewLayout(const vtkViewLayout&) VTK_DELETE_FUNCTION;
  void operator=(const vtkViewLayout&) VTK_DELETE_FUNCTION;
};

#endif
