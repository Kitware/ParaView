#include "vtkVRStyleTracking.h"

#include "pqActiveObjects.h"
#include "pqDataRepresentation.h"
#include "pqView.h"
#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkPVXMLElement.h"
#include "vtkVRQueue.h"
#include <sstream>
#include <algorithm>

vtkVRStyleTracking::vtkVRStyleTracking(QObject* parentObject) :
  Superclass(parentObject)
{
  this->Proxy =0;
  this->Property =0;
  this->PropertyName = "";
  this->IsFoundProxyProperty = GetProxyNProperty();
}

//-----------------------------------------------------------------------public
bool vtkVRStyleTracking::configure(vtkPVXMLElement* child,
                                   vtkSMProxyLocator* locator)
{
  if ( Superclass::configure( child,locator ) )
    {
    if ( child->GetNumberOfNestedElements() !=2 )
      {
      std::cerr << "vtkVRStyleTracking::configure(): "
                << "There has to be only 2 elements present " << std::endl
                << "<Property name=\"ProxyName.PropertyName\"/>" << std::endl
                << "<Tracker name=\"trackerEventName\"/>"
                << std::endl;
      }
    vtkPVXMLElement* property = child->GetNestedElement(0);
    if (property && property->GetName() && strcmp(property->GetName(), "Property")==0)
      {
      std::string propertyStr = property->GetAttributeOrEmpty("name");
      std::vector<std::string> token = this->tokenize( propertyStr );
      if ( token.size()!=2 )
        {
        std::cerr << "Expected Property \"name\" Format:  Proxy.Property" << std::endl;
        return false;
        }
      else
        {
        this->ProxyName = token[0];
        this->PropertyName = token[1];
        this->IsFoundProxyProperty = GetProxyNProperty();
        }
      }
    else
      {
      std::cerr << "vtkVRStyleTracking::configure(): "
                << "Please Specify Property" <<std::endl
                << "<Property name=\"ProxyName.PropertyName\"/>"
                << std::endl;
      return false;
      }
    vtkPVXMLElement* tracker = child->GetNestedElement(1);
    if (tracker && tracker->GetName() && strcmp(tracker->GetName(), "Tracker")==0)
      {
      this->Tracker = tracker->GetAttributeOrEmpty("name");
      }
    else
      {
      std::cerr << "vtkVRStyleTracking::configure(): "
                << "Please Specify Tracker event" <<std::endl
                << "<Tracker name=\"TrackerEventName\"/>"
                << std::endl;
      return false;
      }
    return true;
    }
  return false;
}

// -----------------------------------------------------------------------public
vtkPVXMLElement* vtkVRStyleTracking::saveConfiguration() const
{
  vtkPVXMLElement* child = Superclass::saveConfiguration();

  vtkPVXMLElement* property = vtkPVXMLElement::New();
  property->SetName("Property");
  std::stringstream propertyStr;
  propertyStr << this->ProxyName << "." << this->PropertyName;
  property->AddAttribute("name", propertyStr.str().c_str() );
  child->AddNestedElement(property);
  property->FastDelete();

  vtkPVXMLElement* tracker = vtkPVXMLElement::New();
  tracker->SetName("Tracker");
  tracker->AddAttribute("name",this->Tracker.c_str() );
  child->AddNestedElement(tracker);
  tracker->FastDelete();

  return child;
}

// ----------------------------------------------------------------------------
void vtkVRStyleTracking::HandleTracker( const vtkVREventData& data )
{
  if ( this->Tracker == data.name)
    {
    this->SetProperty( data );
    }
}

// ----------------------------------------------------------------------------
bool vtkVRStyleTracking::GetProxyNProperty()
{
  pqView *view = 0;
  view = pqActiveObjects::instance().activeView();
  if ( view )
    {
    this->Proxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() );
    if ( this->Proxy )
      {
      this->Property = vtkSMDoubleVectorProperty::SafeDownCast(this->Proxy->GetProperty( this->PropertyName.c_str() ) );
      if ( this->Property )
        {
        return true;
        }
      }
    }
  return false;
}

// ----------------------------------------------------------------------------
bool vtkVRStyleTracking::SetProperty(const vtkVREventData &data)
{
  for (int i = 0; i < 16; ++i)
    {
    this->OutProperty->SetElement( i, data.data.tracker.matrix[i] );
    }
  return true;
}
