/*=========================================================================

   Program:   ParaQ
   Module:    pqPipelineObject.h

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

/// \file pqPipelineObject.h
///
/// \date 11/16/2005

#ifndef _pqPipelineObject_h
#define _pqPipelineObject_h


#include "pqWidgetsExport.h"
#include <QString> // Needed for proxy name.

class pqPipelineObjectInternal;
class pqPipelineWindow;
class vtkSMDisplayProxy;
class vtkSMProxy;


class PQWIDGETS_EXPORT pqPipelineObject
{
public:
  enum ObjectType {
    Source,
    Filter,
    Bundle
  };

public:
  pqPipelineObject(vtkSMProxy *proxy, ObjectType type);
  ~pqPipelineObject();

  ObjectType GetType() const {return this->Type;}
  void SetType(ObjectType type) {this->Type = type;}

  const QString &GetProxyName() const {return this->ProxyName;}
  void SetProxyName(const QString &name) {this->ProxyName = name;}

  vtkSMProxy *GetProxy() const {return this->Proxy;}
  void SetProxy(vtkSMProxy *proxy) {this->Proxy = proxy;}

  // TODO: Store the display proxy name.
  vtkSMDisplayProxy *GetDisplayProxy() const {return this->Display;}
  void SetDisplayProxy(vtkSMDisplayProxy *display) {this->Display = display;}

  pqPipelineWindow *GetParent() const {return this->Window;}
  void SetParent(pqPipelineWindow *parent) {this->Window = parent;}

  /// \name Connection Methods
  //@{
  int GetInputCount() const;
  pqPipelineObject *GetInput(int index) const;
  bool HasInput(pqPipelineObject *input) const;

  void AddInput(pqPipelineObject *input);
  void RemoveInput(pqPipelineObject *input);

  int GetOutputCount() const;
  pqPipelineObject *GetOutput(int index) const;
  bool HasOutput(pqPipelineObject *output) const;

  void AddOutput(pqPipelineObject *output);
  void RemoveOutput(pqPipelineObject *output);

  void ClearConnections();
  //@}

private:
  pqPipelineObjectInternal *Internal; ///< Stores the object connections.
  vtkSMDisplayProxy *Display;         ///< Stores the display proxy;
  ObjectType Type;                    ///< Stores the object type.
  QString ProxyName;                  ///< Stores the proxy name.
  vtkSMProxy *Proxy;                  ///< Stores the proxy pointer.
  pqPipelineWindow *Window;           ///< Stores the parent window.
};

#endif
