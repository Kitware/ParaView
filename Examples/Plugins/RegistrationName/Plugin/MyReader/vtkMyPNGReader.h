// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
// .NAME vtkMyPNGReader - read a png file

#ifndef vtkMyPNGReader_h
#define vtkMyPNGReader_h

#include <vtkPNGReader.h>

#include "MyReaderModule.h" // for export macro

class MYREADER_EXPORT vtkMyPNGReader : public vtkPNGReader
{
public:
  static vtkMyPNGReader* New();
  vtkTypeMacro(vtkMyPNGReader, vtkPNGReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  const char* GetRegistrationName();

protected:
  vtkMyPNGReader();
  ~vtkMyPNGReader();

private:
  vtkMyPNGReader(const vtkMyPNGReader&) = delete;
  void operator=(const vtkMyPNGReader&) = delete;
};

#endif
