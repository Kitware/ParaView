/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCredits.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCredits.h"

#include "vtkKWLabel.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWPushButton.h"
#include "vtkKWSplashScreen.h"
#include "vtkKWText.h"
#include "vtkKWTextWithScrollbars.h"
#include "vtkKWTkUtilities.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVGUIClientOptions.h"

#include "Resources/vtkPVSplashScreen.h" // for image.

#include <vtkstd/string>

vtkStandardNewMacro(vtkPVCredits);
vtkCxxRevisionMacro(vtkPVCredits, "1.6");
//-----------------------------------------------------------------------------
vtkPVCredits::vtkPVCredits()
{
  this->SplashScreen = 0;
  this->AboutDialog = 0;
  this->AboutDialogImage = 0;
  this->AboutRuntimeInfo = 0;
  this->SplashScreenPhotoCreated = 0;
  this->SaveRuntimeInfoButton = 0;
}

//-----------------------------------------------------------------------------
vtkPVCredits::~vtkPVCredits()
{
  this->SetApplication(0);
  if (this->SplashScreen)
    {
    this->SplashScreen->Delete();
    this->SplashScreen = 0;
    }

  if (this->AboutDialogImage)
    {
    this->AboutDialogImage->Delete();
    this->AboutDialogImage = 0;
    }

  if (this->AboutDialog)
    {
    this->AboutDialog->Delete();
    this->AboutDialog = 0;
    }

  if (this->AboutRuntimeInfo)
    {
    this->AboutRuntimeInfo->Delete();
    this->AboutRuntimeInfo = 0;
    }

  if (this->SaveRuntimeInfoButton)
    {
    this->SaveRuntimeInfoButton->Delete();
    this->SaveRuntimeInfoButton = 0;
    }
}

//-----------------------------------------------------------------------------
vtkKWSplashScreen* vtkPVCredits::GetSplashScreen()
{
  if (!this->SplashScreen)
    {
    this->SplashScreen = vtkKWSplashScreen::New();
    }

  if (!this->SplashScreen->IsCreated())
    {
    this->SplashScreen->SetApplication(this->GetApplication());
    this->SplashScreen->Create();
    }
  return this->SplashScreen;
}

//-----------------------------------------------------------------------------
void vtkPVCredits::ShowAboutDialog(vtkKWWidget* master)
{
  if (!this->GetApplication())
    {
    vtkErrorMacro("Application pointer must be set!");
    return;
    }
  if (!this->AboutDialog)
    {
    this->AboutDialog = vtkKWMessageDialog::New();
    }

  if (!this->AboutDialog->IsCreated())
    {
    this->AboutDialog->SetMasterWindow(master);
    this->AboutDialog->HideDecorationOn();
    this->AboutDialog->Create();
    this->AboutDialog->SetBorderWidth(1);
    this->AboutDialog->SetReliefToSolid();
    }

  this->ConfigureAboutDialog();

  this->AboutDialog->Invoke();
}

//-----------------------------------------------------------------------------
void vtkPVCredits::ShowSplashScreen()
{
  if (!this->GetApplication())
    {
    vtkErrorMacro("Application pointer must be set!");
    return;
    }
  
  this->GetSplashScreen();
  this->ConfigureSplashScreen();
}

//-----------------------------------------------------------------------------
void vtkPVCredits::SetSplashScreenProgressMessage(const char* message)
{
  if (this->SplashScreen)
    {
    this->SplashScreen->SetProgressMessage(message);
    }
}

//-----------------------------------------------------------------------------
void vtkPVCredits::HideSplashScreen()
{
  if (this->SplashScreen)
    {
    this->SplashScreen->Withdraw();
    }
}

