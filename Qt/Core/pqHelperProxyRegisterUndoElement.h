/*=========================================================================

   Program: ParaView
   Module:    pqHelperProxyRegisterUndoElement.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
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
#ifndef __pqHelperProxyRegisterUndoElement_h
#define __pqHelperProxyRegisterUndoElement_h

#include "vtkSMUndoElement.h"
#include "pqCoreExport.h"

class pqProxy;

/// This is a special undo element that gets added after a proxy 
/// (referred to as "source" proxy) and all its
/// helpers have been registered.
/// AND before it's proxy and all its helpers have been UNregistered.
/// On undo, this element does nothing since the creation of 
/// helper proxies will automatically be undone before the undo for creation 
/// of the "source" proxy. However, on redo,
/// there is no means for the "source" proxy to be made aware of the helper 
/// proxies which will get registered after the "source". That's where this 
/// element comes handy. It sets the proxies as helpers on the "source" proxy. 
/// For this to work, application must push this element on the stack after 
/// the "source" proxy and all its helpers have been registered.
class PQCORE_EXPORT pqHelperProxyRegisterUndoElement : public vtkSMUndoElement
{
public:
  static pqHelperProxyRegisterUndoElement* New();
  vtkTypeMacro(pqHelperProxyRegisterUndoElement, vtkSMUndoElement);
  void PrintSelf(ostream& os, vtkIndent indent);
  void SetOperationTypeToUndo() { this->ForUndo = true; }
  void SetOperationTypeToRedo() { this->ForUndo = false; }

  /// Description:
  /// Undo the operation encapsulated by this element.
  /// We make the pqProxy aware of its helper proxies.
  virtual int Undo() { return this->ForUndo ? this->DoTheJob() : 1; }

  /// Description:
  /// Redo the operation encaspsulated by this element.
  /// We make the pqProxy aware of its helper proxies.
  virtual int Redo() { return this->ForUndo ? 1 : this->DoTheJob(); }

  /// Description:
  /// Creates the element to setup helper proxies for the proxy.
  void RegisterHelperProxies(pqProxy* proxy);

protected:
  pqHelperProxyRegisterUndoElement();
  ~pqHelperProxyRegisterUndoElement();

  virtual int DoTheJob();
  bool ForUndo;

private:
  pqHelperProxyRegisterUndoElement(const pqHelperProxyRegisterUndoElement&); // Not implemented.
  void operator=(const pqHelperProxyRegisterUndoElement&); // Not implemented.

  struct vtkInternals;
  vtkInternals* Internal;
};

#endif


