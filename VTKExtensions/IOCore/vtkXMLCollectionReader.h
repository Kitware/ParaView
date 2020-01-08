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
/**
 * @class   vtkXMLCollectionReader
 * @brief   Read a file wrapping many other XML files.
 *
 * vtkXMLCollectionReader will read a "Collection" VTKData XML file.
 * This file format references an arbitrary number of other XML data
 * sets.  Each referenced data set has a list of associated
 * attribute/value pairs.  One may use the SetRestriction method to
 * set requirements on attribute's values.  Only those data sets in
 * the file matching the restrictions will be read.  Each matching
 * data set becomes an output of this reader in the order in which
 * they appear in the file.
*/

#ifndef vtkXMLCollectionReader_h
#define vtkXMLCollectionReader_h

#include "vtkPVVTKExtensionsIOCoreModule.h" //needed for exports
#include "vtkXMLReader.h"

class vtkXMLCollectionReaderInternals;

class VTKPVVTKEXTENSIONSIOCORE_EXPORT vtkXMLCollectionReader : public vtkXMLReader
{
public:
  static vtkXMLCollectionReader* New();
  vtkTypeMacro(vtkXMLCollectionReader, vtkXMLReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the required value for a particular named attribute.
   * Only data sets matching this value will be read.  A NULL value or
   * empty string will disable any restriction from the given
   * attribute.
   */
  virtual void SetRestriction(const char* name, const char* value);
  virtual const char* GetRestriction(const char* name);
  //@}

  //@{
  /**
   * Get/set the required value for a particular named attribute.  The
   * value should be referenced by its index.  Only data sets matching
   * this value will be read.  An out-of-range index will remove the
   * restriction.
   * Make sure to call UpdateInformation() before using these methods.
   */
  virtual void SetRestrictionAsIndex(const char* name, int index);
  virtual int GetRestrictionAsIndex(const char* name);
  //@}

  /**
   * Get the number of distinct attribute values present in the file.
   * Valid after UpdateInformation.
   */
  int GetNumberOfAttributes();

  /**
   * Get the name of an attribute.  The order of attributes with
   * respect to the index is not specified, but will be the same every
   * time the same instance of the reader reads the same input file.
   */
  const char* GetAttributeName(int attribute);

  /**
   * Get the index of the attribute with the given name.  Returns -1
   * if no such attribute exists.
   */
  int GetAttributeIndex(const char* name);

  /**
   * Get the number of distinct values for the given attribute.
   */
  int GetNumberOfAttributeValues(int attribute);

  //@{
  /**
   * Get one of the possible values for a given attribute.  The order
   * of values for the attribute with respect to the index is not
   * specified, but will be the same every time the same instance of
   * the reader reads the same input file.
   */
  const char* GetAttributeValue(int attribute, int index);
  const char* GetAttributeValue(const char* name, int index);
  //@}

  //@{
  /**
   * Get the index of the attribute value with the given name.  Returns -1
   * if no such attribute or value exists.
   */
  int GetAttributeValueIndex(int attribute, const char* value);
  int GetAttributeValueIndex(const char* name, const char* value);
  //@}

  /**
   * Get the vtkXMLDataElement representing the collection element
   * corresponding to the output with the given index.  Valid when a
   * FileName has been set.  May change when Restriction settings are
   * changed.
   */
  vtkXMLDataElement* GetOutputXMLDataElement(int index);

  //@{
  /**
   * If ForceOutputTypeToMultiBlock is set to 1, the output of this reader
   * will always be a multi-block dataset, even if there is 1 simple output.
   */
  vtkSetMacro(ForceOutputTypeToMultiBlock, int);
  vtkGetMacro(ForceOutputTypeToMultiBlock, int);
  vtkBooleanMacro(ForceOutputTypeToMultiBlock, int);
  //@}

protected:
  vtkXMLCollectionReader();
  ~vtkXMLCollectionReader() override;

  void BuildRestrictedDataSets();

  bool InternalForceMultiBlock;
  int ForceOutputTypeToMultiBlock;

  // Get the name of the data set being read.
  const char* GetDataSetName() override;

  int ReadPrimaryElement(vtkXMLDataElement* ePrimary) override;
  int FillOutputPortInformation(int, vtkInformation* info) override;

  vtkDataObject* SetupOutput(const std::string& filePath, int index);

  /**
   * Ensures that an appropriate reader is setup for the specified index.
   */
  vtkXMLReader* SetupReader(const std::string& filePath, int index);

  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  // Overload of vtkXMLReader function, so we can handle updating the
  // information on multiple outputs
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  // Setup the output with no data available.  Used in error cases.
  void SetupEmptyOutput() override;

  void ReadXMLData() override;
  void ReadXMLDataImpl();

  // Progress callback from XMLParser.
  virtual void InternalProgressCallback();

  // Internal implementation details.
  vtkXMLCollectionReaderInternals* Internal;

  void AddAttributeNameValue(const char* name, const char* value);

  virtual void SetRestrictionImpl(const char* name, const char* value, bool doModify);

  void ReadAFile(int index, int updatePiece, int updateNumPieces, int updateGhostLevels,
    vtkDataObject* actualOutput);

  /**
   * iterating over all readers (which corresponds to number of distinct
   * datasets in the file, and not distinct timesteps), populate
   * this->*ArraySelection objects.
   */
  void FillArraySelectionUsingReaders(const std::string& filePath);

private:
  vtkXMLCollectionReader(const vtkXMLCollectionReader&) = delete;
  void operator=(const vtkXMLCollectionReader&) = delete;

  int CurrentOutput;
};

#endif
