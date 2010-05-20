/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPCSVWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPCSVWriter - writes CSV files in parallel.
// .SECTION Description
// vtkPCSVWriter inherits from vtkCSVWriter for writing CSV files in parallel
//
// .SECTION Caveats
//
// .SECTION See Also
// vtkCSVWriter
// vtkSMPWriterProxy
// .SECTION Thanks
// Jon Goldman of Sandia National Laboratories supplied this class.
// Thanks to Utkarsh Ayachit of Kitware, and Ken Moreland of Sandia for
// pointing Jon in the "write" directions to get this writer codified  :-)

#ifndef __vtkPCSVWriter_h
#define __vtkPCSVWriter_h

#include "vtkCSVWriter.h"

class vtkMultiProcessController;
class vtkDataSet;

class VTK_EXPORT vtkPCSVWriter : public vtkCSVWriter
{
public:
  vtkTypeMacro(vtkPCSVWriter, vtkCSVWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  static vtkPCSVWriter *New();
  
  // Description:
  // Get/Set the piece number to write.  The same piece number is used
  // for all inputs.
  vtkGetMacro(Piece, int);
  vtkSetMacro(Piece, int);
  
  // Description:
  // Get/Set the number of pieces into which the inputs are split.
  vtkGetMacro(NumberOfPieces, int);
  vtkSetMacro(NumberOfPieces, int);
  
  // Description:
  // Get/Set the number of ghost levels (Note: this setting is ignored for this writer)
  vtkGetMacro(GhostLevel, int);
  vtkSetMacro(GhostLevel, int);

  // Description:
  // Set and get the controller.
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

protected:
  vtkPCSVWriter();
  ~vtkPCSVWriter();

//BTX
  enum 
    {
    PCSV_COMMUNICATION_TAG=7755
    };
//ETX

  virtual void WriteData();

  virtual void WriteRectilinearDataInParallel(
    vtkRectilinearGrid* rectilinearGrid);

  virtual void AppendCSVDataSet(vtkRectilinearGrid* remoteCSVOutput,
    vtkRectilinearGrid* rectilinearGrid);
  
  // Usual data generation method
  //   NOTE: rely upon superclass for these methods:
  //      RequestData, FillInputPortInformation
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *);

  vtkMultiProcessController* Controller;
  
  // The piece number to write.
  int Piece;
  
  // The number of pieces into which the inputs are split.
  int NumberOfPieces;
  
  // The number of ghost levels to write for unstructured data.
  int GhostLevel;

private:
  vtkPCSVWriter(const vtkPCSVWriter&);  // Not implemented.
  void operator=(const vtkPCSVWriter&);  // Not implemented.
};

#endif
