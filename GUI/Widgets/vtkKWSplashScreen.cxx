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
#include "vtkKWResourceUtilities.h"
#include "vtkKWTkUtilities.h"

#include <vtksys/stl/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWSplashScreen );
vtkCxxRevisionMacro(vtkKWSplashScreen, "1.36");

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
void vtkKWSplashScreen::Create()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create();

  // Create and pack the canvas

  this->Canvas->Create();
  this->Canvas->SetBorderWidth(0);
  this->Canvas->SetHighlightThickness(0);

  this->Script("pack %s -side top -fill both -expand y",
               this->Canvas->GetWidgetName());

  this->Canvas->AddBinding("<ButtonPress>", this, "Withdraw");

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
    vtkKWApplication *app = this->GetApplication();
    this->Canvas->SetWidth(
      vtkKWTkUtilities::GetPhotoWidth(app, this->ImageName));
    this->Canvas->SetHeight(
      vtkKWTkUtilities::GetPhotoHeight(app, this->ImageName));
    }
}

//----------------------------------------------------------------------------
void vtkKWSplashScreen::UpdateProgressMessagePosition()
{
  if (this->IsCreated())
    {
    int width = this->Canvas->GetWidth();
    int height = this->Canvas->GetHeight();

    this->Script("%s coords msg %lf %d", 
                 this->Canvas->GetWidgetName(), 
                 (double)width * 0.5, 
                 (this->ProgressMessageVerticalOffset < 0 
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

  vtkKWTkUtilities::ProcessPendingEvents(this->GetApplication());
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
    const char *res = this->Canvas->Script(
      "%s itemconfigure image -image %s", 
      this->Canvas->GetWidgetName(), this->ImageName);
    if (res && *res)
      {
      vtkErrorMacro("Error setting ImageName: " << res);
      }
    }
} 

//----------------------------------------------------------------------------
int vtkKWSplashScreen::ReadImage(const char *filename)
{
  int width, height, pixel_size;
  unsigned char *image_buffer = NULL;

  // Try to load the image

  if (!vtkKWResourceUtilities::ReadImage(
        filename, &width, &height, &pixel_size, &image_buffer))
    {
    vtkErrorMacro("Error reading image: " << (filename ? filename : ""));
    return 0;
    }

  // If no image name, make up one

  vtksys_stl::string new_image_name;
  if (!this->ImageName)
    {
    new_image_name = this->GetTclName();
    new_image_name += "Photo";
    }
  const char *image_name = 
    (this->ImageName ? this->ImageName : new_image_name.c_str());

  // Update the Tk image (or create it if it did not exist)

  int res = vtkKWTkUtilities::UpdatePhoto(
    this->GetApplication(), image_name, image_buffer,width,height,pixel_size);
  if (!res)
    {
    vtkErrorMacro("Error updating photo: " << image_name);
    }

  // Assign the new image name (now that it has been created)

  if (new_image_name.size())
    {
    this->SetImageName(new_image_name.c_str());
    }

  delete [] image_buffer;
  return res;
}

//----------------------------------------------------------------------------
void vtkKWSplashScreen::SetProgressMessage(const char *txt)
{
  if (!this->IsCreated() || !txt)
    {
    return;
    }

  const char *val = this->ConvertInternalStringToTclString(
    txt, vtkKWCoreWidget::ConvertStringEscapeInterpretable);
  this->Script("%s itemconfigure msg -text \"%s\"",
               this->Canvas->GetWidgetName(), (val ? val : ""));

  if (!this->IsMapped())
    {
    this->Display();
    }
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
int vtkKWSplashScreen::GetRequestedWidth()
{
  if (this->IsCreated() && this->ImageName)
    {
    return vtkKWTkUtilities::GetPhotoWidth(this->GetApplication(), 
                                           this->ImageName);
    }
  return this->Superclass::GetRequestedWidth();
}

//----------------------------------------------------------------------------
int vtkKWSplashScreen::GetRequestedHeight()
{
  if (this->IsCreated() && this->ImageName)
    {
    return vtkKWTkUtilities::GetPhotoHeight(this->GetApplication(), 
                                            this->ImageName);
    }
  return this->Superclass::GetRequestedHeight();
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

