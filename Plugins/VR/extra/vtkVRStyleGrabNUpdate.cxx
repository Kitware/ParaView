#include "vtkVRStyleGrabNUpdateMatrix.h"

#include "pqActiveObjects.h"
#include "pqDataRepresentation.h"
#include "pqView.h"
#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkPVXMLElement.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkTransform.h"
#include "vtkVRQueue.h"
#include <algorithm>
#include <sstream>

vtkVRStyleGrabNUpdateMatrix::vtkVRStyleGrabNUpdateMatrix(QObject* parentObject)
  : Superclass(parentObject)
{
  this->Proxy = 0;
  this->Property = 0;
  this->ProxyName = "";
  this->PropertyName = "";
  this->IsFoundProxyProperty = false;
  this->Enabled = false;
  this->IsInitialRecorded = false;
  this->InitialInvertedPose = vtkTransform::New();
}

vtkVRStyleGrabNUpdateMatrix::~vtkVRStyleGrabNUpdateMatrix()
{
  this->InitialInvertedPose->Delete();
}

//-----------------------------------------------------------------------public
bool vtkVRStyleGrabNUpdateMatrix::configure(vtkPVXMLElement* child, vtkSMProxyLocator* locator)
{
  if (Superclass::configure(child, locator))
  {
    vtkPVXMLElement* button = child->GetNestedElement(1);
    if (button && button->GetName() && strcmp(button->GetName(), "Button") == 0)
    {
      this->Button = button->GetAttributeOrEmpty("name");
    }
    else
    {
      std::cerr << "vtkVRStyleGrabNUpdateMatrix::configure(): "
                << "Please Specify Button event" << std::endl
                << "<Button name=\"ButtonEventName\"/>" << std::endl;
      return false;
    }
    vtkPVXMLElement* property = child->GetNestedElement(2);
    if (property && property->GetName() && strcmp(property->GetName(), "MatrixProperty") == 0)
    {
      std::string propertyStr = property->GetAttributeOrEmpty("name");
      std::vector<std::string> token = this->tokenize(propertyStr);
      if (token.size() != 2)
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
      std::cerr << "vtkVRStyleGrabNUpdateMatrix::configure(): "
                << "Please Specify Property" << std::endl
                << "<Property name=\"ProxyName.PropertyName\"/>" << std::endl;
      return false;
    }
    return true;
  }
  return false;
}

// -----------------------------------------------------------------------public
vtkPVXMLElement* vtkVRStyleGrabNUpdateMatrix::saveConfiguration() const
{
  vtkPVXMLElement* child = Superclass::saveConfiguration();

  vtkPVXMLElement* button = vtkPVXMLElement::New();
  button->SetName("Button");
  button->AddAttribute("name", this->Button.c_str());
  child->AddNestedElement(button);
  button->FastDelete();

  vtkPVXMLElement* property = vtkPVXMLElement::New();
  property->SetName("MatrixProperty");
  std::stringstream propertyStr;
  propertyStr << this->ProxyName << "." << this->PropertyName;
  property->AddAttribute("name", propertyStr.str().c_str());
  child->AddNestedElement(property);
  property->FastDelete();

  return child;
}

// ----------------------------------------------------------------------------
void vtkVRStyleGrabNUpdateMatrix::HandleButton(const vtkVREventData& data)
{
  if (this->Button == data.name)
  {
    this->Enabled = data.data.button.state;
  }
}

// ----------------------------------------------------------------------------
bool vtkVRStyleGrabNUpdateMatrix::GetProxyNProperty()
{
  if (this->GetProxy(this->ProxyName, &this->Proxy))
  {
    if (!this->GetProperty(this->Proxy, this->PropertyName, &this->Property))
    {
      std::cerr << this->metaObject()->className() << "::GetProxyNProperty" << std::endl
                << "Proxy ( " << this->ProxyName << ") :"
                << "Property ( " << this->PropertyName << ") :Not Found" << std::endl;
      return false;
    }
  }
  else
  {
    std::cerr << this->metaObject()->className() << "::GetProxyNProperty" << std::endl
              << "Proxy ( " << this->ProxyName << ") :Not Found" << std::endl;
    return false;
  }
  return true;
}

// ----------------------------------------------------------------------------
void vtkVRStyleGrabNUpdateMatrix::HandleTracker(const vtkVREventData& data)
{
  if (this->Tracker == data.name)
  {
    if (this->Enabled)
    {
      if (!this->IsInitialRecorded)
      {
        this->InitialInvertedPose->SetMatrix(data.data.tracker.matrix);
        this->InitialInvertedPose->Inverse();
        double wandPose[16];
        vtkSMPropertyHelper(this->Proxy, this->PropertyName.c_str()).Get(wandPose, 16);
        this->InitialInvertedPose->Concatenate(wandPose);
        this->GetPropertyData();
        this->IsInitialRecorded = true;
      }
      else
      {
        this->OutPose->SetMatrix(data.data.tracker.matrix);
        this->OutPose->Concatenate(this->InitialInvertedPose);
        this->SetProperty();
      }
    }
    else
    {
      this->IsInitialRecorded = false;
    }
  }
}
