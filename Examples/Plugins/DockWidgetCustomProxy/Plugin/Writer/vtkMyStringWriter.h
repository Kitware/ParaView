/*=========================================================================

  Program:   ParaView
  Module:    vtkMyStringWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
