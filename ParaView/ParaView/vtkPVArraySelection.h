/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVArraySelection.h
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
// .NAME vtkPVArraySelection - widget to select a set of data arrays.
// .SECTION Description
// vtkPVArraySelection is used for selecting which set of data arrays to 
// load when a reader has the ability to selectively load arrays.

#ifndef __vtkPVArraySelection_h
#define __vtkPVArraySelection_h

#include "vtkPVWidget.h"

class vtkKWRadioButton;
class vtkPVData;
class vtkKWPushButton;
class vtkCollection;
class vtkKWLabeledFrame;

class VTK_EXPORT vtkPVArraySelection : public vtkPVWidget
{
public:
  static vtkPVArraySelection* New();
  vtkTypeMacro(vtkPVArraySelection, vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // This specifies whether to ues Cell or Point data.
  // Options are "Cell" or "Point".  Possible "Field" in the future.
  vtkSetStringMacro(AttributeName);
  vtkGetStringMacro(AttributeName);

  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app);

  // Description:
  // This is the name of the VTK reader.
  vtkSetStringMacro(VTKReaderTclName);
  vtkGetStringMacro(VTKReaderTclName);
    
  // Description:
  // Methods for setting the value of the VTKReader from the widget.
  // User internally when user hits Accept.
  virtual void Accept();

  // Description:
  // Methods for setting the value of the widget from the VTKReader.
  // User internally when user hits Reset.
  virtual void Reset();

  // Description:
  // Callback for the AllOn and AllOff buttons.
  void AllOnCallback();
  void AllOffCallback();

  // Description:
  // Access to change this widgets state from a script. Used for tracing.
  void SetArrayStatus(const char *name, int status);

  // Description:
  // Save this widget to a file.  
  virtual void SaveInTclScript(ofstream *file);

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVArraySelection* ClonePrototype(vtkPVSource* pvSource,
				 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  
protected:
  vtkPVArraySelection();
  ~vtkPVArraySelection();

  char* AttributeName;
  char* VTKReaderTclName;

  vtkKWLabeledFrame* LabeledFrame;

  vtkKWWidget* ButtonFrame;
  vtkKWPushButton* AllOnButton;
  vtkKWPushButton* AllOffButton;

  vtkKWWidget *CheckFrame;
  vtkCollection* ArrayCheckButtons;

  // Description:
  // Stores the file name to descide when to rebuild the array check list.
  vtkSetStringMacro(FileName);
  char *FileName;

  vtkPVArraySelection(const vtkPVArraySelection&); // Not implemented
  void operator=(const vtkPVArraySelection&); // Not implemented

//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
			      vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);
};

#endif
