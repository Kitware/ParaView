/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWMenu.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkKWMenu - a menu widget
// .SECTION Description
// This class is the Menu abstraction for the
// Kitware toolkit. It provides a c++ interface to
// the TK menu widgets used by the Kitware toolkit.

#ifndef __vtkKWMenu_h
#define __vtkKWMenu_h

#include "vtkKWWidget.h"


class VTK_EXPORT vtkKWMenu : public vtkKWWidget
{
public:
  static vtkKWMenu* New();
  vtkTypeMacro(vtkKWMenu,vtkKWWidget);
  
  void Create(vtkKWApplication* app, const char* args);
  
  // Description: 
  // Append a separator to the menu.
  void AddSeparator();
  
  // Description: 
  // Append a sub menu to the current menu.
  void AddCascade(const char* label, vtkKWMenu*, int underline , const char* help = 0);
  
  // Description: 
  // Append a CheckButton menu item to the current menu.
  void AddCheckButton(const char* label, vtkKWObject* Object, 
		      const char* MethodAndArgString , const char* help = 0);
  void AddCheckButton(const char* label, vtkKWObject* Object, 
		      const char* MethodAndArgString , int underline,
		      const char* help = 0);

  // Description: 
  // Append a standard menu item and command to the current menu.
  void AddCommand(const char* label, vtkKWObject* Object,
		  const char* MethodAndArgString , const char* help = 0);
  void AddCommand(const char* label, vtkKWObject* Object,
		  const char* MethodAndArgString , int underline, 
		  const char* help = 0);

  // Description: 
  // Append a radio menu item and command to the current menu.
  // The radio group is specified by the buttonVar value.
  void AddRadioButton(int value, const char* label, const char* buttonVar, 
		      vtkKWObject* Called, 
		      const char* MethodAndArgString, const char* help = 0);
  void AddRadioButton(int value, const char* label, const char* buttonVar, 
		      vtkKWObject* Called, 
		      const char* MethodAndArgString, int underline,  
		      const char* help = 0);

  // Description:
  // Same as add commands, but insert at a given integer position.
  void InsertSeparator(int position);
  
  void InsertCascade(int position, const char* label,  vtkKWMenu*, int underline, const char* help = 0  );
  
  // Description:
  // Insert a check button at a given position.
  void InsertCheckButton(int position, 
			 const char* label, vtkKWObject* Object, 
			 const char* MethodAndArgString , const char* help = 0);
  void InsertCheckButton(int position, 
			 const char* label, vtkKWObject* Object, 
			 const char* MethodAndArgString , 
			 int underline, const char* help = 0);
  
  // Description:
  // Insert a menu item at a given position.
  void InsertCommand(int position, const char* label, vtkKWObject* Object,
		     const char* MethodAndArgString , const char* help = 0);
  void InsertCommand(int position, const char* label, vtkKWObject* Object,
		     const char* MethodAndArgString , 
		     int underline, const char* help = 0);
  
  // Description: 
  // Add a radio button menu item.  You must create a variable to store
  // the value of the button.
  char* CreateRadioButtonVariable(vtkKWObject* Object, const char* varname);
  int GetRadioButtonValue(vtkKWObject* Object, const char* varname);
  void CheckRadioButton(vtkKWObject *Object, const char *varname, int id);
  void InsertRadioButton(int position, int value, const char* label, 
                         const char* buttonVar, vtkKWObject* Called, 
			 const char* MethodAndArgString, const char* help = 0);
  void InsertRadioButton(int position, int value, const char* label, 
                         const char* buttonVar, vtkKWObject* Called, 
			 const char* MethodAndArgString, 
			 int underline, const char* help = 0);

  // Description:
  // Call the menu item callback at the given index
  void Invoke(int position);

  // Description:
  // Delete the menu item at the given position.
  // Be careful, there is a bug in tk, that will break other items
  // in the menu below the one being deleted, unless a new item is added.
  void DeleteMenuItem(int position);
  void DeleteMenuItem(const char* menuname);
  void DeleteAllMenuItems();
  
  // Description:
  // Return the integer index of the menu item by string
  int GetIndex(const char* menuname);

  // Description:
  // Checks if an item is in the menu
  int IsItemPresent(const char* menuname);
  
  // Description:
  // Call back for active menu item doc line help
  void DisplayHelp(const char*);
  
  // Description:
  // Option to make this menu a tearoff menu.  By dafault this value is off.
  void SetTearOff(int val);
  vtkGetMacro(TearOff, int);
  vtkBooleanMacro(TearOff, int);

  // Description:
  // Set state of the menu entry with a given index.
  void SetState(int index, int state);
  void SetState(const char* item, int state);

  // Description:
  // Set command of the menu entry with a given index.
  void SetCommand(int index, vtkKWObject* object, const char* MethodAndArgString);
  void SetCommand(const char* item, vtkKWObject* object, const char* method);

//BTX
  enum { Normal = 0, Active, Disabled };
//ETX
protected:
  
  vtkKWMenu();
  ~vtkKWMenu();

  void AddGeneric(const char* addtype, const char* label, vtkKWObject* Object,
		  const char* MethodAndArgString, const char* extra, const char* help);
  void InsertGeneric(int position, const char* addtype, const char* label, 
		     vtkKWObject* Object,
		     const char* MethodAndArgString, const char* extra, const char* help);

  int TearOff;
  
private:
  vtkKWMenu(const vtkKWMenu&); // Not implemented
  void operator=(const vtkKWMenu&); // Not implemented
};


#endif


