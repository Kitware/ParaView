/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWTkUtilities.cxx
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
#include "vtkKWTkUtilities.h"

#include "vtkObjectFactory.h"
#include "vtkImageFlip.h"
#include "vtkBase64Utility.h"

// This has to be here because on HP varargs are included in 
// tcl.h and they have different prototypes for va_start so
// the build fails. Defining HAS_STDARG prevents that.
#if defined(__hpux) && !defined(HAS_STDARG)
#  define HAS_STDARG
#endif

#include "tk.h"
#include "zlib.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWTkUtilities);
vtkCxxRevisionMacro(vtkKWTkUtilities, "1.8");

//----------------------------------------------------------------------------
void vtkKWTkUtilities::GetRGBColor(Tcl_Interp *interp,
                                   const char *window, 
                                   const char *color, 
                                   int *rr, int *gg, int *bb)
{
  ostrstream command;
  command << "winfo rgb " << window << " " << color << ends;
  if (Tcl_GlobalEval(interp, command.str()) != TCL_OK)
    {
    vtkGenericWarningMacro(<< "Unable to get RGB color: " << interp->result);
    command.rdbuf()->freeze(0);     
    return;
    }
  command.rdbuf()->freeze(0);     

  int r, g, b;
  sscanf(interp->result, "%d %d %d", &r, &g, &b);
  *rr = static_cast<int>((static_cast<float>(r) / 65535.0)*255.0);
  *gg = static_cast<int>((static_cast<float>(g) / 65535.0)*255.0);
  *bb = static_cast<int>((static_cast<float>(b) / 65535.0)*255.0); 
}

