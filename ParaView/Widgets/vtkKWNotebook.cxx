/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWNotebook.cxx
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
#include "vtkKWApplication.h"
#include "vtkKWNotebook.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWNotebook );




int vtkKWNotebookCommand(ClientData cd, Tcl_Interp *interp,
		      int argc, char *argv[]);

vtkKWNotebook::vtkKWNotebook()
{
  this->CommandFunction = vtkKWNotebookCommand;
  this->Height = 30;
  this->Pad = 0;
  this->NumberOfPages = 0;
  this->Buttons = NULL;
  this->Frames = NULL;
  this->Titles = NULL;
  this->Current = -1;
  this->BorderWidth = 2;
  
  this->TabsFrame = vtkKWWidget::New();
  this->TabsFrame->SetParent(this);
  this->Body = vtkKWWidget::New();
  this->Body->SetParent(this);
  this->Mask = vtkKWWidget::New();
  this->Mask->SetParent(this);
  this->MaskLeft = vtkKWWidget::New();
  this->MaskLeft->SetParent(this);
  this->MaskLeft2 = vtkKWWidget::New();
  this->MaskLeft2->SetParent(this->MaskLeft);
  this->MaskRight = vtkKWWidget::New();
  this->MaskRight->SetParent(this);
  this->MaskRight2 = vtkKWWidget::New();
  this->MaskRight2->SetParent(this->MaskRight);

  this->MinimumWidth = 360;
  this->MinimumHeight = 600;
  this->Expanding = 0;
}

vtkKWNotebook::~vtkKWNotebook()
{
  this->TabsFrame->Delete();
  this->Body->Delete();
  this->Mask->Delete();
  this->MaskLeft->Delete();
  this->MaskLeft2->Delete();
  this->MaskRight->Delete();
  this->MaskRight2->Delete();

  int cnt;
  for (cnt = 0; cnt < this->NumberOfPages; cnt++)
    {
    this->Frames[cnt]->Delete();
    delete [] this->Titles[cnt];
    this->Buttons[cnt]->Delete();
    }
  if (this->Buttons)
    {
    delete [] this->Buttons;
    }
  if (this->Frames)
    {
    delete [] this->Frames;
    }
  if (this->Titles)
    {
    delete [] this->Titles;
    }
}

void vtkKWNotebook::Create(vtkKWApplication *app, const char *args)
{
  const char *wname;
  
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("Notebook already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s -width %d -height %d -borderwidth 0 -relief flat %s",
               wname, this->MinimumWidth, this->MinimumHeight, args);

  this->TabsFrame->Create(app,"frame","-borderwidth 0 -relief flat");
  this->Script("bind %s <Configure> {%s ScheduleResize}",
               this->TabsFrame->GetWidgetName(), this->GetTclName());
  this->Body->Create(app,"frame","-borderwidth 2 -relief raised");
  this->Script("bind %s <Configure> {%s ScheduleResize}",
               this->Body->GetWidgetName(), this->GetTclName());  
  this->Mask->Create(app,"frame","-borderwidth 0 -relief flat");
  this->MaskLeft->Create(app,"frame","-borderwidth 0 -relief flat");
#ifdef _WIN32
  this->MaskLeft2->Create(app,"frame","-borderwidth 2 -relief raised");
  this->Script("place %s -x 0 -y %d -width %d -height %d",
               this->MaskLeft2->GetWidgetName(), -this->BorderWidth,
               2*this->BorderWidth, 4*this->BorderWidth);  
#else
  this->MaskLeft2->Create(app,"frame","-borderwidth 2 -relief sunken");
  this->Script("place %s -x %d -y %d -width %d -height %d",
               this->MaskLeft2->GetWidgetName(), 
               -this->BorderWidth, -this->BorderWidth,
               2*this->BorderWidth, 3*this->BorderWidth);
#endif

  this->MaskRight->Create(app,"frame","-borderwidth 0 -relief flat");

#ifdef _WIN32
  this->MaskRight2->Create(app,"frame","-borderwidth 2 -relief raised");
  this->Script("place %s -x %d -y %d -width %d -height %d",
               this->MaskRight2->GetWidgetName(), -this->BorderWidth,
               -this->BorderWidth, 2*this->BorderWidth, 
               4*this->BorderWidth);
#else
  this->MaskRight2->Create(app,"frame","-borderwidth 2 -relief sunken");
  this->Script("place %s -x 0 -y %d -width %d -height %d",
                      this->MaskRight2->GetWidgetName(),
                      -this->BorderWidth, 2*this->BorderWidth, 
                      3*this->BorderWidth);
#endif 
  this->Script("place %s -x 0 -y 0 -relwidth 1.0 -relheight 1.0",
               this->Body->GetWidgetName());
}

