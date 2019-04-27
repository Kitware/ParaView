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

#include <sstream>
#include <string>

#include <QFile>
#include <QWidget>
#include <QtDebug>

#include "pqCoreTestUtility.h"
#include "pqObjectNaming.h"
#include "pqOptions.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkProcessModule.h"
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

pqXMLEventSource::pqXMLEventSource(QObject* p)
  : pqEventSource(p)
  , Implementation(new pqImplementation())
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

  vtkSmartPointer<vtkPVXMLParser> parser = vtkSmartPointer<vtkPVXMLParser>::New();

  if (!parser->Parse(dat.data()))
  {
    qDebug() << "Failed to parse " << xmlfilename;
    xml.close();
    return;
  }

  vtkPVXMLElement* elem = parser->GetRootElement();
  if (QString(elem->GetName()) != "pqevents")
  {
    qCritical() << xmlfilename << " is not an XML test case document";
    return;
  }

  this->Implementation->XML = elem;
  this->Implementation->CurrentEvent = 0;
}

int pqXMLEventSource::getNextEvent(
  QString& object, QString& command, QString& arguments, int& eventType)
{
  if (this->Implementation->XML->GetNumberOfNestedElements() == this->Implementation->CurrentEvent)
  {
    return DONE;
  }

  int index = this->Implementation->CurrentEvent;
  this->Implementation->CurrentEvent++;

  auto options = pqOptions::SafeDownCast(vtkProcessModule::GetProcessModule()->GetOptions());

  vtkPVXMLElement* elem = this->Implementation->XML->GetNestedElement(index);

  // First handle specific cases
  if (elem->GetName() && strcmp(elem->GetName(), "pqcompareview") == 0 &&
    elem->GetAttribute("object") && elem->GetAttribute("baseline"))
  {
    // add support for elements of the form:
    // <pqcompareview object="../Viewport"
    //                baseline="ExtractBlock.png"
    //                width="300" height="300" />

    // Recover object name
    QString widgetName = elem->GetAttribute("object");

    // Recover baseline file location
    QString baseline = pqCoreTestUtility::fixPath(elem->GetAttribute("baseline"));

    // Recover optional width and height
    int width = 300, height = 300;
    elem->GetScalarAttribute("width", &width);
    elem->GetScalarAttribute("height", &height);

    // Recover optional threshold
    int threshold = 0;
    if (elem->GetScalarAttribute("threshold", &threshold) && threshold >= 0)
    {
      // use the threshold specified by the XML
    }
    else
    {
      threshold = options->GetCurrentImageThreshold();
    }

    // Find widget to screenshot with
    QWidget* widget = qobject_cast<QWidget*>(pqObjectNaming::GetObject(widgetName));
    if (!widget)
    {
      return FAILURE;
    }

    // Recover test directory
    QString testDir = pqCoreTestUtility::TestDirectory();

    // If test directory is empty, try to create one along the xml test.
    if (testDir.isEmpty())
    {
      testDir = pqCoreTestUtility::BaselineDirectory();
      QString path("Testing/Temporary");
      QDir testQDir(testDir);
      testQDir.mkpath(path);
      if (!testQDir.cd(path))
      {
        qCritical()
          << "ERROR: The following event FAILED !!! Impossible to find or create a Test directory.";
        qCritical() << "Yet will continue with the rest of the test.";
        qCritical()
          << "----------------------------------------------------------------------------------";
        std::stringstream ss;
        elem->PrintXML(ss, vtkIndent());
        std::string sstr(ss.str());
        qCritical() << sstr.c_str();
        qCritical()
          << "----------------------------------------------------------------------------------";
        return this->getNextEvent(object, command, arguments, eventType);
      }
      else
      {
        testDir = testQDir.path();
      }
    }

    // Compare image with widget
    bool retVal = pqCoreTestUtility::CompareImage(
      widget, baseline, threshold, std::cerr, testDir, QSize(width, height));
    if (!retVal)
    {
      qCritical()
        << "ERROR: The following event FAILED !!! Yet will continue with the rest of the test.";
      qCritical()
        << "----------------------------------------------------------------------------------";
      std::stringstream ss;
      elem->PrintXML(ss, vtkIndent());
      std::string sstr(ss.str());
      qCritical() << sstr.c_str();
      qCritical()
        << "----------------------------------------------------------------------------------";
    }
    return this->getNextEvent(object, command, arguments, eventType);
  }
  else if (elem->GetName() && strcmp(elem->GetName(), "pqcompareimage") == 0 &&
    elem->GetAttribute("image") && elem->GetAttribute("baseline"))
  {
    // add support for elements of the form:
    // This only support PNG files.
    // <pqcompareimage image="GeneratedImage.png"
    //                baseline="ExtractBlock.png"
    //                width="300" height="300" />
    QString image = pqCoreTestUtility::fixPath(elem->GetAttribute("image"));

    QString baseline = pqCoreTestUtility::fixPath(elem->GetAttribute("baseline"));

    if (!pqCoreTestUtility::CompareImage(image, baseline, options->GetCurrentImageThreshold(),
          std::cerr, pqCoreTestUtility::TestDirectory()))
    {
      qCritical()
        << "ERROR: The following event FAILED !!! Yet will continue with the rest of the test.";
      qCritical()
        << "----------------------------------------------------------------------------------";
      std::stringstream ss;
      elem->PrintXML(ss, vtkIndent());
      std::string sstr(ss.str());
      qCritical() << sstr.c_str();
      qCritical()
        << "----------------------------------------------------------------------------------";
    }
    return this->getNextEvent(object, command, arguments, eventType);
  }
  else if (elem->GetName() && strcmp(elem->GetName(), "pqcomparetile") == 0 &&
    elem->GetAttribute("object") && elem->GetAttribute("rank") && elem->GetAttribute("baseline"))
  {
    // support elements of the following form.
    //
    // <pqcomparetile rank="0" baseline="image.png" object="a-view-widget" tdx="1" tdy="1" />

    // find widget; while we really need active layout, since pqCore has no
    // concept of active, we determine the layout to use from the view.
    QWidget* widget =
      qobject_cast<QWidget*>(pqObjectNaming::GetObject(elem->GetAttribute("object")));
    if (!widget)
    {
      return FAILURE;
    }

    int rank = 0;
    elem->GetScalarAttribute("rank", &rank);

    // tdx, tdy helps used determine when this image compare is active.
    // 0 means not specified, indicating always do the compare.
    int tdx = 0, tdy = 0;
    elem->GetScalarAttribute("tdx", &tdx);
    elem->GetScalarAttribute("tdy", &tdy);
    const QString baseline = pqCoreTestUtility::fixPath(elem->GetAttribute("baseline"));
    int threshold = 0;
    if (!elem->GetScalarAttribute("threshold", &threshold))
    {
      threshold = options->GetCurrentImageThreshold();
    }

    if (!pqCoreTestUtility::CompareTile(widget, rank, tdx, tdy, baseline, threshold, std::cerr,
          pqCoreTestUtility::TestDirectory()))
    {
      qCritical()
        << "ERROR: The following event FAILED !!! Yet will continue with the rest of the test.";
      qCritical()
        << "----------------------------------------------------------------------------------";
      std::stringstream ss;
      elem->PrintXML(ss, vtkIndent());
      std::string sstr(ss.str());
      qCritical() << sstr.c_str();
      qCritical()
        << "----------------------------------------------------------------------------------";
    }
    return this->getNextEvent(object, command, arguments, eventType);
  }

  // Then generic cases
  else if (elem->GetName() && strcmp(elem->GetName(), "pqevent") == 0)
  {
    eventType = pqEventTypes::ACTION_EVENT;
    object = elem->GetAttribute("object");
    command = elem->GetAttribute("command");
    arguments = elem->GetAttribute("arguments");
    arguments = pqCoreTestUtility::fixPath(arguments);
    return SUCCESS;
  }
  else if (elem->GetName() && strcmp(elem->GetName(), "pqcheck") == 0)
  {
    eventType = pqEventTypes::CHECK_EVENT;
    object = elem->GetAttribute("object");
    command = elem->GetAttribute("property");
    arguments = elem->GetAttribute("arguments");
    return SUCCESS;
  }

  qCritical() << "Invalid xml element: " << elem->GetName();
  return FAILURE;
}
