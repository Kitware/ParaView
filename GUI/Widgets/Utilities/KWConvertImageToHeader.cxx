/*=========================================================================

  Module:    KWConvertImageToHeader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWResourceUtilities.h"
#include "vtkOutputWindow.h"

#include <kwsys/CommandLineArguments.hxx>
#include <kwsys/SystemTools.hxx>

//----------------------------------------------------------------------------
int unknown_argument_handler(const char *, void *) { return 0; };

//----------------------------------------------------------------------------
void display_usage(kwsys::CommandLineArguments &args)
{
  kwsys_stl::string exe_basename = 
    kwsys::SystemTools::GetFilenameName(args.GetArgv0());
  kwsys_ios::cerr << "Usage: " << exe_basename << " [--update] [--zlib] [--base64] header.h image.png [image.png image.png...]" << kwsys_ios::endl;
  kwsys_ios::cerr << args.GetHelp();
}

//----------------------------------------------------------------------------
int main(int argc, char **argv)
{
  vtkOutputWindow::GetInstance()->PromptUserOn();

  kwsys::CommandLineArguments args;

  int option_update = 0;
  int option_zlib = 0;
  int option_base64 = 0;

  args.Initialize(argc, argv);
  args.SetUnknownArgumentCallback(unknown_argument_handler);

  args.AddArgument(
    "--update", kwsys::CommandLineArguments::NO_ARGUMENT, 
    &option_update, 
    "Update header only if one of the image is more recent than the header.");

  args.AddArgument(
    "--zlib", kwsys::CommandLineArguments::NO_ARGUMENT, 
    &option_zlib, 
    "Compress the image buffer using zlib.");

  args.AddArgument(
    "--base64", kwsys::CommandLineArguments::NO_ARGUMENT, 
    &option_base64, 
    "Convert the image buffer to base64.");

  args.Parse();

  // Process the remaining args (note that rem_argv[0] is still the prog name)

  int res = 1;

  int rem_argc;
  char **rem_argv;
  args.GetRemainingArguments(&rem_argc, &rem_argv);

  if (rem_argc < 3)
    {
    kwsys_ios::cerr << "Invalid or missing arguments" << kwsys_ios::endl;
    display_usage(args);
    res = 0;
    }
  else
    {
    option_update *= 
      vtkKWResourceUtilities::CONVERT_IMAGE_TO_HEADER_OPTION_UPDATE;
    option_zlib *= 
      vtkKWResourceUtilities::CONVERT_IMAGE_TO_HEADER_OPTION_ZLIB;
    option_base64 *=
      vtkKWResourceUtilities::CONVERT_IMAGE_TO_HEADER_OPTION_BASE64;
   
    kwsys_ios::cout << "- " << rem_argv[1] << endl;

    vtkKWResourceUtilities::ConvertImageToHeader(
      rem_argv[1], (const char **)&rem_argv[2], rem_argc - 2, 
      option_update | option_zlib | option_base64);
    }

  delete [] rem_argv;
  return res;
}
