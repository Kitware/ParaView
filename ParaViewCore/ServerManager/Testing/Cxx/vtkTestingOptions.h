/*=========================================================================
  
  Program:   ParaView
  Module:    vtkTestingOptions.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Command line options for SM Testing.


#ifndef __vtkTestingOptions_h 
#define __vtkTestingOptions_h

#include "vtkPVOptions.h"

class VTK_EXPORT vtkTestingOptions : public vtkPVOptions
{
public:
  static vtkTestingOptions* New();
  vtkTypeMacro(vtkTestingOptions,vtkPVOptions);
  void PrintSelf(ostream& os, vtkIndent indent);

  // The name of the State XML to load.
  vtkGetStringMacro(SMStateXMLName);
  vtkGetStringMacro(DataDir);
  vtkGetStringMacro(TempDir);
  vtkGetStringMacro(BaselineImage);
  vtkGetMacro(Threshold, double);

protected:
  // Description:
  // Default constructor.
  vtkTestingOptions();

  // Description:
  // Destructor.
  virtual ~vtkTestingOptions();

  // Description:
  // Initialize arguments.
  virtual void Initialize();

  // Description:
  // After parsing, process extra option dependencies.
  virtual int PostProcess(int argc, const char* const* argv);

  // Description:
  // This method is called when wrong argument is found. If it returns 0, then
  // the parsing will fail.
  virtual int WrongArgument(const char* argument);

  // Options:
  vtkSetStringMacro(SMStateXMLName);
  vtkSetStringMacro(DataDir);
  vtkSetStringMacro(TempDir);
  vtkSetStringMacro(BaselineImage);
  
  char* SMStateXMLName;
  char* DataDir;
  char* TempDir;
  char* BaselineImage;
  double Threshold;
private:
  vtkTestingOptions(const vtkTestingOptions&); // Not implemented
  void operator=(const vtkTestingOptions&); // Not implemented
};

#endif

