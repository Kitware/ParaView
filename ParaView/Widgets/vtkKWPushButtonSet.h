/*=========================================================================

  Module:    vtkKWPushButtonSet.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWPushButtonSet - a "set of push buttons" widget
// .SECTION Description
// A simple widget representing a set of push buttons. Pushbuttons
// can be created, removed or queried based on unique ID provided by the user
// (ids are not handled by the class since it is likely that they will be defined
// as enum's or #define by the user for easier retrieval, instead of having
// ivar's that would store the id's returned by the class).
// Pushbuttons are packed (gridded) in the order they were added.

#ifndef __vtkKWPushButtonSet_h
#define __vtkKWPushButtonSet_h

#include "vtkKWWidget.h"

class vtkKWApplication;
class vtkKWPushButton;

//BTX
template<class DataType> class vtkLinkedList;
template<class DataType> class vtkLinkedListIterator;
//ETX

class VTK_EXPORT vtkKWPushButtonSet : public vtkKWWidget
{
public:
  static vtkKWPushButtonSet* New();
  vtkTypeRevisionMacro(vtkKWPushButtonSet,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget (a frame holding all the pushbuttons).
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Add a pushbutton to the set.
  // The id has to be unique among the set.
  // Text can be provided to set the pushbutton label.
  // Object and method parameters, if any, will be used to set the command.
  // A help string will be used, if any, to set the baloon help. 
  // Return 1 on success, 0 otherwise.
  int AddButton(int id, 
                const char *text = 0, 
                vtkKWObject *object = 0, 
                const char *method_and_arg_string = 0,
                const char *balloonhelp_string = 0);

  // Description:
  // Get a pushbutton from the set, given its unique id.
  // It is advised not to temper with the pushbutton var name or value :)
  // Return a pointer to the pushbutton, or NULL on error.
  vtkKWPushButton* GetButton(int id);
  int HasButton(int id);

  // Description:
  // Convenience method to hide/show a button
  void HideButton(int id);
  void ShowButton(int id);
  void SetButtonVisibility(int id, int flag);
  int GetNumberOfVisibleButtons();

  // Description:
  // Remove all buttons
  void DeleteAllButtons();

  // Description:
  // Set the widget packing order to be horizontal (default is vertical).
  // This means that given the insertion order of the button in the set,
  // the buttons will be packed in the horizontal direction.
  void SetPackHorizontally(int);
  vtkBooleanMacro(PackHorizontally, int);
  vtkGetMacro(PackHorizontally, int);

  // Description:
  // Set the maximum number of widgets that will be packed in the packing
  // direction (i.e. horizontally or vertically). Default is 0, meaning that
  // all widgets are packed along the same direction. If 3 (for example) and
  // direction is horizontal, you end up with 3 columns.
  void SetMaximumNumberOfWidgetInPackingDirection(int);
  vtkGetMacro(MaximumNumberOfWidgetInPackingDirection, int);

  // Description:
  // Set the buttons padding.
  virtual void SetPadding(int x, int y);

  // Description:
  // Set the buttons border width.
  virtual void SetBorderWidth(int bd);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWPushButtonSet();
  ~vtkKWPushButtonSet();

  int PackHorizontally;
  int MaximumNumberOfWidgetInPackingDirection;
  int PadX;
  int PadY;

  //BTX

  // A pushbutton slot associates a pushbutton to a unique Id
  // No, I don't want to use a map between those two, for the following 
  // reasons:
  // a), we might need more information in the future, b) a map 
  // Register/Unregister pointers if they are pointers to VTK objects.
 
  class ButtonSlot
  {
  public:
    int Id;
    vtkKWPushButton *Button;
  };

  typedef vtkLinkedList<ButtonSlot*> ButtonsContainer;
  typedef vtkLinkedListIterator<ButtonSlot*> ButtonsContainerIterator;
  ButtonsContainer *Buttons;

  // Helper methods

  ButtonSlot* GetButtonSlot(int id);

  //ETX

  void Pack();

private:
  vtkKWPushButtonSet(const vtkKWPushButtonSet&); // Not implemented
  void operator=(const vtkKWPushButtonSet&); // Not implemented
};

#endif

