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
// .NAME vtkKWScale - a scale (slider) widget
// .SECTION Description
// A widget that repsentes a scale (or slider) with options for 
// a label string and a text entry box.

#ifndef __vtkKWScale_h
#define __vtkKWScale_h

#include "vtkKWWidget.h"

class vtkKWApplication;
class vtkKWEntry;
class vtkKWLabel;
class vtkKWPushButton;

class VTK_EXPORT vtkKWScale : public vtkKWWidget
{
public:
  static vtkKWScale* New();
  vtkTypeRevisionMacro(vtkKWScale,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Method to set/get the resolution of the slider.  Be sure to set the
  // resolution of the scale prior to setting the scale value.
  virtual void SetResolution(float r);
  vtkGetMacro(Resolution, float);
  
  // Description:
  // Set/Get the value of the scale. If you are changing the default
  // resolution of the scale, set it before setting the value.
  virtual void SetValue(float v);
  vtkGetMacro(Value, float);

  // Description:
  // Set the range for this scale.
  virtual void SetRange(float min, float max);
  virtual void SetRange(float *range) { this->SetRange(range[0], range[1]); };
  vtkGetVector2Macro(Range, float);
  virtual float GetRangeMin() { return this->GetRange()[0]; };
  virtual float GetRangeMax() { return this->GetRange()[1]; };

  // Description:
  // Set/get whether to display the range of the scale
  void SetDisplayRange(int flag);
  vtkGetMacro(DisplayRange, int);
  vtkBooleanMacro(DisplayRange, int);
  
  // Description:
  // Display a label and/or a text entry box. These are optional.
  // Get the corresponding internal objects.
  void DisplayEntry();  
  void DisplayLabel(const char *l);  
  vtkGetObjectMacro(Label, vtkKWLabel);
  vtkGetObjectMacro(Entry, vtkKWEntry);
  vtkSetStringMacro(ShortLabel);
  vtkGetStringMacro(ShortLabel);
  
  // Description:
  // Set/Get the position of the label and/or entry (on top, or on the side).
  virtual void SetDisplayEntryAndLabelOnTop(int flag);
  vtkGetMacro(DisplayEntryAndLabelOnTop, int);
  vtkBooleanMacro(DisplayEntryAndLabelOnTop, int);  

  // Description:
  // Set/Get a popup scale. 
  // WARNING: must be set *before* Create() is called.
  vtkSetMacro(PopupScale, int);
  vtkGetMacro(PopupScale, int);
  vtkBooleanMacro(PopupScale, int);  
  void DisplayPopupScaleCallback();
  void WithdrawPopupScaleCallback();
  vtkGetObjectMacro(PopupPushButton, vtkKWPushButton);

  // Description:
  // Set/Get the entry expansion flag. This flag is only used if PopupScale 
  // mode is On. In that case, the default behaviour is to provide a widget
  // as compact as possible, i.e. the Entry won't be expanded if the widget
  // grows. Set ExpandEntry to On to override this behaviour.
  virtual void SetExpandEntry(int flag);
  vtkGetMacro(ExpandEntry, int);
  vtkBooleanMacro(ExpandEntry, int);  

  // Description:
  // Method that gets invoked when the sliders value has changed.
  virtual void ScaleValueChanged(float num);
  virtual void EntryValueChanged();
  virtual void InvokeStartCommand();
  virtual void InvokeEndCommand();
  virtual void InvokeEntryCommand();

  // Description:
  // A method to set callback functions on objects.  The first argument is
  // the KWObject that will have the method called on it.  The second is the
  // name of the method to be called and any arguments in string form.
  // The calling is done via TCL wrappers for the KWObject.
  virtual void SetCommand      (vtkKWObject* Object, 
                                const char *MethodAndArgString);
  virtual void SetStartCommand (vtkKWObject* Object, 
                                const char *MethodAndArgString);
  virtual void SetEndCommand   (vtkKWObject* Object, 
                                const char *MethodAndArgString);
  virtual void SetEntryCommand (vtkKWObject* Object, 
                                const char *MethodAndArgString);

  // Description:
  // Set/get whether the above commands should be called or not.
  // This allow you to disable the commands while you are setting the scale
  // value for example.
  vtkSetMacro(DisableCommands, int);
  vtkGetMacro(DisableCommands, int);
  vtkBooleanMacro(DisableCommands, int);

  // Description:
  // Setting this string enables balloon help for this widget.
  // Override to pass down to children for cleaner behavior
  virtual void SetBalloonHelpString(const char *str);
  virtual void SetBalloonHelpJustification(int j);

  // Description:
  // Bind/Unbind all components so that values can be changed, but
  // no command will be called.
  void Bind();
  void UnBind();

  // Description:
  // Set/Get the resize mode to be smart. In that mode, some elements like the
  // label and the entry will disappear if the widget gets to small.
  vtkSetMacro(SmartResize, int);
  vtkGetMacro(SmartResize, int);
  vtkBooleanMacro(SmartResize, int);  

  void Resize();

  // Description
  // Convenience method to set the width of the label or entry
  virtual void SetLabelWidth(int width);
  virtual void SetEntryWidth(int width);

  // Description
  // Convenience method to set the narrow dimension of scale. For vertical 
  // scales this is the trough's width; for horizontal scales this is the 
  // trough's height. In pixel.
  virtual void SetWidth(int width);

protected:
  vtkKWScale();
  ~vtkKWScale();

  int         DisplayEntryAndLabelOnTop;
  int         PopupScale;
  int         ExpandEntry;
  int         DisplayRange;
  int         SmartResize;
  int         DisableCommands;

  char        *Command;
  char        *StartCommand;
  char        *EndCommand;
  char        *EntryCommand;

  float       Value;
  float       Resolution;
  float       Range[2];

  int         EntryResolution;

  vtkKWWidget *Scale;
  vtkKWEntry  *Entry;
  vtkKWLabel  *Label;
  vtkKWWidget *TopLevel;
  vtkKWPushButton *PopupPushButton;

  vtkKWLabel *RangeMinLabel;
  vtkKWLabel *RangeMaxLabel;
  
  char *NormalLabel;
  char *ShortLabel;
  int LongWidth;
  int MediumWidth;
  int MediumShortWidth;
  int ShortWidth;
  int PackEntry;
  int PackRange;
  
  vtkSetStringMacro(NormalLabel);
  
  void PackWidget();
  void UpdateEntryResolution();

  // Update the enable state. This should propagate similar calls to the
  // internal widgets.
  virtual void UpdateEnableState();

private:
  vtkKWScale(const vtkKWScale&); // Not implemented
  void operator=(const vtkKWScale&); // Not implemented
};


#endif



