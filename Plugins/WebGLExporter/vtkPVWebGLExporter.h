/*=========================================================================

  Program:   ParaView Web
  Module:    vtkPVWebGLExporter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkPVWebGLExporter_h
#define __vtkPVWebGLExporter_h

#include "vtkExporter.h"

class vtkPVWebGLExporter : public vtkExporter
{
public:
  static vtkPVWebGLExporter *New();
  vtkTypeMacro(vtkPVWebGLExporter,vtkExporter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the name of the VRML file to write.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkPVWebGLExporter();
  ~vtkPVWebGLExporter();

  void WriteData();

  char *FileName;
private:
  vtkPVWebGLExporter(const vtkPVWebGLExporter&);  // Not implemented.
  void operator=(const vtkPVWebGLExporter&);  // Not implemented.
};

#endif

