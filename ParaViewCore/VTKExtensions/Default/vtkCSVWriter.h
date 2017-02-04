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
/**
 * @class   vtkCSVWriter
 * @brief   CSV writer for vtkTable
 * Writes a vtkTable as a delimited text file (such as CSV).
*/

#ifndef vtkCSVWriter_h
#define vtkCSVWriter_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkWriter.h"

class vtkStdString;
class vtkTable;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkCSVWriter : public vtkWriter
{
public:
  static vtkCSVWriter* New();
  vtkTypeMacro(vtkCSVWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get/Set the delimiter use to separate fields ("," by default.)
   */
  vtkSetStringMacro(FieldDelimiter);
  vtkGetStringMacro(FieldDelimiter);
  //@}

  //@{
  /**
   * Get/Set the delimiter used for string data, if any
   * eg. double quotes(").
   */
  vtkSetStringMacro(StringDelimiter);
  vtkGetStringMacro(StringDelimiter);
  //@}

  //@{
  /**
   * Get/Set the filename for the file.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Get/Set if StringDelimiter must be used for string data.
   * True by default.
   */
  vtkSetMacro(UseStringDelimiter, bool);
  vtkGetMacro(UseStringDelimiter, bool);
  //@}

  //@{
  /**
   * Get/Set the precision to use for printing numeric values.
   * Default is 5.
   */
  vtkSetClampMacro(Precision, int, 0, VTK_INT_MAX);
  vtkGetMacro(Precision, int);
  //@}

  //@{
  /**
   * Get/Set whether scientific notation is used for numeric values.
   */
  vtkSetMacro(UseScientificNotation, bool);
  vtkGetMacro(UseScientificNotation, bool);
  vtkBooleanMacro(UseScientificNotation, bool);
  //@}

  //@{
  /**
   * Internal method: decortes the "string" with the "StringDelimiter" if
   * UseStringDelimiter is true.
   */
  vtkStdString GetString(vtkStdString string);

protected:
  vtkCSVWriter();
  ~vtkCSVWriter();
  //@}

  bool OpenFile();

  virtual void WriteData() VTK_OVERRIDE;
  virtual void WriteTable(vtkTable* rectilinearGrid);

  // see algorithm for more info.
  // This writer takes in vtkTable.
  virtual int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  char* FileName;
  char* FieldDelimiter;
  char* StringDelimiter;
  bool UseStringDelimiter;
  int Precision;
  bool UseScientificNotation;

  ofstream* Stream;

private:
  vtkCSVWriter(const vtkCSVWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCSVWriter&) VTK_DELETE_FUNCTION;
};

#endif
