/*=========================================================================

  Module:    vtkKWTclInteractor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWTclInteractor - a KW version of interactor.tcl
// .SECTION Description
// A widget to interactively execute Tcl commands

#ifndef __vtkKWTclInteractor_h
#define __vtkKWTclInteractor_h

#include "vtkKWTopLevel.h"

class vtkKWFrame;
class vtkKWPushButton;
class vtkKWEntry;
class vtkKWLabel;
class vtkKWTextWithScrollbars;

class KWWidgets_EXPORT vtkKWTclInteractor : public vtkKWTopLevel
{
public:
  static vtkKWTclInteractor* New();
  vtkTypeRevisionMacro(vtkKWTclInteractor, vtkKWTopLevel);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Append text to the display window. Can be used for sending
  // debugging information to the command prompt when no standard
  // output is available.
  virtual void AppendText(const char* text);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Specifies the font to use when drawing text inside the widget. 
  // You can use predefined font names (e.g. 'system'), or you can specify
  // a set of font attributes with a platform-independent name, for example,
  // 'times 12 bold'. In this example, the font is specified with a three
  // element list: the first element is the font family, the second is the
  // size, the third is a list of style parameters (normal, bold, roman, 
  // italic, underline, overstrike). Example: 'times 12 {bold italic}'.
  // The Times, Courier and Helvetica font families are guaranteed to exist
  // and will be matched to the corresponding (closest) font on your system.
  // If you are familiar with the X font names specification, you can also
  // describe the font that way (say, '*times-medium-r-*-*-12*').
  virtual void SetFont(const char *font);
  virtual const char* GetFont();

  // Description:
  // Callbacks. Internal, do not use.
  virtual void EvaluateCallback();
  virtual void DownCallback();
  virtual void UpCallback();

protected:
  vtkKWTclInteractor();
  ~vtkKWTclInteractor();

  // Description:
  // Create the widget.
  virtual void CreateWidget();
  
  vtkKWFrame      *ButtonFrame;
  vtkKWPushButton *DismissButton;
  vtkKWFrame      *CommandFrame;
  vtkKWLabel      *CommandLabel;
  vtkKWEntry      *CommandEntry;
  vtkKWTextWithScrollbars *DisplayText;
  
  int TagNumber;
  int CommandIndex;

private:
  vtkKWTclInteractor(const vtkKWTclInteractor&); // Not implemented
  void operator=(const vtkKWTclInteractor&); // Not implemented
};

#endif

