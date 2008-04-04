/*=========================================================================

   Program: ParaView
   Module:    pqPipelineBrowserStateManager.cxx

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

/// \file pqPipelineBrowserStateManager.cxx
/// \date 1/10/2007

#include "pqPipelineBrowserStateManager.h"

#include "pqFlatTreeView.h"
#include "pqPipelineModel.h"

#include <QItemSelectionModel>
#include <QMap>
#include <QModelIndex>
#include <QString>

#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include <vtksys/ios/sstream>


class pqPipelineBrowserStateManagerInternal : public QMap<QString, QString> {};


pqPipelineBrowserStateManager::pqPipelineBrowserStateManager(
    QObject *parentObject)
  : QObject(parentObject)
{
  this->Internal = new pqPipelineBrowserStateManagerInternal();
  this->Model = 0;
  this->View = 0;
}

pqPipelineBrowserStateManager::~pqPipelineBrowserStateManager()
{
  delete this->Internal;
}

void pqPipelineBrowserStateManager::setModelAndView(pqPipelineModel *model,
    pqFlatTreeView *view)
{
  if(this->Model == model && this->View == view)
    {
    return;
    }

  // Clean up any remaining state.
  this->Internal->clear();
  if(this->Model)
    {
    this->disconnect(this->Model, 0, this, 0);
    }

  this->Model = model && view ? model : 0;
  this->View = this->Model ? view : 0;
  if(this->Model)
    {
    this->connect(this->Model, SIGNAL(movingIndex(const QModelIndex &)),
        this, SLOT(saveState(const QModelIndex &)));
    this->connect(this->Model, SIGNAL(indexRestored(const QModelIndex &)),
        this, SLOT(restoreState(const QModelIndex &)));
    }
}

void pqPipelineBrowserStateManager::saveState(vtkPVXMLElement *root) const
{
  if(this->View && root)
    {
    this->saveState(this->View->getRootIndex(), root);
    }
}

void pqPipelineBrowserStateManager::restoreState(vtkPVXMLElement *root)
{
  if(this->View && root)
    {
    this->restoreState(this->View->getRootIndex(), root);
    }
}

void pqPipelineBrowserStateManager::saveState(const QModelIndex &index)
{
  if(this->Model && index.isValid() && index.model() == this->Model)
    {
    // Get the name for the index, which will be used to look up the
    // state.
    QString name = this->Model->data(index).toString();
    if(!name.isEmpty())
      {
      // Get the state for the index.
      vtkPVXMLElement *root = vtkPVXMLElement::New();
      root->SetName("MoveState");
      this->saveState(index, root);

      // Save the state in the map.
      vtksys_ios::ostringstream xml_stream;
      root->PrintXML(xml_stream, vtkIndent());
      root->Delete();
      QString state = xml_stream.str().c_str();
      this->Internal->insert(name, state);
      }
    }
}

void pqPipelineBrowserStateManager::restoreState(const QModelIndex &index)
{
  if(this->Model && index.isValid() && index.model() == this->Model)
    {
    QString name = this->Model->data(index).toString();
    QMap<QString, QString>::Iterator iter = this->Internal->find(name);
    if(iter != this->Internal->end())
      {
      // Use the map entry to restore the state.
      vtkPVXMLParser *xmlParser = vtkPVXMLParser::New();
      xmlParser->InitializeParser();
      xmlParser->ParseChunk(iter->toAscii().data(), static_cast<unsigned int>(
          iter->size()));
      xmlParser->CleanupParser();

      this->restoreState(index, xmlParser->GetRootElement());

      // Remove the entry from the map.
      xmlParser->Delete();
      this->Internal->erase(iter);
      }
    }
}

void pqPipelineBrowserStateManager::saveState(const QModelIndex &index,
    vtkPVXMLElement *root) const
{
  // First, save the root index name and attributes.
  QItemSelectionModel *selection = this->View->getSelectionModel();
  QModelIndex current = selection->currentIndex();
  if(index.isValid())
    {
    if(this->View->isIndexExpanded(index))
      {
      root->SetAttribute("expanded", "true");
      }

    if(selection->isSelected(index))
      {
      root->SetAttribute("selected", "true");
      }

    if(index == current)
      {
      root->SetAttribute("current", "true");
      }
    }

  // Next, step through the indexes to save the expanded/selected
  // state.
  QModelIndex next = this->View->getNextVisibleIndex(index, index);
  while(next.isValid())
    {
    QString id;
    vtkPVXMLElement *element = vtkPVXMLElement::New();
    element->SetName("Index");
    this->View->getRelativeIndexId(next, id, index);
    element->SetAttribute("id", id.toAscii().data());
    if(this->View->isIndexExpanded(next))
      {
      root->SetAttribute("expanded", "true");
      }

    if(selection->isSelected(next))
      {
      root->SetAttribute("selected", "true");
      }

    if(next == current)
      {
      root->SetAttribute("current", "true");
      }

    root->AddNestedElement(element);
    element->Delete();
    next = this->View->getNextVisibleIndex(next, index);
    }
}

void pqPipelineBrowserStateManager::restoreState(const QModelIndex &index,
    vtkPVXMLElement *root)
{
  // First, restore the root index if it's valid.
  QItemSelectionModel *selection = this->View->getSelectionModel();
  if(index.isValid())
    {
    if(root->GetAttribute("expanded") != 0)
      {
      this->View->expand(index);
      }

    if(root->GetAttribute("selected") != 0)
      {
      selection->select(index, QItemSelectionModel::Select);
      }

    if(root->GetAttribute("current") != 0)
      {
      selection->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
      }
    }

  // Next, loop through the child elements.
  QModelIndex next;
  QString elemName = "Index";
  for(unsigned int i = 0; i < root->GetNumberOfNestedElements(); i++)
    {
    vtkPVXMLElement *element = root->GetNestedElement(i);
    if(elemName == element->GetName())
      {
      QString id = element->GetAttribute("id");
      next = this->View->getRelativeIndex(id, index);
      if(next.isValid())
        {
        if(element->GetAttribute("expanded") != 0)
          {
          this->View->expand(next);
          }

        if(element->GetAttribute("selected") != 0)
          {
          selection->select(next, QItemSelectionModel::Select);
          }

        if(element->GetAttribute("current") != 0)
          {
          selection->setCurrentIndex(next, QItemSelectionModel::NoUpdate);
          }
        }
      }
    }
}


