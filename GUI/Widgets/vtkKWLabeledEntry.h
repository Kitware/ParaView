/*=========================================================================

  Module:    vtkKWLabeledEntry.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWLabeledEntry - an entry with a label
// .SECTION Description
// The LabeledEntry creates an entry with a label in front of it; both are
// contained in a frame


#ifndef __vtkKWLabeledEntry_h
#define __vtkKWLabeledEntry_h

#include "vtkKWLabeledWidget.h"

class vtkKWEntry;

class VTK_EXPORT vtkKWLabeledEntry : public vtkKWLabeledWidget
{
public:
  static vtkKWLabeledEntry* New();
  vtkTypeRevisionMacro(vtkKWLabeledEntry, vtkKWLabeledWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args = 0);

  // Description:
  // Get the internal object
  vtkGetObjectMacro(Entry, vtkKWEntry);
  
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
  vtkKWLabeledEntry();
  ~vtkKWLabeledEntry();

  vtkKWEntry *Entry;

  // Pack or repack the widget

  virtual void Pack();

private:
  vtkKWLabeledEntry(const vtkKWLabeledEntry&); // Not implemented
  void operator=(const vtkKWLabeledEntry&); // Not implemented
};

#endif

