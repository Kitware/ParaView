/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCinemaDatabase.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkCinemaDatabase
 * @brief class that provides access to `cinema_python.database.file_store`
 * API.
 *
 * vtkCinemaDatabase is an abstraction that provides access to a
 * `cinema_python.database.file_store.FileStore` instance. The API is
 * limited to the functionality needed for the rendering Cinema layers in
 *  ParaView.
 */

#ifndef vtkCinemaDatabase_h
#define vtkCinemaDatabase_h

#include "vtkObject.h"
#include "vtkPVCinemaReaderModule.h" // for export macros
#include "vtkSmartPointer.h"         // for vtkSmartPointer

#include <string> // for string
#include <vector> // for vector

class vtkImageData;
class vtkCamera;

class VTKPVCINEMAREADER_EXPORT vtkCinemaDatabase : public vtkObject
{
public:
  static vtkCinemaDatabase* New();
  vtkTypeMacro(vtkCinemaDatabase, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum Spec
  {
    UNKNOWN = -1,
    CINEMA_SPEC_A,
    CINEMA_SPEC_C
  };

  /**
   * Loads the cinema database.
   * @param[in] fname path to the `info.json` file corresponding to a Cinema store.
   * @returns true on success
   */
  bool Load(const char* fname);

  /**
   * Returns a list of objects in the **vis** layer in the database. This
   * typically corresponds to pipeline objects in visualization.
   */
  std::vector<std::string> GetPipelineObjects() const;

  /**
   * Returns names of parent for a pipeline object.
   */
  std::vector<std::string> GetPipelineObjectParents(const std::string& objectname) const;

  /**
   * Returns a pipeline objects default visibility, as recorded in the database.
   */
  bool GetPipelineObjectVisibility(const std::string& objectname) const;

  // Description:
  // Given a object's name, it returns a list of control parameters (may be empty).
  std::vector<std::string> GetControlParameters(const std::string& objectname) const;

  // Description:
  // Get values for a control parameter. For convenience they are returned as
  // strings.
  std::vector<std::string> GetControlParameterValues(const std::string& parameter) const;

  // Description:
  // Get the name for the field associated with the object. `GetFieldValues` will
  // returns values for this field.
  std::string GetFieldName(const std::string& objectname) const;

  // Description:
  // Given an object's name, returns a list of fields available for that object.
  std::vector<std::string> GetFieldValues(
    const std::string& objectname, const std::string& valuetype) const;

  /**
   * Gets the range for a field value.
   * @param[in] objectname name of the pipeline object
   * @param[in] fieldvalue name of the field value
   * @param[out] range range for the field value
   * @returns true if range was available.
   */
  bool GetFieldValueRange(
    const std::string& objectname, const std::string& fieldvalue, double range[2]) const;

  // Description:
  // Return timesteps available in the datastore. May returns an empty vector if
  // no timesteps are present in the dataset.
  std::vector<std::string> GetTimeSteps() const;

  /**
   * Get the layers for a specific query.
   */
  std::vector<vtkSmartPointer<vtkImageData> > TranslateQuery(const std::string& query) const;

  /**
   * Get cameras
   */
  std::vector<vtkSmartPointer<vtkCamera> > Cameras(
    const std::string& timestep = std::string()) const;

  /**
   * Gets the spec used by the database.
   * @return value from Spec enum.
   */
  int GetSpec() const;

  /**
   * Gets the nearest value in parameter list as a string.
   * Values should be double and in ascending order.
   * @param param parameter name
   * @param value input data
   * @return the nearest value as string
   */
  std::string GetNearestParameterValue(const std::string& param, double value) const;

protected:
  vtkCinemaDatabase();
  ~vtkCinemaDatabase() override;

private:
  vtkCinemaDatabase(const vtkCinemaDatabase&) = delete;
  void operator=(const vtkCinemaDatabase&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
