/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVInteractorStyleControl.h
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
// .NAME vtkPVInteractorStyleControl - a frame with a scroll bar
// .SECTION Description
// The ScrollableFrame creates a frame with an attached scrollbar


#ifndef __vtkPVInteractorStyleControl_h
#define __vtkPVInteractorStyleControl_h

#include "vtkKWWidget.h"

class vtkCollection;
class vtkKWApplication;
class vtkKWFrame;
class vtkKWLabel;
class vtkKWLabeledFrame;
class vtkKWOptionMenu;
class vtkPVCameraManipulator;
class vtkPVWidget;

//BTX
template<class KeyType,class DataType> class vtkArrayMap;
template<class KeyType,class DataType> class vtkArrayMapIterator;
template<class DataType> class vtkVector;
//ETX

class VTK_EXPORT vtkPVInteractorStyleControl : public vtkKWWidget
{  
public:
  static vtkPVInteractorStyleControl* New();
  vtkTypeMacro(vtkPVInteractorStyleControl,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app, const char*);

  // Description:
  // Get the vtkKWWidget for the internal frame.
  vtkGetObjectMacro(LabeledFrame, vtkKWLabeledFrame);

  // Description:
  // Add manipulator to the list of manipulators.
  void AddManipulator(const char*, vtkPVCameraManipulator*);

  // Description:
  // Update menus after adding manipulators.
  void UpdateMenus();

  // Description:
  // Set label of the control widget.
  void SetLabel(const char*);

  // Description:
  // Set the specific manipulator for a mouse button and key
  // combination.
  int SetManipulator(int pos, const char*);
  int SetManipulator(int mouse, int key, const char*);
  vtkPVCameraManipulator* GetManipulator(int pos);
  vtkPVCameraManipulator* GetManipulator(int mouse, int key);
  vtkPVCameraManipulator* GetManipulator(const char* name);

  // Description:
  // Set the current manipulator to the specified one for the
  // mouse button and keypress combination.
  void SetCurrentManipulator(int pos, const char*);
  void SetCurrentManipulator(int mouse, int key, const char*);

  // Description:
  // In order for manipulators to work, you have to set them
  // on the window. This method sets the window.
  void SetManipulatorCollection(vtkCollection*);
  vtkGetObjectMacro(ManipulatorCollection, vtkCollection);

  // Description:
  // Set or get the default manipulator. The default manipulator is
  // the one that is present in menus (after UpdateMenus) which do not
  // have any manipulator set.
  vtkSetStringMacro(DefaultManipulator);
  vtkGetStringMacro(DefaultManipulator);

  // Description:
  // Read and store information to the registery.
  void ReadRegistery();
  void StoreRegistery();

  // Description:
  // Type or name of manipulator is used for storing in the registery.
  vtkSetStringMacro(RegisteryName);
  vtkGetStringMacro(RegisteryName);

  // Description:
  // Add argument that can be modified for specific manipulator.
  void AddArgument(const char* name, const char* manipulator,
                   vtkPVWidget* widget);

  // Description:
  // Callback for widget to call when user modifies UI.
  void ChangeArgument(const char* name, const char* widget);

  // Description
  // This is hack to convert it to Tcl.
  vtkGetObjectMacro(CurrentManipulator, vtkPVCameraManipulator);

protected:
  vtkPVInteractorStyleControl();
  ~vtkPVInteractorStyleControl();

  vtkKWLabeledFrame *LabeledFrame;
  vtkKWLabel *Labels[6];
  vtkKWOptionMenu *Menus[9];
  vtkKWFrame *ArgumentsFrame;
  vtkKWFrame *MenusFrame;


  vtkCollection *ManipulatorCollection;
  char* DefaultManipulator;
  char* RegisteryName;

//BTX
  typedef vtkVector<const char*> ArrayStrings;
  typedef vtkArrayMap<const char*,vtkPVCameraManipulator*> ManipulatorMap;
  typedef vtkArrayMapIterator<const char*,vtkPVCameraManipulator*> 
    ManipulatorMapIterator;
  typedef vtkArrayMap<const char*,vtkPVWidget*> WidgetsMap;
  typedef vtkArrayMap<const char*,vtkPVInteractorStyleControl::ArrayStrings*>
    MapStringToArrayStrings;

  vtkPVInteractorStyleControl::ManipulatorMap*          Manipulators;
  vtkPVInteractorStyleControl::WidgetsMap*              Widgets;
  vtkPVInteractorStyleControl::MapStringToArrayStrings* Arguments;
//ETX

  // This is hack to get tcl name;
  vtkPVCameraManipulator *CurrentManipulator;

private:
  vtkPVInteractorStyleControl(const vtkPVInteractorStyleControl&); // Not implemented
  void operator=(const vtkPVInteractorStyleControl&); // Not implemented
};


#endif


