/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEnsembleDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkEnsembleDataReader
 * @brief   reader for ensemble data sets
 *
 * vtkEnsembleDataReader reads a collection of data sources from a metadata
 * file (of extension .pve).
 * 'pve' a simply CSV file with the last column being the relative filename and
 * other columns for each of the variables in the ensemble.
*/

#ifndef vtkEnsembleDataReader_h
#define vtkEnsembleDataReader_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkPVVTKExtensionsIOGeneralModule.h" //needed for exports

#include <string> // for std::string

class VTKPVVTKEXTENSIONSIOGENERAL_EXPORT vtkEnsembleDataReader : public vtkDataObjectAlgorithm
{
public:
  static vtkEnsembleDataReader* New();
  vtkTypeMacro(vtkEnsembleDataReader, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/Get the filename of the ensemble (.pve extension).
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Set/Get the current ensemble member to process.
   */
  vtkSetMacro(CurrentMember, unsigned int);
  vtkGetMacro(CurrentMember, unsigned int);
  //@}

  //@{
  /**
   * Returns the number of ensemble members
   */
  unsigned int GetNumberOfMembers() const;
  vtkGetVector2Macro(CurrentMemberRange, unsigned int);
  //@}

  /**
   * Get the file path associated with the specified row of the meta data
   */
  std::string GetFilePath(unsigned int rowIndex) const;

  /**
   * Set the file reader for the specified row of data
   */
  void SetReader(unsigned int rowIndex, vtkAlgorithm* reader);

  /**
   * Removes all readers set using SetReader().
   */
  void ResetReaders();

  /**
   * Use this method to update the meta data, if needed. This will only read the
   * file again if cache is obsolete.
   */
  bool UpdateMetaData();

protected:
  vtkEnsembleDataReader();
  ~vtkEnsembleDataReader() override;

  int ProcessRequest(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  vtkAlgorithm* GetCurrentReader();

private:
  char* FileName;
  unsigned int CurrentMember;
  unsigned int CurrentMemberRange[2];

  class vtkInternal;
  vtkInternal* Internal;

  vtkEnsembleDataReader(const vtkEnsembleDataReader&) = delete;
  void operator=(const vtkEnsembleDataReader&) = delete;
};

#endif
