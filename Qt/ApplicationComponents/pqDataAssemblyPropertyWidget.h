// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqDataAssemblyPropertyWidget_h
#define pqDataAssemblyPropertyWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"

#include "vtkParaViewDeprecation.h" // for PARAVIEW_DEPRECATED

#include <QScopedPointer> // for QScopedPointer

/**
 * @class pqDataAssemblyPropertyWidget
 * @brief pqPropertyWidget for properties with vtkDataAssembly
 *
 * pqDataAssemblyPropertyWidget is intended for properties that rely on a
 * vtkDataAssembly i.e. use a vtkSMDataAssemblyDomain. This
 * supports getting/setting the list of selectors for checked nodes based on the
 * chosen vtkDataAssembly. Further more, it supports show the state and resetting
 * block properties.
 *
 * pqDataAssemblyPropertyWidget can be used on a single property with a
 * vtkSMDataAssemblyDomain or for a group of properties. For a single property,
 * it allows for editing on selectors for checked nodes.
 *
 * Here's an example proxy XML for a single property.
 *
 * @code{xml}
 * <Proxy ...>
 *   <StringVectorProperty name="Selectors"
 *                         repeat_command="1"
 *                         number_of_elements_per_command="1"
 *                         default_values="/" >
 *     <DataAssemblyDomain name="data_assembly">
 *       <RequiredProperties>
 *         <Property function="Input" name="Input" />
 *       </RequiredProperties>
 *     </DataAssemblyDomain>
 *   </StringVectorProperty>
 *   ...
 * </Proxy>
 * @endcode
 *
 * The widget will use the assembly provided by
 * `vtkSMDataAssemblyDomain::GetDataAssembly` to render a tree in UI.
 *
 * A property-group for choosing which named-assembly to use is as follows.
 * All properties in the group are optional and one may specify on subset
 * that is relevant for their use-case. It is assumed, however, that all properties
 * in the group use the same data assembly. If that's not the case, one should use
 * separate property groups, hence separate widgets, for each.
 *
 * @code{xml}
 * <Proxy ...>
 *   <StringVectorProperty name="Assembly"
 *                         command="SetAssemblyName"
 *                         number_of_elements="1">
 *     <DataAssemblyListDomain name="data_assembly_list">
 *       <RequiredProperties>
 *         <Property function="Input" name="Input" />
 *       </RequiredProperties>
 *     </DataAssemblyListDomain>
 *     <Documentation>
 *       Select which assembly is used when specify selectors
 *       to choose blocks to show for composite datasets.
 *     </Documentation>
 *   </StringVectorProperty>
 *
 *
 *   <StringVectorProperty name="SelectedSelectors"
 *                         repeat_command="1"
 *                         number_of_elements_per_command="1"
 *                         number_of_elements="1"
 *                         panel_visibility="multiblock_inspector">
 *     <RepresentedArrayListDomain name="array_list" input_domain_name="input_array_any">
 *       <RequiredProperties>
 *         <Property function="Input" name="Input"/>
 *         <Property function="ActiveAssembly" name="Assembly"/>
 *         <Property function="Selectors" name="SelectedSelectors"/>
 *       </RequiredProperties>
 *     </RepresentedArrayListDomain>
 *     <Documentation>
 *       For composite datasets, specify the color array name associated with selectors
 *       on the assembly chosen using **Assembly**.
 *     </Documentation>
 *     <Hints>
 *       <NoDefault/>
 *     </Hints>
 *   </StringVectorProperty>
 *
 *   <StringVectorProperty name="Selectors"
 *                         command="AddSelector"
 *                         clean_command="ClearSelectors"
 *                         repeat_command="1"
 *                         number_of_elements_per_command="1">
 *     <DataAssemblyDomain name="data_assembly">
 *       <RequiredProperties>
 *         <Property function="Input" name="Input" />
 *         <Property function="ActiveAssembly" name="Assembly" />
 *       </RequiredProperties>
 *     </DataAssemblyDomain>
 *     <Documentation>
 *       For composite datasets, specify selectors to limit the view
 *       to a chosen subset of blocks.
 *     </Documentation>
 *   </StringVectorProperty>
 *
 *   <PropertyGroup label="Blocks" panel_widget="DataAssemblyEditor">
 *     <Property name="Assembly"  function="ActiveAssembly" />
 *     <Property name="SelectedSelectors" function="SelectedSelectors" />
 *     <Property name="Selectors" function="Selectors" />
 *   </PropertyGroup>
 * </Proxy>
 * @endcode
 *
 *
 * This widget is primarily intended for vtkSMDataAssemblyDomain. However, to
 * support legacy code that uses vtkSMCompositeTreeDomain instead, we add
 * support for vtkSMCompositeTreeDomain as well. It is required, however, that
 * all properties in the group to consistently use vtkSMDataAssemblyDomain or
 * vtkSMCompositeTreeDomain and mixing is not allowed.
 *
 * TODO: Remove the usage of vtkSMCompositeTreeDomain.
 *
 * @section Hints Hints
 *
 * This widget supports hints that can be added to the group (or property). For
 * example:
 *
 * @code{xml}
 *   <PropertyGroup label="AssemblyOnly" panel_widget="DataAssemblyEditor">
 *      <Property name="Assembly"  function="ActiveAssembly" />
 *      <Property name="Selectors" function="Selectors" />
 *      <Hints>
 *        <DataAssemblyPropertyWidget
 *          is_checkable="0"
 *          use_inputname_as_header="1"
 *          link_active_selection="1"
 *          />
 *      </Hints>
 *    </PropertyGroup>
 * @endcode
 *
 * `is_checkable` can be set to `0` to avoid showing checkboxes for items in the
 * tree.
 *
 * `use_inputname_as_header`, when set to `1` causes the widget to use the
 * input's registration name as the header for the tree widget rather than the
 * XML label for the group (or property) attached to the widget.
 *
 * `link_active_selection`, when set to `1` causes the widget to track the
 * active block-selection and vice-versa.
 */
