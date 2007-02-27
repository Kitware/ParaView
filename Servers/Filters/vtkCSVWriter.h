/*=========================================================================

  Program:   ParaView
  Module:    vtkCSVWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCSVWriter - writer for 1D vtkRectilinearGrid.
// Writes the data as a delimited text file (such as CSV). The vtkRectilinearGrid must
// be 1 dimensional.
#ifndef __vtkCSVWriter_h
#define __vtkCSVWriter_h

#include "vtkWriter.h"

class vtkStdString;

class VTK_EXPORT vtkCSVWriter : public vtkWriter
{
public:
  static vtkCSVWriter* New();
  vtkTypeRevisionMacro(vtkCSVWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the delimiter use to separate fields ("," by default.)
  vtkSetStringMacro(FieldDelimiter);
  vtkGetStringMacro(FieldDelimiter);

  // Description:
  // Get/Set the delimiter used for string data, if any 
  // eg. double quotes(").
  vtkSetStringMacro(StringDelimiter);
  vtkGetStringMacro(StringDelimiter);

  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Get/Set if StringDelimiter must be used for string data.
  // True by default.
  vtkSetMacro(UseStringDelimiter, bool);
  vtkGetMacro(UseStringDelimiter, bool);
//BTX
  vtkStdString GetString(vtkStdString string);
protected:
  vtkCSVWriter();
  ~vtkCSVWriter();

  bool OpenFile();

  virtual void WriteData();

  // see algorithm for more info
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  char* FileName;
  char* FieldDelimiter;
  char* StringDelimiter;
  bool UseStringDelimiter;
  ofstream* Stream;
private:
  vtkCSVWriter(const vtkCSVWriter&); // Not implemented.
  void operator=(const vtkCSVWriter&); // Not implemented.
//ETX
};



#endif

