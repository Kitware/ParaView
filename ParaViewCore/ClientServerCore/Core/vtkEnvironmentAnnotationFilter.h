/*=========================================================================

  Program:   ParaView
  Module:    vtkEnvironmentAnnotationFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkEnvironmentAnnotationFilter - filter used to generate text annotation
// for the current project.
// .SECTION Description
// vtkEnvironmentAnnotationFilter is designed to help annotate the scene with 
// frequently needed information. 
//
// The variables available in the expression evaluation scope are as follows:
// \li FileName: the name of the file that the user is working on.
// \li DisplayPath: Boolean value representing whether the file path is visible.
// \li DisplayFileName: Boolean value representing whether the file name is visible.
// \li DisplayDate: Boolean value representing whether thedate/time is visible.
// \li DisplaySystemName: Boolean value representing whether the system type is visible.
// \li DisplayUserName: Boolean value representing whether the username is visible.


#ifndef __vtkEnvironmentAnnotationFilter_h
#define __vtkEnvironmentAnnotationFilter_h

#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
#include "vtkTableAlgorithm.h"

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkEnvironmentAnnotationFilter : public vtkTableAlgorithm
{
public:
  static vtkEnvironmentAnnotationFilter* New();
  vtkTypeMacro(vtkEnvironmentAnnotationFilter, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  vtkSetStringMacro(PathName);
  vtkGetStringMacro(PathName);

  vtkSetMacro(DisplayUserName, bool);
  vtkGetMacro(DisplayUserName, bool);

  vtkSetMacro(DisplaySystemName, bool);
  vtkGetMacro(DisplaySystemName, bool);

  vtkSetMacro(DisplayFileName, bool);
  vtkGetMacro(DisplayFileName, bool);

  vtkSetMacro(DisplayDate, bool);
  vtkGetMacro(DisplayDate, bool);

  vtkSetMacro(DisplayPath, bool);
  vtkGetMacro(DisplayPath, bool);

  //------------------------------------------------------------------------------
  // Description:
  // Get methods for use in annotation.py.
  // The values are only valid during RequestData().

  void UpdateAnnotationValue();

//BTX
protected:
  vtkEnvironmentAnnotationFilter();
  ~vtkEnvironmentAnnotationFilter();

  virtual int FillInputPortInformation(int port, vtkInformation* info);
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  char* AnnotationValue;
  char* FileName;
  char* PathName;

private:
  vtkEnvironmentAnnotationFilter(const vtkEnvironmentAnnotationFilter&); // Not implemented
  void operator=(const vtkEnvironmentAnnotationFilter&); // Not implemented

  bool DisplayUserName;
  bool DisplaySystemName;
  bool DisplayFileName;
  bool DisplayDate;
  bool DisplayPath;

//ETX
};

#endif
