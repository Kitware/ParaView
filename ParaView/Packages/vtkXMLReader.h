/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLReader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLReader - Superclass for VTK's XML format readers.
// .SECTION Description
// vtkXMLReader uses vtkXMLDataParser to parse a VTK XML input file.

#ifndef __vtkXMLReader_h
#define __vtkXMLReader_h

#include "vtkSource.h"

class vtkXMLDataParser;
class vtkXMLDataElement;
class vtkDataSet;
class vtkDataArray;

class VTK_EXPORT vtkXMLReader : public vtkSource
{
public:
  vtkTypeRevisionMacro(vtkXMLReader,vtkSource);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get/Set the name of the input file.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  
  // Description:
  // Test whether the file with the given name can be read by this
  // reader.
  virtual int CanReadFile(const char* name);
  
  // Description:
  // Get the output as a vtkDataSet pointer.
  vtkDataSet* GetOutputAsDataSet();
  
protected:
  vtkXMLReader();
  ~vtkXMLReader();
  
  // Standard pipeline exectution methods.
  void ExecuteInformation();
  void ExecuteData(vtkDataObject* output);
  
  // Pipeline execution methods to be defined by subclass.  Called by
  // corresponding Execute methods after appropriate setup has been
  // done.
  virtual void ReadXMLInformation();
  virtual void ReadXMLData();
  
  // Get the name of the data set being read.
  virtual const char* GetDataSetName()=0;
  
  // Test if the reader can read a file with the given version number.
  virtual int CanReadFileVersion(int major, int minor);
  
  // Setup the output's information and data without allocation.
  virtual void SetupOutputInformation();
  
  // Setup the output's information and data with allocation.
  virtual void SetupOutputData();
  
  // Read the primary element from the file.  This is the element
  // whose name is the value returned by GetDataSetName().
  virtual int ReadPrimaryElement(vtkXMLDataElement* ePrimary);
  
  // Read the top-level element from the file.  This is always the
  // VTKFile element.
  int ReadVTKFile(vtkXMLDataElement* eVTKFile);  
  
  // Create a vtkDataArray from its cooresponding XML representation.
  // Does not allocate.
  vtkDataArray* CreateDataArray(vtkXMLDataElement* da);
  
  // Internal utility methods.
  int OpenVTKFile();
  void CloseVTKFile();
  void CreateXMLParser();
  void DestroyXMLParser();
  void SetupCompressor(const char* type);
  int CanReadFileVersionString(const char* version);
  
  // Utility methods for subclasses.
  int IntersectExtents(int* extent1, int* extent2, int* result);
  int Min(int a, int b);
  int Max(int a, int b);
  void ComputeDimensions(int* extent, int* dimensions, int isPoint);
  void ComputeIncrements(int* extent, int* increments, int isPoint);
  unsigned int GetStartTuple(int* extent, int* increments,
                             int i, int j, int k);
  
  // The vtkXMLDataParser instance used to hide XML reading details.
  vtkXMLDataParser* XMLParser;
  
  // The input file's name.
  char* FileName;
  
  // The file stream used to read the input file.
  ifstream* FileStream;
  
private:
  vtkXMLReader(const vtkXMLReader&);  // Not implemented.
  void operator=(const vtkXMLReader&);  // Not implemented.
};

#endif
