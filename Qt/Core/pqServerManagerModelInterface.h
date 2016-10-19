/*=========================================================================

   Program: ParaView
   Module:    pqServerManagerModelInterface.h

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

========================================================================*/
#ifndef pqServerManagerModelInterface_h
#define pqServerManagerModelInterface_h

#include "pqCoreModule.h"
#include <QtPlugin>

class vtkSMProxy;
class pqServer;
class pqProxy;

/**
* pqServerManagerModelInterface defines an interface that can be used to
* register new types of pqProxy subclasses to create when a vtkSMProxy is
* registered with the ProxyManager in a ParaView application.
*/
class PQCORE_EXPORT pqServerManagerModelInterface
{
public:
  pqServerManagerModelInterface();
  virtual ~pqServerManagerModelInterface();

  /**
  * Creates a pqProxy subclass for the vtkSMProxy given the details for its
  * registration with the proxy manager.
  * \arg \c regGroup - registration group for the proxy.
  * \arg \c regName  - registration name for the proxy.
  * \arg \c proxy    - vtkSMProxy instance to create the pqProxy for.
  * \arg \c server   - pqServer instance on which the proxy is present.
  */
  virtual pqProxy* createPQProxy(
    const QString& regGroup, const QString& regName, vtkSMProxy* proxy, pqServer* server) const = 0;
};

Q_DECLARE_INTERFACE(pqServerManagerModelInterface, "com.kitware/paraview/servermanagermodel")

#endif
