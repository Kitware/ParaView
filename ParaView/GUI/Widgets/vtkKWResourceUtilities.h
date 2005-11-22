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
  // Read an image given its 'filename'.
  // On success, modifies 'width', 'height', 'pixel_size' and 'pixels' 
  // accordingly. Note that 'pixels' is allocated automatically using the
  // 'new' operator (i.e. it is up to the caller to free the memory using
  // the 'delete' operator).
  // The following formats are recognized (given the file extension):
  // - PNG (.png)
  // Return 1 on success, 0 otherwise.
  static int ReadImage(const char *filename,
                       int *width, int *height, 
                       int *pixel_size,
                       unsigned char **pixels);

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
  // Convert 'nb_files' files (stored in an array of filenames given by 
  // 'filenames') into a C/C++ header given by 'header_filename'.
  // The structure (if any) and contents of each file are decoded and 
  // written into a form that can be used programatically. 
  // An attempt is made to read the file as an image first (using ReadImage).
  // If it succeeds, the width, height and pixel_size are stored in the
  // header file as well as the buffer length and contents, prefixed with
  // 'image_' and the name of the file *without* its extension.
  // For example, the file foobar.png is converted into:
  //   static const unsigned int  image_foobar_width          = 19;
  //   static const unsigned int  image_foobar_height         = 19;
  //   static const unsigned int  image_foobar_pixel_size     = 3;
  //   static const unsigned long image_foobar_length         = 40;
  //   static const unsigned long image_foobar_decoded_length = 1083;
  //   static const unsigned char image_foobar[] = 
  //     "eNpjYCAfPH1wg1Q0qnFU46jGwaaRPAAAa7/zXA==";
  // If the file can not be read as an image, it is treated as a simple
  // stream of bytes and still converted accordingly. Only the buffer length
  // and contents are output, prefixed with 'file_' and the name of the
  // file *with* its extension ('.' are replaced by '_').
  // For example, the file foobar.tcl is converted into:
  //   static const unsigned long file_foobar_tcl_length         = 40
  //   static const unsigned long file_foobar_tcl_decoded_length = 2048
  //   static const unsigned char file_foobar_tcl[] = 
  //     "eNpjYCAfPH1wg1Q0qnFU46jGwaaRPAAAa7/zXA==";
  // Several options can be combined into the 'options' parameter.
  //   CONVERT_IMAGE_TO_HEADER_OPTION_ZLIB: 
  //    => Compress the data using zlib
  //   CONVERT_IMAGE_TO_HEADER_OPTION_BASE64: 
  //    => Encode the data in base64
  //   CONVERT_IMAGE_TO_HEADER_OPTION_UPDATE: 
  //    => Update the header file only if one of the file(s) is/are newer
  // Note that if the contents of the file is encoded using zlib and/or base64
  // the _length field still represents the size of the *encoded* 
  // buffer. The expected size of the decoded buffer can be found using
  // the _decoded_length field (which should match
  // width * height * pixel_size for images)
  //BTX
  enum
  {
    ConvertImageToHeaderOptionZlib   = 1,
    ConvertImageToHeaderOptionBase64 = 2,
    ConvertImageToHeaderOptionUpdate = 4
  };
  //ETX
  static int ConvertImageToHeader(
    const char *header_filename,
    const char **filenames,
    int nb_files,
    int options);

  // Description:
  // Encode a buffer that using zlib and/or base64.
  // output_buffer is automatically allocated using the 'new' operator
  // and should be deallocated using 'delete []'.
  // The 'options' parameter is the same as ConvertImageToHeader.
  // Return 1 on success, 0 otherwise (also sets *output to NULL on error).
  static int EncodeBuffer(
    const unsigned char *input, unsigned long input_length, 
    unsigned char **output, unsigned long *output_length,
    int options);

  // Description:
  // Decode a buffer that was encoded using zlib and/or base64.
  // output_buffer is automatically allocated using the 'new' operator
  // and should be deallocated using 'delete []'.
  // Note that it does allocate an extra-byte (i.e. output_expected_length + 1)
  // for convenience purposes, so that the resulting buffer can be
  // NULL terminated manually.
  // Return 1 on success, 0 otherwise (also sets *output to NULL on error).
  static int DecodeBuffer(
    const unsigned char *input, unsigned long input_length, 
    unsigned char **output, unsigned long output_expected_length);

protected:
  vtkKWResourceUtilities() {};
  ~vtkKWResourceUtilities() {};

private:
  vtkKWResourceUtilities(const vtkKWResourceUtilities&); // Not implemented
  void operator=(const vtkKWResourceUtilities&); // Not implemented
};

#endif

