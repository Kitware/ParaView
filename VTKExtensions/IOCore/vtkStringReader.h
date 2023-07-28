// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStringReader
 * @brief   Read a file and return a string.
 */
#ifndef vtkStringReader_h
#define vtkStringReader_h

#include "vtkAlgorithm.h"
#include "vtkPVVTKExtensionsIOCoreModule.h" //needed for exports

class VTKPVVTKEXTENSIONSIOCORE_EXPORT vtkStringReader : public vtkAlgorithm
{
public:
  static vtkStringReader* New();
  vtkTypeMacro(vtkStringReader, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the read string.
   */
  vtkGetCharFromStdStringMacro(String);
  ///@}

  ///@{
  /**
   * Specify the file name to read from.
   */
  vtkSetStdStringFromCharMacro(FileName);
  vtkGetCharFromStdStringMacro(FileName);
  ///@}
protected:
  vtkStringReader();
  ~vtkStringReader() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;
  vtkTypeBool ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

private:
  vtkStringReader(const vtkStringReader&) = delete;
  void operator=(const vtkStringReader&) = delete;

  std::string String;
  std::string FileName;
};

#endif // vtkStringReader_h