//-----------------------------------------------------------------------------
void vtkKWTkUtilities::GetBackgroundColor(Tcl_Interp *interp,
                                          const char *window,
                                          int *r, int *g, int *b)
{
  ostrstream command;
  command << "lindex [ " << window << " configure -bg ] end" << ends;
  if (Tcl_GlobalEval(interp, command.str()) != TCL_OK)
    {
    vtkGenericWarningMacro(<< "Unable to get -bg option: " << interp->result);
    command.rdbuf()->freeze(0);     
    return;
    }
  command.rdbuf()->freeze(0);     

  vtkKWTkUtilities::GetRGBColor(interp, window, interp->result, r, g, b);
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::UpdatePhoto(Tcl_Interp *interp,
                                  const char *photo_name,
                                  const unsigned char *pixels, 
                                  int width, int height,
                                  int pixel_size,
                                  unsigned long buffer_length,
                                  const char *blend_with_name)
{
  // Find the photo

  Tk_PhotoHandle photo = Tk_FindPhoto(interp,
                                      const_cast<char *>(photo_name));
  if (!photo)
    {
    vtkGenericWarningMacro(<< "Error looking up Tk photo:" << photo_name);
    return 0;
    }  

  Tk_PhotoSetSize(photo, width, height);

#if (TK_MAJOR_VERSION == 8) && (TK_MINOR_VERSION < 4)
  Tk_PhotoBlank(photo);
#endif

  unsigned long nb_of_raw_bytes = width * height * pixel_size;
  const unsigned char *data_ptr = pixels;

  // If the buffer_lenth has been provided, and if it's different than the
  // expected size of the raw image buffer, than it might have been compressed
  // using zlib and/or encoded in base64. In that case, decode and/or
  // uncompress the buffer.

  int base64 = 0;
  unsigned char *base64_buffer = 0;

  int zlib = 0;
  unsigned char *zlib_buffer = 0;

  if (buffer_length && buffer_length != nb_of_raw_bytes)
    {
    // Is it a base64 stream (i.e. not zlib for the moment) ?

    if (data_ptr[0] != 0x78 || data_ptr[1] != 0xDA)
      {
      base64_buffer = new unsigned char [buffer_length];
      buffer_length = vtkBase64Utility::Decode(data_ptr, 0, 
                                               base64_buffer, buffer_length);
      if (buffer_length == 0)
        {
        vtkGenericWarningMacro(<< "Error decoding base64 stream");
        delete [] base64_buffer;
        return 0;
        }
      base64 = 1;
      data_ptr = base64_buffer;
      }
    
    // Is it zlib ?

    if (buffer_length != nb_of_raw_bytes &&
        data_ptr[0] == 0x78 && data_ptr[1] == 0xDA)
      {
      unsigned long zlib_buffer_length = nb_of_raw_bytes;
      zlib_buffer = new unsigned char [zlib_buffer_length];
      if (uncompress(zlib_buffer, &zlib_buffer_length, 
                     data_ptr, buffer_length) != Z_OK ||
          zlib_buffer_length != nb_of_raw_bytes)
        {
        vtkGenericWarningMacro(<< "Error decoding zlib stream");
        delete [] zlib_buffer;
        if (base64)
          {
          delete [] base64_buffer;
          }
        return 0;
        }
      zlib = 1;
      data_ptr = zlib_buffer;
      }
    }

  // Set block struct

  Tk_PhotoImageBlock sblock;

  sblock.width     = width;
  sblock.height    = height;
  sblock.pixelSize = 3;
  sblock.pitch     = width * sblock.pixelSize;
  sblock.offset[0] = 0;
  sblock.offset[1] = 1;
  sblock.offset[2] = 2;

  if (pixel_size == 3)
    {
    sblock.pixelPtr = const_cast<unsigned char *>(data_ptr);
    }
  else 
    {
    unsigned char *pp = sblock.pixelPtr = 
      new unsigned char[sblock.width * sblock.height * sblock.pixelSize];

    // At the moment let's not use the alpha layer inside the photo but 
    // blend with the current background color

    int r, g, b;
    if (blend_with_name)
      {
      vtkKWTkUtilities::GetBackgroundColor(interp, blend_with_name, &r, &g, &b);
      }
    else
      {
      vtkKWTkUtilities::GetBackgroundColor(interp, ".", &r, &g, &b);
      }

    // Create photo pixels

    int xx, yy;
    for (yy=0; yy < height; yy++)
      {
      for (xx=0; xx < width; xx++)
        {
        float alpha = static_cast<float>(*(data_ptr + 3)) / 255.0;
        
        *(pp)     = static_cast<int>(r * (1 - alpha) + *(data_ptr)    * alpha);
        *(pp + 1) = static_cast<int>(g * (1 - alpha) + *(data_ptr+ 1) * alpha);
        *(pp + 2) = static_cast<int>(b * (1 - alpha) + *(data_ptr+ 2) * alpha);

        data_ptr += pixel_size;
        pp += 3;
        }
      }
    }

#if (TK_MAJOR_VERSION == 8) && (TK_MINOR_VERSION >= 4)
  Tk_PhotoPutBlock(photo, &sblock, 0, 0, width, height, TK_PHOTO_COMPOSITE_SET);
#else
  Tk_PhotoPutBlock(photo, &sblock, 0, 0, width, height);
#endif

  // Free mem

  if (pixel_size != 3)
    {
    delete [] sblock.pixelPtr;
    }

  if (base64)
    {
    delete [] base64_buffer;
    }

  if (zlib)
    {
    delete [] zlib_buffer;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::UpdatePhoto(Tcl_Interp *interp,
                                  const char *photo_name,
                                  vtkImageData *image, 
                                  const char *blend_with_name)
{
  if (!image )
    {
    vtkGenericWarningMacro(<< "No image data specified");
    return 0;
    }
  image->Update();

  int *ext = image->GetWholeExtent();
  if ((ext[5] - ext[4]) > 0)
    {
    vtkGenericWarningMacro(<< "Can only handle 2D image data");
    return 0;
    }

  vtkImageFlip *flip = vtkImageFlip::New();
  flip->SetInput(image);
  flip->SetFilteredAxis(1);
  flip->Update();

  int width = ext[1] - ext[0] + 1;
  int height = ext[3] - ext[2] + 1;
  int pixel_size = image->GetNumberOfScalarComponents();

  int res = vtkKWTkUtilities::UpdatePhoto(
    interp,
    photo_name,
    static_cast<unsigned char*>(flip->GetOutput()->GetScalarPointer()),
    width, height,
    pixel_size,
    width * height * pixel_size,
    blend_with_name);

  flip->Delete();
  return res;
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::GetPhotoHeight(Tcl_Interp *interp,
                                     const char *photo_name)
{
  // Find the photo

  Tk_PhotoHandle photo = Tk_FindPhoto(interp,
                                      const_cast<char *>(photo_name));
  if (!photo)
    {
    vtkGenericWarningMacro(<< "Error looking up Tk photo:" << photo_name);
    return 0;
    }  

  // Return height

  int width, height;
  Tk_PhotoGetSize(photo, &width, &height);
  return height;
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::GetPhotoWidth(Tcl_Interp *interp,
                                    const char *photo_name)
{
  // Find the photo

  Tk_PhotoHandle photo = Tk_FindPhoto(interp,
                                      const_cast<char *>(photo_name));
  if (!photo)
    {
    vtkGenericWarningMacro(<< "Error looking up Tk photo:" << photo_name);
    return 0;
    }  

  // Return width

  int width, height;
  Tk_PhotoGetSize(photo, &width, &height);
  return width;
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::ChangeFontToBold(Tcl_Interp *interp,
                                       const char *widget)
{
  int res;

  // First try to modify the old -foundry-family-weigth-*-*-... form

  ostrstream regsub;
  regsub << "regsub -- {(-[^-]*-[^-]*-)([^-]*)(-.*)} [" << widget 
          << " cget -font] {\\1bold\\3} __temp__" << ends;
  res = Tcl_GlobalEval(interp, regsub.str());
  regsub.rdbuf()->freeze(0);
  if (res != TCL_OK)
    {
    vtkGenericWarningMacro(<< "Unable to regsub!");
    return 0;
    }
  if (atoi(Tcl_GetStringResult(interp)) == 1)
    {
    ostrstream replace;
    replace << widget << " config -font $__temp__" << ends;
    res = Tcl_GlobalEval(interp, replace.str());
    replace.rdbuf()->freeze(0);
    if (res != TCL_OK)
      {
      vtkGenericWarningMacro(<< "Unable to replace result of regsub!");
      return 0;
      }
    return 1;
    }

  // Otherwise replace the -weight parameter

  ostrstream regsub2;
  regsub2 << "regsub -- {(.* -weight )(\\w*\\M)(.*)} [font actual [" << widget 
          << " cget -font]] {\\1bold\\3} __temp__" << ends;
  res = Tcl_GlobalEval(interp, regsub2.str());
  regsub2.rdbuf()->freeze(0);
  if (res != TCL_OK)
    {
    vtkGenericWarningMacro(<< "Unable to regsub (2)!");
    return 0;
    }
  if (atoi(Tcl_GetStringResult(interp)) == 1)
    {
    ostrstream replace2;
    replace2 << widget << " config -font $__temp__" << ends;
    res = Tcl_GlobalEval(interp, replace2.str());
    replace2.rdbuf()->freeze(0);
    if (res != TCL_OK)
      {
      vtkGenericWarningMacro(<< "Unable to replace result of regsub (2)!");
      return 0;
      }
    return 1;
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkKWTkUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
