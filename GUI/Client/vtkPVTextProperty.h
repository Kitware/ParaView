/*=========================================================================

  Module:    vtkPVTextProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVTextProperty - a vtkKWTextProperty with tracing
// .SECTION Description
// The vtkPVTextProperty creates a set of GUI components that can be displayed
// and used selectively to edit all or part of a vtkTextProperty object.
// .SECTION See Also
// vtkKWTextProperty

#ifndef __vtkPVTextProperty_h
#define __vtkPVTextProperty_h

#include "vtkKWTextProperty.h"

class vtkPVTraceHelper;

class VTK_EXPORT vtkPVTextProperty : public vtkKWTextProperty
{
public:
  static vtkPVTextProperty* New();
  vtkTypeRevisionMacro(vtkPVTextProperty,vtkKWTextProperty);
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
  vtkPVTextProperty();
  ~vtkPVTextProperty();

  vtkPVTraceHelper* TraceHelper;

private:
  vtkPVTextProperty(const vtkPVTextProperty&); // Not implemented
  void operator=(const vtkPVTextProperty&); // Not implemented
};

#endif

