/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWSplashScreen.cxx
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
#include "vtkKWSplashScreen.h"

#include "vtkKWApplication.h"
#include "vtkKWImageLabel.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWSplashScreen );
vtkCxxRevisionMacro(vtkKWSplashScreen, "1.5");

//-----------------------------------------------------------------------------
vtkKWSplashScreen::vtkKWSplashScreen()
{
  this->MainFrame = vtkKWWidget::New();
  this->MainFrame->SetParent(this);

  this->ProgressMessage = vtkKWWidget::New();
  this->ProgressMessage->SetParent(this->MainFrame);

  this->Image = vtkKWImageLabel::New();
  this->Image->SetParent(this->MainFrame);
}

//-----------------------------------------------------------------------------
vtkKWSplashScreen::~vtkKWSplashScreen()
{
  if (this->MainFrame)
    {
    this->MainFrame->Delete();
    }
  if (this->ProgressMessage)
    {
    this->ProgressMessage->Delete();
    }
  if (this->Image)
    {
    this->Image->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkKWSplashScreen::Create(vtkKWApplication *app, const char *args)
{
  if (this->Application)
    {
    vtkErrorMacro("Dialog already created");
    return;
    }
  this->SetApplication(app);

  this->Script("toplevel %s", this->GetWidgetName());

  this->Script("wm withdraw %s", this->GetWidgetName());
  this->Script("wm overrideredirect %s 1", this->GetWidgetName());

  this->MainFrame->Create(app, "frame", args);
  this->Script("%s config -bd 1 -relief solid",
               this->MainFrame->GetWidgetName());

  this->Script("pack %s -fill both -expand y",
               this->MainFrame->GetWidgetName());

  this->ProgressMessage->Create(app, "label", args);
  this->Image->Create(app, args);

  this->Script("pack %s %s -side top -fill x -expand y",
               this->Image->GetWidgetName(),
               this->ProgressMessage->GetWidgetName());

}

//-----------------------------------------------------------------------------
void vtkKWSplashScreen::ShowWithBind()
{
  this->SetProgressMessage(0);
  this->Show();
  this->Image->SetBind(this, "<ButtonPress>", "Hide");
}

//-----------------------------------------------------------------------------
void vtkKWSplashScreen::Show()
{
  int sw, sh;
  this->Script("concat [winfo screenwidth %s] [winfo screenheight %s]",
               this->GetWidgetName(), this->GetWidgetName());
  sscanf(this->GetApplication()->GetMainInterp()->result, "%d %d", &sw, &sh);

  int w, h;
  const char *photo  = this->Image->GetImageDataName();
  this->Script("concat [%s cget -width] [%s cget -height]", photo, photo);
  sscanf(this->GetApplication()->GetMainInterp()->result, "%d %d", &w, &h);

  int x = (sw - w) / 2;
  int y = (sh - h) / 2;

  this->Script("wm geometry %s +%d+%d", this->GetWidgetName(), x, y);

  this->Script("wm deiconify %s", this->GetWidgetName());
  this->Script("raise %s", this->GetWidgetName());
  this->Script("focus %s", this->GetWidgetName());
  this->Script("update", this->GetWidgetName());
}

//-----------------------------------------------------------------------------
void vtkKWSplashScreen::Hide()
{
  this->Script("wm withdraw %s", this->GetWidgetName());
}

//-----------------------------------------------------------------------------
void vtkKWSplashScreen::SetProgressMessage(const char *txt)
{
  this->Script("%s configure -text {%s}",
               this->ProgressMessage->GetWidgetName(), (txt?txt:""));
  this->Show();
}

//----------------------------------------------------------------------------
void vtkKWSplashScreen::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Image: " << this->GetImage() << endl;
}
