/*=========================================================================

  Program:   ParaView
  Module:    vtkCSVReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCSVReader - reads CSV files into a one-dimensional 
// vtkRectilinearGrid.
// .SECTION Description:
// vtkCSVReader reads CSV files into a rectilinear grid. It internally
// uses vtkDelimitedTextReader. 
// This reader when used to read a csv file saved using vtkCSVWriter can 
// restore the rectilinear grid's points, point data as well as cell data.
// This is possible since vtkCSVWriter adds special headers to columns which 
// this reader can interpret. In absence of such headers this reader
// creates a 1D rectilinear grid with the data arrays CSV treated as
// cell data for the output.
//
// .SECTION Caveats
// It must be remembered that CSV format is not complete to save rectilinear
// grids. Only 1D rectilinear grids may be saved/loaded.
//
// .SECTION See Also
// vtkDelimitedTextReader

#ifndef __vtkCSVReader_h
#define __vtkCSVReader_h

#include "vtkRectilinearGridAlgorithm.h"

class VTK_EXPORT vtkCSVReader : public vtkRectilinearGridAlgorithm
{
public:
  static vtkCSVReader* New();
  vtkTypeRevisionMacro(vtkCSVReader, vtkRectilinearGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the file name.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Get/set the character that will begin and end strings.  Microsoft
  // Excel, for example, will export the following format:
  //
  // "First Field","Second Field","Field, With, Commas","Fourth Field"
  //
  // The third field has a comma in it.  By using a string delimiter,
  // this will be correctly read.  The delimiter defaults to '"'.
  vtkGetMacro(StringDelimiter, char);
  vtkSetMacro(StringDelimiter, char);

  // Description:
  // Set/get whether to use the string delimiter.  Defaults to on.
  vtkSetMacro(UseStringDelimiter, bool);
  vtkGetMacro(UseStringDelimiter, bool);
  vtkBooleanMacro(UseStringDelimiter, bool);

  // Description:
  // Get/set the characters that will be used to separate fields.  For
  // example, set this to "," for a comma-separated value file.  Set
  // it to ".:;" for a file where columns can be separated by a
  // period, colon or semicolon.  The order of the characters in the
  // string does not matter.  Defaults to a comma.
  vtkSetStringMacro(FieldDelimiterCharacters);
  vtkGetStringMacro(FieldDelimiterCharacters);


  // Description:
  // Set/get whether to treat the first line of the file as headers.
  vtkGetMacro(HaveHeaders,bool);
  vtkSetMacro(HaveHeaders,bool);

  // Description:
  // Set/get whether to merge successive delimiters.  Use this if (for
  // example) your fields are separated by spaces but you don't know
  // exactly how many.
  vtkSetMacro(MergeConsecutiveDelimiters, bool);
  vtkGetMacro(MergeConsecutiveDelimiters, bool);
  vtkBooleanMacro(MergeConsecutiveDelimiters, bool);
protected:
  vtkCSVReader();
  ~vtkCSVReader();

  virtual int RequestData(vtkInformation*, vtkInformationVector**, 
    vtkInformationVector*);

  virtual int RequestInformation(vtkInformation *, 
    vtkInformationVector **, vtkInformationVector *);

  // Description:
  // Performs the actual reading.
  virtual int ReadData(vtkRectilinearGrid* output);

  char* FileName;
  char *FieldDelimiterCharacters;
  char StringDelimiter;
  bool UseStringDelimiter;
  bool HaveHeaders;
  bool MergeConsecutiveDelimiters;
  vtkRectilinearGrid* Cache;
  vtkTimeStamp CacheUpdateTime;
private:
  vtkCSVReader(const vtkCSVReader&); // Not implemented.
  void operator=(const vtkCSVReader&); // Not implemented.
};

#endif

