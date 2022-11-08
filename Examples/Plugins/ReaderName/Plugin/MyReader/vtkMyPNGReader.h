/*=========================================================================

  Program:   ParaView
  Module:    vtkMyPNGReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMyPNGReader - read a png file

#ifndef vtkMyPNGReader_h
#define vtkMyPNGReader_h

#include <vtkPNGReader.h>

#include "PNGReaderModule.h" // for export macro

class PNGREADER_EXPORT vtkMyPNGReader : public vtkPNGReader
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
