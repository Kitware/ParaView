/*=========================================================================

  Module:    vtkKWTkUtilities.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWTkUtilities.h"

#include "vtkKWWidget.h"
#include "vtkKWApplication.h"
#include "vtkKWResourceUtilities.h"

#include "vtkObjectFactory.h"

#include "vtkWindows.h"
#include "X11/Xutil.h"

#include <kwsys/SystemTools.hxx>
#include <kwsys/Base64.h>

// This has to be here because on HP varargs are included in 
// tcl.h and they have different prototypes for va_start so
// the build fails. Defining HAS_STDARG prevents that.
#if defined(__hpux) && !defined(HAS_STDARG)
#  define HAS_STDARG
#endif

#include "vtkTk.h"
#if ((VTK_MAJOR_VERSION <= 4) && (VTK_MINOR_VERSION <= 4))
#include "zlib.h" // needed for TIFF
#else
#include "vtk_zlib.h" // needed for TIFF
#endif

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWTkUtilities);
vtkCxxRevisionMacro(vtkKWTkUtilities, "1.46");

//----------------------------------------------------------------------------
const char* vtkKWTkUtilities::EvaluateString(
    Tcl_Interp *interp,
    const char* format, 
    ...)
{
  va_list var_args1, var_args2;
  va_start(var_args1, format);
  va_start(var_args2, format);
  const char* result = vtkKWTkUtilities::EvaluateStringFromArgs(
    interp, format, var_args1, var_args2);
  va_end(var_args1);
  va_end(var_args2);
  return result;
}

//----------------------------------------------------------------------------
const char* vtkKWTkUtilities::EvaluateString(
  vtkKWApplication *app,  
  const char* format, 
  ...)
{
  va_list var_args1, var_args2;
  va_start(var_args1, format);
  va_start(var_args2, format);
  const char* result = vtkKWTkUtilities::EvaluateStringFromArgs(
    app, format, var_args1, var_args2);
  va_end(var_args1);
  va_end(var_args2);
  return result;
}

//----------------------------------------------------------------------------
const char* vtkKWTkUtilities::EvaluateStringFromArgs(
  Tcl_Interp *interp,
  const char* format,
  va_list var_args1,
  va_list var_args2)
{
  return vtkKWTkUtilities::EvaluateStringFromArgsInternal(
    interp,
    NULL,
    format,
    var_args1,
    var_args2);
}

//----------------------------------------------------------------------------
const char* vtkKWTkUtilities::EvaluateStringFromArgs(
  vtkKWApplication *app,  
  const char* format,
  va_list var_args1,
  va_list var_args2)
{
  if (!app)
    {
    return NULL;
    }
  return vtkKWTkUtilities::EvaluateStringFromArgs(
    app->GetMainInterp(),
    format,
    var_args1,
    var_args2);
}

//----------------------------------------------------------------------------
const char* vtkKWTkUtilities::EvaluateStringFromArgsInternal(
  Tcl_Interp *interp,
  vtkObject *dummy,
  const char* format,
  va_list var_args1,
  va_list var_args2)
{
  const int buffer_on_stack_length = 1600;
  char buffer_on_stack[buffer_on_stack_length];
  char* buffer = buffer_on_stack;
  
  // Estimate the length of the result string.  Never underestimates.

  int length = kwsys::SystemTools::EstimateFormatLength(format, var_args1);
  
  // If our stack-allocated buffer is too small, allocate on one on
  // the heap that will be large enough.

  if(length > buffer_on_stack_length - 1)
    {
    buffer = new char[length + 1];
    }
  
  // Print to the buffer.

  vsprintf(buffer, format, var_args2);
  
  // Evaluate the string in Tcl.

  if (Tcl_GlobalEval(interp, buffer) != TCL_OK && dummy)
    {
    vtkErrorWithObjectMacro(
      dummy, "\n    Script: \n" << buffer
      << "\n    Returned Error on line "
      << interp->errorLine << ": \n"  
      << Tcl_GetStringResult(interp) << endl);
    }
  
  // Free the buffer from the heap if we allocated it.

  if (buffer != buffer_on_stack)
    {
    delete [] buffer;
    }
  
  // Convert the Tcl result to its string representation.

  return Tcl_GetStringResult(interp);
}

//----------------------------------------------------------------------------
void vtkKWTkUtilities::GetRGBColor(Tcl_Interp *interp,
                                   const char *widget, 
                                   const char *color, 
                                   int *r, int *g, int *b)
{
  if (!interp || !widget || !color || !r || !g || !b)
    {
    return;
    }

  ostrstream command;
  command << "winfo rgb " << widget << " " << color << ends;
  if (Tcl_GlobalEval(interp, command.str()) != TCL_OK)
    {
    vtkGenericWarningMacro(<< "Unable to get RGB color: " << interp->result);
    command.rdbuf()->freeze(0);     
    return;
    }
  command.rdbuf()->freeze(0);     

  int rr, gg, bb;
  sscanf(interp->result, "%d %d %d", &rr, &gg, &bb);
  *r = static_cast<int>((static_cast<float>(rr) / 65535.0) * 255.0);
  *g = static_cast<int>((static_cast<float>(gg) / 65535.0) * 255.0);
  *b = static_cast<int>((static_cast<float>(bb) / 65535.0) * 255.0); 
}

//----------------------------------------------------------------------------
void vtkKWTkUtilities::GetRGBColor(vtkKWWidget *widget, 
                                   const char *color, 
                                   int *r, int *g, int *b)
{
  if (!widget || !widget->IsCreated())
    {
    return;
    }
  
  vtkKWTkUtilities::GetRGBColor(widget->GetApplication()->GetMainInterp(),
                                widget->GetWidgetName(),
                                color,
                                r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWTkUtilities::GetOptionColor(Tcl_Interp *interp,
                                      const char *widget,
                                      const char *option,
                                      int *r, int *g, int *b)
{
  if (!interp || !widget || !option || !r || !g || !b)
    {
    return;
    }

  ostrstream command;
  command << widget << " cget " << option << ends;
  if (Tcl_GlobalEval(interp, command.str()) != TCL_OK)
    {
    vtkGenericWarningMacro(
      << "Unable to get " << option << " option: " << interp->result);
    command.rdbuf()->freeze(0);     
    return;
    }
  command.rdbuf()->freeze(0);     

  vtkKWTkUtilities::GetRGBColor(interp, widget, interp->result, r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWTkUtilities::GetOptionColor(vtkKWWidget *widget, 
                                      const char *option,
                                      int *r, int *g, int *b)
{
  if (!widget || !widget->IsCreated())
    {
    return;
    }
  
  vtkKWTkUtilities::GetOptionColor(widget->GetApplication()->GetMainInterp(),
                                   widget->GetWidgetName(),
                                   option,
                                   r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWTkUtilities::GetBackgroundColor(Tcl_Interp *interp,
                                          const char *widget,
                                          int *r, int *g, int *b)
{
  vtkKWTkUtilities::GetOptionColor(interp, widget, "-bg", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWTkUtilities::GetBackgroundColor(vtkKWWidget *widget, 
                                          int *r, int *g, int *b)
{
  if (!widget || !widget->IsCreated())
    {
    return;
    }

  vtkKWTkUtilities::GetBackgroundColor(
    widget->GetApplication()->GetMainInterp(),
    widget->GetWidgetName(),
    r, g, b);
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::ContainsCoordinates(Tcl_Interp *interp,
                                          const char *widget, 
                                          int x, int y)
{
  if (!interp || !widget)
    {
    return 0;
    }

  ostrstream geometry;
  geometry << "concat [winfo width " << widget << "] [winfo height "
           << widget << "] [winfo rootx " << widget << "] [winfo rooty "
           << widget << "]" << ends;
  int res = Tcl_GlobalEval(interp, geometry.str());
  geometry.rdbuf()->freeze(0);
  if (res != TCL_OK)
    {
    vtkGenericWarningMacro(<< "Unable to query widget geometry! " << widget);
    return 0;
    }
  
  int ww, wh, wx, wy;
  sscanf(interp->result, "%d %d %d %d", &ww, &wh, &wx, &wy);

  return (x >= wx && x < (wx + ww) && y >= wy && y < (wy + wh)) ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::ContainsCoordinates(vtkKWWidget *widget,
                                          int x, int y)
{
  if (!widget || !widget->IsCreated())
    {
    return 0;
    }

  return vtkKWTkUtilities::ContainsCoordinates(
    widget->GetApplication()->GetMainInterp(),
    widget->GetWidgetName(),
    x, y);
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::UpdatePhoto(Tcl_Interp *interp,
                                  const char *photo_name,
                                  const unsigned char *pixels, 
                                  int width, int height,
                                  int pixel_size,
                                  unsigned long buffer_length,
                                  const char *blend_with_name,
                                  const char *color_option,
                                  int update_options)
{
  // Check params

  if (!interp)
    {
    vtkGenericWarningMacro(<< "Empty interpreter");
    return 0;
    }

  if (!photo_name || !photo_name[0])
    {
    vtkGenericWarningMacro(<< "Empty photo name");
    return 0;
    }

  if (!pixels)
    {
    vtkGenericWarningMacro(<< "No pixel data");
    return 0;
    }

  if (width <= 0 || height <= 0)
    {
    vtkGenericWarningMacro(<< "Invalid size: " << width << "x" << height);
    return 0;
    }

  if (pixel_size != 3 && pixel_size != 4)
    {
    vtkGenericWarningMacro(<< "Unsupported pixel size: " << pixel_size);
    return 0;
    }

  // Find the photo (create it if not found)

  Tk_PhotoHandle photo = Tk_FindPhoto(interp, const_cast<char *>(photo_name));

  if (!photo)
    {
    ostrstream create_photo;
    create_photo << "image create photo " << photo_name << ends;
    int res = Tcl_GlobalEval(interp, create_photo.str());
    create_photo.rdbuf()->freeze(0);
    if (res != TCL_OK)
      {
      vtkGenericWarningMacro(
        << "Unable to create photo " << photo_name << ": " << interp->result);
      return 0;
      }

    photo = Tk_FindPhoto(interp, const_cast<char *>(photo_name));
    if (!photo)
      {
      vtkGenericWarningMacro(<< "Error looking up Tk photo:" << photo_name);
      return 0;
      }
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
      buffer_length = kwsysBase64_Decode(data_ptr, 0, 
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
  sblock.pitch     = sblock.width * sblock.pixelSize;
  sblock.offset[0] = 0;
  sblock.offset[1] = 1;
  sblock.offset[2] = 2;
#if (TK_MAJOR_VERSION == 8) && (TK_MINOR_VERSION >= 3)
  sblock.offset[3] = 0;
#endif
  unsigned long sblock_size = sblock.pitch * sblock.height;

  unsigned char *pp = NULL;

  if (pixel_size <= 3)
    {
    sblock.pixelPtr = const_cast<unsigned char *>(data_ptr);
    }
  else 
    {
    pp = sblock.pixelPtr = new unsigned char[sblock_size];

    // At the moment let's not use the alpha layer inside the photo but 
    // blend with the current background color

    int r, g, b;
    if (blend_with_name)
      {
      vtkKWTkUtilities::GetOptionColor(interp, 
                                       blend_with_name, 
                                       (color_option ? color_option : "-bg"), 
                                       &r, &g, &b);
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

    pp = sblock.pixelPtr;
    }
  
  if (update_options & vtkKWTkUtilities::UPDATE_PHOTO_OPTION_FLIP_V)
    {
    sblock.pitch = -sblock.pitch;
    sblock.pixelPtr += sblock_size + sblock.pitch;
    }

#if (TK_MAJOR_VERSION == 8) && (TK_MINOR_VERSION >= 4) && !defined(USE_COMPOSITELESS_PHOTO_PUT_BLOCK)
  Tk_PhotoPutBlock(photo, &sblock, 0, 0, width, height, TK_PHOTO_COMPOSITE_SET);
#else
  Tk_PhotoPutBlock(photo, &sblock, 0, 0, width, height);
#endif

  // Free mem

  if (pp)
    {
    delete [] pp;
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
int vtkKWTkUtilities::UpdatePhoto(vtkKWApplication *app,
                                  const char *photo_name,
                                  const unsigned char *pixels, 
                                  int width, int height,
                                  int pixel_size,
                                  unsigned long buffer_length,
                                  const char *blend_with_name,
                                  const char *color_option,
                                  int update_options)
{
  if (!app)
    {
    return 0;
    }
  return vtkKWTkUtilities::UpdatePhoto(
    app->GetMainInterp(),
    photo_name,
    pixels, 
    width, height,
    pixel_size,
    buffer_length,
    blend_with_name,
    color_option,
    update_options);
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::UpdateOrLoadPhoto(Tcl_Interp *interp,
                                        const char *photo_name,
                                        const char *file_name,
                                        const char *directory,
                                        const unsigned char *pixels, 
                                        int width, int height,
                                        int pixel_size,
                                        unsigned long buffer_length,
                                        const char *blend_with_name,
                                        const char *color_option)
{
  // Try to find a PNG file with the same name in directory 
  // or directory/Resources

  unsigned char *png_buffer = NULL;

  if (directory && file_name)
    {
    char buffer[1024];
    sprintf(buffer, "%s/%s.png", directory, file_name);
    int found = kwsys::SystemTools::FileExists(buffer);
    if (!found)
      {
      sprintf(buffer, "%s/Resources/%s.png", directory, file_name);
      found = kwsys::SystemTools::FileExists(buffer);
      }
    if (found && 
        vtkKWResourceUtilities::ReadPNGImage(
          buffer, &width, &height, &pixel_size, &png_buffer))
      {
      pixels = png_buffer;
      buffer_length = 0;
      }
    }

  // Otherwise use the provided data

  int res = vtkKWTkUtilities::UpdatePhoto(
    interp,
    (photo_name ? photo_name : file_name), 
    pixels, 
    width, height,
    pixel_size,
    buffer_length,
    blend_with_name,
    color_option);
  
  if (png_buffer)
    {
    delete [] png_buffer;
    }

  return res;
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::UpdateOrLoadPhoto(vtkKWApplication *app,
                                        const char *photo_name,
                                        const char *file_name,
                                        const char *directory,
                                        const unsigned char *pixels, 
                                        int width, int height,
                                        int pixel_size,
                                        unsigned long buffer_length,
                                        const char *blend_with_name,
                                        const char *color_option)
{
  if (!app)
    {
    return 0;
    }
  return vtkKWTkUtilities::UpdateOrLoadPhoto(
    app->GetMainInterp(),
    photo_name,
    file_name,
    directory,
    pixels, 
    width, height,
    pixel_size,
    buffer_length,
    blend_with_name,
    color_option);
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::FindPhoto(Tcl_Interp *interp,
                                const char *photo_name)
{
  return Tk_FindPhoto(interp,const_cast<char *>(photo_name)) ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::FindPhoto(vtkKWApplication *app,
                                     const char *photo_name)
{
  if (!app)
    {
    return 0;
    }
  return vtkKWTkUtilities::FindPhoto(app->GetMainInterp(), photo_name);
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
int vtkKWTkUtilities::GetPhotoHeight(vtkKWApplication *app,
                                     const char *photo_name)
{
  if (!app)
    {
    return 0;
    }
  return vtkKWTkUtilities::GetPhotoHeight(
    app->GetMainInterp(), photo_name);
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
int vtkKWTkUtilities::GetPhotoWidth(vtkKWApplication *app,
                                    const char *photo_name)
{
  if (!app)
    {
    return 0;
    }
  return vtkKWTkUtilities::GetPhotoWidth(
    app->GetMainInterp(), photo_name);
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::ChangeFontWeight(Tcl_Interp *interp,
                                      const char *font, 
                                      char *new_font, 
                                      int weight)
{
  int res;

  // First try to modify the old -foundry-family-weigth-*-*-... form
  // Catch the weight field, replace it with bold or medium.

  ostrstream regsub;
  regsub << "regsub -- {(-[^-]*\\S-[^-]*\\S-)([^-]*)(-.*)} \""
         << font << "\" {\\1" << (weight ? "bold" : "medium") 
         << "\\3} __temp__" << ends;

  res = Tcl_GlobalEval(interp, regsub.str());
  regsub.rdbuf()->freeze(0);
  if (res != TCL_OK)
    {
    vtkGenericWarningMacro(<< "Unable to regsub!");
    return 0;
    }
  if (atoi(Tcl_GetStringResult(interp)) == 1)
    {
    res = Tcl_GlobalEval(interp, "set __temp__");
    if (res != TCL_OK)
      {
      vtkGenericWarningMacro(<< "Unable to replace result of regsub! ("
                             << interp->result << ")");
      return 0;
      }
    strcpy(new_font, Tcl_GetStringResult(interp));
    return 1;
    }

  // Otherwise replace the -weight parameter with either bold or normal

  ostrstream regsub2;
  regsub2 << "regsub -- {(.* -weight )(\\w*\\M)(.*)} [font actual \""
          << font << "\"] {\\1" << (weight ? "bold" : "normal") 
          << "\\3} __temp__" << ends;
  res = Tcl_GlobalEval(interp, regsub2.str());
  regsub2.rdbuf()->freeze(0);
  if (res != TCL_OK)
    {
    vtkGenericWarningMacro(<< "Unable to regsub (2)!");
    return 0;
    }
  if (atoi(Tcl_GetStringResult(interp)) == 1)
    {
    res = Tcl_GlobalEval(interp, "set __temp__");
    if (res != TCL_OK)
      {
      vtkGenericWarningMacro(<< "Unable to replace result of regsub! (2) ("
                             << interp->result << ")");
      return 0;
      }
    strcpy(new_font, Tcl_GetStringResult(interp));
    return 1;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::ChangeFontWeightToBold(Tcl_Interp *interp,
                                             const char *font, 
                                             char *new_font)
{
  return vtkKWTkUtilities::ChangeFontWeight(interp, font, new_font, 1);
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::ChangeFontWeightToNormal(Tcl_Interp *interp,
                                               const char *font, 
                                               char *new_font)
{
  return vtkKWTkUtilities::ChangeFontWeight(interp, font, new_font, 0);
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::ChangeFontWeight(Tcl_Interp *interp,
                                       const char *widget,
                                       int weight)
{
  char font[1024], new_font[1024];
  
  int res;

  // Get the font

  ostrstream getfont;
  getfont << widget << " cget -font" << ends;
  res = Tcl_GlobalEval(interp, getfont.str());
  getfont.rdbuf()->freeze(0);
  if (res != TCL_OK)
    {
    vtkGenericWarningMacro(<< "Unable to getfont!");
    return 0;
    }
  strcpy(font, Tcl_GetStringResult(interp));

  // Change the font weight

  if (!vtkKWTkUtilities::ChangeFontWeight(interp, font, new_font, weight))
    {
    return 0;
    }

  // Set the font

  ostrstream setfont;
  setfont << widget << " config -font \"" << new_font << "\"" << ends;
  res = Tcl_GlobalEval(interp, setfont.str());
  setfont.rdbuf()->freeze(0);
  if (res != TCL_OK)
    {
    vtkGenericWarningMacro(<< "Unable to replace font ! ("
                           << interp->result << ")");
    return 0;
    }
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::ChangeFontWeightToBold(Tcl_Interp *interp,
                                             const char *widget)
{
  return vtkKWTkUtilities::ChangeFontWeight(interp, widget, 1);
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::ChangeFontWeightToBold(vtkKWWidget *widget)
{
  if (!widget || !widget->IsCreated())
    {
    return 0;
    }
  
  return vtkKWTkUtilities::ChangeFontWeightToBold(
    widget->GetApplication()->GetMainInterp(),
    widget->GetWidgetName());
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::ChangeFontWeightToNormal(Tcl_Interp *interp,
                                               const char *widget)
{
  return vtkKWTkUtilities::ChangeFontWeight(interp, widget, 0);
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::ChangeFontWeightToNormal(vtkKWWidget *widget)
{
  if (!widget || !widget->IsCreated())
    {
    return 0;
    }
  
  return vtkKWTkUtilities::ChangeFontWeightToNormal(
    widget->GetApplication()->GetMainInterp(),
    widget->GetWidgetName());
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::ChangeFontSlant(Tcl_Interp *interp,
                                      const char *font, 
                                      char *new_font, 
                                      int slant)
{
  int res;

  // First try to modify the old -foundry-family-weigth-slant-*-*-... form
  // Catch the slant field, replace it with i (italic) or r (roman).

  ostrstream regsub;
  regsub << "regsub -- {(-[^-]*\\S-[^-]*\\S-[^-]*\\S-)([^-]*)(-.*)} \""
         << font << "\" {\\1" << (slant ? "i" : "r") << "\\3} __temp__" 
         << ends;

  res = Tcl_GlobalEval(interp, regsub.str());
  regsub.rdbuf()->freeze(0);
  if (res != TCL_OK)
    {
    vtkGenericWarningMacro(<< "Unable to regsub!");
    return 0;
    }
  if (atoi(Tcl_GetStringResult(interp)) == 1)
    {
    res = Tcl_GlobalEval(interp, "set __temp__");
    if (res != TCL_OK)
      {
      vtkGenericWarningMacro(<< "Unable to replace result of regsub! ("
                             << interp->result << ")");
      return 0;
      }
    strcpy(new_font, Tcl_GetStringResult(interp));
    return 1;
    }

  // Otherwise replace the -slant parameter with either bold or normal

  ostrstream regsub2;
  regsub2 << "regsub -- {(.* -slant )(\\w*\\M)(.*)} [font actual \"" 
          << font << "\"] {\\1" << (slant ? "italic" : "roman") 
          << "\\3} __temp__" << ends;
  res = Tcl_GlobalEval(interp, regsub2.str());
  regsub2.rdbuf()->freeze(0);
  if (res != TCL_OK)
    {
    vtkGenericWarningMacro(<< "Unable to regsub (2)!");
    return 0;
    }
  if (atoi(Tcl_GetStringResult(interp)) == 1)
    {
    res = Tcl_GlobalEval(interp, "set __temp__");
    if (res != TCL_OK)
      {
      vtkGenericWarningMacro(<< "Unable to replace result of regsub! (2) ("
                             << interp->result << ")");
      return 0;
      }
    strcpy(new_font, Tcl_GetStringResult(interp));
    return 1;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::ChangeFontSlantToItalic(Tcl_Interp *interp,
                                              const char *font, 
                                              char *new_font)
{
  return vtkKWTkUtilities::ChangeFontSlant(interp, font, new_font, 1);
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::ChangeFontSlantToRoman(Tcl_Interp *interp,
                                             const char *font, 
                                             char *new_font)
{
  return vtkKWTkUtilities::ChangeFontSlant(interp, font, new_font, 0);
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::ChangeFontSlant(Tcl_Interp *interp,
                                      const char *widget,
                                      int slant)
{
  char font[1024], new_font[1024];
  
  int res;

  // Get the font

  ostrstream getfont;
  getfont << widget << " cget -font" << ends;
  res = Tcl_GlobalEval(interp, getfont.str());
  getfont.rdbuf()->freeze(0);
  if (res != TCL_OK)
    {
    vtkGenericWarningMacro(<< "Unable to getfont!");
    return 0;
    }
  strcpy(font, Tcl_GetStringResult(interp));

  // Change the font slant

  if (!vtkKWTkUtilities::ChangeFontSlant(interp, font, new_font, slant))
    {
    return 0;
    }

  // Set the font

  ostrstream setfont;
  setfont << widget << " config -font \"" << new_font << "\"" << ends;
  res = Tcl_GlobalEval(interp, setfont.str());
  setfont.rdbuf()->freeze(0);
  if (res != TCL_OK)
    {
    vtkGenericWarningMacro(<< "Unable to replace font ! ("
                           << interp->result << ")");
    return 0;
    }
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::ChangeFontSlantToItalic(Tcl_Interp *interp,
                                              const char *widget)
{
  return vtkKWTkUtilities::ChangeFontSlant(interp, widget, 1);
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::ChangeFontSlantToItalic(vtkKWWidget *widget)
{
  if (!widget || !widget->IsCreated())
    {
    return 0;
    }
  
  return vtkKWTkUtilities::ChangeFontSlantToItalic(
    widget->GetApplication()->GetMainInterp(),
    widget->GetWidgetName());
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::ChangeFontSlantToRoman(Tcl_Interp *interp,
                                             const char *widget)
{
  return vtkKWTkUtilities::ChangeFontSlant(interp, widget, 0);
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::ChangeFontSlantToRoman(vtkKWWidget *widget)
{
  if (!widget || !widget->IsCreated())
    {
    return 0;
    }
  
  return vtkKWTkUtilities::ChangeFontSlantToRoman(
    widget->GetApplication()->GetMainInterp(),
    widget->GetWidgetName());
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::GetGridSize(Tcl_Interp *interp,
                                  const char *widget,
                                  int *nb_of_cols,
                                  int *nb_of_rows)
{
  ostrstream size;
  size << "grid size " << widget << ends;
  int res = Tcl_GlobalEval(interp, size.str());
  size.rdbuf()->freeze(0);
  if (res != TCL_OK)
    {
    vtkGenericWarningMacro(<< "Unable to query grid size!");
    return 0;
    }
  sscanf(interp->result, "%d %d", nb_of_cols, nb_of_rows);

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::GetGridSize(vtkKWWidget *widget, 
                                   int *nb_of_cols,
                                   int *nb_of_rows)
{
  if (!widget || !widget->IsCreated())
    {
    return 0;
    }
  
  return vtkKWTkUtilities::GetGridSize(
    widget->GetApplication()->GetMainInterp(),
    widget->GetWidgetName(),
    nb_of_cols, nb_of_rows);
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::GetWidgetPositionInGrid(Tcl_Interp *interp,
                                      const char *widget,
                                      int *col,
                                      int *row)
{
  ostrstream info;
  info << "grid info " << widget << ends;
  int res = Tcl_GlobalEval(interp, info.str());
  info.rdbuf()->freeze(0);
  if (res != TCL_OK)
    {
    vtkGenericWarningMacro(<< "Unable to query grid info!");
    return 0;
    }
  
  const char *pos;

  pos = strstr(interp->result, "-column ");
  if (pos)
    {
    sscanf(pos, "-column %d", col);
    }

  pos = strstr(interp->result, "-row ");
  if (pos)
    {
    sscanf(pos, "-row %d", row);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::GetWidgetPositionInGrid(vtkKWWidget *widget, 
                                       int *col,
                                       int *row)
{
  if (!widget || !widget->IsCreated())
    {
    return 0;
    }
  
  return vtkKWTkUtilities::GetWidgetPositionInGrid(
    widget->GetApplication()->GetMainInterp(),
    widget->GetWidgetName(),
    col, row);
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::GetWidgetPaddingInPack(Tcl_Interp *interp,
                                          const char *widget,
                                          int *ipadx,
                                          int *ipady,
                                          int *padx,
                                          int *pady)
{
  ostrstream packinfo;
  packinfo << "pack info " << widget << ends;
  int res = Tcl_GlobalEval(interp, packinfo.str());
  packinfo.rdbuf()->freeze(0);
  if (res != TCL_OK || !interp->result || !interp->result[0])
    {
    vtkGenericWarningMacro(<< "Unable to get pack info!");
    return 0;
    }
  
  // Parse (ex: -ipadx 0 -ipady 0 -padx 0 -pady 0)

  char *ptr;
  if (ipadx)
    {
    ptr = strstr(interp->result, "-ipadx ");
    if (ptr)
      {
      sscanf(ptr + 7, "%d", ipadx);
      }
    }

  if (ipady)
    {
    ptr = strstr(interp->result, "-ipady ");
    if (ptr)
      {
      sscanf(ptr + 7, "%d", ipady);
      }
    }

  if (padx)
    {
    ptr = strstr(interp->result, "-padx ");
    if (ptr)
      {
      sscanf(ptr + 6, "%d", padx);
      }
    }

  if (pady)
    {
    ptr = strstr(interp->result, "-pady ");
    if (ptr)
      {
      sscanf(ptr + 6, "%d", pady);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::GetMasterInPack(Tcl_Interp *interp,
                                     const char *widget,
                                     ostream &in)
{
  ostrstream packinfo;
  packinfo << "pack info " << widget << ends;
  int res = Tcl_GlobalEval(interp, packinfo.str());
  packinfo.rdbuf()->freeze(0);
  if (res != TCL_OK || !interp->result || !interp->result[0])
    {
    vtkGenericWarningMacro(<< "Unable to get pack info!");
    return 0;
    }
  
  // Parse for -in

  const char *pack_in = strstr(interp->result, "-in ");
  if (!pack_in)
    {
    return 0;
    }

  pack_in += 4;
  const char *pack_in_end = strchr(pack_in, ' ');

  if (pack_in_end)
    {
    char *pack_in_buffer = new char [strlen(pack_in) + 1];
    strncpy(pack_in_buffer, pack_in, pack_in_end - pack_in);
    pack_in_buffer[pack_in_end - pack_in] = '\0';
    in << pack_in_buffer;
    delete [] pack_in_buffer;
    }
  else
    {
    in << pack_in;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::GetMasterInPack(vtkKWWidget *widget, 
                                      ostream &in)
{
  if (!widget || !widget->IsCreated())
    {
    return 0;
    }

  return vtkKWTkUtilities::GetMasterInPack(
    widget->GetApplication()->GetMainInterp(),
    widget->GetWidgetName(),
    in);
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::GetSlavesBoundingBoxInPack(Tcl_Interp *interp,
                                        const char *widget,
                                        int *width,
                                        int *height)
{
  ostrstream slaves;
  slaves << "pack slaves " << widget << ends;
  int res = Tcl_GlobalEval(interp, slaves.str());
  slaves.rdbuf()->freeze(0);
  if (res != TCL_OK)
    {
    vtkGenericWarningMacro(<< "Unable to get pack slaves!");
    return 0;
    }
  
  // No slaves
  
  if (!interp->result || !interp->result[0])
    {
    return 1;
    }
  
  // Browse each slave for reqwidth, reqheight

  int buffer_length = strlen(interp->result);
  char *buffer = new char [buffer_length + 1];
  strcpy(buffer, interp->result);

  char *buffer_end = buffer + buffer_length;
  char *ptr = buffer, *word_end;

  while (ptr < buffer_end)
    {
    // Get the slave name

    word_end = strchr(ptr + 1, ' ');
    if (word_end == NULL)
      {
      word_end = buffer_end;
      }
    else
      {
      *word_end = 0;
      }

    // Get width / height

    ostrstream geometry;
    geometry << "concat [winfo reqwidth " << ptr << "] [winfo reqheight " 
             << ptr << "]"<< ends;
    res = Tcl_GlobalEval(interp, geometry.str());
    geometry.rdbuf()->freeze(0);
    if (res != TCL_OK)
      {
      vtkGenericWarningMacro(<< "Unable to query slave geometry!");
      }
    else
      {
      int w, h;
      sscanf(interp->result, "%d %d", &w, &h);

      // If w == h == 1 then again it might not have been packed, so call
      // recursively

      if (w == 1 && h == 1)
        {
        vtkKWTkUtilities::GetSlavesBoundingBoxInPack(interp, ptr, &w, &h);
        }

      // Don't forget the padding

      int ipadx = 0, ipady = 0, padx = 0, pady = 0;
      vtkKWTkUtilities::GetWidgetPaddingInPack(interp, ptr, 
                                            &ipadx, &ipady, &padx, &pady);

      w += 2 * (padx + ipadx);
      h += 2 * (pady + ipady);

      if (w > *width)
        {
        *width = w;
        }
      if (h > *height)
        {
        *height = h;
        }
      }
    
    ptr = word_end + 1;
    }

  delete [] buffer;

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::GetSlavesBoundingBoxInPack(vtkKWWidget *widget,
                                        int *width,
                                        int *height)
{
  if (!widget || !widget->IsCreated())
    {
    return 0;
    }

  return vtkKWTkUtilities::GetSlavesBoundingBoxInPack(
    widget->GetApplication()->GetMainInterp(),
    widget->GetWidgetName(),
    width, height);
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::GetSlaveHorizontalPositionInPack(Tcl_Interp *interp,
                                                     const char *widget,
                                                     const char *slave,
                                                     int *x)
{
  ostrstream slaves;
  slaves << "pack slaves " << widget << ends;
  int res = Tcl_GlobalEval(interp, slaves.str());
  slaves.rdbuf()->freeze(0);
  if (res != TCL_OK)
    {
    vtkGenericWarningMacro(<< "Unable to get pack slaves!");
    return 0;
    }
  
  // No slaves
  
  if (!interp->result || !interp->result[0])
    {
    vtkGenericWarningMacro(<< "Unable to find slaves!");
    return 0;
    }
  
  // Browse each slave until the right one if found

  int buffer_length = strlen(interp->result);
  char *buffer = new char [buffer_length + 1];
  strcpy(buffer, interp->result);

  char *buffer_end = buffer + buffer_length;
  char *ptr = buffer, *word_end;

  int pos = 0;

  while (ptr < buffer_end)
    {
    // Get the slave name

    word_end = strchr(ptr + 1, ' ');
    if (word_end == NULL)
      {
      word_end = buffer_end;
      }
    else
      {
      *word_end = 0;
      }

    // If slave found, add one padx and leave
    
    if (!strcmp(ptr, slave))
      {
      int padx = 0;
      vtkKWTkUtilities::GetWidgetPaddingInPack(interp, ptr, 0, 0, &padx, 0);
      pos += padx;
      break;
      }

    // Get width

    ostrstream geometry;
    geometry << "winfo reqwidth " << ptr << ends;
    res = Tcl_GlobalEval(interp, geometry.str());
    geometry.rdbuf()->freeze(0);
    if (res != TCL_OK)
      {
      vtkGenericWarningMacro(<< "Unable to query slave geometry!");
      }
    else
      {
      int w = atoi(interp->result);
      
      // If w == 1 then again it might not have been packed, so get bbox

      if (w == 1)
        {
        int h = 0;
        vtkKWTkUtilities::GetSlavesBoundingBoxInPack(interp, ptr, &w, &h);
        }

      // Don't forget the padding

      int ipadx = 0, padx = 0;
      vtkKWTkUtilities::GetWidgetPaddingInPack(interp, ptr, &ipadx, 0, &padx, 0);
      
      pos += w + 2 * (padx + ipadx);
      }
    
    ptr = word_end + 1;
    }

  delete [] buffer;

  *x = pos;

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::GetSlaveHorizontalPositionInPack(vtkKWWidget *widget, 
                                                     vtkKWWidget *slave, 
                                                     int *x)
{
  if (!widget || !widget->IsCreated() ||
      !slave || !slave->IsCreated())
    {
    return 0;
    }
  
  return vtkKWTkUtilities::GetSlaveHorizontalPositionInPack(
    widget->GetApplication()->GetMainInterp(),
    widget->GetWidgetName(),
    slave->GetWidgetName(),
    x);
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::GetGridColumnWidths(Tcl_Interp *interp,
                                          const char *widget,
                                          int *nb_of_cols,
                                          int **col_widths,
                                          int allocate)
{
  // First get grid size

  int nb_of_rows;
  if (!vtkKWTkUtilities::GetGridSize(interp, widget, nb_of_cols, &nb_of_rows))
    {
    vtkGenericWarningMacro(<< "Unable to query grid size!");
    return 0;
    }

  // Iterate over the columns and get the largest widget
  // (I'm expecting only one widget per cell here)

  if (allocate)
    {
    *col_widths = new int[*nb_of_cols];
    }

  int col, row;
  for (col = 0; col < *nb_of_cols; col++)
    {
    (*col_widths)[col] = 0;
    for (row = 0; row < nb_of_rows; row++)
      {
      // Get the slave

      ostrstream slave;
      slave << "grid slaves " << widget << " -column " << col 
            << " -row " << row << ends;
      int res = Tcl_GlobalEval(interp, slave.str());
      slave.rdbuf()->freeze(0);
      if (res != TCL_OK)
        {
        vtkGenericWarningMacro(<< "Unable to get grid slave!");
        continue;
        }

      // No slave, let's process the next row

      if (!interp->result || !interp->result[0])
        {
        continue;
        }

      // Get the slave reqwidth

      ostrstream reqwidth;
      reqwidth << "winfo reqwidth " << interp->result << ends;
      res = Tcl_GlobalEval(interp, reqwidth.str());
      reqwidth.rdbuf()->freeze(0);
      if (res != TCL_OK)
        {
        vtkGenericWarningMacro(<< "Unable to query slave width!");
        continue;
        }
      int width = 0;
      sscanf(interp->result, "%d", &width);

      if (width > (*col_widths)[col])
        {
        (*col_widths)[col] = width;
        }
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::SynchroniseGridsColumnMinimumSize(
  Tcl_Interp *interp,
  int nb_of_widgets,
  const char **widgets,
  const float *factors,
  const int *weights)
{
  // Allocate mem for nb of colums and widths

  int *nb_of_cols = new int [nb_of_widgets];
  int **col_widths = new int* [nb_of_widgets];
  int widget;

  // Collect column widths

  int min_nb_of_cols = 10000;
  for (widget = 0; widget < nb_of_widgets; widget++)
    {
    if (vtkKWTkUtilities::GetGridColumnWidths(
      interp, widgets[widget], &nb_of_cols[widget], &col_widths[widget], 1))
      {
      if (nb_of_cols[widget] < min_nb_of_cols)
        {
        min_nb_of_cols = nb_of_cols[widget];
        }
      }
    }

  // Synchronize columns (for each column, configure -minsize to the largest
  // column width for all grids)

  ostrstream minsize;
  for (int col = 0; col < min_nb_of_cols; col++)
    {
    int col_width_max = 0;
    for (widget = 0; widget < nb_of_widgets; widget++)
      {
      if (col_widths[widget][col] > col_width_max)
        {
        col_width_max = col_widths[widget][col];
        }
      }
    if (factors)
      {
      col_width_max = (int)((float)col_width_max * factors[col]);
      }
    for (widget = 0; widget < nb_of_widgets; widget++)
      {
      minsize << "grid columnconfigure " << widgets[widget] << " " << col 
              << " -minsize " << col_width_max;
      if (weights)
        {
        minsize << " -weight " << weights[col];
        }
      minsize << endl;
      }
    }
  minsize << ends;

  int ok = 1;
  if (Tcl_GlobalEval(interp, minsize.str()) != TCL_OK)
    {
    vtkGenericWarningMacro(<< "Unable to synchronize grid columns!");
    ok = 0;
    }
  minsize.rdbuf()->freeze(0);

  // Free mem

  delete [] nb_of_cols;
  for (widget = 0; widget < nb_of_widgets; widget++)
    {
    delete [] col_widths[widget];
    }
  delete [] col_widths;

  return ok;
}


//----------------------------------------------------------------------------
int vtkKWTkUtilities::SynchroniseLabelsMaximumWidth(
  Tcl_Interp *interp,
  int nb_of_widgets,
  const char **widgets,
  const char *options)
{
  // Get the maximum width

  int width, length, maxwidth = 0;

  int widget;
  for (widget = 0; widget < nb_of_widgets; widget++)
    {
    // Get the -width

    ostrstream getwidth;
    getwidth << widgets[widget] << " cget -width" << ends;
    int res = Tcl_GlobalEval(interp, getwidth.str());
    getwidth.rdbuf()->freeze(0);
    if (res != TCL_OK || !interp->result || !interp->result[0])
      {
      vtkGenericWarningMacro(<< "Unable to get label -width! " 
                             <<interp->result);
      continue;
      }
    width = atoi(interp->result);

    // Get the -text length

    ostrstream getlength;
    getlength << widgets[widget] << " cget -text" << ends;
    res = Tcl_GlobalEval(interp, getlength.str());
    getlength.rdbuf()->freeze(0);
    if (res != TCL_OK)
      {
      vtkGenericWarningMacro(<< "Unable to get label -text! " 
                             << interp->result);
      continue;
      }
    length = interp->result ? strlen(interp->result) : 0;

    // Store the max

    if (width > maxwidth)
      {
      maxwidth = width;
      }
    if (length > maxwidth)
      {
      maxwidth = length;
      }
    }

  // Synchronize labels

  ostrstream setwidth;
  for (widget = 0; widget < nb_of_widgets; widget++)
    {
    setwidth << widgets[widget] << " config -width " << maxwidth;
    if (options)
      {
      setwidth << " " << options;
      }
    setwidth << endl;
    }
  setwidth << ends;
  int res = Tcl_GlobalEval(interp, setwidth.str());
  setwidth.rdbuf()->freeze(0);
  if (res != TCL_OK)
    {
    vtkGenericWarningMacro(<< "Unable to synchronize labels width! " 
                           << interp->result);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::SynchroniseLabelsMaximumWidth(vtkKWApplication *app,
                                                    int nb_of_widgets,
                                                    const char **widgets,
                                                    const char *options)
{
  if (!app)
    {
    return 0;
    }
  return vtkKWTkUtilities::SynchroniseLabelsMaximumWidth(
    app->GetMainInterp(), nb_of_widgets, widgets, options);
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::GetSlavesInPack(
  Tcl_Interp *interp,
  const char *widget,
  char ***slaves)
{
  int res;

  // Get number of slaves

  ostrstream nb_slaves_str;
  nb_slaves_str << "llength [pack slaves " << widget << "]" << ends;
  res = Tcl_GlobalEval(interp, nb_slaves_str.str());
  nb_slaves_str.rdbuf()->freeze(0);
  if (res != TCL_OK || !interp->result || !interp->result[0])
    {
    vtkGenericWarningMacro(<< "Unable to get number of packed slaves!");
    return 0;
    }

  int nb_slaves = atoi(interp->result);
  if (!nb_slaves)
    {
    return 0;
    }

  // Get the slaves as a space-separated list

  ostrstream slaves_str;
  slaves_str << "pack slaves " << widget << ends;
  res = Tcl_GlobalEval(interp, slaves_str.str());
  slaves_str.rdbuf()->freeze(0);
  if (res != TCL_OK || !interp->result || !interp->result[0])
    {
    vtkGenericWarningMacro(<< "Unable to get packed slaves!");
    return 0;
    }
  
  // Allocate slaves

  *slaves = new char* [nb_slaves];
  
  // Browse each slave and store it

  int buffer_length = strlen(interp->result);
  char *buffer = new char [buffer_length + 1];
  strcpy(buffer, interp->result);

  char *buffer_end = buffer + buffer_length;
  char *ptr = buffer, *word_end;
  int i = 0;

  while (ptr < buffer_end && i < nb_slaves)
    {
    word_end = strchr(ptr + 1, ' ');
    if (word_end == NULL)
      {
      word_end = buffer_end;
      }
    else
      {
      *word_end = 0;
      }

    (*slaves)[i] = new char [strlen(ptr) + 1];
    strcpy((*slaves)[i], ptr);

    i++;
    ptr = word_end + 1;
    }

  delete [] buffer;

  return nb_slaves;
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::GetSlavesInPack(vtkKWWidget *widget, 
                                char ***slaves)
{
  if (!widget || !widget->IsCreated())
    {
    return 0;
    }
  
  return vtkKWTkUtilities::GetSlavesInPack(
    widget->GetApplication()->GetMainInterp(),
    widget->GetWidgetName(),
    slaves);
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::GetPreviousAndNextSlaveInPack(
  Tcl_Interp *interp,
  const char *widget,
  const char *slave,
  ostream &previous_slave,
  ostream &next_slave)
{
  // Search (and allocate) the slaves

  char **slaves = 0;
  int nb_slaves = vtkKWTkUtilities::GetSlavesInPack(interp, widget, &slaves);
  if (!nb_slaves)
    {
    return 0;
    }

  // Browse each of them

  int i, found = 0;
  for (i = 0; i < nb_slaves; i++)
    {
    if (!strcmp(slaves[i], slave))
      {
      if (i > 0)
        {
        previous_slave << slaves[i - 1];
        }
      if (i < nb_slaves - 1)
        {
        next_slave << slaves[i + 1];
        }
      found = 1;
      break;
      }
    }

  // Deallocate slaves

  for (i = 0 ; i < nb_slaves; i++)
    {
    delete [] slaves[i];
    }
  delete [] slaves;

  return found;
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::GetPreviousAndNextSlaveInPack(
  vtkKWWidget *widget, 
  vtkKWWidget *slave, 
  ostream &previous_slave,
  ostream &next_slave)
{
  if (!widget || !widget->IsCreated() ||
      !slave || !slave->IsCreated())
    {
    return 0;
    }
  
  return vtkKWTkUtilities::GetPreviousAndNextSlaveInPack(
    widget->GetApplication()->GetMainInterp(),
    widget->GetWidgetName(),
    slave->GetWidgetName(),
    previous_slave, next_slave);
}

/*
 *--------------------------------------------------------------
 *
 * TkImageGetColor --
 *
 *  This procedure converts a pixel value to three floating
 *      point numbers, representing the amount of red, green, and 
 *      blue in that pixel on the screen.  It makes use of colormap
 *      data passed as an argument, and should work for all Visual
 *      types.
 *
 *  This implementation is bogus on Windows because the colormap
 *  data is never filled in.  Instead all postscript generated
 *  data coming through here is expected to be RGB color data.
 *  To handle lower bit-depth images properly, XQueryColors
 *  must be implemented for Windows.
 *
 * Results:
 *  Returns red, green, and blue color values in the range 
 *      0 to 1.  There are no error returns.
 *
 * Side effects:
 *  None.
 *
 *--------------------------------------------------------------
 */

