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
  // Set/Get the matrix size. Default to 1x1.
  virtual void SetNumberOfColumns(int col);
  vtkGetMacro(NumberOfColumns, int);
  virtual void SetNumberOfRows(int col);
  vtkGetMacro(NumberOfRows, int);

  // Description:
  // Set/Get the value of a given element.
  virtual void SetElementValue(int row, int col, const char *val);
  virtual const char* GetElementValue(int row, int col);
  virtual void SetElementValueAsInt(int row, int col, int val);
  virtual int GetElementValueAsInt(int row, int col);
  virtual void SetElementValueAsDouble(int row, int col, double val);
  virtual double GetElementValueAsDouble(int row, int col);

  // Description:
  // The width is the number of charaters each element can fit.
  virtual void SetElementWidth(int width);
  vtkGetMacro(ElementWidth, int);

  // Description:
  // Set/Get readonly flag. This flags makes each element read only.
  virtual void SetReadOnly(int);
  vtkBooleanMacro(ReadOnly, int);
  vtkGetMacro(ReadOnly, int);

  // Description:
  // Restrict the value of an element to a given type
  // (integer, double, or no restriction).
  //BTX
  enum
  {
    RestrictNone = 0,
    RestrictInteger,
    RestrictDouble
  };
  //ETX
  vtkGetMacro(RestrictElementValue, int);
  virtual void SetRestrictElementValue(int);
  virtual void SetRestrictElementValueToInteger();
  virtual void SetRestrictElementValueToDouble();
  virtual void SetRestrictElementValueToNone();

  // Description:
  // Specifies a command to be invoked when the value of an element in the
  // matrix has changed.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // The following parameters are also passed to the command:
  // - the element location, i.e. its row and column indices: int, int
  // - the element's new value: const char*
  virtual void SetElementChangedCommand(vtkObject *object, const char *method);

  // Description:
  // Events. The ElementChangedEvent is triggered when the value of an
  // element in the matrix has changed.
  // The following parameters are also passed as client data:
  // - the element location, i.e. its row and column indices: int, int
  // - the element's new value: const char*
  // Note that given the heterogeneous nature of types passed as client data,
  // you should treat it as an array of void*[3], each one a pointer to
  // the parameter (i.e., &int, &int, &const char*).
  //BTX
  enum
  {
    ElementChangedEvent = 10000
  };
  //ETX

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Callbacks.
  virtual void ElementChangedCallback(int id, const char *value);

protected:
  vtkKWMatrixWidget();
  virtual ~vtkKWMatrixWidget();

  int NumberOfColumns;
  int NumberOfRows;

  int ElementWidth;
  int ReadOnly;
  int RestrictElementValue;

  // Description:
  // Create the widget.
  virtual void CreateWidget();
  virtual void UpdateWidget();

  vtkKWEntrySet *EntrySet;

  char *ElementChangedCommand;
  void InvokeElementChangedCommand(int row, int col, const char *value);

private:
  vtkKWMatrixWidget(const vtkKWMatrixWidget&); // Not implemented
  void operator=(const vtkKWMatrixWidget&); // Not implemented
};

#endif

