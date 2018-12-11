/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCinemaDatabaseInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVCinemaDatabaseInformation
 * @brief provides information about a CinemaDatabase.
 *
 * vtkPVCinemaDatabaseInformation is a vtkPVInformation subclass that can be
 * used to gather information about a vtkCinemaDatabase instance. It is a
 * `RootOnly` information object, hence information is only gathered from the root
 * node in multi-rank configurations. vtkSMCinemaDatabaseImporter uses this
 * information object to collect what we know about pipelines objects and their
 * parameters. vtkSMCinemaDatabaseImporter can then create proxies for each of
 * the pipeline objects and add dynamic properties to match the control
 * parameters available for each pipeline object in the database.
 */

#ifndef vtkPVCinemaDatabaseInformation_h
#define vtkPVCinemaDatabaseInformation_h

#include "vtkPVCinemaReaderModule.h" // for export macros
#include "vtkPVInformation.h"
#include <map>    // needed for map
#include <string> // needed for string
#include <vector> // needed for vector

class VTKPVCINEMAREADER_EXPORT vtkPVCinemaDatabaseInformation : public vtkPVInformation
{
public:
  static vtkPVCinemaDatabaseInformation* New();
  vtkTypeMacro(vtkPVCinemaDatabaseInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void CopyFromObject(vtkObject*) override;
  void AddInformation(vtkPVInformation*) override;
  void CopyToStream(vtkClientServerStream*) override;
  void CopyFromStream(const vtkClientServerStream*) override;

  typedef std::vector<std::string> VectorOfStrings;
  typedef VectorOfStrings PipelineObjectsType;
  typedef std::map<std::string, VectorOfStrings> ControlParametersType;
  typedef std::map<std::string, VectorOfStrings> PipelineObjectParentsType;
  typedef std::map<std::string, bool> PipelineObjectVisibilitiesType;

  /**
   * Returns a list of the pipeline objects in the cinema database
   */
  const PipelineObjectsType& GetPipelineObjects() const { return this->PipelineObjects; }

  /**
   * Returns a map of control parameters.
   */
  const ControlParametersType& GetControlParameters() const { return this->ControlParameters; }

  /**
   * Returns the list of control parameter for a specific parameter.
   */
  const VectorOfStrings& GetControlParameters(const std::string& parameter) const;

  /**
   * Get values for a control parameter. For convenience they are returned as strings.
   */
  const VectorOfStrings& GetControlParameterValues(const std::string& parameter) const;

  /**
   * Returns if the pipeline object is visible by default in the database.
   */
  bool GetPipelineObjectVisibility(const std::string& object) const;

  /**
   * Returns a map with information about parents for each pipeline object.
   * Useful to setup pipeline connections.
   * @returns a map where key is the name of the pipeline object and value is a
   * vector of names of pipeline objects that are its parents i.e. upstream or
   * inputs.
   */
  const PipelineObjectParentsType& GetPipelineObjectParents() const
  {
    return this->PipelineObjectParents;
  }

  /**
   * Returns the list of parents for a particular object.
   */
  const VectorOfStrings& GetPipelineObjectParents(const std::string& object) const;

protected:
  vtkPVCinemaDatabaseInformation();
  ~vtkPVCinemaDatabaseInformation() override;

  void Reset();

  PipelineObjectsType PipelineObjects;
  ControlParametersType ControlParameters;
  ControlParametersType ControlParameterValues;
  PipelineObjectParentsType PipelineObjectParents;
  PipelineObjectVisibilitiesType PipelineObjectVisibilities;

private:
  vtkPVCinemaDatabaseInformation(const vtkPVCinemaDatabaseInformation&) = delete;
  void operator=(const vtkPVCinemaDatabaseInformation&) = delete;
};

#endif