//-----------------------------------------------------------------------------
void vtkPVCredits::ConfigureAboutDialog()
{
  const char* photo_name = this->CreateSplashScreenPhoto();
  if (photo_name)
    {
    if (!this->AboutDialogImage)
      {
      this->AboutDialogImage = vtkKWLabel::New();
      }
    if (!this->AboutDialogImage->IsCreated())
      {
      this->AboutDialogImage->SetParent(this->AboutDialog->GetTopFrame());
      this->AboutDialogImage->Create();
      }
    this->AboutDialogImage->SetConfigurationOption("-image", photo_name);
    this->AboutDialogImage->Script("pack %s -side top", 
      this->AboutDialogImage->GetWidgetName());
    int w = vtkKWTkUtilities::GetPhotoWidth(this->GetApplication()->GetMainInterp(), 
      photo_name);
    int h = vtkKWTkUtilities::GetPhotoHeight(this->GetApplication()->GetMainInterp(), 
      photo_name);
    this->AboutDialog->GetTopFrame()->SetWidth(w);
    this->AboutDialog->GetTopFrame()->SetHeight(h);
    if (w > this->AboutDialog->GetTextWidth())
      {
      this->AboutDialog->SetTextWidth(w);
      }
    this->AboutDialog->Script(
      "pack %s -side bottom",  // -expand 1 -fill both
      this->AboutDialog->GetMessageDialogFrame()->GetWidgetName());
    }

  if (!this->AboutRuntimeInfo)
    {
    this->AboutRuntimeInfo = vtkKWTextWithScrollbars::New();
    }
  if (!this->AboutRuntimeInfo->IsCreated())
    {
    this->AboutRuntimeInfo->SetParent(this->AboutDialog->GetBottomFrame());
    this->AboutRuntimeInfo->Create();
    this->AboutRuntimeInfo->VerticalScrollbarVisibilityOn();
    this->AboutRuntimeInfo->HorizontalScrollbarVisibilityOff();

    vtkKWText *text = this->AboutRuntimeInfo->GetWidget();
    text->SetWidth(60);
    text->SetHeight(8);
    text->SetWrapToWord();
    text->ReadOnlyOn();

    double r, g, b;
    vtkKWFrame *parent = vtkKWFrame::SafeDownCast(text->GetParent());
    parent->GetBackgroundColor(&r, &g, &b);
    text->SetBackgroundColor(r, g, b);
    this->AboutRuntimeInfo->Script("pack %s -side top -padx 2 -expand 1 -fill both",
      this->AboutRuntimeInfo->GetWidgetName());
    }

  ostrstream title;
  title << "About " << this->GetApplication()->GetPrettyName() << ends;
  this->AboutDialog->SetTitle(title.str());
  title.rdbuf()->freeze(0);

  ostrstream str;
  this->AddAboutText(str);
  str << endl;
  this->AddAboutCopyrights(str);
  str << ends;
  this->AboutRuntimeInfo->GetWidget()->SetText( str.str() );
  str.rdbuf()->freeze(0);

  if (!this->SaveRuntimeInfoButton)
    {
    this->SaveRuntimeInfoButton = vtkKWPushButton::New();
    }
  if (!this->SaveRuntimeInfoButton->IsCreated())
    {
    this->SaveRuntimeInfoButton->SetParent(
      this->AboutDialog->GetBottomFrame());
    this->SaveRuntimeInfoButton->SetText("Save Information");
    this->SaveRuntimeInfoButton->Create();
    this->SaveRuntimeInfoButton->SetWidth(16);
    this->SaveRuntimeInfoButton->SetCommand(this,
      "SaveRuntimeInformation");
    }
  this->SaveRuntimeInfoButton->Script("pack %s -side bottom",
    this->SaveRuntimeInfoButton->GetWidgetName());
  this->AboutRuntimeInfo->GetWidget()->SetHeight(14);
  this->AboutRuntimeInfo->GetWidget()->SetConfigurationOption(
    "-font", "Helvetica 9");
}

//-----------------------------------------------------------------------------
void vtkPVCredits::ConfigureSplashScreen()
{
  const char* photo_name = this->CreateSplashScreenPhoto();
  if (this->SplashScreen)
    {
    this->SplashScreen->SetProgressMessageVerticalOffset(-17);
    this->SplashScreen->SetImageName(photo_name);
    }
}

