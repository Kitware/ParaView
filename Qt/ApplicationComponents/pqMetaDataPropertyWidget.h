/*=========================================================================

   Program: ParaView
   Module: pqMetaDataPropertyWidget.h

   Copyright (c) 2005-2022 Sandia Corporation, Kitware Inc.
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
 *   <PropertyGroup name="DatabaseProperties", panel_widget="MetadataPropertyWidget">
 *     <Property name="Foo"/>
 *     <Property name="Bar"/>
 *     <Property name="VeryInterestingProperty"/>
 *   </PropertyGroup>
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
