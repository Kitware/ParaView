// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkVRInteractorStyleFactory_h
#define vtkVRInteractorStyleFactory_h

#include "vtkCommand.h" // For UserEvent
#include "vtkObject.h"

#include <string>
#include <vector>

class vtkSMVRInteractorStyleProxy;

class vtkVRInteractorStyleFactory : public vtkObject
{
public:
  static vtkVRInteractorStyleFactory* New();
  vtkTypeMacro(vtkVRInteractorStyleFactory, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // Get the singleton instance of this class
  static vtkVRInteractorStyleFactory* GetInstance();

  // Description:
  // Get the list of interactor style classes
  std::vector<std::string> GetInteractorStyleClassNames();

  // Description:
  // Get a list of action descriptions for the styles (e.g. "Grab", "Track",
  //   etc).
  std::vector<std::string> GetInteractorStyleDescriptions();

  // Description:
  // Get the action description for a style classname
  std::string GetDescriptionFromClassName(const std::string& className);

  // Description:
  // Create a new interactor style instance. The input string
  //   must be in the vector returned by GetInteractorStyleClassNames().
  vtkSMVRInteractorStyleProxy* NewInteractorStyleFromClassName(const std::string&);

  // Description:
  // Create a new interactor style instance. The input string
  //   must be in the vector returned by GetInteractorStyleDescriptions().
  vtkSMVRInteractorStyleProxy* NewInteractorStyleFromDescription(const std::string&);

  enum
  {
    INTERACTOR_STYLES_UPDATED = vtkCommand::UserEvent + 7369
  };

  friend class pqVRStarter;

protected:
  vtkVRInteractorStyleFactory();
  ~vtkVRInteractorStyleFactory();

  static void SetInstance(vtkVRInteractorStyleFactory*);
  static vtkVRInteractorStyleFactory* Instance;

  std::vector<std::string> InteractorStyleClassNames; // store the name of each Interactor class
  std::vector<std::string>
    InteractorStyleDescriptions; // store a short description of each Interactor
  std::vector<std::string> InteractorStyleNewMethods; // store the New() method of each Interactor
                                                      // // WRS-TODO: this was deleted in "Kitware"
                                                      // version.  Why?
  void Initialize();
  bool Initialized;
};

#endif // vtkVRInteractorStyleFactory_h
