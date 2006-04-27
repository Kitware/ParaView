/*=========================================================================

   Program:   ParaQ
   Module:    pqPipelineSource.h

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

/// \file pqPipelineSource.h
/// \date 4/17/2006

#ifndef _pqPipelineSource_h
#define _pqPipelineSource_h


#include "pqWidgetsExport.h"
#include "pqPipelineObject.h"
#include <QString> // Needed for proxy name.

class pqPipelineDisplay;
class pqPipelineSourceInternal;
class vtkSMProxy;


class PQWIDGETS_EXPORT pqPipelineSource : public pqPipelineObject
{
public:
  pqPipelineSource(vtkSMProxy *proxy,
      pqPipelineModel::ItemType type=pqPipelineModel::Source);
  virtual ~pqPipelineSource();

  virtual void ClearConnections();

  const QString &GetProxyName() const {return this->ProxyName;}
  void SetProxyName(const QString &name) {this->ProxyName = name;}

  vtkSMProxy *GetProxy() const {return this->Proxy;}
  void SetProxy(vtkSMProxy *proxy);

  pqPipelineDisplay *GetDisplay() const {return this->Display;}

  int GetOutputCount() const;
  pqPipelineObject *GetOutput(int index) const;
  int GetOutputIndexFor(pqPipelineObject *output) const;
  bool HasOutput(pqPipelineObject *output) const;

  void AddOutput(pqPipelineObject *output);
  void InsertOutput(int index, pqPipelineObject *output);
  void RemoveOutput(pqPipelineObject *output);

private:
  pqPipelineSourceInternal *Internal; ///< Stores the output connections.
  pqPipelineDisplay *Display;         ///< Stores the display proxies;
  QString ProxyName;                  ///< Stores the proxy name.
  vtkSMProxy *Proxy;                  ///< Stores the proxy pointer.
};

#endif
