/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNewWidgetRepresentationProxyAbstract.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkSMNewWidgetRepresentationProxyAbstract_h
#define vtkSMNewWidgetRepresentationProxyAbstract_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMProxy.h"

#include "vtkCommand.h"      // for vtkCommand
#include "vtkSmartPointer.h" // for vtkSmartPointer
#include "vtkWeakPointer.h"  // for vtkWeakPointer

#include <list> // for std::list

class vtkSMLink;
class vtkSMPropertyGroup;

/**
 * @class   vtkSMNewWidgetRepresentationProxyAbstract
 * @brief   Abstract class for proxies for 2D and 3D widgets.
 *
 * vtkSMNewWidgetRepresentationProxyAbstract is an abstract class for
 * proxies representing 2D or 3D widgets. It sets up all the properties link
 * needed so proxies are in sync.
 */
class VTKREMOTINGVIEWS_EXPORT vtkSMNewWidgetRepresentationProxyAbstract : public vtkSMProxy
{
public:
  vtkTypeMacro(vtkSMNewWidgetRepresentationProxyAbstract, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Called to link properties from a Widget to \c controlledProxy i.e. a
   * proxy whose properties are being manipulated using this Widget.
   * Currently, we only support linking with one controlled proxy at a time. One
   * must call UnlinkProperties() before one can call this method on another
   * controlledProxy. The \c controlledPropertyGroup is used to determine the
   * mapping between this widget properties and controlledProxy properties.
   */
  bool LinkProperties(vtkSMProxy* controlledProxy, vtkSMPropertyGroup* controlledPropertyGroup);
  bool UnlinkProperties(vtkSMProxy* controlledProxy);
  //@}

protected:
  vtkSMNewWidgetRepresentationProxyAbstract();
  ~vtkSMNewWidgetRepresentationProxyAbstract() override;

  /**
   * Called every time the user interacts with the widget. This should be override
   * by each subclass.
   */
  virtual void ExecuteEvent(unsigned long event) = 0;

  /**
   * Called everytime a controlled property's unchecked values change.
   */
  void ProcessLinkedPropertyEvent(vtkSMProperty* caller, unsigned long event);

  /**
   * Setup all links between properties and informations. It also update properties
   * information before anything else. Usually called in CreateVTKObject() function.
   */
  void SetupPropertiesLinks();

  class vtkSMWidgetObserver;
  vtkNew<vtkSMWidgetObserver> Observer;

private:
  vtkSMNewWidgetRepresentationProxyAbstract(
    const vtkSMNewWidgetRepresentationProxyAbstract&) = delete;
  void operator=(const vtkSMNewWidgetRepresentationProxyAbstract&) = delete;

  vtkWeakPointer<vtkSMProxy> ControlledProxy;
  vtkWeakPointer<vtkSMPropertyGroup> ControlledPropertyGroup;

  typedef std::list<vtkSmartPointer<vtkSMLink>> LinksType;
  LinksType Links;
};

//----------------------------------------------------------------------------
class vtkSMNewWidgetRepresentationProxyAbstract::vtkSMWidgetObserver : public vtkCommand
{
public:
  static vtkSMWidgetObserver* New()
  {
    return new vtkSMNewWidgetRepresentationProxyAbstract::vtkSMWidgetObserver();
  }

  vtkSMWidgetObserver();
  void Execute(vtkObject* caller, unsigned long event, void*) override;

  vtkWeakPointer<vtkSMNewWidgetRepresentationProxyAbstract> WidgetRepresentation;
};

#endif
