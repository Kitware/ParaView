// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

// self
#include "pqLinksManager.h"
#include "ui_pqLinksManager.h"

// Qt
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>

// SM
#include "vtkSMPropertyLink.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLink.h"
#include "vtkSMProxyManager.h"

// pqCore
#include "pqApplicationCore.h"
#include "pqPipelineSource.h"

// pqComponents
#include "pqLinksEditor.h"
#include "pqLinksModel.h"

//------------------------------------------------------------------------------
pqLinksManager::pqLinksManager(QWidget* p)
  : QDialog(p)
  , Ui(new Ui::pqLinksManager())
{
  this->Ui->setupUi(this);
  this->setWindowFlags(this->windowFlags().setFlag(Qt::WindowContextHelpButtonHint, false));
  pqLinksModel* model = pqApplicationCore::instance()->getLinksModel();
  this->Ui->treeView->setModel(model);
  QObject::connect(this->Ui->treeView, SIGNAL(clicked(const QModelIndex&)), this,
    SLOT(selectionChanged(const QModelIndex&)));
  QObject::connect(
    this->Ui->treeView, SIGNAL(activated(const QModelIndex&)), this, SLOT(editLink()));
  QObject::connect(this->Ui->addButton, SIGNAL(clicked(bool)), this, SLOT(addLink()));
  QObject::connect(this->Ui->editButton, SIGNAL(clicked(bool)), this, SLOT(editLink()));
  QObject::connect(this->Ui->removeButton, SIGNAL(clicked(bool)), this, SLOT(removeLink()));
  this->Ui->editButton->setEnabled(false);
  this->Ui->removeButton->setEnabled(false);
}

//------------------------------------------------------------------------------
pqLinksManager::~pqLinksManager() = default;

//------------------------------------------------------------------------------
void pqLinksManager::addLink()
{
  pqLinksModel* model = pqApplicationCore::instance()->getLinksModel();
  pqLinksEditor editor(nullptr, this);
  editor.setWindowTitle(tr("Add Link"));
  if (editor.exec() == QDialog::Accepted)
  {
    if (editor.linkType() == pqLinksModel::Proxy)
    {
      vtkSMProxy* inP = editor.selectedProxy1();
      vtkSMProxy* outP = editor.selectedProxy2();

      if (inP->IsA("vtkSMRenderViewProxy") && outP->IsA("vtkSMRenderViewProxy"))
      {
        if (editor.cameraWidgetViewLinkChecked())
        {
          model->addCameraWidgetLink(editor.linkName(), inP, outP);
        }
        else
        {
          model->addCameraLink(editor.linkName(), inP, outP, editor.interactiveViewLinkChecked());
        }
      }
      else
      {
        model->addProxyLink(editor.linkName(), inP, outP);
      }
    }
    else if (editor.linkType() == pqLinksModel::Property)
    {
      model->addPropertyLink(editor.linkName(), editor.selectedProxy1(), editor.selectedProperty1(),
        editor.selectedProxy2(), editor.selectedProperty2());
    }
    else if (editor.linkType() == pqLinksModel::Selection)
    {
      model->addSelectionLink(editor.linkName(), editor.selectedProxy1(), editor.selectedProxy2(),
        editor.convertToIndicesChecked());
    }
  }
}

//------------------------------------------------------------------------------
void pqLinksManager::editLink()
{
  pqLinksModel* model = pqApplicationCore::instance()->getLinksModel();
  QModelIndex idx = this->Ui->treeView->selectionModel()->currentIndex();
  vtkSMLink* link = model->getLink(idx);
  pqLinksEditor editor(link, this);
  editor.setWindowTitle(tr("Edit Link"));
  if (editor.exec() == QDialog::Accepted)
  {
    model->removeLink(idx);

    if (editor.linkType() == pqLinksModel::Proxy)
    {
      vtkSMProxy* inP = editor.selectedProxy1();
      vtkSMProxy* outP = editor.selectedProxy2();

      if (inP->IsA("vtkSMRenderViewProxy") && outP->IsA("vtkSMRenderViewProxy"))
      {
        if (editor.cameraWidgetViewLinkChecked())
        {
          model->addCameraWidgetLink(editor.linkName(), inP, outP);
        }
        else
        {
          model->addCameraLink(editor.linkName(), inP, outP, editor.interactiveViewLinkChecked());
        }
      }
      else
      {
        model->addProxyLink(editor.linkName(), inP, outP);
      }
    }
    else if (editor.linkType() == pqLinksModel::Property)
    {
      model->addPropertyLink(editor.linkName(), editor.selectedProxy1(), editor.selectedProperty1(),
        editor.selectedProxy2(), editor.selectedProperty2());
    }
    else if (editor.linkType() == pqLinksModel::Selection)
    {
      model->addSelectionLink(editor.linkName(), editor.selectedProxy1(), editor.selectedProxy2(),
        editor.convertToIndicesChecked());
    }
  }
}

//------------------------------------------------------------------------------
void pqLinksManager::removeLink()
{
  pqLinksModel* model = pqApplicationCore::instance()->getLinksModel();
  QModelIndexList idxs = this->Ui->treeView->selectionModel()->selectedIndexes();
  QStringList names;
  // convert indexes to names so our indexes don't become invalid during removal
  Q_FOREACH (QModelIndex idx, idxs)
  {
    QString name = model->getLinkName(idx);
    if (!names.contains(name))
    {
      names.append(name);
    }
  }

  Q_FOREACH (QString name, names)
  {
    model->removeLink(name);
  }
}

//------------------------------------------------------------------------------
void pqLinksManager::selectionChanged(const QModelIndex& idx)
{
  if (!idx.isValid())
  {
    this->Ui->editButton->setEnabled(false);
    this->Ui->removeButton->setEnabled(false);
  }
  else
  {
    this->Ui->editButton->setEnabled(true);
    this->Ui->removeButton->setEnabled(true);
  }
}
