/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWNotebook.h
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
// .NAME vtkKWNotebook - a tabbed notebook of UI pages
// .SECTION Description
// The notebook represents a tabbed notebook component where you can
// add or remove pages.

#ifndef __vtkKWNotebook_h
#define __vtkKWNotebook_h

#include "vtkKWWidget.h"
class vtkKWApplication;
class vtkKWFrame;
class vtkKWIcon;
class vtkKWImageLabel;

class VTK_EXPORT vtkKWNotebook : public vtkKWWidget
{
public:
  static vtkKWNotebook* New();
  vtkTypeMacro(vtkKWNotebook,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Raise the specified tab to be on the top.
  virtual void Raise(int n);
  void Raise(const char *name);
  void RaiseCurrent(){this->Raise(this->Current);};

  // Description:
  // Some callback routines to handle rezise events.
  void ScheduleResize();
  void Resize();
  
  // Description:
  // Get the vtkKWWidget for the frame of the specified page (Tab).
  // The UI components should be inserted into these frames.
  vtkKWWidget *GetFrame(int n);
  vtkKWWidget *GetFrame(const char *name);

  // Description:
  // Get the number of pages in the notebook.
  vtkGetMacro(NumberOfPages,int);

  // Description:
  // Add new page to the notebook. By setting balloon string, 
  // the page will have balloon help.
  void AddPage(const char *title, const char* balloon, 
               vtkKWIcon *icon);
  void AddPage(const char *title, const char* balloon);
  void AddPage(const char *title);
  
  // Description:
  // The notebook will automatically resize itself to fit its
  // contents. This can lead to a lot of resizing. So you can
  // specify a minimum width and height for the notebook. This
  // can be used to significantly reduce or eliminate the resizing
  // of the notebook.
  vtkSetMacro(MinimumWidth,int);
  vtkSetMacro(MinimumHeight,int);
  vtkGetMacro(MinimumWidth,int);
  vtkGetMacro(MinimumHeight,int);

  // Description:
  // Normally, the tab frame is not shown when there is only
  // one page. Turn this on to override that behaviour.
  vtkSetMacro(AlwaysShowTabs, int);
  vtkGetMacro(AlwaysShowTabs, int);
  vtkBooleanMacro(AlwaysShowTabs, int);
  
protected:
  vtkKWNotebook();
  ~vtkKWNotebook();

  int MinimumWidth;
  int MinimumHeight;
  int Current;
  int Height;
  int Pad;
  int BorderWidth;
  int Expanding;
  int AlwaysShowTabs;
  
  vtkKWWidget *TabsFrame;
  vtkKWWidget *Body;
  vtkKWWidget *Mask;
  vtkKWWidget *MaskLeft;
  vtkKWWidget *MaskLeft2;
  vtkKWWidget *MaskRight;
  vtkKWWidget *MaskRight2;
  int NumberOfPages;
  vtkKWFrame **Frames;
  vtkKWWidget **Buttons;
  vtkKWImageLabel **Icons;
  char **Titles;
  
  // Description:
  // Store background color of the back tab and the front tab.
  char BackgroundColor[10];
  char ForegroundColor[10];
private:
  vtkKWNotebook(const vtkKWNotebook&); // Not implemented
  void operator=(const vtkKWNotebook&); // Not implemented
};


#endif


