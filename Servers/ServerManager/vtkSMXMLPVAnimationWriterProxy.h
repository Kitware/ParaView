/*=========================================================================

  Program:   ParaView
  Module:    vtkSMXMLPVAnimationWriterProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMXMLPVAnimationWriterProxy - ServerManager object to write out animation
// geometry.
// .SECTION Description
//

#ifndef __vtkSMXMLPVAnimationWriterProxy_h
#define __vtkSMXMLPVAnimationWriterProxy_h

#include "vtkSMSourceProxy.h"
class vtkSMXMLPVAnimationWriterProxyInternals;
class vtkSMSummaryHelperProxy;

class VTK_EXPORT vtkSMXMLPVAnimationWriterProxy : public vtkSMSourceProxy
{
public:
  static vtkSMXMLPVAnimationWriterProxy* New();
  vtkTypeMacro(vtkSMXMLPVAnimationWriterProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Connects filters/sinks to an input. If the filter(s) is not
  // created, this will create it. 
  virtual void AddInput(unsigned int inputPort,
                        vtkSMSourceProxy* input,
                        unsigned int outputPort,
                        const char* method);
  virtual void AddInput(vtkSMSourceProxy* input,
                        const char* method)
  {
    this->AddInput(0, input, 0, method);
  }

  void WriteTime(double time);

  void Start();
  void Finish();

  vtkGetMacro(ErrorCode, int);
protected:
  vtkSMXMLPVAnimationWriterProxy();
  ~vtkSMXMLPVAnimationWriterProxy();
  
  virtual void CreateVTKObjects();
  int ErrorCode;

//BTX
  friend class vtkSMXMLPVAnimationWriterProxyInternals;
  vtkSMXMLPVAnimationWriterProxyInternals* Internals;
//ETX

private:
  vtkSMXMLPVAnimationWriterProxy(const vtkSMXMLPVAnimationWriterProxy&); // Not implemented.
  void operator=(const vtkSMXMLPVAnimationWriterProxy&); // Not implemented.
};

#endif

