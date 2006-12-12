/*=========================================================================

  Module:    vtkKWTkcon.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWTkcon - a wrapper around a tkcon console.
// .SECTION Description
// A widget to interactively execute Tcl commands using a tkcon console.
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.

#ifndef __vtkKWTkcon_h
#define __vtkKWTkcon_h

#include "vtkKWTclInteractor.h"

class vtkKWTkconInternals;

class KWWidgets_EXPORT vtkKWTkcon : public vtkKWTclInteractor
{
public:
  static vtkKWTkcon* New();
  vtkTypeRevisionMacro(vtkKWTkcon, vtkKWTclInteractor);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Append text to the display window.
  virtual void AppendText(const char* text);

  // Description:
  // Set focus to this widget. 
  // Override the superclass to focus on the console.
  virtual void Focus();

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
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWTkcon();
  ~vtkKWTkcon();

  // Description:
  // Create the widget.
  virtual void CreateWidget();
  
  // PIMPL Encapsulation for STL containers
  //BTX
  vtkKWTkconInternals *Internals;
  //ETX

private:
  vtkKWTkcon(const vtkKWTkcon&); // Not implemented
  void operator=(const vtkKWTkcon&); // Not implemented
};

#endif

