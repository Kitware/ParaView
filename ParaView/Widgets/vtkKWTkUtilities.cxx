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

// This has to be here because on HP varargs are included in 
// tcl.h and they have different prototypes for va_start so
// the build fails. Defining HAS_STDARG prevents that.
#if defined(__hpux) && !defined(HAS_STDARG)
#  define HAS_STDARG
#endif

#include "tk.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWTkUtilities);
vtkCxxRevisionMacro(vtkKWTkUtilities, "1.5");

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
    sblock.pixelPtr = const_cast<unsigned char *>(pixels);
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
        float alpha = static_cast<float>(*(pixels + 3)) / 255.0;
        
        *(pp)     = static_cast<int>(r * (1 - alpha) + *(pixels)     * alpha);
        *(pp + 1) = static_cast<int>(g * (1 - alpha) + *(pixels + 1) * alpha);
        *(pp + 2) = static_cast<int>(b * (1 - alpha) + *(pixels + 2) * alpha);

        pixels += pixel_size;
        pp += 3;
        }
      }
    }

#if (TK_MAJOR_VERSION == 8) && (TK_MINOR_VERSION >= 4)
  Tk_PhotoPutBlock(photo, &sblock, 0, 0, width, height, TK_PHOTO_COMPOSITE_SET);
#else
  Tk_PhotoPutBlock(photo, &sblock, 0, 0, width, height);
#endif

  if (pixel_size != 3)
    {
    delete [] sblock.pixelPtr;
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

  int res = vtkKWTkUtilities::UpdatePhoto(
    interp,
    photo_name,
    static_cast<unsigned char*>(flip->GetOutput()->GetScalarPointer()),
    ext[1] - ext[0] + 1, 
    ext[3] - ext[2] + 1,
    image->GetNumberOfScalarComponents(),
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
void vtkKWTkUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
