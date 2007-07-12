/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDataLabelRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMDataLabelRepresentationProxy 
// .SECTION Description

#ifndef __vtkSMDataLabelRepresentationProxy_h
#define __vtkSMDataLabelRepresentationProxy_h

#include "vtkSMDataRepresentationProxy.h"

class vtkSMRenderModuleProxy;
class vtkSMSourceProxy;
class vtkUnstructuredGrid;

class VTK_EXPORT vtkSMDataLabelRepresentationProxy : public vtkSMDataRepresentationProxy
{
public:
  static vtkSMDataLabelRepresentationProxy* New();
  vtkTypeRevisionMacro(vtkSMDataLabelRepresentationProxy, vtkSMDataRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the input. 
  virtual void AddInput(unsigned int inputPort,
                        vtkSMSourceProxy* input,
                        unsigned int outputPort,
                        const char* method);
  virtual void AddInput(vtkSMSourceProxy* input,
                        const char* method)
  {
    this->AddInput(0, input, 0, method);
  }

  // Description:
  // Called when a representation is added to a view. 
  // Returns true on success.
  // Currently a representation can be added to only one view.
  virtual bool AddToView(vtkSMViewProxy* view);

  // Description:
  // Called to remove a representation from a view.
  // Returns true on success.
  // Currently a representation can be added to only one view.
  virtual bool RemoveFromView(vtkSMViewProxy* view);

  // Description:
  // This method updates the piece that has been assigned to this process.
  // Leads to a call to ForceUpdate on UpdateSuppressorProxy 
  // GeometryIsValid==0;
  virtual void Update(vtkSMViewProxy* view);
  virtual void Update() { this->Update(0); }

  // Description:
  // Set the update time passed on to the update suppressor.
  virtual void SetUpdateTime(double time);

  //BTX
  // Description:
  // The Pick needs access to this to fill in the UI point values.
  // TODO: I have to find a means to get rid of this!!
  vtkUnstructuredGrid* GetCollectedData();
  //ETX
  
  // Description:
  // Accessors to the font size in the sub proxy.
  void SetFontSizeCM(int size);
  int GetFontSizeCM();

  // Description:
  // Overridden to make the Strategy modified as well.
  // The strategy is not marked modified if the modifiedProxy == this, 
  // thus if the changes to representation itself invalidates the data pipelines
  // it must explicitly mark the strategy invalid.
  virtual void MarkModified(vtkSMProxy* modifiedProxy);

  // Description:
  // Get Visibility of the representation
  // Return true, if both cell and point labels are invisible; 
  //        false, if either is visible
  virtual bool GetVisibility();

  // Description:
  // Set Visibility on Cell labels actor 
  virtual void SetCellLabelVisibility(int);

  // Description:
  // Set Visibility on Point labels actor
  virtual void SetPointLabelVisibility(int);

protected:
  vtkSMDataLabelRepresentationProxy();
  ~vtkSMDataLabelRepresentationProxy();

  void SetupPipeline();
  void SetupDefaults();

  // Description:
  // This method is called at the beginning of CreateVTKObjects().
  // This gives the subclasses an opportunity to set the servers flags
  // on the subproxies.
  // If this method returns false, CreateVTKObjects() is aborted.
  virtual bool BeginCreateVTKObjects();

  // Description:
  // This method is called after CreateVTKObjects(). 
  // This gives subclasses an opportunity to do some post-creation
  // initialization.
  virtual bool EndCreateVTKObjects();

  // Description:
  // Marks for Update.
  virtual void InvalidateGeometryInternal(int useCache);

  void SetInputInternal (vtkSMSourceProxy* input, unsigned int outputPort);

  vtkSMSourceProxy* CollectProxy;
  vtkSMProxy* UpdateSuppressorProxy;
  vtkSMProxy* MapperProxy;
  vtkSMProxy* ActorProxy;
  vtkSMProxy* TextPropertyProxy;

  vtkSMSourceProxy* CellCenterFilter;
  vtkSMProxy* CellTextPropertyProxy;
  vtkSMProxy* CellMapperProxy;
  vtkSMProxy* CellActorProxy;

  int GeometryIsValid;

private:
  vtkSMDataLabelRepresentationProxy(const vtkSMDataLabelRepresentationProxy&); // Not implemented.
  void operator=(const vtkSMDataLabelRepresentationProxy&); // Not implemented.
};


#endif
