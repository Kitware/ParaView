/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWTclInteractor.h
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
// .NAME vtkKWTclInteractor - a KW version of interactor.tcl
// .SECTION Description
// A widget to interactively execute Tcl commands

#ifndef __vtkKWTclInteractor_h
#define __vtkKWTclInteractor_h

#include "vtkKWWidget.h"
class vtkKWApplication;
class vtkKWPushButton;
class vtkKWEntry;
class vtkKWLabel;
class vtkKWText;

class VTK_EXPORT vtkKWTclInteractor : public vtkKWWidget
{
public:
  static vtkKWTclInteractor* New();
  vtkTypeMacro(vtkKWTclInteractor, vtkKWWidget);
  
  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app);
  
  // Description:
  // Display the interactor
  void Display();

  // Description:
  // Evaluate the tcl string
  void Evaluate();

  // Description:
  // Set the title of the TclInteractor to appear in the titlebar
  vtkSetStringMacro(Title);
  
  // Description:
  // Callback for the down arrow key
  void DownCallback();
  
  // Description:
  // Callback for the up arrow key
  void UpCallback();
  
protected:
  vtkKWTclInteractor();
  ~vtkKWTclInteractor();
  vtkKWTclInteractor(const vtkKWTclInteractor&) {};
  void operator=(const vtkKWTclInteractor&) {};

  vtkKWWidget *ButtonFrame;
  vtkKWPushButton *DismissButton;
  vtkKWWidget *CommandFrame;
  vtkKWLabel *CommandLabel;
  vtkKWEntry *CommandEntry;
  vtkKWWidget *DisplayFrame;
  vtkKWText *DisplayText;
  vtkKWWidget *DisplayScrollBar;
  
  char *Title;
  int TagNumber;
  int CommandIndex;
};

#endif
