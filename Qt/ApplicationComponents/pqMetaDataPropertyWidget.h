// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqMetaDataPropertyWidget_h
#define pqMetaDataPropertyWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"

#include <memory>

class vtkSMProxy;
class vtkSMPropertyGroup;

/**
 * Utilize pqMetaDataPropertyWidget when you need to update
 * the metadata information of a vtkSMSourceProxy.
 *
 * For example,
 *  The list of arrays in array selection views may need to be refreshed when data access properties
 * on the reader are modified.
 *
 * How to use?
 *  Group all properties that could affect the metadata inside a property group, then, set
 *  this widget as the `panel_widget`.
 *
 * Example:
 * \verbatim
 *   <PropertyGroup name="DatabaseProperties", panel_widget="MetadataPropertyWidget">
 *     <Property name="Foo"/>
 *     <Property name="Bar"/>
 *     <Property name="VeryInterestingProperty"/>
 *   </PropertyGroup>
 * \endverbatim
 *
 * Under the hood, it uses pqProxyWidget::createPropertyWidget` to auto generate
 * pq[Int,Double,String]VectorPropertyWidget instances for simple properties - (Int, Double,
 * String)VectorProperty.

 * When any of those properties are modified, from GUI or Python or wherever, the unchecked values
 * are pushed to the source proxy's vtk object.
 * After that, `UpdatePipelineInformation()` is invoked on the source proxy.
 * This enables it to reload the information based on the new property values.
 *
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqMetaDataPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  using Superclass = pqPropertyWidget;

public:
  pqMetaDataPropertyWidget(
    vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  ~pqMetaDataPropertyWidget() override;

  void updateMetadata(vtkObject*, unsigned long, void*);

private:
  Q_DISABLE_COPY(pqMetaDataPropertyWidget);
  class pqInternals;
  std::unique_ptr<pqInternals> Internals;
};

#endif
