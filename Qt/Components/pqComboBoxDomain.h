// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqComboBoxDomain_h
#define pqComboBoxDomain_h

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

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void domainChanged();
protected Q_SLOTS:
  virtual void internalDomainChanged();

protected: // NOLINT(readability-redundant-access-specifiers)
  void markForUpdate(bool mark);

  class pqInternal;
  pqInternal* Internal;
};

#endif