//-----------------------------------------------------------------------------
const char* vtkPVCredits::CreateSplashScreenPhoto()
{
  if (!this->SplashScreenPhotoCreated)
    {
    // copy the image from the header file into memory
    unsigned char *buffer = 
      new unsigned char [image_PVSplashScreen_length];

    unsigned int i;
    unsigned char *curPos = buffer;
    for (i = 0; i < image_PVSplashScreen_nb_sections; i++)
      {
      size_t len = strlen((const char*)image_PVSplashScreen_sections[i]);
      memcpy(curPos, image_PVSplashScreen_sections[i], len);
      curPos += len;
      }

    vtkPVApplication* pvapp = vtkPVApplication::SafeDownCast(
      this->GetApplication());

    pvapp->CreatePhoto("PVSplashScreen", 
      buffer, 
      image_PVSplashScreen_width, 
      image_PVSplashScreen_height,
      image_PVSplashScreen_pixel_size,
      image_PVSplashScreen_length);
    delete [] buffer; 

    this->SplashScreenPhotoCreated = 1;
    }
  return "PVSplashScreen";
}
//----------------------------------------------------------------------------
void vtkPVCredits::AddAboutText(ostream &os)
{
  os << this->GetApplication()->GetName() << " was developed by Kitware Inc." << endl
    << "http://www.paraview.org" << endl
    << "http://www.kitware.com" << endl
    << "This is version " << this->GetApplication()->GetMajorVersion() 
    << "." << this->GetApplication()->GetMinorVersion()
    << ", release " << this->GetApplication()->GetReleaseName() << endl;

  ostrstream str;
  vtkIndent indent;
  vtkPVApplication::SafeDownCast(this->GetApplication())
    ->GetOptions()->PrintSelf( str, indent.GetNextIndent() );
  str << ends;
  vtkstd::string tmp = str.str();
  os << endl << tmp.substr( tmp.find( "Runtime information:" ) ).c_str();
  str.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkPVCredits::AddAboutCopyrights(ostream &os)
{
  int tcl_major, tcl_minor, tcl_patch_level;
  Tcl_GetVersion(&tcl_major, &tcl_minor, &tcl_patch_level, NULL);

  os << "Tcl/Tk " 
     << tcl_major << "." << tcl_minor << "." << tcl_patch_level << endl
     << "  - Copyright (c) 1989-1994 The Regents of the University of "
     << "California." << endl
     << "  - Copyright (c) 1994 The Australian National University." << endl
     << "  - Copyright (c) 1994-1998 Sun Microsystems, Inc." << endl
     << "  - Copyright (c) 1998-2000 Ajuba Solutions." << endl;
}

//----------------------------------------------------------------------------
void vtkPVCredits::SaveRuntimeInformation()
{
  vtkKWLoadSaveDialog *dialog = vtkKWLoadSaveDialog::New();
  dialog->RetrieveLastPathFromRegistry("RuntimeInformationPath");
  dialog->SaveDialogOn();
  dialog->SetParent(this->AboutDialog);
  dialog->SetTitle("Save Runtime Information");
  dialog->SetFileTypes("{{text file} {.txt}}");
  dialog->Create();

  if (dialog->Invoke() &&
      strlen(dialog->GetFileName()) > 0)
    {
    const char *filename = dialog->GetFileName();
    ofstream file;
    file.open(filename, ios::out);
    if (file.fail())
      {
      vtkErrorMacro("Could not write file " << filename);
      dialog->Delete();
      return;
      }
    this->AddAboutText(file);
    file << endl;
    this->AddAboutCopyrights(file);
    dialog->SaveLastPathToRegistry("RuntimeInformationPath");
    }
  dialog->Delete();
}
//-----------------------------------------------------------------------------
void vtkPVCredits::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