/*
 * The following definition is used in generating postscript for images
 * and windows.
 */

struct vtkKWTkUtilitiesTkColormapData {  /* Hold color information for a window */
  int separated;    /* Whether to use separate color bands */
  int color;      /* Whether window is color or black/white */
  int ncolors;    /* Number of color values stored */
  XColor *colors;    /* Pixel value -> RGB mappings */
  int red_mask, green_mask, blue_mask;  /* Masks and shifts for each */
  int red_shift, green_shift, blue_shift;  /* color band */
};

static void
vtkKWTkUtilitiesTkImageGetColor(vtkKWTkUtilitiesTkColormapData* 
#ifdef WIN32
#else
                           cdata
#endif
                           ,
                           unsigned long pixel, 
                           double *red, 
                           double *green, 
                           double *blue)
#ifdef WIN32

/*
 * We could just define these instead of pulling in windows.h.
#define GetRValue(rgb)  ((BYTE)(rgb))
#define GetGValue(rgb)  ((BYTE)(((WORD)(rgb)) >> 8))
#define GetBValue(rgb)  ((BYTE)((rgb)>>16))
*/

{
  *red   = (double) GetRValue(pixel) / 255.0;
  *green = (double) GetGValue(pixel) / 255.0;
  *blue  = (double) GetBValue(pixel) / 255.0;
}
#else
{
  if (cdata->separated) {
    int r = (pixel & cdata->red_mask) >> cdata->red_shift;
    int g = (pixel & cdata->green_mask) >> cdata->green_shift;
    int b = (pixel & cdata->blue_mask) >> cdata->blue_shift;
    *red   = cdata->colors[r].red / 65535.0;
    *green = cdata->colors[g].green / 65535.0;
    *blue  = cdata->colors[b].blue / 65535.0;
  } else {
    *red   = cdata->colors[pixel].red / 65535.0;
    *green = cdata->colors[pixel].green / 65535.0;
    *blue  = cdata->colors[pixel].blue / 65535.0;
  }
}
#endif

