/*=========================================================================

   Program: ParaView
   Module:    pqComboBoxDomain.h

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

#ifndef pq_ComboBoxDomain_h
#define pq_ComboBoxDomain_h

#include "pqComponentsModule.h"
#include <QObject>

class QComboBox;
class QIcon;
class vtkSMDomain;
class vtkSMProperty;

/**
* combo box domain
* observes the domain for a combo box and updates accordingly.
* the list of values in the combo box is automatically
* updated when the domain changes
*/
class PQCOMPONENTS_EXPORT pqComboBoxDomain : public QObject
{
  Q_OBJECT
public:
  /**
  * constructor requires a QComboBox,
  * and the property with the domain to observe.
  * optionally pass in a domain if a specific one
  * needs to be watched
  */
  pqComboBoxDomain(QComboBox* p, vtkSMProperty* prop, vtkSMDomain* domain = nullptr);
  ~pqComboBoxDomain() override;

  // explicitly trigger a domain change.
  // simply calls internalDomainChanged();
  void forceDomainChanged() { this->internalDomainChanged(); }

  /**
  * Provides a mechanism to always add a set of strings to the combo box
  * irrespective of what the domain tells us.
  */
  void addString(const QString&);
  void insertString(int, const QString&);
  void removeString(const QString&);
  void removeAllStrings();

  vtkSMProperty* getProperty() const;
  vtkSMDomain* getDomain() const;
  const QStringList& getUserStrings() const;

  /**
   * Returns an icon for the provided field association.
   * This isn't the best place for this API, but leaving this here for now.
   */
  static QIcon getIcon(int fieldAssociation);

public Q_SLOTS:
  void domainChanged();
protected Q_SLOTS:
  virtual void internalDomainChanged();

protected:
  void markForUpdate(bool mark);

  class pqInternal;
  pqInternal* Internal;
};

#endif
