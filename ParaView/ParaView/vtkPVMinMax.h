/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVMinMax.h
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
// .NAME vtkPVMinMax -
// .SECTION Description

#ifndef __vtkPVMinMax_h
#define __vtkPVMinMax_h

#include "vtkPVObjectWidget.h"
#include "vtkKWApplication.h"
#include "vtkKWScale.h"
#include "vtkKWLabel.h"

class VTK_EXPORT vtkPVMinMax : public vtkPVObjectWidget
{
public:
  static vtkPVMinMax* New();
  vtkTypeMacro(vtkPVMinMax, vtkPVObjectWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  void Create(vtkKWApplication *pvApp);

  // Description:
  // Called when accept button is pushed.  
  // Sets objects variable to the widgets value.
  // Adds a trace entry.  Side effect is to turn modified flag off.
  virtual void Accept();
  
  // Description:
  // Called when the reset button is pushed.
  // Sets widget's value to the object-variable's value.
  // Side effect is to turn the modified flag off.
  virtual void Reset();

  // Description:
  // This method allows scripts to modify the widgets value.
  void SetMinValue(float val);
  float GetMinValue() { return this->MinScale->GetValue(); }
  void SetMaxValue(float val);
  float GetMaxValue() { return this->MaxScale->GetValue(); }
  void SetResolution(float res);
  float GetResolution() { return this->MinScale->GetResolution(); }
  void SetRange(float min, float max);

  // Description:
  // These commands are used to set/get values on/from the
  // underlying VTK object.
  vtkSetStringMacro(GetMinCommand);
  vtkSetStringMacro(GetMaxCommand);
  vtkSetStringMacro(SetCommand);
  vtkGetStringMacro(GetMinCommand);
  vtkGetStringMacro(GetMaxCommand);
  vtkGetStringMacro(SetCommand);
  
  // Description:
  // Save this widget to a file
  virtual void SaveInTclScript(ofstream *file);

  // Description:
  // Callback for min scale
  void MinValueCallback();
  
  // Description:
  // Callback for max scale
  void MaxValueCallback();
  
  // Description:
  // Label for the minimum value scale.
  void SetMinimumLabel(const char* label);

  // Description:
  // Label for the maximum value scale.
  void SetMaximumLabel(const char* label);

  // Description:
  // Set the balloon help string for the minimum value scale.
  void SetMinimumHelp(const char* help);

  // Description:
  // Set the balloon help string for the maximum value scale.
  void SetMaximumHelp(const char* help);

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVMinMax* ClonePrototype(vtkPVSource* pvSource,
				 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

protected:
  vtkPVMinMax();
  ~vtkPVMinMax();
  
  vtkKWLabel *MinLabel;
  vtkKWLabel *MaxLabel;
  vtkKWScale *MinScale;
  vtkKWScale *MaxScale;
  vtkKWWidget *MinFrame;
  vtkKWWidget *MaxFrame;

  char *GetMinCommand;
  char *GetMaxCommand;
  char *SetCommand;

  char* MinHelp;
  char* MaxHelp;
  vtkSetStringMacro(MinHelp);
  vtkSetStringMacro(MaxHelp);

//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
			      vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  

  vtkPVMinMax(const vtkPVMinMax&); // Not implemented
  void operator=(const vtkPVMinMax&); // Not implemented

  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);
};

#endif
