/*=========================================================================

  Module:    vtkKWLabeledWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWLabeledWidget - an abstract class widget with a label
// .SECTION Description
// This class creates a superclass for composite widgets associating a label to 
// a widget.
// .SECTION See Also
// vtkKWLabeledEntry

#ifndef __vtkKWLabeledWidget_h
#define __vtkKWLabeledWidget_h

#include "vtkKWWidget.h"

class vtkKWApplication;
class vtkKWLabel;

class VTK_EXPORT vtkKWLabeledWidget : public vtkKWWidget
{
public:
  static vtkKWLabeledWidget* New();
  vtkTypeRevisionMacro(vtkKWLabeledWidget, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char *args = 0);

  // Description:
  // Get the internal label
  // The label is lazy evaluated/created, i.e. GetLabel() 
  // will create the label if it does not exist, otherwise it is
  // not allocated/created. In subclasses, use HasLabel() to check
  // that this widget has a label instead of accessing GetLabel() which
  // would automatically allocate the label. 
  virtual vtkKWLabel* GetLabel();
  virtual int HasLabel();

  // Description:
  // Show/Hide the label.
  // Note: On by default. If you know you are not going to show the label,
  // you may want to set that flag as early as possible (ideally, before
  // calling Create() in order to lower the footprint of the widget: the
  // label won't be allocated and created if there is no need to show it).
  // Later on, you can still show the label, it will be allocated and 
  // created on the fly.
  virtual void SetShowLabel(int);
  vtkBooleanMacro(ShowLabel, int);
  vtkGetMacro(ShowLabel, int);

  // Description:
  // Convenience method to set the contents label.
  // Note: it will create the label on the fly, so use it only if
  // you are sure that you will display/use the label.
  void SetLabel(const char *);
  
  // Description:
  // Convenience method to set/get the label width.
  // Note: it will create the label on the fly, so use it only if
  // you are sure that you will display/use the label.
  void SetLabelWidth(int width);
  int GetLabelWidth();
  
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
  vtkKWLabeledWidget();
  ~vtkKWLabeledWidget();

  int ShowLabel;

  // Pack or repack the widget. To be implemented by subclasses.

  virtual void Pack() {};

  virtual void CreateLabel(vtkKWApplication *app);

private:

  // In private: to allow lazy evaluation. GetLabel() will create the
  // label if it does not exist. This allow this object to remain lightweight. 

  vtkKWLabel      *Label;

  vtkKWLabeledWidget(const vtkKWLabeledWidget&); // Not implemented
  void operator=(const vtkKWLabeledWidget&); // Not implemented
};


#endif

