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

class vtkMultiProcessController;
class vtkStdString;
class vtkTable;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkCSVWriter : public vtkWriter
{
public:
  static vtkCSVWriter* New();
  vtkTypeMacro(vtkCSVWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the controller to use. By default,
   * `vtkMultiProcessController::GetGlobalController` will be used.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

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
   * Get/set the attribute data to write if the input is either
   * a vtkDataSet or composite of vtkDataSets. 0 is for point data (vtkDataObject::POINT),
   * 1 is for cell data (vtkDataObject::CELL) and 2 is for field data (vtkDataObject::FIELD).
   * Default is 0.
   */
  vtkSetClampMacro(FieldAssociation, int, 0, 2);
  vtkGetMacro(FieldAssociation, int);
  //@}

  //@{
  /**
   * Get/Set whether to add additional meta-data to the field data such as
   * point coordinates (when point attributes are selected and input is pointset)
   * or structured coordinates etc. if the input is either a vtkDataSet or composite
   * of vtkDataSets.
   */
  vtkSetMacro(AddMetaData, bool);
  vtkGetMacro(AddMetaData, bool);
  vtkBooleanMacro(AddMetaData, bool);
  //@}

  //@{
  /**
   * When set to true (default is false), if the input data set has time, then the
   * time information will be saved under the column named "Time".
   */
  vtkSetMacro(AddTime, bool);
  vtkGetMacro(AddTime, bool);
  vtkBooleanMacro(AddTime, bool);
  //@}

  //@{
  /**
   * Internal method: decorates the "string" with the "StringDelimiter" if
   * UseStringDelimiter is true.
   */
  vtkStdString GetString(vtkStdString string);
  //@}

protected:
  vtkCSVWriter();
  ~vtkCSVWriter() override;

  void WriteData() override;

  // see algorithm for more info.
  // This writer takes in vtkTable, vtkDataSet or vtkCompositeDataSet.
  int FillInputPortInformation(int port, vtkInformation* info) override;

  // see algorithm for more info. needed here so we can request pieces.
  int ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  char* FileName;
  char* FieldDelimiter;
  char* StringDelimiter;
  bool UseStringDelimiter;
  int Precision;
  bool UseScientificNotation;
  int FieldAssociation;
  bool AddMetaData;
  bool AddTime;

  vtkMultiProcessController* Controller;

private:
  vtkCSVWriter(const vtkCSVWriter&) = delete;
  void operator=(const vtkCSVWriter&) = delete;

  class CSVFile;
};

#endif