void vtkKWNotebook::Raise(int num)
{
  if (num < 0 || num >= this->NumberOfPages)
    {
    return;
    }

  vtkKWApplication *app = this->Application;
  
  if (num != this->Current)
    {
    if (this->Current >= 0)
      {
      this->Script("pack forget %s",
                   this->Frames[this->Current]->GetWidgetName());
      }
    this->Script("pack %s -fill both -anchor n",
                 this->Frames[num]->GetWidgetName());
    }

  this->Current = num;

  if (this->NumberOfPages <= 1)
    {
    return;
    }
  
  int bw = this->BorderWidth;

  this->Script("winfo x %s",this->Buttons[num]->GetWidgetName());
  int xb = vtkKWObject::GetIntegerResult(app);
  this->Script("winfo y %s",this->Buttons[num]->GetWidgetName());
  int yb = vtkKWObject::GetIntegerResult(app);
  this->Script("winfo x %s",this->TabsFrame->GetWidgetName());
  int x = vtkKWObject::GetIntegerResult(app);
  this->Script("winfo y %s",this->TabsFrame->GetWidgetName());
  int y = vtkKWObject::GetIntegerResult(app);
  this->Script("winfo height %s", this->Buttons[num]->GetWidgetName());
  int height = vtkKWObject::GetIntegerResult(app);
  this->Script("winfo width %s", this->Buttons[num]->GetWidgetName());
  int width = vtkKWObject::GetIntegerResult(app);
  
#ifdef _WIN32    
  int h0 = bw * 2; // Should be 3
#else
  int h0 = bw * 2;
#endif

  int x0 = xb + x + bw-1;
  int y0 = yb +  y  + height - bw - h0;
  int w0 = width - (bw * 2);



  this->Script("place %s -x %d -y %d -width %d -height %d",
	       this->Mask->GetWidgetName(),x0,y0,w0,h0);
  
  int x1 = x0 - bw;
  int y1 = y0;
  int w1 = bw;
  int h1 = h0;
  //this->Script("place %s -x %d -y %d -width %d -height %d",
  //	       this->MaskLeft->GetWidgetName(),x1,y1,w1,h1);
  
  int x2 = x0 + w0;
  int y2 = y0;
  int w2 = bw;
#ifdef _WIN32
  int h2 = h0; // Should be 2
#else
  int h2 = h0;
#endif
  //this->Script("place %s -x %d -y %d -width %d -height %d",
  //	       this->MaskRight->GetWidgetName(),x2,y2,w2,h2);
}


void vtkKWNotebook::Raise(const char *name)
{
  int cnt;
  
  for (cnt = 0; cnt < this->NumberOfPages; cnt++)
    {
    if (!strcmp(name,this->Titles[cnt]))
      {
      this->Raise(cnt);
      }
    }
}


// Return the frame associated with a given page of the notebook.
vtkKWWidget *vtkKWNotebook::GetFrame(const char *name)
{
  int cnt;
  
  for (cnt = 0; cnt < this->NumberOfPages; cnt++)
    {
    if (!strcmp(name,this->Titles[cnt]))
      {
      return this->GetFrame(cnt);
      }
    }
  return NULL;
}

vtkKWWidget *vtkKWNotebook::GetFrame(int n)
{
  if (n < 0 || n >= this->NumberOfPages)
    {
    return NULL;
    }
  return this->Frames[n];
}

// Add a page to the notebook
void vtkKWNotebook::AddPage(const char *title)
{
  this->AddPage(title, 0);
}

