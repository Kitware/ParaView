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
// .NAME vtkPVView
// .SECTION Description
//

#ifndef __vtkPVView_h
#define __vtkPVView_h

#include "vtkView.h"

class VTK_EXPORT vtkPVView : public vtkView
{
public:
  vtkTypeMacro(vtkPVView, vtkView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the position on this view in the multiview configuration.
  // This can be called only after Initialize().
  // @CallOnAllProcessess
  virtual void SetPosition(int, int) = 0;

  // Description:
  // Set the size of this view in the multiview configuration.
  // This can be called only after Initialize().
  // @CallOnAllProcessess
  virtual void SetSize(int, int) = 0;

  // Description:
  // Triggers a high-resolution render.
  // @CallOnAllProcessess
  virtual void StillRender();

  // Description:
  // Triggers a interactive render. Based on the settings on the view, this may
  // result in a low-resolution rendering or a simplified geometry rendering.
  // @CallOnAllProcessess
  virtual void InteractiveRender();

  // Description:
  // Returns if distributed rendering is enabled. Representations may use this
  // value to decide where the geometry should be delivered for rendering.
  vtkGetMacro(UseDistributedRendering, bool);

  vtkGetMacro(UseLevelOfDetail, bool);
  vtkGetMacro(UseOrderedCompositing, bool);

  vtkGetMacro(LODRefinement, double);
  vtkSetClampMacro(LODRefinement, double, 0.0, 1.0);

//BTX
protected:
  vtkPVView();
  ~vtkPVView();

  bool UseDistributedRendering;
  bool UseLevelOfDetail;
  double LODRefinement; // 0.0 is min res., 1.1 is max res.
  bool UseOrderedCompositing;

private:
  vtkPVView(const vtkPVView&); // Not implemented
  void operator=(const vtkPVView&); // Not implemented
//ETX
};

#endif

