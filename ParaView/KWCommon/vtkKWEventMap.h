/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWEventMap.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWEventMap - map between mouse/keyboard event and actions
// .SECTION Description
//

#ifndef __vtkKWEventMap_h
#define __vtkKWEventMap_h

#include "vtkObject.h"

class VTK_EXPORT vtkKWEventMap : public vtkObject
{
public:
  static vtkKWEventMap *New();
  vtkTypeRevisionMacro(vtkKWEventMap, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  void AddMouseEvent(int button, int modifier, char *action);
  void AddKeyEvent(char key, int modifier, char *action);
  void AddKeySymEvent(char *keySym, char *action);
  
  void SetMouseEvent(int button, int modifier, char *action);
  void SetKeyEvent(char key, int modifier, char *action);
  void SetKeySymEvent(char *keySym, char *action);
  
  void RemoveMouseEvent(int button, int modifier, char *action);
  void RemoveKeyEvent(char key, int modifier, char *action);
  void RemoveKeySymEvent(char *keySym, char *action);
  
  char* FindMouseAction(int button, int modifier);
  char* FindKeyAction(char key, int modifier);
  char* FindKeySymAction(char *keySym);
  
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
    char *Action;
  };
//ETX
  
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
