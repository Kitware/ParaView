/*=========================================================================

  Module:    vtkPVCornerAnnotation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCornerAnnotation - a corner annotation widget
// .SECTION Description
// A class that provides a UI for vtkCornerAnnotation. User can set the
// text for each corner, set the color of the text, and turn the annotation
// on and off.

#ifndef __vtkPVCornerAnnotation_h
#define __vtkPVCornerAnnotation_h

#include "vtkKWCornerAnnotation.h"

class vtkCornerAnnotation;
class vtkKWView;
class vtkPVRenderView;

class VTK_EXPORT vtkPVCornerAnnotation : public vtkKWCornerAnnotation
{
public:
  static vtkPVCornerAnnotation* New();
  vtkTypeRevisionMacro(vtkPVCornerAnnotation,vtkKWCornerAnnotation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the vtkKWView  that owns this annotation.
  virtual void SetView(vtkKWView*);
  vtkGetObjectMacro(View,vtkPVRenderView);

  // Description:
  // Export the corner annotation to a file.
  void SaveState(ofstream *file);

  // Description:
  // Set/Get the annotation visibility
  virtual void SetVisibility(int i);
  virtual int GetVisibility();
  vtkBooleanMacro(Visibility, int);
  
protected:
  vtkPVCornerAnnotation();
  ~vtkPVCornerAnnotation();

  vtkPVRenderView* View;

  virtual void Render();

  vtkCornerAnnotation     *InternalCornerAnnotation;

private:
  vtkPVCornerAnnotation(const vtkPVCornerAnnotation&); // Not implemented
  void operator=(const vtkPVCornerAnnotation&); // Not Implemented
};

#endif

