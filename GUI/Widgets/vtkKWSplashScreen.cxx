/*=========================================================================

  Module:    vtkKWSplashScreen.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWSplashScreen.h"

#include "vtkKWApplication.h"
#include "vtkKWCanvas.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWSplashScreen );
vtkCxxRevisionMacro(vtkKWSplashScreen, "1.27");

//----------------------------------------------------------------------------
vtkKWSplashScreen::vtkKWSplashScreen()
{
  this->Canvas = vtkKWCanvas::New();
  this->Canvas->SetParent(this);

  this->ImageName = NULL;
  this->ProgressMessageVerticalOffset = -10;

  this->DisplayPosition = vtkKWTopLevel::DisplayPositionScreenCenter;
  this->HideDecoration  = 1;
}

//----------------------------------------------------------------------------
vtkKWSplashScreen::~vtkKWSplashScreen()
{
  if (this->Canvas)
    {
    this->Canvas->Delete();
    }

  this->SetImageName(NULL);
}

//----------------------------------------------------------------------------
void vtkKWSplashScreen::Create(vtkKWApplication *app, const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create(app, args);

  // Create and pack the canvas

  this->Canvas->Create(app, "-bd 0 -highlightthickness 0");
  this->Canvas->ConfigureOptions(args);

  this->Script("pack %s -side top -fill both -expand y",
               this->Canvas->GetWidgetName());

  this->Canvas->SetBind(this, "<ButtonPress>", "Withdraw");

  // Insert the image

  this->Script("%s create image 0 0 -tags image -anchor nw", 
               this->Canvas->GetWidgetName());

  if (this->ImageName)
    {
    this->Script("%s itemconfigure image -image %s", 
                 this->Canvas->GetWidgetName(), this->ImageName);
    }
  
  // Insert the text

  this->Script("%s create text 0 0 -tags msg -anchor c", 
               this->Canvas->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWSplashScreen::UpdateCanvasSize()
{
  // Resize the canvas according to the image

  if (this->IsCreated() && this->ImageName)
    {
    int w, h;
    const char *res = 
      this->Script("concat [%s cget -width] [%s cget -height]", 
                   this->ImageName, this->ImageName);
    sscanf(res, "%d %d", &w, &h);

    this->Script("%s config -width %d -height %d",
                 this->Canvas->GetWidgetName(), w, h);
    }
}

//----------------------------------------------------------------------------
void vtkKWSplashScreen::UpdateProgressMessagePosition()
{
  if (this->IsCreated())
    {
    int height = atoi(
      this->Script("%s cget -height", this->Canvas->GetWidgetName()));

    this->Script("%s coords msg [expr 0.5 * [%s cget -width]] %d", 
                 this->Canvas->GetWidgetName(), 
                 this->Canvas->GetWidgetName(), 
                 (ProgressMessageVerticalOffset < 0 
                  ? height + ProgressMessageVerticalOffset 
                  : ProgressMessageVerticalOffset));
    }
}

//----------------------------------------------------------------------------
void vtkKWSplashScreen::Display()
{
  // Update canvas size and message position

  this->UpdateCanvasSize();
  this->UpdateProgressMessagePosition();

  this->Superclass::Display();

  // As much as call to 'update' are evil, this is the only way to bring
  // the splashscreen up-to-date and in front. 'update idletasks' will not
  // do the trick because this code is usually executed during initialization
  // or creation of the UI, not in the event loop

  this->Script("update");
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

  if (this->ImageName && this->Canvas && this->Canvas->IsCreated())
    {
    this->Canvas->Script("%s itemconfigure image -image %s", 
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

  const char *str = this->ConvertInternalStringToTclString(txt);;
  this->Script("catch {%s itemconfigure msg -text {%s}}",
               this->Canvas->GetWidgetName(), (str ? str : ""));

  this->Display();
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

// ---------------------------------------------------------------------------
void vtkKWSplashScreen::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->Canvas);
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

