/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVGhostLevelDialog.h
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
// .NAME vtkPVGhostLevelDialog - asks the user which ghostlevel he wants
// .SECTION Description
// Asks the user which ghostlevel he wants.
// .SECTION See Also
// vtkPVApplication

#ifndef __vtkPVGhostLevelDialog_h
#define __vtkPVGhostLevelDialog_h

#include "vtkKWDialog.h"

class vtkKWFrame;
class vtkKWLabel;
class vtkKWWidget;

class VTK_EXPORT vtkPVGhostLevelDialog : public vtkKWDialog
{
public:
  static vtkPVGhostLevelDialog* New();
  vtkTypeRevisionMacro(vtkPVGhostLevelDialog,vtkKWDialog);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app, const char *args);

  // Description:
  // GhostLevel selected by the user. SetGhostLevel is for
  // internal use only.
  void SetGhostLevel(int level);
  vtkGetMacro(GhostLevel, int);

  // Description:
  // Invoke the dialog and display it in a modal manner. 
  // This method returns a zero if the dilaog was killed or 
  // canceled, nonzero otherwise. After Invoke(), the
  // ghostlevel chosen by the user can by obtained with
  // GetGhostLevel
  virtual int Invoke();

protected:
  vtkPVGhostLevelDialog();
  ~vtkPVGhostLevelDialog();

  int GhostLevel;

  vtkKWWidget* Separator;
  vtkKWLabel*  Label;
  vtkKWFrame*  ButtonFrame;

  vtkKWWidget *SelFrame1;
  vtkKWWidget *SelFrame2;
  vtkKWWidget *SelFrame3;

  vtkKWWidget* SelButton1;
  vtkKWWidget* SelButton2;
  vtkKWWidget* SelButton3;

private:
  vtkPVGhostLevelDialog(const vtkPVGhostLevelDialog&); // Not implemented
  void operator=(const vtkPVGhostLevelDialog&); // Not implemented
};


#endif


