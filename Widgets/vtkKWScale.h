/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWScale.h
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
// .NAME vtkKWScale - a scale (slider) widget
// .SECTION Description
// A widget that repsentes a scale (or slider) with options for 
// a label string and a text entry box.

#ifndef __vtkKWScale_h
#define __vtkKWScale_h

#include "vtkKWEntry.h"
class vtkKWApplication;

class VTK_EXPORT vtkKWScale : public vtkKWWidget
{
public:
  static vtkKWScale* New();
  vtkTypeMacro(vtkKWScale,vtkKWWidget);

  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Set/Get the value of the scale.  If you are changing the default
  // resoltion of the scale, set it before setting the value.
  void SetValue(float v);
  virtual float GetValue() {return this->Value;};

  /// Description:
  // Set the range for this scale.
  void SetRange(float min, float max);

  // Description:
  // Display a label and or a text entry box. These are options. 
  void DisplayEntry();  
  void DisplayLabel(const char *l);  

  // Description:
  // Method that gets invoked when the sliders value has changed.
  virtual void ScaleValueChanged(float num);
  virtual void EntryValueChanged();
  virtual void InvokeStartCommand();
  virtual void InvokeEndCommand();

  // Description:
  // Method to set / get the resolution of the slider.  Be sure to set the
  // resolution of the scale prior to setting the scale value.
  vtkGetMacro( Resolution, float );
  void SetResolution( float r );
  
  // Description:
  // A method to set callback functions on objects.  The first argument is
  // the KWObject that will have the method called on it.  The second is the
  // name of the method to be called and any arguments in string form.
  // The calling is done via TCL wrappers for the KWObject.
  virtual void SetCommand(vtkKWObject* Object, const char *MethodAndArgString);
  virtual void SetStartCommand(vtkKWObject* Object, const char *MethodAndArgString);
  virtual void SetEndCommand(vtkKWObject* Object, const char *MethodAndArgString);

  // Description:
  // Setting this string enables balloon help for this widget.
  // Override to pass down to children for cleaner behavior
  virtual void SetBalloonHelpString(const char *str);
  virtual void SetBalloonHelpJustification( int j );

protected:
  vtkKWScale();
  ~vtkKWScale();
  vtkKWScale(const vtkKWScale&) {};
  void operator=(const vtkKWScale&) {};

  char        *Command;
  char        *StartCommand;
  char        *EndCommand;
  float       Value;
  float       Resolution;
  vtkKWEntry  *Entry;
  vtkKWWidget *ScaleWidget;
  vtkKWWidget *ScaleLabel;
};


#endif


