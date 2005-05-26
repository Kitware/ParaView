/*=========================================================================

  Module:    vtkKWResourceUtilities.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWResourceUtilities - class that supports resource functions
// .SECTION Description
// vtkKWResourceUtilities provides methods to perform common resources
//  operations.

#ifndef __vtkKWResourceUtilities_h
#define __vtkKWResourceUtilities_h

#include "vtkObject.h"
#include "vtkKWWidgets.h" // Needed for export symbols directives

class vtkKWWidget;
class vtkKWApplication;

class KWWIDGETS_EXPORT vtkKWResourceUtilities : public vtkObject
{
public:
  static vtkKWResourceUtilities* New();
  vtkTypeRevisionMacro(vtkKWResourceUtilities,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Read a PNG file given its 'filename'.
  // On success, modifies 'width', 'height', 'pixel_size' and 'pixels' 
  // accordingly. Note that 'pixels' is allocated automatically using the
  // 'new' operator (i.e. it is up to the caller to free the memory using
  // the 'delete' operator).
  // Note that grayscale images are promoted to RGB. Transparent images are
  // promoted to RGBA. The bit depth is promoted (or shrunk) to 8 bits
  // per component. The resulting 'pixel_size' is therefore always
  // 3 (RGB) or 4 (RGBA).
  // Return 1 on success, 0 otherwise.
  static int ReadPNGImage(const char *filename,
                          int *width, int *height, 
                          int *pixel_size,
                          unsigned char **pixels);

  // Description:
  // Write a PNG file given its 'filename'.
  // The bit depth has to be 8 bit (i.e. unsigned char).
  // The 'pixel_size' can be 1 (grayscale), 2 (grayscale + alpha), 3 (RGB),
  // or 4 (RGB + alpha).
  // Return 1 on success, 0 otherwise.
  static int WritePNGImage(const char *filename,
                           int width, int height, 
                           int pixel_size,
                           unsigned char *pixels);

  // Description:
  // Convert 'nb_images' images (stored in an array of filenames given by 
  // 'image_filenames') into a C/C++ header given by 'header_filename'.
  // Note that only PNG images are supported at the moment.
  // The structure and contents of each image are decoded and 
  // written into a form that can be used programatically. 
  // For example, the file foobar.png is converted into:
  //   #define image_foobar_width         19
  //   #define image_foobar_height        19
  //   #define image_foobar_pixel_size    3
  //   #define image_foobar_buffer_length 40
  //   static unsigned char image_foobar[] = 
  //     "eNpjYCAfPH1wg1Q0qnFU46jGwaaRPAAAa7/zXA==";
  // Several options can be combined into the 'options' parameter.
  //   CONVERT_IMAGE_TO_HEADER_OPTION_ZLIB: 
  //    => Compress the pixels using zlib
  //   CONVERT_IMAGE_TO_HEADER_OPTION_BASE64: 
  //    => Encode the pixels in base64
  //   CONVERT_IMAGE_TO_HEADER_OPTION_UPDATE: 
  //    => Update the header file only if one of the image is newer
  //BTX
  enum
  {
    CONVERT_IMAGE_TO_HEADER_OPTION_ZLIB   = 1,
    CONVERT_IMAGE_TO_HEADER_OPTION_BASE64 = 2,
    CONVERT_IMAGE_TO_HEADER_OPTION_UPDATE = 4
  };
  //ETX
  static int ConvertImageToHeader(
    const char *header_filename,
    const char **image_filenames,
    int nb_images,
    int options);

protected:
  vtkKWResourceUtilities() {};
  ~vtkKWResourceUtilities() {};

private:
  vtkKWResourceUtilities(const vtkKWResourceUtilities&); // Not implemented
  void operator=(const vtkKWResourceUtilities&); // Not implemented
};

#endif

