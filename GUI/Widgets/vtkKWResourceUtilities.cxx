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

#include <vtksys/SystemTools.hxx>
#include <vtksys/Base64.h>

#ifdef _MSC_VER
// Let us get rid of this funny warning on /W4:
// warning C4611: interaction between '_setjmp' and C++ object 
// destruction is non-portable
#pragma warning( disable : 4611 )
#endif 

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWResourceUtilities);
vtkCxxRevisionMacro(vtkKWResourceUtilities, "1.14.2.1");

//----------------------------------------------------------------------------
int vtkKWResourceUtilities::ReadImage(
  const char *filename,
  int *widthp, int *heightp, 
  int *pixel_size,
  unsigned char **pixels)
{
  if (!filename || !vtksys::SystemTools::FileExists(filename))
    {
    return 0;
    }

  vtksys_stl::string ext = vtksys::SystemTools::LowerCase(
    vtksys::SystemTools::GetFilenameExtension(filename));

  if (!strcmp(ext.c_str(), ".png"))
    {
    return vtkKWResourceUtilities::ReadPNGImage(
      filename, widthp, heightp, pixel_size, pixels);
    }

  return 0;
}

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
  const char **filenames,
  int nb_files,
  int options)
{
  // Check parameters

  if (!filenames || nb_files <= 0 || !header_filename)
    {
    vtkGenericWarningMacro("Unable to convert file, invalid parameters!");
    return 0;
    }

  // Options

  int opt_update = 
    options & vtkKWResourceUtilities::ConvertImageToHeaderOptionUpdate;
  int opt_zlib = 
    options & vtkKWResourceUtilities::ConvertImageToHeaderOptionZlib;
  int opt_base64 = 
    options & vtkKWResourceUtilities::ConvertImageToHeaderOptionBase64;

  // Update only, bail out if the header is more recent than all the files

  if (opt_update && vtksys::SystemTools::FileExists(header_filename))
    {
    long int header_mod_time = 
      vtksys::SystemTools::ModifiedTime(header_filename);
    int up_to_date = 1;
    for (int img_idx = 0; img_idx < nb_files; img_idx++)
      {
      if (filenames[img_idx] && 
          (vtksys::SystemTools::ModifiedTime(filenames[img_idx]) >
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

  // Loop over the files

  int all_ok = 1;

  for (int img_idx = 0; img_idx < nb_files; img_idx++)
    {
    const char *filename = filenames[img_idx];

    // Is the filename valid ?

    if (!filename)
      {
      continue;
      }

    // File exists ?

    if (!vtksys::SystemTools::FileExists(filename))
      {
      vtkGenericWarningMacro("Unable to find file " << filename);
      all_ok = 0;
      continue;
      }

    // Read file
    // If failed, read it as a buffer

    int width = 0;
    int height = 0;
    int pixel_size = 0;

    unsigned long buffer_length = 0;
    unsigned char *buffer = NULL;

    int file_is_image = 0;
    vtksys::SystemTools::FileTypeEnum file_type = 
      vtksys::SystemTools::FileTypeUnknown;

    if (vtkKWResourceUtilities::ReadImage(
          filename, &width, &height, &pixel_size, &buffer))
      {
      buffer_length = width * height * pixel_size;
      file_is_image = 1;
      file_type = vtksys::SystemTools::FileTypeBinary;
      }
    else
      {
      FILE *filep = NULL;
      file_type = vtksys::SystemTools::DetectFileType(filename);
      if (file_type == vtksys::SystemTools::FileTypeText)
        {
        filep = fopen(filename, "rt");
        }
      else
        {
        filep = fopen(filename, "rb");
        file_type = vtksys::SystemTools::FileTypeBinary;
        }
      
      int success = 1;
      if (!filep)
        {
        vtkGenericWarningMacro("Unable to open file " << filename);
        success = 0;
        }
      else
        {
        unsigned long file_length = vtksys::SystemTools::FileLength(filename);
        buffer = new unsigned char [file_length];
        buffer_length = fread(buffer, 1, file_length, filep);
        if (ferror(filep))
          {
          if (feof(filep))
            {
            vtkGenericWarningMacro("Unable to read file " << filename
                                   << ", end of file reached");
            }
          else
            {
            vtkGenericWarningMacro("Unable to read file " << filename);
            }
          success = 0;
          }
        if (fclose(filep))
          {
          vtkGenericWarningMacro("Unable to close file " << filename);
          success = 0;
          }
        }
      if (!success)
        {
        delete [] buffer;
        all_ok = 0;
        continue;
        }
      }

    unsigned long buffer_decoded_length = buffer_length;

    unsigned char *data_ptr = buffer;

    // Zlib the buffer

    unsigned char *zlib_buffer = NULL;
    if (opt_zlib)
      {
      unsigned long zlib_buffer_size = 
        (unsigned long)((float)buffer_length * 1.2 + 12);
      zlib_buffer = new unsigned char [zlib_buffer_size];
      if (compress2(zlib_buffer, &zlib_buffer_size, 
                    data_ptr, buffer_length, 
                    Z_BEST_COMPRESSION) != Z_OK)
        {
        vtkGenericWarningMacro("Unable to compress buffer!");
        delete [] zlib_buffer;
        delete [] buffer;
        all_ok = 0;
        continue;
        }
      data_ptr = zlib_buffer;
      buffer_length = zlib_buffer_size;
      }
  
    // Base64 the buffer

    unsigned char *base64_buffer = NULL;
    if (opt_base64)
      {
      base64_buffer = new unsigned char [buffer_length * 2];
      buffer_length = 
        vtksysBase64_Encode(data_ptr, buffer_length, base64_buffer, 0);
      if (buffer_length == 0)
        {
        vtkGenericWarningMacro("Unable to base64 buffer!");
        delete [] zlib_buffer;
        delete [] base64_buffer;
        delete [] buffer;
        all_ok = 0;
        continue;
        }
      data_ptr = base64_buffer;
      }
    
    // Output the file in the header

    vtksys_stl::string filename_base = 
      vtksys::SystemTools::GetFilenameName(filename);
    vtksys_stl::string prefix;
    
    if (file_is_image)
      {
      prefix = "image_";
      prefix += 
        vtksys::SystemTools::GetFilenameWithoutExtension(filename_base);
      }
    else
      {
      prefix = "file_";
      vtksys_stl::string filename_base_clean(filename_base);
      vtksys::SystemTools::ReplaceString(filename_base_clean, ".", "_");
      prefix += filename_base_clean;
      }
    
    out << "/* " << endl
        << " * Resource generated for file:" << endl
        << " *    " << filename_base.c_str();

    if (opt_base64 || opt_zlib)
      {
      out << " (" 
          << (opt_zlib ? "zlib" : "") 
          << (opt_zlib && opt_base64 ? ", " : "") 
          << (opt_base64 ? "base64" : "")
          << ")";
      }

    if (file_is_image)
      {
      out << " (image file)";
      }
    else
      {
      if (file_type == vtksys::SystemTools::FileTypeText)
        {
        out << " (text file)";
        }
      else
        {
        out << " (binary file)";
        }
      }

    out << endl << " */" << endl;

    int section_idx = 0;
    unsigned long max_bytes = 32760; // 65530; was too high for AIX

    const char *pixel_byte_type = "const unsigned char";
    
    if (width)
      {
      out << "static const unsigned int  " << prefix.c_str() 
          << "_width          = " << width <<  ";" << endl;
      }

    if (height)
      {
      out << "static const unsigned int  " << prefix.c_str() 
          << "_height         = " << height << ";" << endl;
      }

    if (pixel_size)
      {
      out << "static const unsigned int  " << prefix.c_str() 
          << "_pixel_size     = " << pixel_size << ";" << endl;
      }

    if (buffer_length)
      {
      out << "static const unsigned long " << prefix.c_str() 
          << "_length         = " << buffer_length << ";" << endl;
      }

    if (opt_zlib || opt_base64)
      {
      out << "static const unsigned long " << prefix.c_str() 
          << "_decoded_length = " 
          << buffer_decoded_length << ";" << endl;
      }
    
    out << endl;

    out << "static " << pixel_byte_type << " " << prefix.c_str();
    if (buffer_length >= max_bytes)
      {
      out << "_section_" << ++section_idx;
      }
    out << "[] = " << endl
        << (opt_base64 ? "  \"" : "{\n  ");

    // Loop over pixels

    unsigned char *ptr = data_ptr;
    unsigned char *end = data_ptr + buffer_length;

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
            << "static " << pixel_byte_type << " " << prefix.c_str()
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
          << "static " << pixel_byte_type << " *" << prefix.c_str()
          << "_sections[" << section_idx << "] = {" << endl;
      for (int i = 1; i <= section_idx; i++)
        {
        out << "  " << prefix.c_str() << "_section_" << i 
            << (i < section_idx ? "," : "") << endl;
        }
      out << "};" << endl
          << endl
          << "static const unsigned int " << prefix.c_str() 
          << "_nb_sections = " << section_idx << ";" << endl;
      }

    out << endl;

    // Free mem

    delete [] base64_buffer;
    delete [] zlib_buffer;
    delete [] buffer;

    } // Next file

  // Close file, free objects

  out.close();

  return all_ok;
}

//----------------------------------------------------------------------------
int vtkKWResourceUtilities::DecodeBuffer(
  const unsigned char *input, unsigned long input_length, 
  unsigned char **output, unsigned long output_expected_length)
{
  *output = NULL;
  if (!input || input_length == 0 || !output || output_expected_length == 0)
    {
    return 0;
    }

  unsigned char *base64_buffer = NULL;
  unsigned char *zlib_buffer = NULL;

  // Is it a base64 stream (i.e. not zlib for the moment) ?

  if (input[0] != 0x78 || input[1] != 0xDA)
    {
    base64_buffer = new unsigned char [output_expected_length + 1];
    input_length = vtksysBase64_Decode(input, 0, base64_buffer, input_length);
    if (input_length == 0)
      {
      vtkGenericWarningMacro(<< "Error decoding base64 stream");
      delete [] base64_buffer;
      return 0;
      }
    input = base64_buffer;
    }
    
  // Is it zlib ?

  if (input_length != output_expected_length &&
      input[0] == 0x78 && input[1] == 0xDA)
    {
    zlib_buffer = new unsigned char [output_expected_length + 1];
    unsigned long zlib_buffer_length = output_expected_length; // IMPORTANT
    int z_status = uncompress(zlib_buffer, &zlib_buffer_length, 
                              input, input_length);
    switch (z_status)
      {
      case Z_MEM_ERROR:
        vtkGenericWarningMacro(
          << "Error decoding zlib stream: not enough memory");
        break;
      case Z_BUF_ERROR:
        vtkGenericWarningMacro(
          << "Error decoding zlib stream: not enough room in output buffer");
        break;
      case Z_DATA_ERROR:
        vtkGenericWarningMacro(
          << "Error decoding zlib stream: input data was corrupted");
        break;
      }
    if (z_status == Z_OK && zlib_buffer_length != output_expected_length)
      {
      vtkGenericWarningMacro(
        << "Error decoding zlib stream: uncompressed buffer size (" 
        << zlib_buffer_length << ") different than expected length ("
        << output_expected_length << ")");
      }
    if (base64_buffer)
      {
      delete [] base64_buffer;
      base64_buffer = NULL;
      }
    if (z_status != Z_OK || zlib_buffer_length != output_expected_length)
      {
      delete [] zlib_buffer;
      return 0;
      }
    *output = zlib_buffer;
    return 1;
    }

  if (base64_buffer)
    {
    *output = base64_buffer;
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkKWResourceUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

