/*=========================================================================

  Module:    KWConvertImageToHeader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBase64Utilities.h"
#include "vtkImageData.h"
#include "vtkImageFlip.h"
#include "vtkPNGReader.h"

#if ((VTK_MAJOR_VERSION <= 4) && (VTK_MINOR_VERSION <= 4))
#include "zlib.h" // needed for TIFF
#else
#include "vtk_zlib.h" // needed for TIFF
#endif

#include <sys/stat.h>

#ifdef VTK_USE_ANSI_STDLIB
#define VTK_IOS_NOCREATE 
#else
#define VTK_IOS_NOCREATE | ios::nocreate
#endif

//----------------------------------------------------------------------------
int file_exists(const char *filename)
{
  struct stat fs;
  return (stat(filename, &fs) != 0) ? 0 : 1;
}

//----------------------------------------------------------------------------
long int modified_time(const char *filename)
{
  struct stat fs;
  if (stat(filename, &fs) != 0) 
    {
    return 0;
    }
  else
    {
    return (long int)fs.st_mtime;
    }
}

//----------------------------------------------------------------------------
const char* name(const char *filename)
{
  const char *forward = strrchr(filename, '/');
  const char *backward = strrchr(filename, '\\');
  if (forward || backward)
    {
    return ((forward > backward) ? forward : backward) + 1;
    }
  return filename;
}

//----------------------------------------------------------------------------
int main(int argc, char **argv)
{
  // Usage

  if (argc < 3)
    {
    cerr << "Usage: " << argv[0] << " header.h image.png [image.png image.png...] [UPDATE] [ZLIB] [BASE64] [NOFLIP]" << endl;
    return 1;
    }

  // Get parameters:

  // UPDATE: do something only if one of the images is newer
  //         than the header file
  // ZLIB:   compress the resulting image buffer
  // BASE64: convert to base64

  int update     = 0;
  int zlib       = 0;
  int base64     = 0;
  int noflip     = 0;

  int has_params;
  do
    {
    has_params = 0;
    if (!strcmp(argv[argc - 1], "UPDATE"))
      {
      update = has_params = 1;
      }
    else if (!strcmp(argv[argc - 1], "ZLIB"))
      {
      zlib = has_params = 1;
      }
    else if (!strcmp(argv[argc - 1], "BASE64"))
      {
      base64 = has_params = 1;
      }
    else if (!strcmp(argv[argc - 1], "NOFLIP"))
      {
      noflip = has_params = 1;
      }
    if (has_params)
      {
      argc--;
      }
    } while (has_params);

  // Update mode ?

  if (update)
    {
    if (file_exists(argv[1]))
      {
      long int header_modified_time = modified_time(argv[1]);
      int i = 2;
      while (i < argc && modified_time(argv[i]) <= header_modified_time)
        {
        i++;
        }
      if (i == argc)
        {
        cout << name(argv[1]) << " is up-to-date" << endl;
        return 0;
        }
      }
    }

  // Open header file

  ofstream out(argv[1], ios::out);
  if (out.fail())
    {
    cerr << "Cannot open: " << argv[2] << " for writing" << endl;
    return 3;
    }

  cout << "Creating " << name(argv[1]) << endl;

  // Loop over each image

  int i;
  for (i = 2; i < argc; i++)
    {
    // Check if image exists

    if (!file_exists(argv[i]))
      {
      cerr << "Cannot open: " << argv[2] << " for reading" << endl;
      continue;
      }
  
    // Read as PNG

    char image_name[1024];
    strcpy(image_name, name(argv[i]));

    cout << "  - from: " << image_name << endl;

    vtkPNGReader *pr = vtkPNGReader::New();
    pr->SetFileName(argv[i]);
    pr->Update();

    vtkImageData *output = pr->GetOutput();

    // Flip image (in VTK, [0,0] is lower left)

    vtkImageFlip *flip = vtkImageFlip::New();
    if (!noflip)
      {
      flip->SetInput(pr->GetOutput());
      flip->SetFilteredAxis(1);
      flip->Update();
      output = flip->GetOutput();
      }

    unsigned char *data_ptr = (unsigned char *)(output->GetScalarPointer());

    // Image info

    int *dim = output->GetDimensions();
    int width = dim[0];
    int height = dim[1];
    int pixel_size = output->GetNumberOfScalarComponents();
    unsigned long nb_of_pixels = width * height;
    unsigned long nb_of_bytes = nb_of_pixels * pixel_size;

    // Zlib

    unsigned char *zlib_buffer = 0;
    if (zlib)
      {
      unsigned long zlib_buffer_size = 
        (unsigned long)((float)nb_of_bytes * 1.2 + 12);
      zlib_buffer = new unsigned char [zlib_buffer_size];
      if (compress2(zlib_buffer, &zlib_buffer_size, 
                    data_ptr, nb_of_bytes, 
                    Z_BEST_COMPRESSION) != Z_OK)
        {
        cerr << "Error: zlib compress2 failed." << endl;
        delete [] zlib_buffer;
        pr->Delete();
        flip->Delete();
        continue;
        }
      data_ptr = zlib_buffer;
      nb_of_bytes = zlib_buffer_size;
      }
  
    // Base64

    unsigned char *base64_buffer = 0;
    if (base64)
      {
      base64_buffer = new unsigned char [nb_of_bytes * 2];
      nb_of_bytes = 
        vtkBase64Utilities::Encode(data_ptr, nb_of_bytes, base64_buffer);
      if (nb_of_bytes == 0)
        {
        cerr << "Error: base64 coding failed." << endl;
        if (zlib)
          {
          delete [] zlib_buffer;
          }
        delete [] base64_buffer;
        pr->Delete();
        flip->Delete();
        continue;
        }
      data_ptr = base64_buffer;
      }
    
    // Output image info

    out << "/* " << endl
        << " * This part was generated by KWConvertImageToHeader from image:" << endl
        << " *    " << image_name;

    if (base64 || zlib)
      {
      out << " (" 
          << (zlib ? "zlib" : "") 
          << (zlib && (base64 || noflip) ? ", " : "") 
          << (base64 ? "base64" : "") 
          << ((zlib || base64) && noflip ? ", " : "") 
          << (noflip ? "noflip" : "") 
          << ")";
      }

    out << endl << " */" << endl;

    image_name[strlen(image_name) - 4] = 0;
  
    out 
      << "#define image_" << image_name << "_width         " << width << endl
      << "#define image_" << image_name << "_height        " << height << endl
      << "#define image_" << image_name << "_pixel_size    " << pixel_size << endl
      << "#define image_" << image_name << "_buffer_length " << nb_of_bytes << endl
      << endl
      << "static unsigned char image_" << image_name << "[] = " << endl
      << (base64 ? "  \"" : "{\n  ");

    // Loop over pixels

    unsigned char *ptr = data_ptr;
    unsigned char *end = data_ptr + nb_of_bytes;

    int cc = 0;
    while (ptr < (end - 1))
      {
      if (base64)
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
      cc++;
      ptr++;
      }

    if (base64)
      {
      out << *ptr << "\";";
      }
    else
      {
      out << (unsigned int)*ptr << endl << "};";
      }

    out << endl << endl;

    // Free mem

    if (base64)
      {
      delete [] base64_buffer;
      }

    if (zlib)
      {
      delete [] zlib_buffer;
      }
    
    pr->Delete();
    flip->Delete();

    } // Next file

  // Close file, free objects

  out.close();

  return 0;
}
