/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNetworkImageSourceProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMNetworkImageSourceProxy - proxy for an image reader that can read
// an image file from any process and make the data available on all processes.
// .SECTION Description
// vtkSMNetworkImageSourceProxy is a proxy for an image reader that can read
// an image file from any process and make the data available on all rendering 
// processes i.e. render server and client.
// It is used to load small images such as those used as textures.

#ifndef __vtkSMNetworkImageSourceProxy_h
#define __vtkSMNetworkImageSourceProxy_h

#include "vtkSMSourceProxy.h"
#include "vtkProcessModule.h" //for flags.
class VTK_EXPORT vtkSMNetworkImageSourceProxy : public vtkSMSourceProxy
{
public:
  static vtkSMNetworkImageSourceProxy* New();
  vtkTypeMacro(vtkSMNetworkImageSourceProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the file name of the image to load.
  void SetFileName(const char*);
  vtkGetStringMacro(FileName);

  //BTX
  enum {
    CLIENT = vtkProcessModule::CLIENT,
    DATA_SERVER_ROOT = vtkProcessModule::DATA_SERVER_ROOT,
    RENDER_SERVER_ROOT = vtkProcessModule::RENDER_SERVER_ROOT
  };
  //ETX
  
  // Description:
  // Set the process on which the image is available.
  void SetSourceProcess(int);
  void SetSourceProcessToClient()
    { this->SetSourceProcess(CLIENT); }
  void SetSourceProcessToDataServerRoot()
    { this->SetSourceProcess(DATA_SERVER_ROOT); }
  void SetSourceProcessToRenderServerRoot()
    { this->SetSourceProcess(RENDER_SERVER_ROOT); }

  virtual void UpdateVTKObjects()
    { this->Superclass::UpdateVTKObjects(); }


//BTX
protected:
  vtkSMNetworkImageSourceProxy();
  ~vtkSMNetworkImageSourceProxy();

  void UpdateImage();
  virtual void ReviveVTKObjects();
  virtual void UpdateVTKObjects(vtkClientServerStream& stream);

  char* FileName;
  int SourceProcess;
  bool ForceNoUpdates;
  bool UpdateNeeded;
private:
  vtkSMNetworkImageSourceProxy(const vtkSMNetworkImageSourceProxy&); // Not implemented
  void operator=(const vtkSMNetworkImageSourceProxy&); // Not implemented
//ETX
};

#endif

