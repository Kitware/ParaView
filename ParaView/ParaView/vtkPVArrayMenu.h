/*=========================================================================

  Program:   ParaView
  Module:    vtkPVArrayMenu.h
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
// .NAME vtkPVArrayMenu menu for selecting arrays from the input.
// .SECTION Description
// This menu is pretty general, but expects a certain pattern to the 
// set get method.  It may be better to have explicit set/get method strings.
// I have been trying to get rid of the reference to PVSource, but this 
// reference is the only way I have to track when the input changes.
// I could make a subclass to do this, but I do not know how the more 
// general widget will be used.

#ifndef __vtkPVArrayMenu_h
#define __vtkPVArrayMenu_h

#include "vtkPVWidget.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWLabel.h"
#include "vtkDataSet.h"

class vtkCollection;

class VTK_EXPORT vtkPVArrayMenu : public vtkPVWidget
{
public:
  static vtkPVArrayMenu* New();
  vtkTypeMacro(vtkPVArrayMenu, vtkPVWidget);
  
  // Description:
  // Create the widget.
  void Create(vtkKWApplication *app);


  // Description:
  // Set the label.  The label can be used to get this widget
  // from a script.
  void SetLabel (const char* label);
  const char* GetLabel() {return this->Label->GetLabel();}


  // Description:
  // This only thing this reference is used for is getting the input data
  // which is used to add items to the menu.  It is not reference counted 
  // because the PVSource owns this widget.  I have been trying to get
  // rid of this reference to make the widget more general, but
  // it is the only (best) way I can track the changes of the input.
  void SetPVSource(vtkPVSource *pvs) { this->PVSource = pvs;}
  vtkPVSource *GetPVSource() {return this->PVSource;}

  // Description:
  // Only arrays with this number of components are added to the menu.
  // If this value is 0 or less, then all arrays are added to the menu.
  // The default value is 1.
  vtkGetMacro(NumberOfComponents, int);
  vtkSetMacro(NumberOfComponents, int);

  // Description:
  // This is one value that lets this widget interact with its associated 
  // object.  This specifies the name of the input (i.e Input, Source ...).
  // Most of the time this value should be set to "Input".
  // It defaults to NULL.
  vtkSetStringMacro(InputName);
  vtkGetStringMacro(InputName);
  
  // Description:
  // This is one value that lets this widget interact with its associated 
  // object.  This specifies the type of the attribute (i.e Scalars, Vectors ...).
  // It defaults to NULL.
  vtkSetStringMacro(AttributeName);
  vtkGetStringMacro(AttributeName);
  
  // Description:
  // This is the filter/object that will be modified by the widgtet when the 
  // selected array gets changed in the menu.  It should have methods like:
  // SelectInputScalars and GetInputScalarsSelection.
  // Description:
  vtkSetStringMacro(ObjectTclName);
  vtkGetStringMacro(ObjectTclName);



  // Description:
  // Gets called when the accept button is pressed.
  // This method may add an entry to the trace file.
  virtual void Accept();

  // Description:
  // Gets called when the reset button is pressed.
  virtual void Reset();

  // Description:
  // Set the menus value as a string.
  // Used by the Accept and Reset callbacks.
  // Can also be used from a script.
  void SetValue(const char* name);
  const char* GetValue() { return this->ArrayName;}



  // Description:
  // Direct access to the ArrayName is used internally by the Reset method. 
  // The methods "SetValue" should be used instead of this method.
  vtkSetStringMacro(ArrayName);

  // Description:
  // This is an internal method that is called when the menu is changed.
  void MenuEntryCallback(const char* name);

protected:
  vtkPVArrayMenu();
  ~vtkPVArrayMenu();
  vtkPVArrayMenu(const vtkPVArrayMenu&) {};
  void operator=(const vtkPVArrayMenu&) {};

  // The selected array name in the menu.  Current value of the widget.
  char *ArrayName;

  vtkPVSource *PVSource;
  int NumberOfComponents;

  // These are options that allow the widget to interact with its associated object.
  char*       InputName;
  char*       AttributeName;
  char*       ObjectTclName;

  // Subwidgets.
  vtkKWLabel *Label;
  vtkKWOptionMenu *Menu;


};

#endif
