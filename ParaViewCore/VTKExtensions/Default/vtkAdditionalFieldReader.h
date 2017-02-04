/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAdditionalFieldReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAdditionalFieldReader
 * @brief   read field data arrays and add to an input
 *
 *
*/

#ifndef vtkAdditionalFieldReader_h
#define vtkAdditionalFieldReader_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkPassInputTypeAlgorithm.h"

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkAdditionalFieldReader : public vtkPassInputTypeAlgorithm
{
public:
  vtkTypeMacro(vtkAdditionalFieldReader, vtkPassInputTypeAlgorithm);
  static vtkAdditionalFieldReader* New();
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * The file to open to retrieve field data arrays
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

protected:
  vtkAdditionalFieldReader();
  ~vtkAdditionalFieldReader();

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  /**
   * The name of the file to be opened.
   */
  char* FileName;

private:
  vtkAdditionalFieldReader(const vtkAdditionalFieldReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkAdditionalFieldReader&) VTK_DELETE_FUNCTION;
};
#endif
