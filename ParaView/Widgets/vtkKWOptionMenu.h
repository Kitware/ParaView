/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
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
// .NAME vtkKWOptionMenu - an option menu widget
// .SECTION Description
// A widget that looks like a button but when pressed provides a list
// of options that the user can select.

#ifndef __vtkKWOptionMenu_h
#define __vtkKWOptionMenu_h

#include "vtkKWWidget.h"

class vtkKWApplication;
class vtkKWMenu;

class VTK_EXPORT vtkKWOptionMenu : public vtkKWWidget
{
public:
  static vtkKWOptionMenu* New();
  vtkTypeRevisionMacro(vtkKWOptionMenu,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Set/Get the current entry of this optionmenu
  const char *GetValue();
  void SetValue(const char *);
  void SetCurrentEntry(const char *name);
  void SetCurrentImageEntry(const char *image_name);

  // Description:
  // Get the menu.
  vtkGetObjectMacro(Menu, vtkKWMenu);

  // Description:
  // Add/Insert entries to an option menu, with or without a command.
  void AddEntry(const char *name);
  void AddEntryWithCommand(const char *name, vtkKWObject *obj,
                           const char *method, const char *options = 0);
  void AddImageEntryWithCommand(const char *image_name, vtkKWObject *obj,
                                const char *method, const char *options = 0);
  void AddSeparator();

  // Description:
  // Remove entry from an option menu.
  void DeleteEntry(const char *name);
  void DeleteEntry(int index);
  
  // Description:
  // Has entry ?
  int HasEntry(const char *name);
  int GetNumberOfEntries();

  // Description:
  // Get entry label
  const char *GetEntryLabel(int index);
  
  // Description:
  // Remove all entries from the option menu.
  void ClearEntries();
  
  // Description
  // Set the indicator On/Off. To be called after creation.
  void IndicatorOn();
  void IndicatorOff();

protected:
  vtkKWOptionMenu();
  ~vtkKWOptionMenu();

  char *CurrentValue;  
  vtkKWMenu *Menu;

private:
  vtkKWOptionMenu(const vtkKWOptionMenu&); // Not implemented
  void operator=(const vtkKWOptionMenu&); // Not implemented
};


#endif