class vtkObject;
class PQAPPLICATIONCOMPONENTS_EXPORT pqDataAssemblyPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

  ///@{
  /**
   * Property with selectors/composite indices for checked nodes in the hierarchy.
   */
  Q_PROPERTY(QList<QVariant> selectors READ selectorsAsVariantList WRITE setSelectors NOTIFY
      selectorsChanged);
  Q_PROPERTY(QList<QVariant> compositeIndices READ compositeIndicesAsVariantList WRITE
      setCompositeIndices NOTIFY selectorsChanged);
  ///@}

  ///@{
  /**
   * Property with selectors/composite indices and associated colors (as RGB).
   */
  // PARAVIEW_DEPRECATED_IN_5_13_0("No longer used.")
  Q_PROPERTY(QList<QVariant> selectorColors READ selectorColorsAsVariantList WRITE setSelectorColors
      NOTIFY colorsChanged);
  // PARAVIEW_DEPRECATED_IN_5_13_0("No longer used.")
  Q_PROPERTY(QList<QVariant> compositeIndexColors READ compositeIndexColorsAsVariantList WRITE
      setCompositeIndexColors NOTIFY colorsChanged);
  ///@}

  ///@{
  /**
   * Properties with selectors/composite indieces and associated opacities.
   */
  // PARAVIEW_DEPRECATED_IN_5_13_0("No longer used.")
  Q_PROPERTY(QList<QVariant> selectorOpacities READ selectorOpacitiesAsVariantList WRITE
      setSelectorOpacities NOTIFY opacitiesChanged);
  // PARAVIEW_DEPRECATED_IN_5_13_0("No longer used.")
  Q_PROPERTY(QList<QVariant> compositeIndexOpacities READ compositeIndexOpacitiesAsVariantList WRITE
      setCompositeIndexOpacities NOTIFY opacitiesChanged);
  ///@}
public:
  pqDataAssemblyPropertyWidget(
    vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  pqDataAssemblyPropertyWidget(
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parent = nullptr);
  ~pqDataAssemblyPropertyWidget() override;

  ///@{
  /**
   * API for getting/setting selected/chosen path strings.
   */
  void setSelectors(const QStringList& paths);
  const QStringList& selectors() const;
  void setSelectors(const QList<QVariant>& paths);
  QList<QVariant> selectorsAsVariantList() const;
  ///@}

  ///@{
  /**
   * API for getting/settings composite indices.
   */
  void setCompositeIndices(const QList<QVariant>& values);
  QList<QVariant> compositeIndicesAsVariantList() const;
  ///@}

  ///@{
  /**
   * API to get/set colors. Colors are specified either as a list of selectors
   * followed by corresponding RGB color or list of composite indices followed by
   * the color.
   */
  PARAVIEW_DEPRECATED_IN_5_13_0("No longer used.")
  void setCompositeIndexColors(const QList<QVariant>& values) { Q_UNUSED(values); }
  PARAVIEW_DEPRECATED_IN_5_13_0("No longer used.")
  QList<QVariant> compositeIndexColorsAsVariantList() const { return QList<QVariant>(); }

  PARAVIEW_DEPRECATED_IN_5_13_0("No longer used.")
  void setSelectorColors(const QList<QVariant>& values) { Q_UNUSED(values); }
  PARAVIEW_DEPRECATED_IN_5_13_0("No longer used.")
  QList<QVariant> selectorColorsAsVariantList() const { return QList<QVariant>(); }
  ///@}

  ///@{
  /**
   * API to get/set opacities. Opacities are specified either as a list of selectors
   * followed by corresponding opacity or list of composite indices followed by
   * the opacity.
   */
  PARAVIEW_DEPRECATED_IN_5_13_0("No longer used.")
  void setCompositeIndexOpacities(const QList<QVariant>& values) { Q_UNUSED(values); }
  PARAVIEW_DEPRECATED_IN_5_13_0("No longer used.")
  QList<QVariant> compositeIndexOpacitiesAsVariantList() const { return QList<QVariant>(); }

  PARAVIEW_DEPRECATED_IN_5_13_0("No longer used.")
  void setSelectorOpacities(const QList<QVariant>& values) { Q_UNUSED(values); }
  PARAVIEW_DEPRECATED_IN_5_13_0("No longer used.")
  QList<QVariant> selectorOpacitiesAsVariantList() const { return QList<QVariant>(); }
  ///@}

  void updateWidget(bool showing_advanced_properties) override;

Q_SIGNALS:
  void selectorsChanged();
  PARAVIEW_DEPRECATED_IN_5_13_0("No longer used.")
  void colorsChanged();
  PARAVIEW_DEPRECATED_IN_5_13_0("No longer used.")
  void opacitiesChanged();

private Q_SLOTS:
  void updateDataAssembly(vtkObject* sender);
  void assemblyTreeModified(int role);
  void selectorsTableModified();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqDataAssemblyPropertyWidget);
  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
