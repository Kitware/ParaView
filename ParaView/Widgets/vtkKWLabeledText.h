/*=========================================================================

  Module:    vtkKWLabeledText.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWLabeledText - a text widget with a label
// .SECTION Description
// This class creates an text widget with another label on top of it; both are
// contained in a frame
// .SECTION See Also
// vtkKWText

#ifndef __vtkKWLabeledText_h
#define __vtkKWLabeledText_h

#include "vtkKWLabeledWidget.h"

class vtkKWText;

class VTK_EXPORT vtkKWLabeledText : public vtkKWLabeledWidget
{
public:
  static vtkKWLabeledText* New();
  vtkTypeRevisionMacro(vtkKWLabeledText, vtkKWLabeledWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args = 0);

  // Description:
  // Get the internal object
  vtkGetObjectMacro(Text, vtkKWText);

  // Description:
  // Set the string that enables balloon help for this widget.
  // Override to pass down to children.
  virtual void SetBalloonHelpString(const char *str);
  virtual void SetBalloonHelpJustification(int j);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWLabeledText();
  ~vtkKWLabeledText();

  vtkKWText *Text;

  // Pack or repack the widget

  virtual void Pack();

private:
  vtkKWLabeledText(const vtkKWLabeledText&); // Not implemented
  void operator=(const vtkKWLabeledText&); // Not implemented
};


#endif

