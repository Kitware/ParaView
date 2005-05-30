/*=========================================================================

  Program:   ParaView
  Module:    vtkPVComparativeVis.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVComparativeVis
// .SECTION Description

#ifndef __vtkPVComparativeVis_h
#define __vtkPVComparativeVis_h

#include "vtkObject.h"

class vtkPVAnimationCue;
class vtkPVApplication;
class vtkSMProxy;
//BTX
struct vtkPVComparativeVisInternals;
//ETX

class VTK_EXPORT vtkPVComparativeVis : public vtkObject
{
public:
  static vtkPVComparativeVis* New();
  vtkTypeRevisionMacro(vtkPVComparativeVis, vtkObject);
  void PrintSelf(ostream& os ,vtkIndent indent);

  void SetApplication(vtkPVApplication*);

  // Description:
  // Given properties, generate comparative visualization. Call this
  // once. To create a new comparative vis., create a new vtkPVComparativeVis.
  // To delete all caches, displays etc., delete vtkPVComparativeVis
  void Generate();

  // Description:
  // Returns true if comparative vis cannot be showed without
  // calling Generate().
  vtkGetMacro(IsGenerated, int);

  // Description:
  // Put all displays on the window. Returns 0 on failure.
  int Show();

  // Description:
  // Remove all displays from the window.
  void Hide();

  // Description:
  // Add the property associated with the given cue.
  void AddProperty(vtkPVAnimationCue* acue, vtkSMProxy* cue, int numValues);

  // Description:
  // Removes all properties and initializes internal data structures.
  void RemoveAllProperties();

  // Description:
  // Delete all cached data: geometry, displays etc...
  void Initialize();

  // Description:
  // Returns the number of assigned properties
  unsigned int GetNumberOfProperties();

  // Description:
  // Returns the cue associated with a property.
  vtkPVAnimationCue* GetAnimationCue(unsigned int idx);

  // Description:
  // MultiActorHelper stores pointer to all actors in the scene.
  // The interactor later transforms these.
  vtkGetObjectMacro(MultiActorHelper, vtkSMProxy);

  // Description:
  // Set/Get the comparative vis name. Used by the comparative vis
  // manager.
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);

protected:
  vtkPVComparativeVis();
  ~vtkPVComparativeVis();

  vtkPVApplication* Application;
  vtkPVComparativeVisInternals* Internal;

  // Create all the geometry for a property (and all the properties
  // after it). Call PlayOne(0) to create the geometry for all.
  void PlayOne(unsigned int idx);

  // Set the number of values for a given property
  void SetNumberOfPropertyValues(unsigned int idx, unsigned int numValues);

  // Create geometry caches and displays for one case (i.e. fixed set 
  // of property values)
  void StoreGeometry();

  // All geometry caches and displays are stored consecutively in a vector.
  // To figure out which property indices an entry in the vector corresponds
  // to, call ComputeIndices(entryIdx). After this call, 
  // Internal->Indices vector will contain the right indices pointing to
  // the properties
  void ComputeIndices(unsigned int gidx);
  void ComputeIndex(unsigned int paramIdx, unsigned int gidx);

  void ExecuteEvent(vtkObject* , unsigned long event, unsigned int paramIdx);

  // Gather two bounds (result is stored in the second argument)
  static void AddBounds(double bounds[6], double totalB[6]);

  vtkSMProxy* MultiActorHelper;

  char* Name;

  int InFirstShow;

  int IsGenerated;

//BTX
  friend class vtkCVAnimationSceneObserver;
//ETX

private:
  vtkPVComparativeVis(const vtkPVComparativeVis&); // Not implemented.
  void operator=(const vtkPVComparativeVis&); // Not implemented.
};

#endif

