/*=========================================================================

   Program: ParaView
   Module: pqArrayListDomain.h

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
#ifndef pqArrayListDomain_h
#define pqArrayListDomain_h

#include "pqComponentsModule.h"
#include <QObject>

class QWidget;
class vtkSMProperty;
class vtkSMProxy;
class vtkSMDomain;

/**
* pqArrayListDomain is used to connect a widget showing a selection of arrays
* with its vtkSMArrayListDomain. Whenever the vtkSMArrayListDomain changes,
* the widget is "reset" to update using the property's new domain. This is
* useful for DescriptiveStatistics panel, for example. Whenever the attribute
* selection changes (i.e. user switches from cell-data to point-data), we need
* to update the widget's contents to show the list of array in the
* corresponding attribute. This class takes care of that.
*/
class PQCOMPONENTS_EXPORT pqArrayListDomain : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqArrayListDomain(QWidget* selectorWidget, const QString& qproperty, vtkSMProxy* proxy,
    vtkSMProperty* smproperty, vtkSMDomain* domain);
  ~pqArrayListDomain() override;

private Q_SLOTS:
  void domainChanged();

private:
  Q_DISABLE_COPY(pqArrayListDomain)
  class pqInternals;
  pqInternals* Internals;
};

#endif
