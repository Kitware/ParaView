/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVLineWidget.h
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
// .NAME vtkPVLineWidget -
// .SECTION Description

// Todo:
// Cleanup GUI:
//       * Visibility
//       * Resolution
//       *

#ifndef __vtkPVLineWidget_h
#define __vtkPVLineWidget_h

#include "vtkPV3DWidget.h"

class vtkKWEntry;
class vtkKWLabel;
class vtkLineWidget;

class VTK_EXPORT vtkPVLineWidget : public vtkPV3DWidget
{
public:
  static vtkPVLineWidget* New();
  vtkTypeMacro(vtkPVLineWidget, vtkPV3DWidget);

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
  // Callbacks to set the points of the 3D widget from the
  // entry values. Bound to <KeyPress-Return>.
  void SetPoint1();
  void SetPoint2();
  void SetPoint1(float x, float y, float z);
  void SetPoint2(float x, float y, float z);

  // Description:
  // Set the resolution of the line widget.
  void SetResolution();
  void SetResolution(float f);

  // Description:
  // Set the tcl variables that are modified when accept is called.
  void SetPoint1Method(const char* wname, const char* varname);
  void SetPoint2Method(const char* wname, const char* varname);
  void SetResolutionMethod(const char* wname, const char* varname);
    
//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVLineWidget* ClonePrototype(vtkPVSource* pvSource,
				 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

protected:
  vtkPVLineWidget();
  ~vtkPVLineWidget();
  
//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
			      vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);

  friend class vtkLineWidgetObserver;
//ETX
  
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

  // Description:
  // Call creation on the child.
  virtual void ChildCreate(vtkPVApplication*);

  // Description:
  // Execute event of the 3D Widget.
  virtual void ExecuteEvent(vtkObject*, unsigned long, void*);

  vtkKWEntry* Point1[3];
  vtkKWEntry* Point2[3];
  vtkKWLabel* Labels[2];
  vtkKWLabel* CoordinateLabel[3];
  vtkKWLabel* ResolutionLabel;
  vtkKWEntry* ResolutionEntry;

  vtkSetStringMacro(Point1Variable);
  vtkSetStringMacro(Point2Variable);
  vtkSetStringMacro(ResolutionVariable);
  vtkSetStringMacro(Point1Object);
  vtkSetStringMacro(Point2Object);
  vtkSetStringMacro(ResolutionObject);

  char *Point1Variable;
  char *Point2Variable;
  char *ResolutionVariable;
  char *Point1Object;
  char *Point2Object;
  char *ResolutionObject;

private:  
  vtkPVLineWidget(const vtkPVLineWidget&); // Not implemented
  void operator=(const vtkPVLineWidget&); // Not implemented
};

#endif
