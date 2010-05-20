/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLCollectionReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLCollectionReader - Read a file wrapping many other XML files.
// .SECTION Description
// vtkXMLCollectionReader will read a "Collection" VTKData XML file.
// This file format references an arbitrary number of other XML data
// sets.  Each referenced data set has a list of associated
// attribute/value pairs.  One may use the SetRestriction method to
// set requirements on attribute's values.  Only those data sets in
// the file matching the restrictions will be read.  Each matching
// data set becomes an output of this reader in the order in which
// they appear in the file.

#ifndef __vtkXMLCollectionReader_h
#define __vtkXMLCollectionReader_h

#include "vtkXMLReader.h"

class vtkXMLCollectionReaderInternals;

class VTK_EXPORT vtkXMLCollectionReader : public vtkXMLReader
{
public:
  static vtkXMLCollectionReader* New();
  vtkTypeMacro(vtkXMLCollectionReader,vtkXMLReader);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get/Set the required value for a particular named attribute.
  // Only data sets matching this value will be read.  A NULL value or
  // empty string will disable any restriction from the given
  // attribute.
  virtual void SetRestriction(const char* name, const char* value);
  virtual const char* GetRestriction(const char* name);
  
  // Description:
  // Get/set the required value for a particular named attribute.  The
  // value should be referenced by its index.  Only data sets matching
  // this value will be read.  An out-of-range index will remove the
  // restriction.
  // Make sure to call UpdateInformation() before using these methods.
  virtual void SetRestrictionAsIndex(const char* name, int index);
  virtual int GetRestrictionAsIndex(const char* name);
  
  // Description:
  // Get the number of distinct attribute values present in the file.
  // Valid after UpdateInformation.
  int GetNumberOfAttributes();
  
  // Description:
  // Get the name of an attribute.  The order of attributes with
  // respect to the index is not specified, but will be the same every
  // time the same instance of the reader reads the same input file.
  const char* GetAttributeName(int attribute);
  
  // Description:
  // Get the index of the attribute with the given name.  Returns -1
  // if no such attribute exists.
  int GetAttributeIndex(const char* name);
  
  // Description:
  // Get the number of distinct values for the given attribute.
  int GetNumberOfAttributeValues(int attribute);
  
  // Description:
  // Get one of the possible values for a given attribute.  The order
  // of values for the attribute with respect to the index is not
  // specified, but will be the same every time the same instance of
  // the reader reads the same input file.
  const char* GetAttributeValue(int attribute, int index);
  const char* GetAttributeValue(const char* name, int index);
  
  // Description:
  // Get the index of the attribute value with the given name.  Returns -1
  // if no such attribute or value exists.
  int GetAttributeValueIndex(int attribute, const char* value);
  int GetAttributeValueIndex(const char* name, const char* value);

  // Description:
  // Get the vtkXMLDataElement representing the collection element
  // corresponding to the output with the given index.  Valid when a
  // FileName has been set.  May change when Restriction settings are
  // changed.
  vtkXMLDataElement* GetOutputXMLDataElement(int index);

  // Description:
  // If ForceOutputTypeToMultiBlock is set to 1, the output of this reader
  // will always be a multi-block dataset, even if there is 1 simple output.
  vtkSetMacro(ForceOutputTypeToMultiBlock, int);
  vtkGetMacro(ForceOutputTypeToMultiBlock, int);
  vtkBooleanMacro(ForceOutputTypeToMultiBlock, int);

protected:
  vtkXMLCollectionReader();
  ~vtkXMLCollectionReader();  
  
  void BuildRestrictedDataSets();

  bool InternalForceMultiBlock;
  int ForceOutputTypeToMultiBlock;

  // Get the name of the data set being read.
  virtual const char* GetDataSetName();
  
  virtual int ReadPrimaryElement(vtkXMLDataElement* ePrimary);
  virtual int FillOutputPortInformation(int, vtkInformation* info);

  vtkDataObject* SetupOutput(const char* filePath, int index);

  virtual int RequestDataObject(vtkInformation* request, 
                                vtkInformationVector** inputVector, 
                                vtkInformationVector* outputVector);

  // Overload of vtkXMLReader function, so we can handle updating the
  // information on multiple outputs
  virtual int RequestInformation(vtkInformation *request,
    vtkInformationVector **inputVector, vtkInformationVector *outputVector);

  // Setup the output with no data available.  Used in error cases.
  virtual void SetupEmptyOutput();

  void ReadXMLData();
  void ReadXMLDataImpl();
  
  // Callback registered with the InternalProgressObserver.
  static void InternalProgressCallbackFunction(vtkObject*, unsigned long, void*,
                                           void*);
  // Progress callback from XMLParser.
  virtual void InternalProgressCallback();
  
  // The observer to report progress from the internal readers.
  vtkCallbackCommand* InternalProgressObserver;
  
  // Internal implementation details.
  vtkXMLCollectionReaderInternals* Internal;
  
  void AddAttributeNameValue(const char* name, const char* value);

  virtual void SetRestrictionImpl(const char* name, 
                                  const char* value,
                                  bool doModify);

  void ReadAFile(int index,
                 int updatePiece,
                 int updateNumPieces,
                 int updateGhostLevels,
                 vtkDataObject* actualOutput);
  
private:
  vtkXMLCollectionReader(const vtkXMLCollectionReader&);  // Not implemented.
  void operator=(const vtkXMLCollectionReader&);  // Not implemented.
  
  int CurrentOutput;
};

#endif
