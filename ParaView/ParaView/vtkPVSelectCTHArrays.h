/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSelectCTHArrays.h
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
// .NAME vtkPVSelectCTHArrays - Widget for ExtractCTHPart filter.
// .SECTION Description
// This widget is for the ExtractCTHPart filter.  It allows the user
// to select multiple volume-fraction cell arrays.

#ifndef __vtkPVSelectCTHArrays_h
#define __vtkPVSelectCTHArrays_h

#include "vtkPVWidget.h"
class vtkKWPushButton;
class vtkKWWidget;
class vtkKWListBox;
class vtkCollection;
class vtkPVInputMenu;
class vtkPVSource;
class vtkKWLabel;
class vtkKWCheckButton;
class vtkStringList;


class VTK_EXPORT vtkPVSelectCTHArrays : public vtkPVWidget
{
public:
  static vtkPVSelectCTHArrays* New();
  vtkTypeRevisionMacro(vtkPVSelectCTHArrays, vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Set up the UI for this source
  void Create(vtkKWApplication *app);

  // Description:
  // Save this source to a file.
  void SaveInBatchScript(ofstream *file);

  // Description:
  // Button callbacks.
  void ShowAllArraysCheckCallback();

  // Description:
  // Access metod necessary for scripting.
  void ClearAllSelections();
  void SetSelectState(const char* arrayName, int val);

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // Called when the Accept button is pressed.  It moves the widget values to the 
  // VTK calculator filter.
  virtual void AcceptInternal(const char* vtkSourceTclName);
  
  // Description:
  // This method resets the widget values from the VTK filter.
  virtual void ResetInternal();

  // Description:
  // This input menu supplies the array options.
  virtual void SetInputMenu(vtkPVInputMenu*);
  vtkGetObjectMacro(InputMenu, vtkPVInputMenu);

  // Description:
  // This is called to update the menus if something (InputMenu) changes.
  virtual void Update();

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // parameters.
  vtkPVSelectCTHArrays* ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

protected:
  vtkPVSelectCTHArrays();
  ~vtkPVSelectCTHArrays();

  vtkKWWidget*      ButtonFrame;
  vtkKWLabel*       ShowAllLabel;
  vtkKWCheckButton* ShowAllCheck;

  vtkKWListBox* ArraySelectionList;
  // Labels get substituted for list box after accept is called.
  vtkCollection* ArrayLabelCollection;

  // Called to inactivate widget (after accept is called).
  void Inactivate();
  int Active;
  vtkStringList* SelectedArrayNames;

  vtkPVInputMenu* InputMenu;

  int StringMatch(const char* arrayName);
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);
  
  vtkPVSelectCTHArrays(const vtkPVSelectCTHArrays&); // Not implemented
  void operator=(const vtkPVSelectCTHArrays&); // Not implemented
};

#endif
