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
/**
 * @class   vtkSpyPlotHistoryReader
 * @brief   Read SPCTH Spy Plot history file format
 *
 * vtkSpyPlotHistoryReader is a reader that reads SPCTH Spy Plot history
 * file format files. Each row in the history files represents a time step
 * and columns represent points and properties for the points
*/

#ifndef vtkSpyPlotHistoryReader_h
#define vtkSpyPlotHistoryReader_h

#include "vtkPVVTKExtensionsIOSPCTHModule.h" //needed for exports
#include "vtkTableAlgorithm.h"
class vtkTable;

class VTKPVVTKEXTENSIONSIOSPCTH_EXPORT vtkSpyPlotHistoryReader : public vtkTableAlgorithm
{
public:
  static vtkSpyPlotHistoryReader* New();
  vtkTypeMacro(vtkSpyPlotHistoryReader, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get and set the file name. It is either the name of the case file or the
   * name of the single binary file.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Get and set the comment character for the file
   */
  vtkSetStringMacro(CommentCharacter);
  vtkGetStringMacro(CommentCharacter);
  //@}

  //@{
  /**
   * Get and set the delimeter character for the file
   */
  vtkSetStringMacro(Delimeter);
  vtkGetStringMacro(Delimeter);
  //@}

protected:
  vtkSpyPlotHistoryReader();
  ~vtkSpyPlotHistoryReader() override;

  // Read the case file and the first binary file do get meta
  // information (number of files, number of fields, number of timestep).
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  // Read the data: get the number of pieces (=processors) and get
  // my piece id (=my processor id).
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  void FillCache();
  void ConstructTableColumns(vtkTable* table);

  char* FileName;
  char* CommentCharacter;
  char* Delimeter;

private:
  //@{
  /**
   * Private storage of time information
   */
  class MetaInfo;
  MetaInfo* Info;
  //@}

  //@{
  /**
   * Private storage of cached output tables for each time step.
   */
  class CachedTables;
  CachedTables* CachedOutput;
  //@}

  vtkSpyPlotHistoryReader(const vtkSpyPlotHistoryReader&) = delete;
  void operator=(const vtkSpyPlotHistoryReader&) = delete;
};

#endif
