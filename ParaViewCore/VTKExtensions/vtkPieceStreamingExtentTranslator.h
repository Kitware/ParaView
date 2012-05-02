/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPieceStreamingExtentTranslator
// .SECTION Description

#ifndef __vtkPieceStreamingExtentTranslator_h
#define __vtkPieceStreamingExtentTranslator_h

#include "vtkStreamingExtentTranslator.h"

class VTK_EXPORT vtkPieceStreamingExtentTranslator : public vtkStreamingExtentTranslator
{
public:
  static vtkPieceStreamingExtentTranslator* New();
  vtkTypeMacro(vtkPieceStreamingExtentTranslator, vtkStreamingExtentTranslator);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:;
  // Set/Get in how many passes should the data be streamed.
  vtkSetClampMacro(NumberOfPasses, int, 1, 20);
  vtkGetMacro(NumberOfPasses, int);

  // Description:
  // This is the method that is responsible to convert the pass request to
  // appropriate pipeline request.
  virtual int PassToRequest(int pass, vtkInformation* info);

//BTX
protected:
  vtkPieceStreamingExtentTranslator();
  ~vtkPieceStreamingExtentTranslator();

  int NumberOfPasses;

private:
  vtkPieceStreamingExtentTranslator(const vtkPieceStreamingExtentTranslator&); // Not implemented.
  void operator=(const vtkPieceStreamingExtentTranslator&); // Not implemented.
//ETX
};

#endif
