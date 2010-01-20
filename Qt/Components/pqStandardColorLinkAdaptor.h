/*=========================================================================

   Program: ParaView
   Module:    pqStandardColorLinkAdaptor.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
#ifndef __pqStandardColorLinkAdaptor_h 
#define __pqStandardColorLinkAdaptor_h

#include <QObject>
#include "pqComponentsExport.h"

class vtkEventQtSlotConnect;
class pqStandardColorButton;
class vtkSMProxy;

/// pqStandardColorLinkAdaptor is an adaptor used to connect the
/// pqStandardColorButton with the property ensuring links with global
/// properties are made and broken as and when needed.
class PQCOMPONENTS_EXPORT pqStandardColorLinkAdaptor : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
public:
  pqStandardColorLinkAdaptor(
    pqStandardColorButton*, vtkSMProxy* proxy, const char* propname);
  ~pqStandardColorLinkAdaptor();

  /// Break a global-property link.
  static void breakLink(vtkSMProxy* proxy, const char* pname);

protected slots:
  /// called when pqStandardColorButton fires standardColorChanged() signal.
  /// We update the property link between the standard color and the
  /// colorProperty.
  void onStandardColorChanged(const QString&);

  /// called when vtkSMGlobalPropertiesManager fires the modified event
  /// indicating that some links have been made or broken.
  void onGlobalPropertiesChanged();

private:
  pqStandardColorLinkAdaptor(const pqStandardColorLinkAdaptor&); // Not implemented.
  void operator=(const pqStandardColorLinkAdaptor&); // Not implemented.

  vtkSMProxy* Proxy;
  QString PropertyName;
  vtkEventQtSlotConnect* VTKConnect;
  bool IgnoreModifiedEvents;
};

#endif


