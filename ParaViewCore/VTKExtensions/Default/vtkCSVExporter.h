/*=========================================================================

  Program:   ParaView
  Module:    vtkCSVExporter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCSVExporter - exporter used to save vtkFieldData as CSV.
// .SECTION Description
// This is used by vtkSMCSVExporterProxy to export the data shown in the
// spreadsheet view as a CSV.

#ifndef __vtkCSVExporter_h
#define __vtkCSVExporter_h

#include "vtkObject.h"
class vtkFieldData;

class VTK_EXPORT vtkCSVExporter : public vtkObject
{
public:
  static vtkCSVExporter* New();
  vtkTypeMacro(vtkCSVExporter, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the filename for the file.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Get/Set the delimiter use to separate fields ("," by default.)
  vtkSetStringMacro(FieldDelimiter);
  vtkGetStringMacro(FieldDelimiter);

  bool Open();
  void WriteHeader(vtkFieldData*);
  void WriteData(vtkFieldData*);
  void Close();

//BTX
protected:
  vtkCSVExporter();
  ~vtkCSVExporter();

  char* FileName;
  char* FieldDelimiter;

  ofstream *FileStream;
private:
  vtkCSVExporter(const vtkCSVExporter&); // Not implemented
  void operator=(const vtkCSVExporter&); // Not implemented
//ETX
};

#endif

