/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWTextProperty.h
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
// .NAME vtkKWTextProperty - a frame with a grooved border and a label
// .SECTION Description
// The LabeledFrame creates a frame with a grooved border, and a label
// embedded in the upper left corner of the grooved border.


#ifndef __vtkKWTextProperty_h
#define __vtkKWTextProperty_h

#include "vtkKWWidget.h"

class vtkActor2D;
class vtkKWApplication;
class vtkKWChangeColorButton;
class vtkKWCheckButton;
class vtkKWOptionMenu;
class vtkKWPushButton;
class vtkKWScale;
class vtkTextProperty;

class VTK_EXPORT vtkKWTextProperty : public vtkKWWidget
{
public:
  static vtkKWTextProperty* New();
  vtkTypeRevisionMacro(vtkKWTextProperty,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app);

  // Description:
  // Set/Get the text property to control.
  virtual void SetTextProperty(vtkTextProperty*);
  vtkGetObjectMacro(TextProperty, vtkTextProperty);

  // Description:
  // Set/Get the actor that uses TextProperty. This is optional, but might
  // help to solve some backward compatibility issues. For example, the
  // default vtkTextProperty color is -1, -1, -1 to specify to the mapper
  // that the vtkActor2D color has to be used instead.
  virtual void SetActor2D(vtkActor2D*);
  vtkGetObjectMacro(Actor2D, vtkActor2D);

  // Description:
  // Show color.
  void SetShowColor(int);
  vtkBooleanMacro(ShowColor, int);
  vtkGetMacro(ShowColor, int);

  // Description:
  // Show font family.
  void SetShowFontFamily(int);
  vtkBooleanMacro(ShowFontFamily, int);
  vtkGetMacro(ShowFontFamily, int);

  // Description:
  // Show style.
  void SetShowStyles(int);
  vtkBooleanMacro(ShowStyles, int);
  vtkGetMacro(ShowStyles, int);

  // Description:
  // Show opacity.
  void SetShowOpacity(int);
  vtkBooleanMacro(ShowOpacity, int);
  vtkGetMacro(ShowOpacity, int);

  // Description:
  // Show horizontal justification.
  void SetShowHorizontalJustification(int);
  vtkBooleanMacro(ShowHorizontalJustification, int);
  vtkGetMacro(ShowHorizontalJustification, int);

  // Description:
  // Show vertical justification.
  void SetShowVerticalJustification(int);
  vtkBooleanMacro(ShowVerticalJustification, int);
  vtkGetMacro(ShowVerticalJustification, int);

  // Description:
  // Set/Get the command executed each time a change is made to the
  // text property.
  vtkSetStringMacro(OnChangeCommand);
  vtkGetStringMacro(OnChangeCommand);

  // Description:
  // Show copy button. This button can be used to synchronize different
  // text property widgets.
  void SetShowCopy(int);
  vtkBooleanMacro(ShowCopy, int);
  vtkGetMacro(ShowCopy, int);
  vtkGetObjectMacro(CopyButton, vtkKWPushButton);

  // Description:
  // Copy the values from another text widget
  void CopyValuesFrom(vtkKWTextProperty*);

  // GUI components callbacks
  void ChangeColorButtonCallback(float, float, float);
  void SetColor(float, float, float);
  void SetColor(float *v) { this->SetColor(v[0], v[1], v[2]); };
  float* GetColor();
  void FontFamilyOptionMenuCallback();
  void SetFontFamily(int);
  void BoldCheckButtonCallback();
  void SetBold(int);
  void ItalicCheckButtonCallback();
  void SetItalic(int);
  void ShadowCheckButtonCallback();
  void SetShadow(int);
  void OpacityScaleCallback();
  void OpacityScaleEndCallback();
  void SetOpacity(float);
  void SetOpacityNoTrace(float);
  float GetOpacity();

  // Description:
  // Save out the text properties to a file.
  void SaveInTclScript(ofstream *file, const char *tcl_name = 0);

protected:
  vtkKWTextProperty();
  ~vtkKWTextProperty();

  void UpdateInterface();
  void UpdateColorButton();
  void UpdateFontFamilyOptionMenu();
  void UpdateStylesFrame();
  void UpdateBoldCheckButton();
  void UpdateItalicCheckButton();
  void UpdateShadowCheckButton();
  void UpdateOpacityScale();
  void UpdateCopyButton();

  vtkTextProperty *TextProperty;
  vtkActor2D *Actor2D;

  int ShowColor;
  vtkKWChangeColorButton *ChangeColorButton;

  int ShowFontFamily;
  vtkKWOptionMenu *FontFamilyOptionMenu;

  int ShowStyles;
  vtkKWWidget *StylesFrame;
  vtkKWCheckButton *BoldCheckButton;
  vtkKWCheckButton *ItalicCheckButton;
  vtkKWCheckButton *ShadowCheckButton;
  
  int ShowOpacity;
  vtkKWScale *OpacityScale;

  int ShowHorizontalJustification;
  vtkKWOptionMenu *HorizontalJustificationOptionMenu;

  int ShowVerticalJustification;
  vtkKWOptionMenu *VerticalJustificationOptionMenu;

  char *OnChangeCommand;

  int ShowCopy;
  vtkKWPushButton *CopyButton;

private:
  vtkKWTextProperty(const vtkKWTextProperty&); // Not implemented
  void operator=(const vtkKWTextProperty&); // Not implemented
};

#endif
