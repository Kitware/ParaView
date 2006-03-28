/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

/// \file pqPipelineServer.h
///
/// \date 11/16/2005

#ifndef _pqPipelineServer_h
#define _pqPipelineServer_h


#include "pqWidgetsExport.h"

class pqMultiView;
class pqPipelineObject;
class pqPipelineServerInternal;
class pqPipelineWindow;
class pqServer;
class QWidget;
class vtkPVXMLElement;
class vtkSMProxy;


class PQWIDGETS_EXPORT pqPipelineServer
{
public:
  pqPipelineServer();
  ~pqPipelineServer();

  void SaveState(vtkPVXMLElement *root, pqMultiView *multiView=0);

  void SetServer(pqServer *server) {this->Server = server;}
  pqServer *GetServer() const {return this->Server;}

  pqPipelineObject *AddSource(vtkSMProxy *source);
  pqPipelineObject *AddFilter(vtkSMProxy *filter);
  pqPipelineObject *AddCompoundProxy(vtkSMProxy *proxy);
  pqPipelineWindow *AddWindow(QWidget *window);

  pqPipelineObject *GetObject(vtkSMProxy *proxy) const;
  pqPipelineWindow *GetWindow(QWidget *window) const;

  bool RemoveObject(vtkSMProxy *proxy);
  bool RemoveWindow(QWidget *window);

  int GetSourceCount() const;
  pqPipelineObject *GetSource(int index) const;

  int GetWindowCount() const;
  pqPipelineWindow *GetWindow(int index) const;

  void ClearPipelines();

private:
  void UnregisterObject(pqPipelineObject *object);

private:
  pqPipelineServerInternal *Internal;
  pqServer *Server;
};

#endif
