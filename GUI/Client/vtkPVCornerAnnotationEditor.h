/*=========================================================================

  Module:    vtkPVCornerAnnotationEditor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCornerAnnotationEditor - a corner annotation widget
// .SECTION Description
// A class that provides a UI for vtkCornerAnnotation. User can set the
// text for each corner, set the color of the text, and turn the annotation
// on and off.

#ifndef __vtkPVCornerAnnotationEditor_h
#define __vtkPVCornerAnnotationEditor_h

#include "vtkKWCornerAnnotationEditor.h"

class vtkCornerAnnotation;
class vtkKWView;
class vtkPVRenderView;
class vtkPVTraceHelper;

class VTK_EXPORT vtkPVCornerAnnotationEditor : public vtkKWCornerAnnotationEditor
{
public:
  static vtkPVCornerAnnotationEditor* New();
  vtkTypeRevisionMacro(vtkPVCornerAnnotationEditor,vtkKWCornerAnnotationEditor);
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

  // Description:
  // Set the maximum line height (override).
  virtual void SetMaximumLineHeight(float);

  // Description:
  // Set corner text
  virtual void SetCornerText(const char *txt, int corner);

  // Description:
  // Update the GUI according to the value of the ivars
  // Temporarily overridden
  virtual void Update();

  void UpdateCornerText();

  // Description:
  // Callbacks
  virtual void CornerTextCallback(int i);
  
  // Description:
  // Get the trace helper framework.
  vtkGetObjectMacro(TraceHelper, vtkPVTraceHelper);

protected:
  vtkPVCornerAnnotationEditor();
  ~vtkPVCornerAnnotationEditor();

  void SetCornerTextInternal(const char* text, int corner);

  vtkPVRenderView* View;

  virtual void Render();

  vtkCornerAnnotation     *InternalCornerAnnotation;
  vtkPVTraceHelper* TraceHelper;

private:
  vtkPVCornerAnnotationEditor(const vtkPVCornerAnnotationEditor&); // Not implemented
  void operator=(const vtkPVCornerAnnotationEditor&); // Not Implemented
};

#endif

