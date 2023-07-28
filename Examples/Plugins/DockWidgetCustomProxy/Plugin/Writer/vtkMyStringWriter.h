// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME vtkMyStringWriter
// .SECTION Description
// A simple vtk class that write a text to a file

#ifndef vtkMyStringWriter_h
#define vtkMyStringWriter_h

#include "WriterModule.h" // for export macro

#include <vtkObject.h>

class WRITER_EXPORT vtkMyStringWriter : public vtkObject
{
public:
  static vtkMyStringWriter* New();
  vtkTypeMacro(vtkMyStringWriter, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetMacro(FileName, std::string);
  vtkGetMacro(FileName, std::string);

  vtkSetMacro(Text, std::string);
  vtkGetMacro(Text, std::string);

  bool Write();

protected:
  vtkMyStringWriter() = default;
  ~vtkMyStringWriter() override = default;

private:
  vtkMyStringWriter(const vtkMyStringWriter&) = delete;
  void operator=(const vtkMyStringWriter&) = delete;

  std::string FileName;
  std::string Text;
};

#endif
