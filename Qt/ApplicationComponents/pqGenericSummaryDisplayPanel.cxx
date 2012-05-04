/*=========================================================================

   Program: ParaView
   Module:    pqGenericSummaryDisplayPanel.cxx

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

#include "pqGenericSummaryDisplayPanel.h"

#include <QAction>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QDebug>
#include <QComboBox>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QColorDialog>

#include "vtkSMProxy.h"
#include "pqRepresentation.h"
#include "pqDisplayColorWidget.h"
#include "pqDataRepresentation.h"
#include "pqPipelineRepresentation.h"
#include "pqScalarsToColors.h"
#include "pqScalarBarRepresentation.h"
#include "pqRenderViewBase.h"
#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqLookupTableManager.h"
#include "pqUndoStack.h"
#include "pqColorChooserButton.h"
#include "pqSignalAdaptors.h"
#include "vtkSMStringListDomain.h"
#include "vtkSMProperty.h"
#include "pqSignalAdaptors.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMDimensionsDomain.h"
#include "vtkSMIntRangeDomain.h"
#include "vtkSMProxyProperty.h"
#include "pqColorScaleEditor.h"
#include "pqCoreUtilities.h"
#include "pqStandardColorLinkAdaptor.h"
#include "pqSMAdaptor.h"
#include "pqEditColorMapReaction.h"
#include "pqResetScalarRangeReaction.h"
#include "pqScalarBarVisibilityReaction.h"

//-----------------------------------------------------------------------------
pqGenericSummaryDisplayPanel::pqGenericSummaryDisplayPanel(pqRepresentation *representation,
                                                           const QList<DisplayAttributes> &attributes,
                                                           QWidget *p)
  : QWidget(p)
{
  this->Representation = representation;
  this->Attributes = attributes;

  vtkSMProxy *proxy = representation->getProxy();
  QFormLayout *l = new QFormLayout;
  l->setMargin(2);
  l->setVerticalSpacing(4);

  // add color by combo box
  if(attributes.contains(ColorBy))
    {
    // color by combo box
    pqDisplayColorWidget *colorBy = new pqDisplayColorWidget(this);
    colorBy->setRepresentation(qobject_cast<pqDataRepresentation *>(representation));
    l->addRow(colorBy);

    // scalar bar buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;

    // show scalar bar button
    QPushButton *scalarBarButton = new QPushButton(QIcon(QString(":/pqWidgets/Icons/pqScalarBar24.png")), "Show", this);
    scalarBarButton->setCheckable(true);
    QAction *scalarBarAction = new QAction(this);
    connect(scalarBarButton, SIGNAL(clicked(bool)), scalarBarAction, SLOT(trigger()));
    buttonLayout->addWidget(scalarBarButton);
    new pqScalarBarVisibilityReaction(scalarBarAction);

    // edit color map button
    QPushButton *editColorMapButton = new QPushButton(QIcon(QString(":/pqWidgets/Icons/pqEditColor24.png")), "Edit", this);
    QAction *editColorMapAction = new QAction(this);
    connect(editColorMapButton, SIGNAL(clicked()), editColorMapAction, SLOT(trigger()));
    buttonLayout->addWidget(editColorMapButton);
    new pqEditColorMapReaction(editColorMapAction);

    // reset range button
    QPushButton *resetRangeButton = new QPushButton(QIcon(QString(":/pqWidgets/Icons/pqResetRange24.png")), "Rescale", this);
    QAction *resetRangeAction = new QAction(this);
    connect(resetRangeButton, SIGNAL(clicked()), resetRangeAction, SLOT(trigger()));
    buttonLayout->addWidget(resetRangeButton);
    new pqResetScalarRangeReaction(resetRangeAction);

    l->addRow(buttonLayout);
    }

  // add line width spin box
  if(attributes.contains(LineWidth))
    {
    QDoubleSpinBox *spinBox = new QDoubleSpinBox(this);
    l->addRow("Line Width:", spinBox);

    this->Links.addPropertyLink(spinBox,
                                "value",
                                SIGNAL(valueChanged(double)),
                                proxy,
                                proxy->GetProperty("LineWidth"));
    }

  // add point size spin box
  if(attributes.contains(PointSize))
    {
    QDoubleSpinBox *spinBox = new QDoubleSpinBox(this);
    l->addRow("Point Size:", spinBox);

    this->Links.addPropertyLink(spinBox,
                                "value",
                                SIGNAL(valueChanged(double)),
                                proxy,
                                proxy->GetProperty("PointSize"));
    }

  // add edge color chooser
  if(attributes.contains(EdgeColor))
    {
    pqColorChooserButton *button = new pqColorChooserButton(this);
    pqSignalAdaptorColor *adaptor = new pqSignalAdaptorColor(button,
                                                             "chosenColor",
                                                             SIGNAL(chosenColorChanged(const QColor&)),
                                                             false);

    this->Links.addPropertyLink(adaptor,
                                "color",
                                SIGNAL(colorChanged(const QVariant&)),
                                proxy,
                                proxy->GetProperty("EdgeColor"));

    l->addRow("Edge Color:", button);
    }

  // add slice direction combo box
  if(attributes.contains(SliceDirection))
    {
    vtkSMProperty *sliceDirectionProperty = proxy->GetProperty("SliceMode");

    if(sliceDirectionProperty)
      {
      vtkSMEnumerationDomain *domain = vtkSMEnumerationDomain::SafeDownCast(sliceDirectionProperty->GetDomain("enum"));

      if(domain)
        {
        QComboBox *directions = new QComboBox(this);

        for(unsigned int i = 0; i < domain->GetNumberOfEntries(); i++)
          {
          const char *text = domain->GetEntryText(i);

          directions->addItem(text);
          }

        pqSignalAdaptorComboBox *adaptor = new pqSignalAdaptorComboBox(directions);
        this->Links.addPropertyLink(adaptor,
                                    "currentText",
                                    SIGNAL(currentIndexChanged(int)),
                                    proxy,
                                    sliceDirectionProperty);

        l->addRow("Slice Direction:", directions);
        }
      }
    }

  // add slice number slider
  if(attributes.contains(SliceNumber))
    {
    vtkSMProperty *slice = proxy->GetProperty("Slice");

    QWidget *sliceNumberWidget = new QWidget(this);
    QHBoxLayout *sliceNumberLayout = new QHBoxLayout;

    QSlider *slider = new QSlider(Qt::Horizontal, sliceNumberWidget);
    sliceNumberLayout->addWidget(slider);

    QSpinBox *spinBox = new QSpinBox(sliceNumberWidget);
    sliceNumberLayout->addWidget(spinBox);

    connect(slider, SIGNAL(valueChanged(int)), spinBox, SLOT(setValue(int)));
    connect(spinBox, SIGNAL(valueChanged(int)), slider, SLOT(setValue(int)));

    sliceNumberWidget->setLayout(sliceNumberLayout);

    vtkSMIntRangeDomain *dimensions = vtkSMIntRangeDomain::SafeDownCast(slice->GetDomain("dims"));
    if(dimensions)
      {
      slider->setMinimum(dimensions->GetMinimum(0));
      slider->setMaximum(dimensions->GetMaximum(0));
      }

    this->Links.addPropertyLink(slider, "value", SIGNAL(valueChanged(int)), proxy, slice);

    l->addRow("Slice Number:", sliceNumberWidget);
    }

  // add volume mapper combo box
  if(attributes.contains(VolumeMapper))
    {
    vtkSMProperty *volumeMapperProperty = proxy->GetProperty("SelectMapper");
    if(volumeMapperProperty)
      {
      vtkSMStringListDomain *domain =
        vtkSMStringListDomain::SafeDownCast(volumeMapperProperty->GetDomain("list"));

      if(domain)
        {
        QComboBox *mappers = new QComboBox(this);

        for(unsigned int i = 0; i < domain->GetNumberOfStrings(); i++)
          {
          const char *mapperName = domain->GetString(i);

          mappers->addItem(mapperName);
          }

        pqSignalAdaptorComboBox *adaptor = new pqSignalAdaptorComboBox(mappers);
        this->Links.addPropertyLink(adaptor,
                                    "currentText",
                                    SIGNAL(currentIndexChanged(int)),
                                    proxy,
                                    volumeMapperProperty);

        l->addRow("Volume Mapper:", mappers);
        }
      }
    }

  this->setLayout(l);
}

//-----------------------------------------------------------------------------
pqGenericSummaryDisplayPanel::~pqGenericSummaryDisplayPanel()
{
}
