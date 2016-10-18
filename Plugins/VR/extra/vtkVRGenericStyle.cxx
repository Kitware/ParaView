#include "vtkVRGenericStyle.h"

#include "pqActiveObjects.h"
#include "pqDataRepresentation.h"
#include "pqView.h"
#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkVRQueue.h"

vtkVRGenericStyle::vtkVRGenericStyle(QObject* parentObject)
  : Superclass(parentObject)
{
}

vtkVRGenericStyle::~vtkVRGenericStyle()
{
}

// ----------------------------------------------------------------------------
// This handler currently only get the
bool vtkVRGenericStyle::handleEvent(const vtkVREventData& data)
{
  std::cout << "Event Name = " << data.name << std::endl;
  switch (data.eventType)
  {
    case BUTTON_EVENT:
      this->HandleButton(data);
      break;
    case ANALOG_EVENT:
      this->HandleAnalog(data);
      break;
    case TRACKER_EVENT:
      this->HandleTracker(data);
      break;
  }
  return false;
}

// ----------------------------------------------------------------------------
void vtkVRGenericStyle::HandleTracker(const vtkVREventData& data)
{
  std::cout << "(Tracker"
            << "\n"
            << "  :from  " << data.connId << "\n"
            << "  :time  " << data.timeStamp << "\n"
            << "  :id    " << data.data.tracker.sensor << "\n"
            << "  :matrix   '( ";
  for (int i = 0; i < 4; ++i)
  {
    std::cout << std::endl;
    for (int j = 0; j < 4; ++j)
    {
      std::cout << data.data.tracker.matrix[i * 4 + j] << " ";
    }
  }
  std::cout << " )"
            << "\n"
            << " ))"
            << "\n";
}

// ----------------------------------------------------------------------------
void vtkVRGenericStyle::HandleButton(const vtkVREventData& data)
{
  std::cout << "(Button"
            << "\n"
            << "  :from  " << data.connId << "\n"
            << "  :time  " << data.timeStamp << "\n"
            << "  :id    " << data.data.button.button << "\n"
            << "  :state " << data.data.button.state << " )"
            << "\n";
}

// ----------------------------------------------------------------------------
void vtkVRGenericStyle::HandleAnalog(const vtkVREventData& data)
{
  std::cout << "(Analog"
            << "\n"
            << "  :from  " << data.connId << "\n"
            << "  :time  " << data.timeStamp << "\n"
            << "  :channel '(";
  for (int i = 0; i < data.data.analog.num_channel; i++)
  {
    std::cout << data.data.analog.channel[i];
  }
  std::cout << " ))"
            << "\n";
}

bool vtkVRGenericStyle::update()
{
  return false;
}
