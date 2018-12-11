/*=========================================================================

  Program:   ParaView
  Module:    vtkCinemaDatabaseReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkCinemaDatabaseReader
 * @brief reader for a Cinema database.
 *
 * vtkCinemaDatabaseReader readers Cinema database files. It produces an output
 * polydata, which is only has relevant metadata used by
 * vtkCinemaLayerRepresentation and vtkCinemaLayerMapper to render cinema layers
 * in a Render View.
 */

#ifndef vtkCinemaDatabaseReader_h
#define vtkCinemaDatabaseReader_h

#include "vtkNew.h"                  // for vtkNew
#include "vtkPVCinemaReaderModule.h" // for export macros
#include "vtkPolyDataAlgorithm.h"

#include <map>    // needed for ivars
#include <set>    // needed for ivars
#include <string> // needed for ivars

class vtkCinemaDatabase;
class VTKPVCINEMAREADER_EXPORT vtkCinemaDatabaseReader : public vtkPolyDataAlgorithm
{
public:
  static vtkCinemaDatabaseReader* New();
  vtkTypeMacro(vtkCinemaDatabaseReader, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set the filename for the index file for the database (typically the
   * info.json).
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Select which pipeline object from the cinema database does this reader
   * read.
   */
  vtkSetStringMacro(PipelineObject);
  vtkGetStringMacro(PipelineObject);
  //@}

  //@{
  /**
   * API to select values for control parameter that form the query used to
   * obtain layer produced by this reader.
   */
  void ClearControlParameter(const char* pname);
  void EnableControlParameterValue(const char* pname, const char* value);
  void EnableControlParameterValue(const char* pname, double value);
  //@}

protected:
  vtkCinemaDatabaseReader();
  ~vtkCinemaDatabaseReader() override;

  int RequestInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outVector) override;
  int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outVector) override;

  /**
   * Builds query string
   */
  std::string GetQueryString(double time) const;

  char* FileName;
  std::string OldFileName;
  char* PipelineObject;

  typedef std::set<std::string> SetOfStrings;
  typedef std::map<std::string, SetOfStrings> MapOfVectorOfString;

  MapOfVectorOfString EnabledControlParameterValues;

private:
  vtkCinemaDatabaseReader(const vtkCinemaDatabaseReader&) = delete;
  void operator=(const vtkCinemaDatabaseReader&) = delete;

  vtkNew<vtkCinemaDatabase> Helper;

  typedef std::map<double, std::string> TimeStepsMapType;
  TimeStepsMapType TimeStepsMap;
};

#endif
