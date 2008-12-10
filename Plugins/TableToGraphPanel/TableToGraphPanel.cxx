#include "TableToGraphPanel.h"

#include <pqProxy.h>
#include <pqPropertyHelper.h>

#include <vtkSMProxy.h>

#include <QLabel>
#include <QLayout>
#include <QMessageBox>

#include <iostream>
#include <set>

TableToGraphPanel::TableToGraphPanel(pqProxy* proxy, QWidget* p) :
  pqObjectPanel(proxy, p)
{
  this->Widgets.setupUi(this);

  vtkSMProxy* const table_to_graph = proxy->getProxy();

  const QStringList link_vertices = pqPropertyHelper(table_to_graph, "LinkVertices").GetAsStringList();
  if(link_vertices.size() > 2)
    {
    this->Widgets.vertexField1->setText(link_vertices[0]);
    this->Widgets.vertexDomain1->setText(link_vertices[1]);
    this->Widgets.vertexHidden1->setChecked(link_vertices[2] == "1");
    }
  if(link_vertices.size() > 5)
    {
    this->Widgets.vertexField2->setText(link_vertices[3]);
    this->Widgets.vertexDomain2->setText(link_vertices[4]);
    this->Widgets.vertexHidden2->setChecked(link_vertices[5] == "1");
    }
  if(link_vertices.size() > 8)
    {
    this->Widgets.vertexField3->setText(link_vertices[6]);
    this->Widgets.vertexDomain3->setText(link_vertices[7]);
    this->Widgets.vertexHidden3->setChecked(link_vertices[8] == "1");
    }
  if(link_vertices.size() > 11)
    {
    this->Widgets.vertexField4->setText(link_vertices[9]);
    this->Widgets.vertexDomain4->setText(link_vertices[10]);
    this->Widgets.vertexHidden4->setChecked(link_vertices[11] == "1");
    }
  if(link_vertices.size() > 14)
    {
    this->Widgets.vertexField5->setText(link_vertices[12]);
    this->Widgets.vertexDomain5->setText(link_vertices[13]);
    this->Widgets.vertexHidden5->setChecked(link_vertices[14] == "1");
    }

  const QStringList link_edges = pqPropertyHelper(table_to_graph, "LinkEdges").GetAsStringList();
  if(link_edges.size() > 1)
    {
    this->Widgets.edgeSource1->setText(link_edges[0]);
    this->Widgets.edgeTarget1->setText(link_edges[1]);
    }
  if(link_edges.size() > 3)
    {
    this->Widgets.edgeSource2->setText(link_edges[2]);
    this->Widgets.edgeTarget2->setText(link_edges[3]);
    }
  if(link_edges.size() > 5)
    {
    this->Widgets.edgeSource3->setText(link_edges[4]);
    this->Widgets.edgeTarget3->setText(link_edges[5]);
    }
  if(link_edges.size() > 7)
    {
    this->Widgets.edgeSource4->setText(link_edges[6]);
    this->Widgets.edgeTarget4->setText(link_edges[7]);
    }
  if(link_edges.size() > 9)
    {
    this->Widgets.edgeSource5->setText(link_edges[8]);
    this->Widgets.edgeTarget5->setText(link_edges[9]);
    }

  QObject::connect(this->Widgets.vertexField1, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexField2, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexField3, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexField4, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexField5, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));

  QObject::connect(this->Widgets.vertexDomain1, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexDomain2, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexDomain3, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexDomain4, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexDomain5, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));

  QObject::connect(this->Widgets.vertexHidden1, SIGNAL(clicked(bool)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexHidden2, SIGNAL(clicked(bool)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexHidden3, SIGNAL(clicked(bool)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexHidden4, SIGNAL(clicked(bool)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexHidden5, SIGNAL(clicked(bool)), this, SLOT(setModified()));

  QObject::connect(this->Widgets.edgeSource1, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.edgeSource2, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.edgeSource3, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.edgeSource4, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.edgeSource5, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));

  QObject::connect(this->Widgets.edgeTarget1, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.edgeTarget2, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.edgeTarget3, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.edgeTarget4, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.edgeTarget5, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
}

void TableToGraphPanel::accept()
{
  QStringList link_vertices;
  if(!this->Widgets.vertexField1->text().isEmpty() && !this->Widgets.vertexDomain1->text().isEmpty())
    link_vertices << this->Widgets.vertexField1->text() << this->Widgets.vertexDomain1->text() << (this->Widgets.vertexHidden1->isChecked() ? "1" : "0");
  if(!this->Widgets.vertexField2->text().isEmpty() && !this->Widgets.vertexDomain2->text().isEmpty())
    link_vertices << this->Widgets.vertexField2->text() << this->Widgets.vertexDomain2->text() << (this->Widgets.vertexHidden2->isChecked() ? "1" : "0");
  if(!this->Widgets.vertexField3->text().isEmpty() && !this->Widgets.vertexDomain3->text().isEmpty())
    link_vertices << this->Widgets.vertexField3->text() << this->Widgets.vertexDomain3->text() << (this->Widgets.vertexHidden3->isChecked() ? "1" : "0");
  if(!this->Widgets.vertexField4->text().isEmpty() && !this->Widgets.vertexDomain4->text().isEmpty())
    link_vertices << this->Widgets.vertexField4->text() << this->Widgets.vertexDomain4->text() << (this->Widgets.vertexHidden4->isChecked() ? "1" : "0");
  if(!this->Widgets.vertexField5->text().isEmpty() && !this->Widgets.vertexDomain5->text().isEmpty())
    link_vertices << this->Widgets.vertexField5->text() << this->Widgets.vertexDomain5->text() << (this->Widgets.vertexHidden5->isChecked() ? "1" : "0");

  QStringList link_edges;
  if(!this->Widgets.edgeSource1->text().isEmpty() && !this->Widgets.edgeTarget1->text().isEmpty())
    link_edges << this->Widgets.edgeSource1->text() << this->Widgets.edgeTarget1->text();
  if(!this->Widgets.edgeSource2->text().isEmpty() && !this->Widgets.edgeTarget2->text().isEmpty())
    link_edges << this->Widgets.edgeSource2->text() << this->Widgets.edgeTarget2->text();
  if(!this->Widgets.edgeSource3->text().isEmpty() && !this->Widgets.edgeTarget3->text().isEmpty())
    link_edges << this->Widgets.edgeSource3->text() << this->Widgets.edgeTarget3->text();
  if(!this->Widgets.edgeSource4->text().isEmpty() && !this->Widgets.edgeTarget4->text().isEmpty())
    link_edges << this->Widgets.edgeSource4->text() << this->Widgets.edgeTarget4->text();
  if(!this->Widgets.edgeSource5->text().isEmpty() && !this->Widgets.edgeTarget5->text().isEmpty())
    link_edges << this->Widgets.edgeSource5->text() << this->Widgets.edgeTarget5->text();

  vtkSMProxy* const table_to_graph = this->referenceProxy()->getProxy();

  pqPropertyHelper(table_to_graph, "LinkVertices").Set(link_vertices);
  pqPropertyHelper(table_to_graph, "LinkEdges").Set(link_edges);
  table_to_graph->UpdateVTKObjects();
 
  Superclass::accept();    
}

void TableToGraphPanel::reset()
{
  Superclass::reset();
}

