/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWCheckButton.h
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
// .NAME vtkKWCheckButton - check button widget
// .SECTION Description
// A simple widget that represents a check button. It can be modified 
// and queried using the GetState and SetState methods.

#ifndef __vtkKWCheckButton_h
#define __vtkKWCheckButton_h

#include "vtkKWWidget.h"
class vtkKWApplication;

class VTK_EXPORT vtkKWCheckButton : public vtkKWWidget
{
public:
  static vtkKWCheckButton* New();
  vtkTypeMacro(vtkKWCheckButton,vtkKWWidget);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Set/Get the state of the check button 0 = off 1 = on
  void SetState(int );
  virtual int GetState();

  // Description:
  // Tell the widget whether it should use an indicator (check box)
  void SetIndicator(int ind);

  // Description:
  // Set the text.
  void SetText(const char* txt);
  const char* GetText();

protected:

  vtkSetStringMacro(MyText);

  vtkKWCheckButton();
  ~vtkKWCheckButton() {};

  int IndicatorOn;
  char* MyText;
private:
  vtkKWCheckButton(const vtkKWCheckButton&); // Not implemented
  void operator=(const vtkKWCheckButton&); // Not Implemented
};


#endif


