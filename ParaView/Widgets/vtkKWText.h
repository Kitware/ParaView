/*=========================================================================

  Module:    vtkKWText.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWText - a multi-line text entry widget
// .SECTION Description
// A simple widget used for collecting keyboard input from the user. This
// widget provides support for multi-line input.

#ifndef __vtkKWText_h
#define __vtkKWText_h

#include "vtkKWWidget.h"
class vtkKWApplication;

class VTK_EXPORT vtkKWText : public vtkKWWidget
{
public:
  static vtkKWText* New();
  vtkTypeRevisionMacro(vtkKWText,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Set/Get the value of the Text.
  void SetValue(const char *);
  char *GetValue();

protected:
  vtkKWText();
  ~vtkKWText();

  vtkGetStringMacro( ValueString );
  vtkSetStringMacro( ValueString );
  
  char *ValueString;
private:
  vtkKWText(const vtkKWText&); // Not implemented
  void operator=(const vtkKWText&); // Not implemented
};


#endif