// Add a page to the notebook
void vtkKWNotebook::AddPage(const char *title, const char *ballon)
{
  int cnt;

  // for the frames we want to add to the list but not delete
  vtkKWWidget **pages = new vtkKWWidget *[this->NumberOfPages+1];
  vtkKWWidget **buttons = new vtkKWWidget *[this->NumberOfPages+1];
  char **titles = new char * [this->NumberOfPages+1];
  // copy the old to the new
  for (cnt = 0; cnt < this->NumberOfPages; cnt++)
    {
    pages[cnt] = this->Frames[cnt];
    titles[cnt] = this->Titles[cnt];
    buttons[cnt] = this->Buttons[cnt];
    }
  if (this->Buttons)
    {
    delete [] this->Buttons;
    }
  if (this->Frames)
    {
    delete [] this->Frames;
    }
  if (this->Titles)
    {
    delete [] this->Titles;
    }
  this->Frames = pages;
  this->Frames[this->NumberOfPages] = vtkKWWidget::New();
  this->Frames[this->NumberOfPages]->SetParent(this->Body);
  this->Frames[this->NumberOfPages]->Create(this->Application,"frame","-bd 0");
  this->Titles = titles;
  this->Titles[this->NumberOfPages] = new char [strlen(title)+1];
  strcpy(this->Titles[this->NumberOfPages],title);

  this->Buttons = buttons;
  this->Buttons[this->NumberOfPages] = vtkKWWidget::New();
  this->Buttons[this->NumberOfPages]->SetParent(this->TabsFrame);

  this->Buttons[this->NumberOfPages]->Create(this->Application,"button",
					     "-bd 2 -highlightthickness 0");
  if ( ballon )
    {
    this->Buttons[this->NumberOfPages]->SetBalloonHelpString(ballon);
    }

  this->Script(
    "%s configure -text {%s} -borderwidth %d -command {%s Raise %d}",
    this->Buttons[this->NumberOfPages]->GetWidgetName(),
    this->Titles[this->NumberOfPages], this->BorderWidth,
    this->GetTclName(),this->NumberOfPages);
  this->Script("pack %s -side left -pady 0 -padx %d -fill y",
               this->Buttons[this->NumberOfPages]->GetWidgetName(),
               this->Pad);
  this->Script("bind %s  <Configure> {%s RaiseCurrent}",
               this->Buttons[this->NumberOfPages]->GetWidgetName(),
               this->GetTclName(),this->NumberOfPages);
  
  this->NumberOfPages++;
  
  if (this->NumberOfPages == 2)
    {
#ifdef _WIN32    
    int h0 = 3;
#else
    int h0 = this->BorderWidth * 2;
#endif
    
    this->Script("place %s -x 10 -y 0 -relwidth 1.0 -height %d",
                 this->TabsFrame->GetWidgetName(), this->Height);
    this->Script("place %s -x 0 -y %d -relwidth 1.0 -relheight 1.0"
		 " -height %d",
                 this->Body->GetWidgetName(), this->Height-h0, 
		 -this->Height-h0);
    }
  
  if (this->NumberOfPages == 1)
    {
    this->Raise(0);
    }
}

void vtkKWNotebook::ScheduleResize()
{  
  if (this->Expanding)
    {
    return;
    }
  this->Expanding = 1;
  this->Script("after idle {%s Resize}", this->GetTclName());
}

void vtkKWNotebook::Resize()
{
  this->Script("winfo reqwidth %s",this->Body->GetWidgetName());
  int width = vtkKWObject::GetIntegerResult(this->Application) +
    this->BorderWidth*2 + 4;
  this->Script("winfo reqwidth %s", this->TabsFrame->GetWidgetName());
  int twidth = vtkKWObject::GetIntegerResult(this->Application);
  this->Script("winfo reqheight %s", this->Body->GetWidgetName());
  
  int height;
  if (this->NumberOfPages <= 1)
    {
    height = vtkKWObject::GetIntegerResult(this->Application) + 
      this->BorderWidth*2 + 4;
    }
  else
    {
    height = vtkKWObject::GetIntegerResult(this->Application) + 
      this->BorderWidth*2 + this->Height + 4;
    }
  
  if (twidth > width)
    {
    width = twidth;
    }
  
  // if the target width and height is less than 10 then
  // we are probably just being unpacked and will soon
  // be repacked. So we'll just ignore it for now.
  if ((width < 10 + this->BorderWidth*2 +4) &&
      (height < 10 + this->BorderWidth*2 + this->Height + 4))
    {
    this->Expanding = 0;
    return;
    }
  
  if (width < this->MinimumWidth)
    {
    width = this->MinimumWidth;
    }
  if (height < this->MinimumHeight)
    {
    height = this->MinimumHeight;
    }

  this->Script("%s configure -width %d -height %d",
               this->GetWidgetName(),width, height);
  this->Expanding = 0;
}


