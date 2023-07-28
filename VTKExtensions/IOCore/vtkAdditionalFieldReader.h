// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAdditionalFieldReader
 * @brief   read field data arrays and add to an input
 *
 *
 */

#ifndef vtkAdditionalFieldReader_h
#define vtkAdditionalFieldReader_h

#include "vtkPVVTKExtensionsIOCoreModule.h" //needed for exports
#include "vtkPassInputTypeAlgorithm.h"

class VTKPVVTKEXTENSIONSIOCORE_EXPORT vtkAdditionalFieldReader : public vtkPassInputTypeAlgorithm
{
public:
  vtkTypeMacro(vtkAdditionalFieldReader, vtkPassInputTypeAlgorithm);
  static vtkAdditionalFieldReader* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * The file to open to retrieve field data arrays
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  ///@}

protected:
  vtkAdditionalFieldReader();
  ~vtkAdditionalFieldReader() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * The name of the file to be opened.
   */
  char* FileName;

private:
  vtkAdditionalFieldReader(const vtkAdditionalFieldReader&) = delete;
  void operator=(const vtkAdditionalFieldReader&) = delete;
};
#endif
