/*=========================================================================

  Module:    vtkPVTextPropertyEditor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVTextPropertyEditor - a vtkKWTextProperty with tracing
// .SECTION Description
// The vtkPVTextPropertyEditor creates a set of GUI components that can be displayed
// and used selectively to edit all or part of a vtkTextProperty object.
// .SECTION See Also
// vtkKWTextProperty

#ifndef __vtkPVTextPropertyEditor_h
#define __vtkPVTextPropertyEditor_h

#include "vtkKWTextPropertyEditor.h"

class vtkPVTraceHelper;

class VTK_EXPORT vtkPVTextPropertyEditor : public vtkKWTextPropertyEditor
{
public:
  static vtkPVTextPropertyEditor* New();
  vtkTypeRevisionMacro(vtkPVTextPropertyEditor,vtkKWTextPropertyEditor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // GUI components callbacks (overriden to provide trace)
  virtual void SetColor(double, double, double);
  virtual void SetColor(double *v) { this->SetColor(v[0], v[1], v[2]); };
  virtual void SetFontFamily(int);
  virtual void SetBold(int);
  virtual void SetItalic(int);
  virtual void SetShadow(int);
  virtual void SetOpacity(float);

  // Description:
  // Get the trace helper framework.
  vtkGetObjectMacro(TraceHelper, vtkPVTraceHelper);

protected:
  vtkPVTextPropertyEditor();
  ~vtkPVTextPropertyEditor();

  vtkPVTraceHelper* TraceHelper;

private:
  vtkPVTextPropertyEditor(const vtkPVTextPropertyEditor&); // Not implemented
  void operator=(const vtkPVTextPropertyEditor&); // Not implemented
};

#endif

