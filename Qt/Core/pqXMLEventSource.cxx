/*=========================================================================

   Program: ParaView
   Module:    pqXMLEventSource.cxx

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

#include "pqXMLEventSource.h"

#include <QFile>
#include <QtDebug>

#include "vtkPVXMLParser.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"

///////////////////////////////////////////////////////////////////////////
// pqXMLEventSource::pqImplementation

class pqXMLEventSource::pqImplementation
{
public:
  vtkSmartPointer<vtkPVXMLElement> XML;
  unsigned int CurrentEvent;
};

///////////////////////////////////////////////////////////////////////////////////////////
// pqXMLEventSource

pqXMLEventSource::pqXMLEventSource(QObject* p) :
  pqEventSource(p),
  Implementation(new pqImplementation())
{
}

pqXMLEventSource::~pqXMLEventSource()
{
  delete this->Implementation;
}

void pqXMLEventSource::setContent(const QString& xmlfilename)
{
  QFile xml(xmlfilename);
  if (!xml.open(QIODevice::ReadOnly))
    {
    qDebug() << "Failed to load " << xmlfilename;
    return;
    }

  QByteArray dat = xml.readAll();
  
  vtkSmartPointer<vtkPVXMLParser> parser = 
    vtkSmartPointer<vtkPVXMLParser>::New();
  
  if(!parser->Parse(dat.data()))
    {
    qDebug() << "Failed to parse " << xmlfilename;
    xml.close();
    return;
    }
  
  vtkPVXMLElement* elem = parser->GetRootElement();
  if(QString(elem->GetName()) != "pqevents")
    {
    qCritical() << xmlfilename << " is not an XML test case document";
    return;
    }

  this->Implementation->XML = elem;
  this->Implementation->CurrentEvent = 0;
}

int pqXMLEventSource::getNextEvent(
  QString& object,
  QString& command,
  QString& arguments)
{
  if(this->Implementation->XML->GetNumberOfNestedElements() ==
    this->Implementation->CurrentEvent)
    {
    return DONE;
    }

  vtkPVXMLElement* elem = this->Implementation->XML->GetNestedElement(
      this->Implementation->CurrentEvent);
    
  object = elem->GetAttribute("object");
  command = elem->GetAttribute("command");
  arguments = elem->GetAttribute("arguments");
  
  this->Implementation->CurrentEvent++;

  return SUCCESS;
}

