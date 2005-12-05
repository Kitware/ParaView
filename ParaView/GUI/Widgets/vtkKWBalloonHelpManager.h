/*=========================================================================

  Module:    vtkKWBalloonHelpManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWBalloonHelpManager - a "balloon help " manager class
// .SECTION Description
// vtkKWBalloonHelpManager is a class that provides functionality
// to display balloon help (tooltips) for an application.
// An instance of this class is created in the vtkKWApplication class.
// When the balloon help string of a vtkKWWidget is set, the balloon help 
// manager instance of the widget's application is called to set up
// bindings automatically for this widget (see AddBindings()). These bindings
// will trigger the manager callbacks to display the balloon help string
// appropriately.
// .SECTION See Also
// vtkKWApplication vtkKWWidget

#ifndef __vtkKWBalloonHelpManager_h
#define __vtkKWBalloonHelpManager_h

#include "vtkKWObject.h"

class vtkKWTopLevel;
class vtkKWLabel;
class vtkKWWidget;

class KWWIDGETS_EXPORT vtkKWBalloonHelpManager : public vtkKWObject
{
public:
  static vtkKWBalloonHelpManager* New();
  vtkTypeRevisionMacro(vtkKWBalloonHelpManager,vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get balloon help visibility for all the widgets bound to this helper.
  virtual void SetVisibility(int);
  vtkGetMacro(Visibility, int);
  vtkBooleanMacro(Visibility, int);

  // Description:
  // Set the delay for the balloon help in milliseconds.
  vtkSetClampMacro(Delay, int, 0, 15000);
  vtkGetMacro(Delay, int);

  // Description:
  // Add/remove bindings for a given widget, effectively providing balloon help
  // feature for this widget.
  // On the widget side, one has to set the balloon help string or image.
  virtual void AddBindings(vtkKWWidget *widget);
  virtual void RemoveBindings(vtkKWWidget *widget);

  // Description:
  // Callbacks. Internal, do not use.
  virtual void TriggerCallback(vtkKWWidget *widget);
  virtual void DisplayCallback(vtkKWWidget *widget);
  virtual void CancelCallback();
  virtual void WithdrawCallback();

protected:
  vtkKWBalloonHelpManager();
  ~vtkKWBalloonHelpManager();

  int Visibility;
  int Delay;

  vtkKWTopLevel *TopLevel;
  vtkKWLabel    *Label;

  // Description:
  // The widget which balloon help is currently being displayed or pending.
  vtkKWWidget *CurrentWidget;
  virtual void SetCurrentWidget(vtkKWWidget *widget);

  // Description:
  // The Id of the Tk 'after' timer that will display the balloon help
  // after some delay.
  char *AfterTimerId;
  vtkSetStringMacro(AfterTimerId);

  // Description:
  // Create the UI.
  virtual void CreateBalloon();

  // Description:
  // Return true if the application is exiting, i.e. if it is not safe
  // to perform anything (balloon help is an asynchronous process)
  virtual int ApplicationInExit();

private:
  vtkKWBalloonHelpManager(const vtkKWBalloonHelpManager&);   // Not implemented.
  void operator=(const vtkKWBalloonHelpManager&);  // Not implemented.
};

#endif
