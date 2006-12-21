/*=========================================================================

   Program: ParaView
   Module:    pqDisplayPolicy.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#ifndef __pqDisplayPolicy_h
#define __pqDisplayPolicy_h

#include <QObject>
#include "pqCoreExport.h" // Needed for PQCORE_EXPORT macro

class pqPipelineSource;
class pqGenericViewModule;
class vtkSMProxy;

// Display policy defines the application specific policy
// for creating display proxies. Given a pair of a proxy to be displayed 
// and a view proxy in which to display, this class must tell the type 
// of display to create, if any. Custom applications can subclass 
// this to define their own policy. The pqApplicationCore maintains
// an instance of the policy used by the application. Custom applications
// should set their own policy instance on the global application core 
// instance.
class PQCORE_EXPORT pqDisplayPolicy : public QObject
{
  Q_OBJECT
public:
  pqDisplayPolicy(QObject* p);
  virtual ~pqDisplayPolicy();

  // Returns if the given source can be displayed
  // in the given view.
  virtual bool canDisplay(
    const pqPipelineSource* source, const pqGenericViewModule* view) const;

  // Returns a new instance of a display proxy for
  // the given (source,view) pair. The display proxy is not initialized at all.
  // This method will return NULL if canDisplay(source,view) returns false.
  virtual vtkSMProxy* newDisplay(
    const pqPipelineSource* source, const pqGenericViewModule* view) const;

protected:
  pqDisplayPolicy(const pqDisplayPolicy&); // Not implemented.
  void operator=(const pqDisplayPolicy&); // Not implemented.
};

#endif

