/*=========================================================================

   Program: ParaView
   Module:    pqAnimatablePropertiesComboBox.h

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
  pqAnimatablePropertiesComboBox(QWidget* parent = 0);
  ~pqAnimatablePropertiesComboBox() override;

  /**
  * Returns the source whose properties are currently being listed, if any.
  */
  vtkSMProxy* source() const;

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

public Q_SLOTS:
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
