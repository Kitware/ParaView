/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkSpyPlotHistoryReader.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSpyPlotHistoryReader - Read SPCTH Spy Plot history file format
// .SECTION Description
// vtkSpyPlotHistoryReader is a reader that reads SPCTH Spy Plot history
// file format files. Each row in the history files represents a time step
// and columns represent points and properties for the points


#ifndef __vtkSpyPlotHistoryReader_h
#define __vtkSpyPlotHistoryReader_h

#include "vtkTableAlgorithm.h"
class vtkTable;

class VTK_EXPORT vtkSpyPlotHistoryReader : public vtkTableAlgorithm
{
public:
  static vtkSpyPlotHistoryReader* New();
  vtkTypeMacro(vtkSpyPlotHistoryReader,vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);


  // Description:
  // Get and set the file name. It is either the name of the case file or the
  // name of the single binary file.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Get and set the comment character for the file
  vtkSetStringMacro(CommentCharacter);
  vtkGetStringMacro(CommentCharacter);

  // Description:
  // Get and set the delimeter character for the file
  vtkSetStringMacro(Delimeter);
  vtkGetStringMacro(Delimeter);

protected:
  vtkSpyPlotHistoryReader();
  ~vtkSpyPlotHistoryReader();

  // Read the case file and the first binary file do get meta
  // informations (number of files, number of fields, number of timestep).
  virtual int RequestInformation(vtkInformation *request,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector);

  // Read the data: get the number of pieces (=processors) and get
  // my piece id (=my processor id).
  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);

  void FillCache();
  void ConstructTableColumns(vtkTable *table);

  char *FileName;
  char *CommentCharacter;
  char *Delimeter;

private:
  // Description:
  //Private storage of time information
  class MetaInfo;
  MetaInfo *Info;

  // Description:
  //Private storage of cached output tables for each time step.
  class CachedTables;
  CachedTables *CachedOutput;

  vtkSpyPlotHistoryReader(const vtkSpyPlotHistoryReader&);  // Not implemented.
  void operator=(const vtkSpyPlotHistoryReader&);  // Not implemented.
};

#endif
