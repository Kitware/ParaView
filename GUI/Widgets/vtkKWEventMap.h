/*=========================================================================

  Module:    vtkKWEventMap.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWEventMap - map between mouse/keyboard event and actions
// .SECTION Description
// vtkKWEventMap maintains 3 lists of events -- for mouse, keyboard, and
// keysym events.  The mouse event list maps between mouse button + modifier
// keys and actions.  The keyboard event list maps between keys + modifier
// keys and actions.  The keysym event list maps between keysyms and actions.

#ifndef __vtkKWEventMap_h
#define __vtkKWEventMap_h

#include "vtkObject.h"

class VTK_EXPORT vtkKWEventMap : public vtkObject
{
public:
  static vtkKWEventMap *New();
  vtkTypeRevisionMacro(vtkKWEventMap, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // In the following code:
  // button:   0 = Left, 1 = Middle, 2 = Right
  // modifier: 0 = button only, 1 = button + shift, 2 = button + control

  //BTX
  enum 
  {
    LeftButton   = 0,
    MiddleButton = 1,
    RightButton  = 2
  };
  
  enum 
  {
    NoModifier      = 0,
    ShiftModifier   = 1,
    ControlModifier = 2
  };
  //ETX


  //BTX
  struct MouseEvent
  {
    int Button;
    int Modifier;
    char *Action;
  };
  
  struct KeyEvent
  {
    char Key;
    int Modifier;
    char *Action;
  };
  
  struct KeySymEvent
  {
    char *KeySym;
    int Modifier;
    char *Action;
  };
  //ETX

  // ---------------------------------------------------------

  // Description:
  // Add a unique action with a specific mouse button and modifier key to
  // the list of mouse events.
  //BTX
  void AddMouseEvent(struct MouseEvent *me);
  //ETX
  void AddMouseEvent(int button, int modifier, const char *action);

  // Description:
  // Change the action to associate with a specific mouse button and modifier
  // key.  A mouse event with this button and modifier must have already have
  // been added.
  //BTX
  void SetMouseEvent(struct MouseEvent *me);
  //ETX
  void SetMouseEvent(int button, int modifier, const char *action);

  // Description:
  // Get the mouse event at the specified index.
  //BTX
  struct MouseEvent* GetMouseEvent(int index);
  //ETX
  
  // Description:
  // Remove the action associated with this mouse button and modifier key from
  // the list of mouse events (or all actions if action is NULL).
  //BTX
  void RemoveMouseEvent(struct MouseEvent *me);
  //ETX
  void RemoveMouseEvent(int button, int modifier, const char *action = NULL);
  void RemoveAllMouseEvents();

  // Description:
  // Return the string for the action of the mouse event indicated by this
  // button and modifier.
  const char* FindMouseAction(int button, int modifier);
  
  // Description:
  // Return the total number of mouse events.
  vtkGetMacro(NumberOfMouseEvents, int);
  
  // ---------------------------------------------------------

  // Description:
  // Add a unique action with a specific key and modifier key to
  // the list of key events.
  //BTX
  void AddKeyEvent(struct KeyEvent *me);
  //ETX
  void AddKeyEvent(char key, int modifier, const char *action);
  
  // Description:
  // Change the action to associate with a specific key and modifier key.
  // A key event with this key and modifier must have already been added.
  void SetKeyEvent(char key, int modifier, const char *action);
  
  // Description:
  // Get the key event at the specified index.
  //BTX
  struct KeyEvent* GetKeyEvent(int index);
  //ETX
  
  // Description:
  // Remove the action associated with this key and modifier key from the list
  // of key events (or all actions if action is NULL)..
  void RemoveKeyEvent(char key, int modifier, const char *action = NULL);
  void RemoveAllKeyEvents();
  
  // Description:
  // Return the string for the action of the key event indicated by this key
  // and modifier.
  const char* FindKeyAction(char key, int modifier);
  
  // Description:
  // Return the total number of key events.
  vtkGetMacro(NumberOfKeyEvents, int);
  
  // ---------------------------------------------------------

  // Description:
  // Add a unique action with a specific keysym to the list of keysym events.
  //BTX
  void AddKeySymEvent(struct KeySymEvent *me);
  //ETX
  void AddKeySymEvent(const char *keySym, int modifier, const char *action);
  
  // Description:
  // Change the action to associate with a specific keysym.  A keysym event
  // with this keysym must have already been added.
  void SetKeySymEvent(const char *keySym, int modifier, const char *action);
  
  // Description:
  // Get the keysym event at the specified index.
  //BTX
  struct KeySymEvent* GetKeySymEvent(int index);
  //ETX
  
  // Description:
  // Remove the action assiciated with this keysym from the list of keysym
  // events (or all actions if action is NULL)..
  void RemoveKeySymEvent(const char *keySym, int modifier, const char *action = NULL);
  void RemoveAllKeySymEvents();
  
  // Description:
  // Return the string for the action of the keysym event indicated by this
  // keysym.
  const char* FindKeySymAction(const char *keySym, int modifier);
  
  // Description:
  // Return the total number of keysym events.
  vtkGetMacro(NumberOfKeySymEvents, int);

  // Description:
  // Shallow copy.
  void ShallowCopy(vtkKWEventMap *tprop);
  
protected:
  vtkKWEventMap();
  ~vtkKWEventMap();
  
  MouseEvent *MouseEvents;
  KeyEvent *KeyEvents;
  KeySymEvent *KeySymEvents;

  int NumberOfMouseEvents;
  int NumberOfKeyEvents;
  int NumberOfKeySymEvents;
  
private:
  vtkKWEventMap(const vtkKWEventMap&);  // Not implemented
  void operator=(const vtkKWEventMap&);  // Not implemented
};

#endif

