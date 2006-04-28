/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCredits.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCredits - class managing splash screen/about dialog in ParaView.
// .SECTION Description
// This class encapsulates the Splash screen and About diablog for ParaView.

#ifndef __vtkPVCredits_h
#define __vtkPVCredits_h

#include "vtkKWObject.h"

class vtkKWLabel;
class vtkKWMessageDialog;
class vtkKWPushButton;
class vtkKWSplashScreen;
class vtkKWTextWithScrollbars;
class vtkKWWidget;
class vtkPVApplication;

class VTK_EXPORT vtkPVCredits : public vtkKWObject
{
public:
  static vtkPVCredits* New();
  vtkTypeRevisionMacro(vtkPVCredits, vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Show the About Dialog.
  virtual void ShowAboutDialog(vtkKWWidget* master);

  // Description:
  // Show the Splash Screen.
  virtual void ShowSplashScreen();

  // Description:
  // Set Splash screen progress message.
  virtual void SetSplashScreenProgressMessage(const char* message);

  // Description:
  // Hide Splash Screen.
  virtual void HideSplashScreen();

  // Description:
  // Retrieve the splash screen object.
  virtual vtkKWSplashScreen* GetSplashScreen();

  // Description:
  // Save to a file the information available in the "About ParaView" dialog.
  virtual void SaveRuntimeInformation();
protected:
  vtkPVCredits();
  ~vtkPVCredits();

  // Description:
  // Method that builds the About dialog.
  virtual void ConfigureAboutDialog();

  // Description:
  // Method that creates the Spalsh screen.
  virtual void ConfigureSplashScreen();

  // Description:
  // Create the image to be shown in Splash Screen.
  virtual const char* CreateSplashScreenPhoto();

  virtual void AddAboutText(ostream&);
  virtual void AddAboutCopyrights(ostream&);

  vtkKWSplashScreen* SplashScreen;
  int SplashScreenPhotoCreated;
  vtkKWMessageDialog* AboutDialog;
  vtkKWLabel* AboutDialogImage;
  vtkKWTextWithScrollbars* AboutRuntimeInfo;
  vtkKWPushButton* SaveRuntimeInfoButton;

private:
  vtkPVCredits(const vtkPVCredits&); // Not implemented.
  void operator=(const vtkPVCredits&); // Not implemented.
};


#endif

