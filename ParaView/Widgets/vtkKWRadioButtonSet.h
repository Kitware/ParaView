/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWRadioButtonSet.h
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
// .NAME vtkKWRadioButtonSet - a "set of radio buttons" widget
// .SECTION Description
// A simple widget representing a set of radio buttons. Only one button
// can be selected at a time, and this widget takes care of unselection/selection
// by sharing the same variable name among the radio buttons. Radiobuttons
// can be created, removed or queried based on unique ID provided by the user
// (ids are not handled by the class since it is likely that they will be defined
// as enum's or #define by the user for easier retrieval, instead of having
// ivar's that would store the id's returned by the class).
// Since the radiobuttons share the same variable name, each button needs to be
// assigned a unique value too. This value is the id by default. 
// Radiobuttons are packed in the order they were added.

#ifndef __vtkKWRadioButtonSet_h
#define __vtkKWRadioButtonSet_h

#include "vtkKWWidget.h"

class vtkKWApplication;
class vtkKWRadioButton;

//BTX
template<class DataType> class vtkLinkedList;
template<class DataType> class vtkLinkedListIterator;
//ETX

class VTK_EXPORT vtkKWRadioButtonSet : public vtkKWWidget
{
public:

  static vtkKWRadioButtonSet* New();
  vtkTypeRevisionMacro(vtkKWRadioButtonSet,vtkKWWidget);

  // Description:
  // Create the widget (a frame holding all the radiobuttons).
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Add a radiobutton to the set.
  // The id has to be unique among the set.
  // Text can be provided to set the radiobutton label.
  // Object and method parameters, if any, will be used to set the command.
  // A help string will be used, if any, to set the baloon help. 
  // Return 1 on success, 0 otherwise.
  int AddRadioButton(int id, 
                     const char *text, 
                     vtkKWObject *object = 0, 
                     const char *method_and_arg_string = 0,
                     const char *balloonhelp_string = 0);

  // Description:
  // Get a radiobutton from the set, given its unique id.
  // It is advised not to temper with the radiobutton var name or value :)
  // Return a pointer to the radiobutton, or NULL on error.
  vtkKWRadioButton* GetRadioButton(int id);
  int HasRadioButton(int id);

  // Description:
  // Convenience method to select a particular button or query if it is selected.
  void SelectRadioButton(int id);
  int IsRadioButtonSelected(int id);

  // Description:
  // Enable/Disable this widget. This propagates SetEnabled() calls to all
  // radiobuttons.
  virtual void SetEnabled(int);

protected:
  vtkKWRadioButtonSet();
  ~vtkKWRadioButtonSet();

  //BTX

  // A radiobutton slot associates a radiobutton to a unique Id
  // No, I don't want to use a map between those two, for the following reasons:
  // a), we might need more information in the future, b) a map 
  // Register/Unregister pointers if they are pointers to VTK objects.
 
  class RadioButtonSlot
  {
  public:
    int Id;
    vtkKWRadioButton *RadioButton;
  };

  typedef vtkLinkedList<RadioButtonSlot*> RadioButtonsContainer;
  typedef vtkLinkedListIterator<RadioButtonSlot*> RadioButtonsContainerIterator;
  RadioButtonsContainer *RadioButtons;

  // Helper methods

  RadioButtonSlot* GetRadioButtonSlot(int id);

  //ETX

private:
  vtkKWRadioButtonSet(const vtkKWRadioButtonSet&); // Not implemented
  void operator=(const vtkKWRadioButtonSet&); // Not implemented
};

#endif
