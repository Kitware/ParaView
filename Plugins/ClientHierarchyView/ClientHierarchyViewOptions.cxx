/*=========================================================================

   Program: ParaView
   Module:    ClientHierarchyViewOptions.cxx

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

#include "ClientHierarchyView.h"
#include "ClientHierarchyViewOptions.h"

#include "ui_ClientHierarchyViewOptions.h"

#include <QPointer>

class ClientHierarchyViewOptions::implementation
{
public:
  Ui::ClientHierarchyViewOptions Widgets;
  QPointer<ClientHierarchyView> View;
};

ClientHierarchyViewOptions::ClientHierarchyViewOptions(QWidget *widgetParent) :
  pqOptionsContainer(widgetParent),
  Implementation(new implementation())
{
  this->Implementation->Widgets.setupUi(this);
}

ClientHierarchyViewOptions::~ClientHierarchyViewOptions()
{
  delete this->Implementation;
}

void ClientHierarchyViewOptions::setPage(const QString& page)
{
  if(page == "Visibility")
    this->Implementation->Widgets.stackedWidget->setCurrentIndex(0);
}

QStringList ClientHierarchyViewOptions::getPageList()
{
  return QStringList() << "Visibility";
}
  
void ClientHierarchyViewOptions::setView(pqView* view)
{
  if(this->Implementation->View)
    {
    QObject::disconnect(this->Implementation->Widgets.showTree, 0, this->Implementation->View, 0);
    QObject::disconnect(this->Implementation->Widgets.showHierarchicalGraph, 0, this->Implementation->View, 0);
    QObject::disconnect(this->Implementation->Widgets.hgraphLayout, 0, this->Implementation->View, 0);
    QObject::disconnect(this->Implementation->Widgets.leafSpacing, 0, this->Implementation->View, 0);
    QObject::disconnect(this->Implementation->Widgets.levelSpacing, 0, this->Implementation->View, 0);
    QObject::disconnect(this->Implementation->Widgets.bundlingStrength, 0, this->Implementation->View, 0);
    QObject::disconnect(this->Implementation->Widgets.radialLayout, 0, this->Implementation->View, 0);
    QObject::disconnect(this->Implementation->Widgets.radialAngle, 0, this->Implementation->View, 0);
    }

  this->Implementation->View = qobject_cast<ClientHierarchyView*>(view);
  
  if(this->Implementation->View)
    {
    this->Implementation->Widgets.showTree->setChecked(this->Implementation->View->treeVisible());
    this->Implementation->Widgets.showHierarchicalGraph->setChecked(this->Implementation->View->hierarchicalGraphVisible());

    QObject::connect(this->Implementation->Widgets.showTree, SIGNAL(clicked(bool)), this->Implementation->View, SLOT(showTree(bool)));
    QObject::connect(this->Implementation->Widgets.showHierarchicalGraph, SIGNAL(clicked(bool)), this->Implementation->View, SLOT(showHierarchicalGraph(bool)));
    QObject::connect(this->Implementation->Widgets.radialLayout, SIGNAL(clicked(bool)), this->Implementation->View, SLOT(setRadialLayout(bool)));

    QObject::connect(
      this->Implementation->Widgets.hgraphLayout,
      SIGNAL(currentIndexChanged(const QString&)),
      this->Implementation->View,
      SLOT(setHierarchicalGraphLayout(const QString&)));
    QObject::connect(
      this->Implementation->Widgets.leafSpacing,
      SIGNAL(valueChanged(double)),
      this->Implementation->View,
      SLOT(setTreeLeafSpacing(double)));
    QObject::connect(
      this->Implementation->Widgets.levelSpacing,
      SIGNAL(valueChanged(double)),
      this->Implementation->View,
      SLOT(setTreeLevelSpacing(double)));
    QObject::connect(
      this->Implementation->Widgets.bundlingStrength,
      SIGNAL(valueChanged(double)),
      this->Implementation->View,
      SLOT(setEdgeBundlingStrength(double)));
    QObject::connect(
      this->Implementation->Widgets.radialAngle,
      SIGNAL(valueChanged(int)),
      this->Implementation->View,
      SLOT(setRadialAngle(int)));

    this->Implementation->Widgets.showTree->setEnabled(true);
    this->Implementation->Widgets.showHierarchicalGraph->setEnabled(true);
    this->Implementation->Widgets.hgraphLayout->setEnabled(true);
    this->Implementation->Widgets.leafSpacing->setEnabled(true);
    this->Implementation->Widgets.levelSpacing->setEnabled(true);
    this->Implementation->Widgets.bundlingStrength->setEnabled(true);
    this->Implementation->Widgets.radialLayout->setEnabled(true);
    this->Implementation->Widgets.radialAngle->setEnabled(true);
    }
  else
    {
    this->Implementation->Widgets.showTree->setEnabled(false);
    this->Implementation->Widgets.showHierarchicalGraph->setEnabled(false);
    this->Implementation->Widgets.hgraphLayout->setEnabled(false);
    this->Implementation->Widgets.leafSpacing->setEnabled(false);
    this->Implementation->Widgets.levelSpacing->setEnabled(false);
    this->Implementation->Widgets.bundlingStrength->setEnabled(false);
    this->Implementation->Widgets.radialLayout->setEnabled(false);
    this->Implementation->Widgets.radialAngle->setEnabled(false);
    }
}

void ClientHierarchyViewOptions::applyChanges()
{
}

void ClientHierarchyViewOptions::resetChanges()
{
}

bool ClientHierarchyViewOptions::isApplyUsed() const
{
  return false;
}

