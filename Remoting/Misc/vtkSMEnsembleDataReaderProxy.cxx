#include "vtkSMEnsembleDataReaderProxy.h"

#include "vtkClientServerStream.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVEnsembleDataReaderInformation.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMReaderFactory.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"

#include <cassert>
#include <vector>

vtkStandardNewMacro(vtkSMEnsembleDataReaderProxy);
//-----------------------------------------------------------------------------
vtkSMEnsembleDataReaderProxy::vtkSMEnsembleDataReaderProxy()
{
  this->FileNamePotentiallyModified = false;
}

//-----------------------------------------------------------------------------
vtkSMEnsembleDataReaderProxy::~vtkSMEnsembleDataReaderProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMEnsembleDataReaderProxy::SetPropertyModifiedFlag(const char* name, int flag)
{
  this->Superclass::SetPropertyModifiedFlag(name, flag);
  if (strcmp(name, "FileName") == 0)
  {
    this->FileNamePotentiallyModified = true;
  }
}

//-----------------------------------------------------------------------------
void vtkSMEnsembleDataReaderProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();
  if (this->FileNamePotentiallyModified)
  {
    this->FetchFileNames();
    this->FileNamePotentiallyModified = false;
  }
}

//-----------------------------------------------------------------------------
bool vtkSMEnsembleDataReaderProxy::FetchFileNames()
{
  // Gather information
  vtkNew<vtkPVEnsembleDataReaderInformation> info;
  assert(info.GetPointer());
  this->GatherInformation(info.GetPointer());

  // Create reader factory
  vtkSMReaderFactory* readerFactory = vtkSMProxyManager::GetProxyManager()->GetReaderFactory();
  if (!readerFactory || readerFactory->GetNumberOfRegisteredPrototypes() == 0)
  {
    vtkErrorMacro("No reader factory found. Cannot determine reader types!!!");
    return false;
  }

  if (info->GetFileCount() == 0)
  {
    return false;
  }

  // Get session and session proxy manager
  vtkSMSession* session = this->GetSession();
  assert(session);
  vtkSMSessionProxyManager* spm = session->GetSessionProxyManager();
  assert(spm);

  // Stream reader proxies to VTK object
  std::vector<vtkSmartPointer<vtkSMProxy> > proxies;
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "ResetReaders"
         << vtkClientServerStream::End;
  for (unsigned int i = 0, max = info->GetFileCount(); i < max; ++i)
  {
    std::string filePath = info->GetFilePath(i);
    if (readerFactory->CanReadFile(filePath.c_str(), session))
    {
      // Create reader proxy
      const char* readerGroup = readerFactory->GetReaderGroup();
      const char* readerName = readerFactory->GetReaderName();
      vtkSMProxy* proxy = spm->NewProxy(readerGroup, readerName);
      if (!proxy)
      {
        return false;
      }
      const char* fileNameProperty = vtkSMCoreUtilities::GetFileNameProperty(proxy);
      assert(fileNameProperty);

      vtkSMPropertyHelper(proxy, fileNameProperty).Set(filePath.c_str());
      proxy->UpdateVTKObjects();

      // Push to stream
      stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "SetReader" << i
             << VTKOBJECT(proxy) << vtkClientServerStream::End;
      proxies.push_back(proxy);
      proxy->Delete();
    }
    else
    {
      vtkErrorMacro("Cannot create a reader for: " << filePath.c_str());
    }
  }
  this->ExecuteStream(stream);
  return true;
}

//-----------------------------------------------------------------------------
void vtkSMEnsembleDataReaderProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
