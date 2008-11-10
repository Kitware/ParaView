/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStreamingHelperProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMStreamingHelperProxy - a proxy to help streaming objects
// .SECTION Description

#ifndef __vtkSMStreamingHelperProxy_h
#define __vtkSMStreamingHelperProxy_h

#include "vtkSMProxy.h"

class vtkSMSourceProxy;

class VTK_EXPORT vtkSMStreamingHelperProxy : public vtkSMProxy
{
public:
  static vtkSMStreamingHelperProxy* New();
  vtkTypeRevisionMacro(vtkSMStreamingHelperProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the instance name used to register the helper proxy with
  // the proxy manager.
  static const char* GetInstanceName();

  // Description:
  // Get the streaming helper proxy.
  static vtkSMStreamingHelperProxy* GetHelper();

  vtkGetMacro(StreamedPasses, int);
  vtkSetMacro(StreamedPasses, int);

  vtkGetMacro(EnableStreamMessages, bool);
  vtkSetMacro(EnableStreamMessages, bool);

  vtkGetMacro(UseCulling, bool);
  vtkSetMacro(UseCulling, bool);

  vtkGetMacro(UseViewOrdering, bool);
  vtkSetMacro(UseViewOrdering, bool);

  vtkGetMacro(PieceCacheLimit, int);
  vtkSetMacro(PieceCacheLimit, int);

  vtkGetMacro(PieceRenderCutoff, int);
  vtkSetMacro(PieceRenderCutoff, int);


protected:
  vtkSMStreamingHelperProxy();
  ~vtkSMStreamingHelperProxy();

  int StreamedPasses;
  bool EnableStreamMessages;
  bool UseCulling;
  bool UseViewOrdering;
  int PieceCacheLimit;
  int PieceRenderCutoff;

  static int StreamingFactoryRegistered;

private:
  vtkSMStreamingHelperProxy(const vtkSMStreamingHelperProxy&); // Not implemented
  void operator=(const vtkSMStreamingHelperProxy&); // Not implemented

//BTX
  class vtkInternal;
  vtkInternal* Internal;
//ETX
};

#endif

