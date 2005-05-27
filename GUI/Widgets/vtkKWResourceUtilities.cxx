/*=========================================================================

  Module:    vtkKWResourceUtilities.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWResourceUtilities.h"

#include "vtkKWWidget.h"
#include "vtkKWApplication.h"
#include "vtkObjectFactory.h"

#include "vtk_png.h"
#include "vtk_zlib.h"

#include <kwsys/SystemTools.hxx>
#include <kwsys/Base64.h>

#ifdef _MSC_VER
// Let us get rid of this funny warning on /W4:
// warning C4611: interaction between '_setjmp' and C++ object 
// destruction is non-portable
#pragma warning( disable : 4611 )
#endif 

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWResourceUtilities);
vtkCxxRevisionMacro(vtkKWResourceUtilities, "1.6");

//----------------------------------------------------------------------------
int vtkKWResourceUtilities::ReadPNGImage(
  const char *filename,
  int *widthp, int *heightp, 
  int *pixel_size,
  unsigned char **pixels)
{
  // Open

  FILE *fp = fopen(filename, "rb");
  if (!fp)
    {
    vtkGenericWarningMacro("Unable to open file " << filename);
    return 0;
    }

  // Is it a PNG file ?

  unsigned char header[8];
  fread(header, 1, 8, fp);
  int is_png = !png_sig_cmp(header, 0, 8);
  if (!is_png)
    {
    vtkGenericWarningMacro("Unknown file type! Not a PNG file!");
    fclose(fp);
    return 0;
    }

  // Get some structures about the PNG file

  png_structp png_ptr = png_create_read_struct
    (PNG_LIBPNG_VER_STRING, (png_voidp)NULL, NULL, NULL);
  if (!png_ptr)
    {
    vtkGenericWarningMacro("Out of memory.");
    fclose(fp);
    return 0;
    }
  
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
    {
    vtkGenericWarningMacro("Out of memory.");
    png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
    fclose(fp);
    return 0;
    }

  png_infop end_info = png_create_info_struct(png_ptr);
  if (!end_info)
    {
    vtkGenericWarningMacro("Unable to read PNG file!");
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
    fclose(fp);
    return 0;
    }
  
  // Set error handling

  if (setjmp(png_jmpbuf(png_ptr)))
    {
    vtkGenericWarningMacro("Unable to set error handler!");
    png_destroy_read_struct (&png_ptr, &info_ptr, (png_infopp)NULL);
    fclose(fp);
    return 0;
    }

  // Read structure

  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);
  png_read_info(png_ptr, info_ptr);

  // Get size and bit-depth of the PNG-image

  png_uint_32 width, height;
  int bit_depth, color_type, interlace_type;
  int compression_type, filter_method;

  png_get_IHDR(png_ptr, info_ptr, 
               &width, &height,
               &bit_depth, &color_type, &interlace_type,
               &compression_type, &filter_method);

  // Strip > 8 bit depth files to 8 bit depth

  if (bit_depth > 8) 
    {
#ifndef VTK_WORDS_BIGENDIAN
    png_set_swap(png_ptr);
#endif
    png_set_strip_16(png_ptr);
    }

  // Convert palettes to RGB

  if (color_type == PNG_COLOR_TYPE_PALETTE)
    {
    png_set_palette_to_rgb(png_ptr);
    }

  // Expand < 8 bit depth files to 8 bit depth
  // Expand grayscale to RGB

  if (color_type == PNG_COLOR_TYPE_GRAY)
    {
    if (bit_depth < 8) 
      {
      png_set_gray_1_2_4_to_8(png_ptr);
      }
    png_set_gray_to_rgb(png_ptr);
    }

  // Expand tRNS chunks to alpha channels

  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) 
    {
    png_set_tRNS_to_alpha(png_ptr);
    }

  // Have libpng handle interlacing
  // int number_of_passes = png_set_interlace_handling(png_ptr);

  // Update the info now that we have defined the transformation filters

  png_read_update_info(png_ptr, info_ptr);

  // Read the image

  int rowbytes = png_get_rowbytes(png_ptr, info_ptr);
  *pixels = new unsigned char [rowbytes * height];
  png_bytep *row_pointers = new png_bytep [height];

  unsigned int ui;
  for (ui = 0; ui < height; ++ui)
    {
    row_pointers[ui] = *pixels + rowbytes * ui;
    }

  png_read_image(png_ptr, row_pointers);

  delete [] row_pointers;

  *widthp = (int)width;
  *heightp = (int)height;
  *pixel_size = (int)png_get_channels(png_ptr, info_ptr);

  // Close the file

  png_read_end(png_ptr, NULL);
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
  fclose(fp);

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWResourceUtilities::WritePNGImage(
  const char *filename,
  int width, int height, 
  int pixel_size,
  unsigned char *pixels)
{
  // Check parameters

  if (!filename || 
      width <= 0 || height <= 0 || 
      pixel_size < 1 || pixel_size > 4 || 
      !pixels)
    {
    vtkGenericWarningMacro("Unable to write PNG file, invalid parameters!");
    return 0;
    }

  // Open file for writing

  FILE *fp = fopen(filename, "wb");
  if (!fp)
    {
    vtkGenericWarningMacro("Unable to write PNG file " << filename);
    return 0;
    }

  // Create some PNG structs

  png_structp png_ptr = png_create_write_struct
    (PNG_LIBPNG_VER_STRING, (png_voidp)NULL, NULL, NULL);
  if (!png_ptr)
    {
    vtkGenericWarningMacro("Unable to write PNG file " << filename);
    fclose(fp);
    return 0;
    }
  
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
    {
    vtkGenericWarningMacro("Unable to write PNG file " << filename);
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    fclose(fp);
    return 0;
    }
  
  png_init_io(png_ptr, fp);

  // Set error handler

  if (setjmp(png_jmpbuf(png_ptr)))
    {
    vtkGenericWarningMacro("Unable to set error handler!");
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    fclose(fp);
    return 0;
    }

  // Write the header, describe the image parameters

  int color_type;
  switch (pixel_size)
    {
    case 1: 
      color_type = PNG_COLOR_TYPE_GRAY;
      break;
    case 2: 
      color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
      break;
    case 3: 
      color_type = PNG_COLOR_TYPE_RGB;
      break;
    default: 
      color_type = PNG_COLOR_TYPE_RGB_ALPHA;
      break;
    }
  
  png_set_IHDR(png_ptr, info_ptr, 
               (png_uint_32)width, (png_uint_32)height,
               8, 
               color_type, 
               PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT, 
               PNG_FILTER_TYPE_DEFAULT);

  png_write_info(png_ptr, info_ptr);

  // Write the pixels

  png_bytep *row_pointers = new png_bytep [height];
  unsigned long stride = width * pixel_size;

  int ui;
  for (ui = 0; ui < height; ui++)
    {
    row_pointers[ui] = (png_bytep)pixels;
    pixels += stride;
    }

  png_write_image(png_ptr, row_pointers);

  delete [] row_pointers;

  // Close the file

  png_write_end(png_ptr, info_ptr);
  png_destroy_write_struct(&png_ptr, &info_ptr);

  if (fp)
    {
    fflush(fp);
    fclose(fp);
    if (ferror(fp))
      {
      vtkGenericWarningMacro("Error while writing PNG file " << filename);
      return 0;
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWResourceUtilities::ConvertImageToHeader(
  const char *header_filename,
  const char **image_filenames,
  int nb_images,
  int options)
{
  // Check parameters

  if (!image_filenames || nb_images <= 0 || !header_filename)
    {
    vtkGenericWarningMacro("Unable to convert image, invalid parameters!");
    return 0;
    }

  // Options

  int opt_update = 
    options & vtkKWResourceUtilities::CONVERT_IMAGE_TO_HEADER_OPTION_UPDATE;
  int opt_zlib = 
    options & vtkKWResourceUtilities::CONVERT_IMAGE_TO_HEADER_OPTION_ZLIB;
  int opt_base64 = 
    options & vtkKWResourceUtilities::CONVERT_IMAGE_TO_HEADER_OPTION_BASE64;

  // Update only, bail out if the header is more recent than all the images

  if (opt_update && kwsys::SystemTools::FileExists(header_filename))
    {
    long int header_mod_time = 
      kwsys::SystemTools::ModifiedTime(header_filename);
    int up_to_date = 1;
    for (int img_idx = 0; img_idx < nb_images; img_idx++)
      {
      if (image_filenames[img_idx] && 
          (kwsys::SystemTools::ModifiedTime(image_filenames[img_idx]) >
           header_mod_time))
        {
        up_to_date = 0;
        break;
        }
      }
    if (up_to_date)
      {
      return 1;
      }
    }

  // Open header file

  ofstream out(header_filename, ios::out);
  if (out.fail())
    {
    vtkGenericWarningMacro("Unable to open header file " << header_filename);
    return 0;
    }

  // Loop over the images

  int all_ok = 1;

  for (int img_idx = 0; img_idx < nb_images; img_idx++)
    {
    const char *image_filename = image_filenames[img_idx];

    // Is the filename valid ?

    if (!image_filename)
      {
      continue;
      }

    // Image exists ?

    if (!kwsys::SystemTools::FileExists(image_filename))
      {
      vtkGenericWarningMacro("Unable to find image " << image_filename);
      all_ok = 0;
      continue;
      }

    // Read as PNG

    int width, height, pixel_size;
    unsigned char *image_buffer = NULL;

    if (!vtkKWResourceUtilities::ReadPNGImage(
          image_filename, &width, &height, &pixel_size, &image_buffer))
      {
      vtkGenericWarningMacro("Unable to read PNG image " << image_filename);
      all_ok = 0;
      continue;
      }

    unsigned long nb_of_pixels = width * height;
    unsigned long nb_of_bytes = nb_of_pixels * pixel_size;

    unsigned char *data_ptr = image_buffer;

    // Zlib the buffer

    unsigned char *zlib_buffer = NULL;
    if (opt_zlib)
      {
      unsigned long zlib_buffer_size = 
        (unsigned long)((float)nb_of_bytes * 1.2 + 12);
      zlib_buffer = new unsigned char [zlib_buffer_size];
      if (compress2(zlib_buffer, &zlib_buffer_size, 
                    data_ptr, nb_of_bytes, 
                    Z_BEST_COMPRESSION) != Z_OK)
        {
        vtkGenericWarningMacro("Unable to compress image buffer!");
        delete [] zlib_buffer;
        delete [] image_buffer;
        all_ok = 0;
        continue;
        }
      data_ptr = zlib_buffer;
      nb_of_bytes = zlib_buffer_size;
      }
  
    // Base64 the buffer

    unsigned char *base64_buffer = NULL;
    if (opt_base64)
      {
      base64_buffer = new unsigned char [nb_of_bytes * 2];
      nb_of_bytes = 
        kwsysBase64_Encode(data_ptr, nb_of_bytes, base64_buffer, 0);
      if (nb_of_bytes == 0)
        {
        vtkGenericWarningMacro("Unable to base64 image buffer!");
        delete [] zlib_buffer;
        delete [] base64_buffer;
        delete [] image_buffer;
        all_ok = 0;
        continue;
        }
      data_ptr = base64_buffer;
      }
    
    // Output the image in the header

    kwsys_stl::string image_basename = 
      kwsys::SystemTools::GetFilenameName(image_filename);
    kwsys_stl::string image_name = 
      kwsys::SystemTools::GetFilenameWithoutExtension(image_basename);
  
    out << "/* " << endl
        << " * Resource generated for image:" << endl
        << " *    " << image_basename.c_str();

    if (opt_base64 || opt_zlib)
      {
      out << " (" 
          << (opt_zlib ? "zlib" : "") 
          << (opt_zlib && opt_base64 ? ", " : "") 
          << (opt_base64 ? "base64" : "")
          << ")";
      }

    out << endl << " */" << endl;

    int section_idx = 0;
    unsigned long max_bytes = 65530;

    const char *pixel_byte_type = "const unsigned char";

    out 
      << "static const unsigned int  image_" << image_name.c_str() 
      << "_width         = " << width <<  ";" << endl
      << "static const unsigned int  image_" << image_name.c_str() 
      << "_height        = " 
      << height << ";" << endl
      << "static const unsigned int  image_" << image_name.c_str() 
      << "_pixel_size    = " 
      << pixel_size << ";" << endl
      << "static const unsigned long image_" << image_name.c_str() 
      << "_buffer_length = " 
      << nb_of_bytes << ";" << endl
      << endl
      << "static " << pixel_byte_type << " image_" << image_name.c_str();
    if (nb_of_bytes >= max_bytes)
      {
      out << "_section_" << ++section_idx;
      }
    out << "[] = " << endl
        << (opt_base64 ? "  \"" : "{\n  ");

    // Loop over pixels

    unsigned char *ptr = data_ptr;
    unsigned char *end = data_ptr + nb_of_bytes;

    int cc = 0;
    while (ptr < (end - 1))
      {
      if (cc % max_bytes == max_bytes - 1)
        {
        if (opt_base64)
          {
          out << *ptr << "\";" << endl;
          }
        else
          {
          out << (unsigned int)*ptr << endl << "};" << endl;
          }
        ++section_idx;
        out << endl
            << "static " << pixel_byte_type << " image_" << image_name.c_str()
            << "_section_" << section_idx << "[] = " << endl
            << (opt_base64 ? "  \"" : "{\n  ");
        }
      else
        {
        if (opt_base64)
          {
          out << *ptr;
          if (cc % 70 == 69)
            {
            out << "\"" << endl << "  \"";
            }
          }
        else
          {
          out << (unsigned int)*ptr << ", ";
          if (cc % 15 == 14)
            {
            out << endl << "  ";
            }
          }
        }
      cc++;
      ptr++;
      }

    if (opt_base64)
      {
      out << *ptr << "\";" << endl;
      }
    else
      {
      out << (unsigned int)*ptr << endl << "};" << endl;
      }

    if (section_idx)
      {
      out << endl 
          << "static " << pixel_byte_type << " *image_" << image_name.c_str()
          << "_sections[" << section_idx << "] = {" << endl;
      for (int i = 1; i <= section_idx; i++)
        {
        out << "  image_" << image_name.c_str() << "_section_" << i 
            << (i < section_idx ? "," : "") << endl;
        }
      out << "};" << endl
          << endl
          << "static const unsigned int image_" << image_name.c_str() 
          << "_nb_sections   = " << section_idx << ";" << endl;
      }

    out << endl;

    // Free mem

    delete [] base64_buffer;
    delete [] zlib_buffer;
    delete [] image_buffer;

    } // Next file

  // Close file, free objects

  out.close();

  return all_ok;
}

//----------------------------------------------------------------------------
void vtkKWResourceUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

