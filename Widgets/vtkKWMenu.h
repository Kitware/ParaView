/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWMenu.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

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

  // Description: 
  // Append a standard menu item and command to the current menu.
  void AddCommand(const char* label, vtkKWObject* Object,
		  const char* MethodAndArgString , const char* help = 0);

  // Description: 
  // Append a radio menu item and command to the current menu.
  // The radio group is specified by the buttonVar value.
  void AddRadioButton(int value, const char* label, const char* buttonVar, 
		      vtkKWObject* Called, 
		      const char* MethodAndArgString, const char* help = 0);

  // Description:
  // Same as add commands, but insert at a given integer position.
  void InsertSeparator(int position);
  
  void InsertCascade(int position, const char* label,  vtkKWMenu*, int underline, const char* help = 0  );
  
  // Description:
  // Insert a check button at a given position.
  void InsertCheckButton(int position, 
			 const char* label, vtkKWObject* Object, 
			 const char* MethodAndArgString , const char* help = 0);
  
  // Description:
  // Insert a menu item at a given position.
  void InsertCommand(int position, const char* label, vtkKWObject* Object,
		     const char* MethodAndArgString , const char* help = 0);
  
  // Description: 
  // Add a radio button menu item.  You must create a variable to store
  // the value of the button.
  char* CreateRadioButtonVariable(vtkKWObject* Object, const char* varname);
  int GetRadioButtonValue(vtkKWObject* Object, const char* varname);
  void CheckRadioButton(vtkKWObject *Object, const char *varname, int id);
  void InsertRadioButton(int position, int value, const char* label, 
                         const char* buttonVar, vtkKWObject* Called, 
			 const char* MethodAndArgString, const char* help = 0);

  // Description:
  // Call the menu item callback at the given index
  void Invoke(int position);

  // Description:
  // Delete the menu item at the give position.
  // Be careful, there is a bug in tk, that will break other items
  // in the menu below the one being deleted, unless a new item is added.
  void DeleteMenuItem(int position);
  void DeleteMenuItem(const char* menuname);

  // Description:
  // Retrun the integer index of the menu item by string
  int GetIndex(const char* menuname);
  
  // Description:
  // Call back for active menu item doc line help
  void DisplayHelp(const char*);
protected:
  vtkKWMenu();
  ~vtkKWMenu();
  vtkKWMenu(const vtkKWMenu&) {};
  void operator=(const vtkKWMenu&) {};

  void AddGeneric(const char* addtype, const char* label, vtkKWObject* Object,
		  const char* MethodAndArgString, const char* extra, const char* help);
  void InsertGeneric(int position, const char* addtype, const char* label, 
		     vtkKWObject* Object,
		     const char* MethodAndArgString, const char* extra, const char* help);
  
};


#endif


