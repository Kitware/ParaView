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
// .NAME vtkKWSelectionFrame
// .SECTION Description

#ifndef __vtkKWSelectionFrame_h
#define __vtkKWSelectionFrame_h

#include "vtkKWWidget.h"

class vtkKWFrame;
class vtkKWLabel;
class vtkKWMenuButton;

class VTK_EXPORT vtkKWSelectionFrame : public vtkKWWidget
{
public:
  static vtkKWSelectionFrame* New();
  vtkTypeRevisionMacro(vtkKWSelectionFrame, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args);
  
  // Description:
  // Set/Get title
  virtual void SetTitle(const char *title);
  virtual const char* GetTitle();

  // Description:
  // Set the selection list (array for num strings) and the command
  // that will be called when a selection is made (this command will be
  // passed both the string selected and a point to this object)
  virtual void SetSelectionList(int num, const char **list);
  virtual void SetSelectCommand(vtkKWObject *object, const char *method);

  // Description:
  // Set/Get the title foregroud/background color (in both normal and 
  // selected mode). 
  vtkGetVector3Macro(TitleColor, float);
  vtkGetVector3Macro(TitleSelectedColor, float);
  vtkGetVector3Macro(TitleBackgroundColor, float);
  vtkGetVector3Macro(TitleBackgroundSelectedColor, float);
  virtual void SetTitleColor(float r, float g, float b);
  virtual void SetTitleColor(float rgb[3])
    { this->SetTitleColor(rgb[0], rgb[1], rgb[2]); };
  virtual void SetTitleSelectedColor(float r, float g, float b);
  virtual void SetTitleSelectedColor(float rgb[3])
    { this->SetTitleSelectedColor(rgb[0], rgb[1], rgb[2]); };
  virtual void SetTitleBackgroundColor(float r, float g, float b);
  virtual void SetTitleBackgroundColor(float rgb[3])
    { this->SetTitleBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  virtual void SetTitleBackgroundSelectedColor(float r, float g, float b);
  virtual void SetTitleBackgroundSelectedColor(float rgb[3])
    { this->SetTitleBackgroundSelectedColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Callbacks
  virtual void SelectionMenuCallback(const char *menuItem);
  
  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Access to sub-widgets
  vtkGetObjectMacro(TitleBarRightSubframe, vtkKWFrame);
  vtkGetObjectMacro(BodyFrame, vtkKWFrame);
  vtkGetObjectMacro(SelectionList, vtkKWMenuButton);
  
protected:
  vtkKWSelectionFrame();
  ~vtkKWSelectionFrame();
  
  vtkKWFrame      *TitleBar;
  vtkKWMenuButton *SelectionList;
  vtkKWLabel      *Title;
  vtkKWFrame      *TitleBarRightSubframe;
  vtkKWFrame      *BodyFrame;

  virtual int SetColor(float *color, float r, float g, float b);
  virtual void UpdateColors();

  float TitleColor[3];
  float TitleSelectedColor[3];
  float TitleBackgroundColor[3];
  float TitleBackgroundSelectedColor[3];

  virtual void SetObjectMethodCommand(
    char **command, vtkKWObject *object, const char *method);
  char *SelectCommand;
  
private:
  vtkKWSelectionFrame(const vtkKWSelectionFrame&);  // Not implemented
  void operator=(const vtkKWSelectionFrame&);  // Not implemented
};

#endif

