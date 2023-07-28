// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStringWriter
 * @brief   Given a string, write it to a file.
 */
#ifndef vtkStringWriter_h
#define vtkStringWriter_h

#include "vtkAlgorithm.h"
#include "vtkPVVTKExtensionsIOCoreModule.h" // needed for exports

class VTKPVVTKEXTENSIONSIOCORE_EXPORT vtkStringWriter : public vtkAlgorithm
{
public:
  static vtkStringWriter* New();
  vtkTypeMacro(vtkStringWriter, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the string to write.
   */
  vtkSetStdStringFromCharMacro(String);
  vtkGetCharFromStdStringMacro(String);
  ///@}

  ///@{
  /**
   * Specify the file name to write to.
   */
  vtkSetStdStringFromCharMacro(FileName);
  vtkGetCharFromStdStringMacro(FileName);
  ///@}

  /**
   * Write the string to the file.
   */
  int Write();

protected:
  vtkStringWriter();
  ~vtkStringWriter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;
  vtkTypeBool ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

private:
  vtkStringWriter(const vtkStringWriter&) = delete;
  void operator=(const vtkStringWriter&) = delete;

  std::string String;
  std::string FileName;
};

#endif // vtkStringWriter_h
