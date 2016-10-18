/*=========================================================================

  Program:   ParaView
  Module:    CompareImages.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageData.h"
#include "vtkPNGReader.h"
#include "vtkSmartPointer.h"
#include "vtkTesting.h"
#include <vtksys/SystemTools.hxx>

// Checks to see if the file exists and that it is a png file.
// Returns 1 for a valid file and 0 for an invalid file.
int IsFileValid(const char* fileName)
{
  if (vtksys::SystemTools::FileExists(fileName) == 0)
  {
    vtkGenericWarningMacro("Could not find file " << fileName);
    return 0;
  }
  if (vtksys::SystemTools::GetFilenameLastExtension(fileName) != ".png")
  {
    vtkGenericWarningMacro("Wrong file type " << fileName);
    return 0;
  }
  return 1;
}

int main(int argc, char* argv[])
{
  if (argc != 7)
  {
    vtkGenericWarningMacro("Must specify files to compare.");
    vtkGenericWarningMacro("<exe> TestImage.png <Threshold> -V BaselineImage.png -T TempDirectory");
    return 1;
  }
  const char* baselineImage = argv[4];
  const char* testImage = argv[1];
  if (IsFileValid(baselineImage) == 0 || IsFileValid(testImage) == 0)
  {
    return 1;
  }

  vtkSmartPointer<vtkPNGReader> reader = vtkSmartPointer<vtkPNGReader>::New();
  reader->SetFileName(testImage);
  reader->Update();

  vtkSmartPointer<vtkTesting> testing = vtkSmartPointer<vtkTesting>::New();
  for (int cc = 1; cc < argc; cc++)
  {
    testing->AddArgument(argv[cc]);
  }

  double threshold = atof(argv[2]);

  if (!testing->RegressionTest(reader, threshold))
  {
    return 1;
  }

  return 0;
}
