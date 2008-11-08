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


  //int GetStreamedPasses();

  // Description:
  // Starting from the given source proxy, the method searches
  // up the pipeline (using vtkSMInputProperty) looking
  // for a reader source that reports the number of passes.
  // If more than one reader source is found, this method will
  // return the minimum number of passes reported by the readers.
  // If no readers are found, this method will return 0.
  virtual int GetPassesFromSource(vtkSMSourceProxy*);

  static const char* GetInstanceName();

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

  bool StreamedPassesNeedsUpdate;
  int GetStreamedPasses(bool recompute);

  static int StreamingFactoryRegistered;

//BTX
  class vtkSMStreamingHelperObserver;
  friend class vtkSMStreamingHelperObserver;
//ETX

  vtkSMStreamingHelperObserver* Observer;
  // Description:
  // Event handler.
  virtual void ExecuteEvent(vtkObject* called, unsigned long eventid, void* data);
  // Description:
  // Handler for specific vtkSMProxyManager events.
  virtual void OnRegisterProxy(const char* group, const char* name, vtkSMProxy*);
  virtual void OnUnRegisterProxy(const char* group, const char* name, vtkSMProxy*);


private:
  vtkSMStreamingHelperProxy(const vtkSMStreamingHelperProxy&); // Not implemented
  void operator=(const vtkSMStreamingHelperProxy&); // Not implemented

//BTX
  class vtkInternal;
  vtkInternal* Internal;
//ETX
};

#endif