//----------------------------------------------------------------------------
int vtkKWTkUtilities::TakeScreenDump(Tcl_Interp *interp,
                                     const char* widget, 
                                     const char* fname,
                                     int top, int bottom, int left, int right)
{
  if (!interp || !fname || !widget)
    {
    return 0;
    }

  cout << "Trying to writing dump to: " << fname << endl;

  ostrstream geometry;
  geometry << "concat [winfo rootx " << widget << "] [winfo rooty "
           << widget << "] [winfo width " << widget << "] [winfo height "
           << widget << "]" << ends;
  int res = Tcl_GlobalEval(interp, geometry.str());
  geometry.rdbuf()->freeze(0);
  if (res != TCL_OK)
    {
    vtkGenericWarningMacro(<< "Unable to query widget geometry! " << widget);
    return 0;
    }
  
  int xx, yy, ww, hh;
  xx = yy = ww = hh = 0;
  if (sscanf(interp->result, "%d %d %d %d", &xx, &yy, &ww, &hh) != 4)
    {
    return 0;
    }

  xx -= left;
  yy -= top;
  ww += left + right;
  hh += top + bottom;

  Tk_Window image_window;

  image_window = Tk_MainWindow(interp);
  Display *dpy = Tk_Display(image_window);
  int screen = DefaultScreen(dpy);
  Window win=RootWindow(dpy, screen);

  XImage *ximage = XGetImage(dpy, win, xx, yy,
    (unsigned int)ww, (unsigned int)hh, AllPlanes, XYPixmap);
  if ( !ximage )
    {
    return 0;
    }
  unsigned int buffer_size = ximage->bytes_per_line * ximage->height;
  if (ximage->format != ZPixmap)
    {
    buffer_size = ximage->bytes_per_line * ximage->height * ximage->depth;
    }

  vtkKWTkUtilitiesTkColormapData cdata;
  Colormap cmap;
  Visual *visual;
  int i, ncolors;
  cmap = Tk_Colormap(image_window);
  visual = Tk_Visual(image_window);

  /*
   * Obtain information about the colormap, ie the mapping between
   * pixel values and RGB values.  The code below should work
   * for all Visual types.
   */

  ncolors = visual->map_entries;
  cdata.colors = (XColor *) ckalloc(sizeof(XColor) * ncolors);
  cdata.ncolors = ncolors;

  if (visual->c_class == DirectColor || visual->c_class == TrueColor) 
    {
    cdata.separated = 1;
    cdata.red_mask = visual->red_mask;
    cdata.green_mask = visual->green_mask;
    cdata.blue_mask = visual->blue_mask;
    cdata.red_shift = 0;
    cdata.green_shift = 0;
    cdata.blue_shift = 0;
    while ((0x0001 & (cdata.red_mask >> cdata.red_shift)) == 0)
      cdata.red_shift ++;
    while ((0x0001 & (cdata.green_mask >> cdata.green_shift)) == 0)
      cdata.green_shift ++;
    while ((0x0001 & (cdata.blue_mask >> cdata.blue_shift)) == 0)
      cdata.blue_shift ++;
    for (i = 0; i < ncolors; i ++)
      cdata.colors[i].pixel =
        ((i << cdata.red_shift) & cdata.red_mask) |
        ((i << cdata.green_shift) & cdata.green_mask) |
        ((i << cdata.blue_shift) & cdata.blue_mask);
    } 
  else 
    {
    cdata.separated=0;
    for (i = 0; i < ncolors; i ++)
      cdata.colors[i].pixel = i;
    }
  if (visual->c_class == StaticGray || visual->c_class == GrayScale)
    {
    cdata.color = 0;
    }
  else
    {
    cdata.color = 1;
    }

  XQueryColors(Tk_Display(image_window), cmap, cdata.colors, ncolors);

  /*
   * Figure out which color level to use (possibly lower than the 
   * one specified by the user).  For example, if the user specifies
   * color with monochrome screen, use gray or monochrome mode instead. 
   */

  int level = 2;
  if (!cdata.color && level == 2) 
    {
    level = 1;
    }

  if (!cdata.color && cdata.ncolors == 2) 
    {
    level = 0;
    }


  unsigned long stride = ww * 3;
  unsigned long buffer_length = stride * hh;
  unsigned char *buffer = new unsigned char [buffer_length];
  unsigned char *ptr = buffer + buffer_length - stride;

  int x, y;
  for (y = 0; y < hh; y++) 
    {
    for (x = 0; x < ww; x++) 
      {
      double red, green, blue;
      vtkKWTkUtilitiesTkImageGetColor(
        &cdata, XGetPixel(ximage, x, hh - y - 1), &red, &green, &blue);
      *ptr++ = (unsigned char)(255 * red);
      *ptr++ = (unsigned char)(255 * green);
      *ptr++ = (unsigned char)(255 * blue);
      }
    ptr -= stride;
    ptr -= stride;
    }

  vtkKWResourceUtilities::WritePNGImage(fname, ww, hh, 3, buffer);

  delete [] buffer;

  XDestroyImage(ximage);

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWTkUtilities::TakeScreenDump(vtkKWWidget *widget, 
                                     const char* fname,
                                     int top, int bottom, int left, int right)
{
  if (!widget || !widget->IsCreated())
    {
    return 0;
    }
  
  return vtkKWTkUtilities::TakeScreenDump(
    widget->GetApplication()->GetMainInterp(),
    widget->GetWidgetName(),
    fname,
    top, bottom, left, right);
}

//----------------------------------------------------------------------------
void vtkKWTkUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

