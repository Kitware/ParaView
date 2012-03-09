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
// .NAME vtkCSVWriter - CSV writer for vtkTable
// Writes a vtkTable as a delimited text file (such as CSV). 
#ifndef __vtkCSVWriter_h
#define __vtkCSVWriter_h

#include "vtkWriter.h"

class vtkStdString;
class vtkTable;

class VTK_EXPORT vtkCSVWriter : public vtkWriter
{
public:
  static vtkCSVWriter* New();
  vtkTypeMacro(vtkCSVWriter, vtkWriter);
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

  // Description:
  // Get/Set the filename for the file.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Get/Set if StringDelimiter must be used for string data.
  // True by default.
  vtkSetMacro(UseStringDelimiter, bool);
  vtkGetMacro(UseStringDelimiter, bool);

  // Description:
  // Get/Set the precision to use for printing numeric values.
  // Default is 5.
  vtkSetClampMacro(Precision, int, 0, VTK_INT_MAX);
  vtkGetMacro(Precision, int);

  // Description:
  // Get/Set whether scientific notation is used for numeric values.
  vtkSetMacro(UseScientificNotation, bool);
  vtkGetMacro(UseScientificNotation, bool);
  vtkBooleanMacro(UseScientificNotation, bool);

//BTX
  // Description:
  // Internal method: decortes the "string" with the "StringDelimiter" if 
  // UseStringDelimiter is true.
  vtkStdString GetString(vtkStdString string);
protected:
  vtkCSVWriter();
  ~vtkCSVWriter();

  bool OpenFile();

  virtual void WriteData();
  virtual void WriteTable(vtkTable* rectilinearGrid);

  // see algorithm for more info.
  // This writer takes in vtkTable.
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  char* FileName;
  char* FieldDelimiter;
  char* StringDelimiter;
  bool UseStringDelimiter;
  int Precision;
  bool UseScientificNotation;

  ofstream* Stream;
private:
  vtkCSVWriter(const vtkCSVWriter&); // Not implemented.
  void operator=(const vtkCSVWriter&); // Not implemented.
//ETX
};



#endif

