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
#include "vtkTransform.h"
#include "vtkVRQueue.h"
#include <sstream>
#include <algorithm>

// -----------------------------------------------------------------------cnstr
vtkVRStyleTracking::vtkVRStyleTracking(QObject* parentObject) :
  Superclass(parentObject)
{
  this->OutPose = vtkTransform::New();
}

// -----------------------------------------------------------------------destr
vtkVRStyleTracking::~vtkVRStyleTracking()
{
  this->OutPose->Delete();
}

//-----------------------------------------------------------------------public
bool vtkVRStyleTracking::configure(vtkPVXMLElement* child,
                                   vtkSMProxyLocator* locator)
{
  if ( Superclass::configure( child,locator ) )
    {
    vtkPVXMLElement* tracker = child->GetNestedElement(0);
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
    this->OutPose->SetMatrix( data.data.tracker.matrix );
    this->SetProperty( );
    }
}

// ----------------------------------------------------------------------------
void vtkVRStyleTracking::SetProperty()
{
  vtkSMPropertyHelper(this->OutProxy,this->OutPropertyName.c_str())
    .Set(&this->OutPose->GetMatrix()->Element[0][0],16 );
}

bool vtkVRStyleTracking::update()
{
  this->OutProxy->UpdateVTKObjects();
  ( ( vtkSMRenderViewProxy* )  this->OutProxy )->StillRender();
  return false;
}
