/*=========================================================================

  Module:    vtkKWSplashScreen.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWSplashScreen - a splash dialog.
// .SECTION Description
// A class for displaying splash screen.

#ifndef __vtkKWSplashScreen_h
#define __vtkKWSplashScreen_h

#include "vtkKWTopLevel.h"

class vtkKWCanvas;

class KWWidgets_EXPORT vtkKWSplashScreen : public vtkKWTopLevel
{
public:
  static vtkKWSplashScreen* New();
  vtkTypeRevisionMacro(vtkKWSplashScreen, vtkKWTopLevel);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the text of the progress message
  void SetProgressMessage(const char *);
  
  // Description:
  // Set/Get the offset of the progress message (negative value means
  // offset from the bottom of the splash, positive value from the top)
  virtual void SetProgressMessageVerticalOffset(int);
  vtkGetMacro(ProgressMessageVerticalOffset, int);

  // Description:
  // Set/Get the name of the splash image (a Tk image name)
  vtkGetStringMacro(ImageName);
  virtual void SetImageName(const char*);
  
  // Description:
  // Read an image and use it as the splash image.
  // If ImageName is set, this method will update the corresponding
  // Tk image, otherwise it will create a new one and assign its name to
  // ImageName.
  // Check vtkKWResourceUtilities::ReadImage for the list of supported
  // image format
  // Return 1 on success, 0 otherwise
  virtual int ReadImage(const char *filename);
  
  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWSplashScreen();
  ~vtkKWSplashScreen();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  vtkKWCanvas *Canvas;
  char *ImageName;
  int ProgressMessageVerticalOffset;

  void UpdateCanvasSize();
  void UpdateProgressMessagePosition();

  // Description:
  // Get the width/height of the toplevel as requested
  // by the window manager. Not exposed in public since it is so Tk
  // related. Is is usually used to get the geometry of a window before
  // it is mapped to screen, as requested by the geometry manager.
  // Override to prevent the splashscreen from flickering at startup.
  // Return the size of the image itself, without explicitly calling
  // 'update' to let the geometry manager figure things out (= flicker)
  virtual int GetRequestedWidth();
  virtual int GetRequestedHeight();

private:
  vtkKWSplashScreen(const vtkKWSplashScreen&); // Not implemented
  void operator=(const vtkKWSplashScreen&); // Not implemented
};


#endif



