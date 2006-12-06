/*=========================================================================

  Module:    vtkKWMatrixWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWMatrixWidget - matrix widget
// .SECTION Description
// vtkKWMatrixWidget is a widget containing entries that help view and
// edit a matrix.
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.

#ifndef __vtkKWMatrixWidget_h
#define __vtkKWMatrixWidget_h

#include "vtkKWCompositeWidget.h"

class vtkKWEntrySet;

class KWWidgets_EXPORT vtkKWMatrixWidget : public vtkKWCompositeWidget
{
public:
  static vtkKWMatrixWidget* New();
  vtkTypeRevisionMacro(vtkKWMatrixWidget,vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the matrix size
  virtual void SetNumberOfColumns(int col);
  vtkGetMacro(NumberOfColumns, int);
  virtual void SetNumberOfRows(int col);
  vtkGetMacro(NumberOfRows, int);

  // Description:
  // Set/Get the values
  virtual void SetValue(int row, int col, const char *val);
  virtual const char* GetValue(int row, int col);
  virtual void SetValueAsInt(int row, int col, int val);
  virtual int GetValueAsInt(int row, int col);
  virtual void SetValueAsDouble(int row, int col, double val);
  virtual double GetValueAsDouble(int row, int col);

  // Description:
  // The width is the number of charaters wide each entry box can fit.
  virtual void SetWidth(int width);
  vtkGetMacro(Width, int);

  // Description:
  // Set/Get readonly flag. This flags makes each entry read only.
  virtual void SetReadOnly(int);
  vtkBooleanMacro(ReadOnly, int);
  vtkGetMacro(ReadOnly, int);

  // Description:
  // Restrict the value to a given type (integer, double, or no restriction).
  //BTX
  enum
  {
    RestrictNone = 0,
    RestrictInteger,
    RestrictDouble
  };
  //ETX
  vtkGetMacro(RestrictValue, int);
  virtual void SetRestrictValue(int);
  virtual void SetRestrictValueToInteger();
  virtual void SetRestrictValueToDouble();
  virtual void SetRestrictValueToNone();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWMatrixWidget();
  virtual ~vtkKWMatrixWidget();

  int NumberOfColumns;
  int NumberOfRows;

  int Width;
  int ReadOnly;
  int RestrictValue;

  // Description:
  // Create the widget.
  virtual void CreateWidget();
  virtual void UpdateWidget();

  vtkKWEntrySet *EntrySet;

private:
  vtkKWMatrixWidget(const vtkKWMatrixWidget&); // Not implemented
  void operator=(const vtkKWMatrixWidget&); // Not implemented
};

#endif

