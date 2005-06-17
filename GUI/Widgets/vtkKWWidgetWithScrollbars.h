/*=========================================================================

  Module:    vtkKWWidgetWithScrollbars.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWWidgetWithScrollbars - an abstract class for composite widgets associated to two vtkKWScrollbar's
// .SECTION Description
// This provide a boilerplate for a composite widget associated to a horizontal
// and vertical scrollbars.

#ifndef __vtkKWWidgetWithScrollbars_h
#define __vtkKWWidgetWithScrollbars_h

#include "vtkKWCompositeWidget.h"

class vtkKWScrollbar;

class KWWIDGETS_EXPORT vtkKWWidgetWithScrollbars : public vtkKWCompositeWidget
{
public:
  vtkTypeRevisionMacro(vtkKWWidgetWithScrollbars,vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Show/Hide vertical scrollbar (default to On).
  virtual void SetShowVerticalScrollbar(int val);
  vtkGetMacro(ShowVerticalScrollbar, int);
  vtkBooleanMacro(ShowVerticalScrollbar, int);

  // Description:
  // Show/Hide horizontal scrollbar (default to On).
  virtual void SetShowHorizontalScrollbar(int val);
  vtkGetMacro(ShowHorizontalScrollbar, int);
  vtkBooleanMacro(ShowHorizontalScrollbar, int);

  // Description:
  // Access the internal scrollbars.
  vtkGetObjectMacro(VerticalScrollBar, vtkKWScrollbar);
  vtkGetObjectMacro(HorizontalScrollBar, vtkKWScrollbar);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWWidgetWithScrollbars();
  ~vtkKWWidgetWithScrollbars();

  // Description:
  // Scrollbar visibility flags
  int ShowVerticalScrollbar;
  int ShowHorizontalScrollbar;

  // Description:
  // Scrollbars
  vtkKWScrollbar *VerticalScrollBar;
  vtkKWScrollbar *HorizontalScrollBar;

  // Description:
  // Create scrollbars and associate the scrollbars to a widget by
  // setting up the callbacks between both instances.
  // The associated *has* to be made for this class to work, but
  // since we do not know the internal widget at that point, it is up
  // to the subclass to reimplement both Create*Scrollbar() methods
  // and have them simply call the super and the Associate*Scrollbar() 
  // methods with the internal argument as parameter.
  virtual void CreateHorizontalScrollbar(vtkKWApplication *app);
  virtual void CreateVerticalScrollbar(vtkKWApplication *app);
  virtual void AssociateHorizontalScrollbarToWidget(vtkKWWidget *widget);
  virtual void AssociateVerticalScrollbarToWidget(vtkKWWidget *widget);

  // Description:
  // Pack or repack the widget. This should be implemented by subclasses,
  // but a convenience function PackScrollbarsWithWidget() can be
  // called from the subclass just as easily.
  virtual void Pack() = 0;
  virtual void PackScrollbarsWithWidget(vtkKWWidget *widget);

private:

  vtkKWWidgetWithScrollbars(const vtkKWWidgetWithScrollbars&); // Not implemented
  void operator=(const vtkKWWidgetWithScrollbars&); // Not implemented
};

#endif
