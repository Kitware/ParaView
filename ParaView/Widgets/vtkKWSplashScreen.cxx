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
#include "vtkKWSplashScreen.h"

#include "vtkKWApplication.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWSplashScreen );
vtkCxxRevisionMacro(vtkKWSplashScreen, "1.13");

//----------------------------------------------------------------------------
vtkKWSplashScreen::vtkKWSplashScreen()
{
  this->Canvas = vtkKWWidget::New();
  this->Canvas->SetParent(this);

  this->ImageName = NULL;
  this->ProgressMessageVerticalOffset = -10;
}

//----------------------------------------------------------------------------
vtkKWSplashScreen::~vtkKWSplashScreen()
{
  if (this->Canvas)
    {
    this->Canvas->Delete();
    }

  this->SetImageName(0);
}

//----------------------------------------------------------------------------
void vtkKWSplashScreen::Create(vtkKWApplication *app, const char *args)
{
  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("Dialog already created");
    return;
    }
  this->SetApplication(app);

  // Create the toplevel

  this->Script("toplevel %s", this->GetWidgetName());

  this->Script("wm withdraw %s", this->GetWidgetName());
  this->Script("wm overrideredirect %s 1", this->GetWidgetName());

  // Create and pack the canvas

  this->Canvas->Create(app, "canvas", "-borderwidth 0 -highlightthickness 0");
  this->Script("%s config %s", 
               this->Canvas->GetWidgetName(), args);
  this->Script("pack %s -side top -fill both -expand y",
               this->Canvas->GetWidgetName());

  this->Canvas->SetBind(this, "<ButtonPress>", "Hide");

  // Insert the image

  this->Script("%s create image 0 0 -tags image -anchor nw", 
               this->Canvas->GetWidgetName());
  
  // Insert the text

  this->Script("%s create text 0 0 -tags msg -anchor c", 
               this->Canvas->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWSplashScreen::UpdateCanvasSize()
{
  // Resize the canvas according to the image

  if (this->Application && this->ImageName)
    {
    int w, h;
    this->Script("concat [%s cget -width] [%s cget -height]", 
                 this->ImageName, this->ImageName);
    sscanf(this->GetApplication()->GetMainInterp()->result, "%d %d", &w, &h);

    this->Script("%s config -width %d -height %d",
                 this->Canvas->GetWidgetName(), w, h);
    }
}

//----------------------------------------------------------------------------
void vtkKWSplashScreen::UpdateProgressMessagePosition()
{
  if (this->Application)
    {
    this->Script("%s cget -height", this->Canvas->GetWidgetName());
    int height = vtkKWObject::GetIntegerResult(this->Application);

    this->Script("%s coords msg [expr 0.5 * [%s cget -width]] %d", 
                 this->Canvas->GetWidgetName(), 
                 this->Canvas->GetWidgetName(), 
                 (ProgressMessageVerticalOffset < 0 
                  ? height + ProgressMessageVerticalOffset 
                  : ProgressMessageVerticalOffset));
    }
}

//----------------------------------------------------------------------------
void vtkKWSplashScreen::Show()
{
  if (!this->Application)
    {
    return;
    }

  // Update canvas size and message position

  this->UpdateCanvasSize();
  this->UpdateProgressMessagePosition();

  // Get screen size

  int sw, sh;
  this->Script("concat [winfo screenwidth %s] [winfo screenheight %s]",
               this->GetWidgetName(), this->GetWidgetName());
  sscanf(this->GetApplication()->GetMainInterp()->result, "%d %d", &sw, &sh);

  // Get size of splash from image size

  int w = 0, h = 0;
  if (this->ImageName)
    {
    this->Script("concat [%s cget -width] [%s cget -height]", 
                 this->ImageName, this->ImageName);
    sscanf(this->GetApplication()->GetMainInterp()->result, "%d %d", &w, &h);
    }

  // Center the splash

  int x = (sw - w) / 2;
  int y = (sh - h) / 2;

  this->Script("wm geometry %s +%d+%d", this->GetWidgetName(), x, y);

  this->Script("wm deiconify %s", this->GetWidgetName());
  this->Script("raise %s", this->GetWidgetName());
  this->Script("focus %s", this->GetWidgetName());
  this->Script("update", this->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWSplashScreen::Hide()
{
  this->Script("wm withdraw %s", this->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWSplashScreen::SetImageName (const char* _arg)
{
  if (this->ImageName == NULL && _arg == NULL) 
    { 
    return;
    }

  if (this->ImageName && _arg && (!strcmp(this->ImageName, _arg))) 
    {
    return;
    }

  if (this->ImageName) 
    { 
    delete [] this->ImageName; 
    }

  if (_arg)
    {
    this->ImageName = new char[strlen(_arg)+1];
    strcpy(this->ImageName, _arg);
    }
  else
    {
    this->ImageName = NULL;
    }

  this->Modified();

  if (this->Application && this->ImageName)
    {
    this->Script("%s itemconfigure image -image %s", 
                 this->Canvas->GetWidgetName(), this->ImageName);
    }
} 

//----------------------------------------------------------------------------
void vtkKWSplashScreen::SetProgressMessage(const char *txt)
{
  if (!this->IsCreated() || !txt)
    {
    return;
    }

  const char *str = this->ConvertInternalStringToTkString(txt);;
  this->Script("%s itemconfigure msg -text {%s}",
               this->Canvas->GetWidgetName(), (str ? str : ""));

  this->Show();
}

//----------------------------------------------------------------------------
void vtkKWSplashScreen::SetProgressMessageVerticalOffset(int _arg)
{
  if (this->ProgressMessageVerticalOffset == _arg)
    {
    return;
    }

  this->ProgressMessageVerticalOffset = _arg;
  this->Modified();

  this->UpdateProgressMessagePosition();
}

//----------------------------------------------------------------------------
void vtkKWSplashScreen::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ProgressMessageVerticalOffset: " 
     << this->ProgressMessageVerticalOffset << endl;
  os << indent << "ImageName: " 
     << (this->ImageName ? this->ImageName : "(none)") << endl;
}

