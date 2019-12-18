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
/**
 * @class   vtkEnvironmentAnnotationFilter
 * @brief   filter used to generate text annotation
 * for the current project.
 *
 * vtkEnvironmentAnnotationFilter is designed to help annotate the scene with
 * frequently needed information.
 *
 * The variables available in the expression evaluation scope are as follows:
 * \li FileName: the name of the file that the user is working on.
 * \li DisplayFileName: Boolean value representing whether the file name is visible.
 * \li DisplayDate: Boolean value representing whether thedate/time is visible.
 * \li DisplaySystemName: Boolean value representing whether the system type is visible.
 * \li DisplayUserName: Boolean value representing whether the username is visible.
*/

#ifndef vtkEnvironmentAnnotationFilter_h
#define vtkEnvironmentAnnotationFilter_h

#include "vtkPVVTKExtensionsMiscModule.h" //needed for exports
#include "vtkTableAlgorithm.h"
#include <string> //needed for iVars

class VTKPVVTKEXTENSIONSMISC_EXPORT vtkEnvironmentAnnotationFilter : public vtkTableAlgorithm
{
public:
  static vtkEnvironmentAnnotationFilter* New();
  vtkTypeMacro(vtkEnvironmentAnnotationFilter, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetMacro(FileName, std::string);
  vtkGetMacro(FileName, std::string);

  vtkSetMacro(DisplayUserName, bool);
  vtkGetMacro(DisplayUserName, bool);

  vtkSetMacro(DisplaySystemName, bool);
  vtkGetMacro(DisplaySystemName, bool);

  vtkSetMacro(DisplayFileName, bool);
  vtkGetMacro(DisplayFileName, bool);

  vtkSetMacro(DisplayFilePath, bool);
  vtkGetMacro(DisplayFilePath, bool);

  vtkSetMacro(DisplayDate, bool);
  vtkGetMacro(DisplayDate, bool);

protected:
  vtkEnvironmentAnnotationFilter();
  ~vtkEnvironmentAnnotationFilter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  std::string AnnotationValue;
  std::string FileName;

private:
  vtkEnvironmentAnnotationFilter(const vtkEnvironmentAnnotationFilter&) = delete;
  void operator=(const vtkEnvironmentAnnotationFilter&) = delete;

  void UpdateAnnotationValue();

  bool DisplayUserName;
  bool DisplaySystemName;
  bool DisplayFileName;
  bool DisplayFilePath;
  bool DisplayDate;
};

#endif
