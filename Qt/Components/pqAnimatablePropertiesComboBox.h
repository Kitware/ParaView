// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqAnimatablePropertiesComboBox_h
#define pqAnimatablePropertiesComboBox_h

#include "pqComponentsModule.h"
#include <QComboBox>

class vtkSMProxy;

/**
 * pqAnimatablePropertiesComboBox is a combo box that can list the animatable
 * properties of any proxy.
 */
class PQCOMPONENTS_EXPORT pqAnimatablePropertiesComboBox : public QComboBox
{
  Q_OBJECT
  typedef QComboBox Superclass;

public:
  pqAnimatablePropertiesComboBox(QWidget* parent = nullptr);
  ~pqAnimatablePropertiesComboBox() override;

  vtkSMProxy* getCurrentProxy() const;
  QString getCurrentPropertyName() const;
  int getCurrentIndex() const;

  /**
   * If true, vector properties will have a single entry. If false, each vector
   * component will be displayed separately. Default: false.
   */
  bool getCollapseVectors() const;

  /**
   * This class can filter the displayed properties to only show those with a
   * specified number of components. Returns the current filter settings. -1
   * means no filtering (default).
   */
  int getVectorSizeFilter() const;

  /**
   * Sometimes, we want the combo to show a empty field that does not represent
   * any property. Set this to true to use such a field.
   */
  void setUseBlankEntry(bool b) { this->UseBlankEntry = b; }

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Set the source whose properties this widget should list. If source is
   * null, the widget gets disabled.
   */
  void setSource(vtkSMProxy* proxy);

  /**
   * Set source without calling buildPropertyList() internally. Thus the user
   * will explicitly call addSMProperty to add properties.
   */
  void setSourceWithoutProperties(vtkSMProxy* proxy);

  /**
   * If true, vector properties will have a single entry. If false, each vector
   * component will be displayed separately. Default: false.
   */
  void setCollapseVectors(bool val);

  /**
   * This class can filter the displayed properties to only show those with a
   * specified number of components. Set the current filter setting. -1
   * means no filtering (default).
   */
  void setVectorSizeFilter(int size);

  /**
   * Add a property to the widget.
   */
  void addSMProperty(const QString& label, const QString& propertyname, int index);

protected Q_SLOTS:
  /**
   * Builds the property list.
   */
  void buildPropertyList();

private:
  Q_DISABLE_COPY(pqAnimatablePropertiesComboBox)

  void buildPropertyListInternal(vtkSMProxy* proxy, const QString& labelPrefix);
  void addSMPropertyInternal(const QString& label, vtkSMProxy* proxy, const QString& propertyname,
    int index, bool is_display_property = false, unsigned int display_port = 0);

  /**
   * Add properties that control the display parameters.
   */
  void addDisplayProperties(vtkSMProxy* proxy);
  bool UseBlankEntry;

public:
  class pqInternal;

private:
  pqInternal* Internal;
};

#endif
