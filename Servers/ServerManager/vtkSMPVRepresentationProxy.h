/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPVRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPVRepresentationProxy - a composite representation proxy suitable
// for showing data in ParaView.
// .SECTION Description
// vtkSMPVRepresentationProxy combines surface representation and volume
// representation proxies typically used for displaying data.
// This class also takes over the selection obligations for all the internal
// representations, i.e. is disables showing of selection in all the internal
// representations, and manages it. This avoids duplicate execution of extract
// selection filter for each of the internal representations.

#ifndef __vtkSMPVRepresentationProxy_h
#define __vtkSMPVRepresentationProxy_h

#include "vtkSMRepresentationProxy.h"

class VTK_EXPORT vtkSMPVRepresentationProxy : public vtkSMRepresentationProxy
{
public:
  static vtkSMPVRepresentationProxy* New();
  vtkTypeMacro(vtkSMPVRepresentationProxy, vtkSMRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the type of representation.
  virtual void SetRepresentation(int type);
  vtkGetMacro(Representation, int);

  // Description:
  // Overridden to setup the "Representation" property's value correctly since
  // it changes based on plugins loaded at run-time.
  virtual int LoadState(vtkPVXMLElement* element, vtkSMProxyLocator* locator);

//BTX
  enum RepresentationType
    {
    POINTS=0,
    WIREFRAME=1,
    SURFACE=2,
    OUTLINE=3,
    VOLUME=4,
    SURFACE_WITH_EDGES=5,
    SLICE=6,
    USER_DEFINED=100,
    // Special identifiers for back faces.
    FOLLOW_FRONTFACE=400,
    CULL_BACKFACE=401,
    CULL_FRONTFACE=402
    };

  // Description:
  // vtkPVCompositeRepresentation has two input ports one of the data, and
  // another for the extracted selection. The data-input is exposed from the
  // proxy, while the extracted selection input is hidden and implicitly defined
  // based on the data input. We override this method to handle that.
  virtual void AddInput(unsigned int inputPort,
    vtkSMSourceProxy* input, unsigned int outputPort, const char* method);
  virtual void AddInput(vtkSMSourceProxy* input, const char* method)
    { this->Superclass::AddInput(input, method); }

protected:
  vtkSMPVRepresentationProxy();
  ~vtkSMPVRepresentationProxy();

  // Description:
  virtual void CreateVTKObjects();

  // Description:
  // Read attributes from an XML element.
  virtual int CreateSubProxiesAndProperties(vtkSMProxyManager* pm,
    vtkPVXMLElement *element);

  int Representation;
private:
  vtkSMPVRepresentationProxy(const vtkSMPVRepresentationProxy&); // Not implemented
  void operator=(const vtkSMPVRepresentationProxy&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif

