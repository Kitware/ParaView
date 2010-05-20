/*=========================================================================

  Program:   ParaView
  Module:    vtkSMGlyphRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMGlyphRepresentationProxy - representation to show images.
// .SECTION Description

#ifndef __vtkSMGlyphRepresentationProxy_h
#define __vtkSMGlyphRepresentationProxy_h

//#include "vtkSMScatterPlotRepresentationProxy.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkStdString.h" // needed for vtkStdString.

class vtkSMNetDMfViewProxy;

//class VTK_EXPORT vtkSMGlyphRepresentationProxy : public vtkSMScatterPlotRepresentationProxy
class VTK_EXPORT vtkSMGlyphRepresentationProxy : public vtkSMPVRepresentationProxy
{
public:
  static vtkSMGlyphRepresentationProxy* New();
  //vtkTypeMacro(vtkSMGlyphRepresentationProxy, vtkSMScatterPlotRepresentationProxy);
  vtkTypeMacro(vtkSMGlyphRepresentationProxy, vtkSMPVRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void AddInput(unsigned int inputPort,
                        vtkSMSourceProxy* input,
                        unsigned int outputPort,
                        const char* method);
  virtual void AddInput(vtkSMSourceProxy* input,
                        const char* method)
  {
    this->AddInput(0, input, 0, method);
  }

  bool InputTypeIsA(const char* type);
  int GetGlyphRepresentation();

  void SetGlyphInput(vtkSMSourceProxy* rep);

//BTX
protected:
  // Description:
  // Protected constructor. Call vtkSMGlyphRepresentationProxy::New() to 
  // create an instance of vtkSMGlyphRepresentationProxy.
  vtkSMGlyphRepresentationProxy();
  
  // Description:
  // Protected destructor.
  virtual ~vtkSMGlyphRepresentationProxy();
  
  virtual void SetCubeAxesVisibility(int visible);
  
  vtkSMDataRepresentationProxy* GetRepresentationProxy(int repr);
  
private:
  vtkSMGlyphRepresentationProxy(const vtkSMGlyphRepresentationProxy&); // Not implemented
  void operator=(const vtkSMGlyphRepresentationProxy&); // Not implemented
//ETX
};

#endif

