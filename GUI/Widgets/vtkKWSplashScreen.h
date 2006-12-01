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
class vtkKWIcon;

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
  // Specifies an image to display in the splashscreen.
  // The SetImageToPredefinedIcon method accepts an index to one of the
  // predefined icon listed in vtkKWIcon.
  // The SetImageToPixels method sets the image using pixel data. It expects
  // a pointer to the pixels and the structure of the image, i.e. its width, 
  // height and the pixel_size (how many bytes per pixel, say 3 for RGB, or
  // 1 for grayscale). If buffer_length = 0, it is computed automatically
  // from the previous parameters. If it is not, it will most likely indicate
  // that the buffer has been encoded using base64 and/or zlib.
  // If pixel_size > 3 (i.e. RGBA), the image is blend the with background
  // color of the widget.
  // The SetImageName method can be used to specify a pre-existing Tk image.
  virtual void SetImageToIcon(vtkKWIcon *icon);
  virtual void SetImageToPredefinedIcon(int icon_index);
  virtual void SetImageToPixels(
    const unsigned char *pixels, int width, int height, int pixel_size,
    unsigned long buffer_length = 0);

  // Description:
  // Read an image and use it as the splash image.
  // Check vtkKWResourceUtilities::ReadImage for the list of supported
  // image format
  // Return 1 on success, 0 otherwise
  virtual int ReadImage(const char *filename);
  
  // Description:
  // Set/Get the name of the splashscreen image, as a Tk image name. 
  // This method is kept for backward compatibility only, as it exposes
  // our dependency to Tk internal data structures. Use ReadImage, 
  // SetImageToIcon or SetImageToPixels instead.
  vtkGetStringMacro(ImageName);
  virtual void SetImageName(const char*);
  
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

  virtual void UpdateImageInCanvas();
  virtual void UpdateCanvasSize();
  virtual void UpdateProgressMessagePosition();

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



