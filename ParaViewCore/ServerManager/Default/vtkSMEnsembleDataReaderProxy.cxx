#include "vtkSMEnsembleDataReaderProxy.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerStream.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include "vtkSMCoreUtilities.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSession.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMReaderFactory.h"

#include "vtkPVEnsembleDataReaderInformation.h"

#include <cassert>
#include <vector>

struct vtkSMEnsembleDataReaderProxyInternal
{
  bool FileNamePotentiallyModified;
};

vtkStandardNewMacro(vtkSMEnsembleDataReaderProxy);

vtkSMEnsembleDataReaderProxy::vtkSMEnsembleDataReaderProxy()
{
  this->Internal = new vtkSMEnsembleDataReaderProxyInternal;
  this->Internal->FileNamePotentiallyModified = false;
}

vtkSMEnsembleDataReaderProxy::~vtkSMEnsembleDataReaderProxy()
{
  delete this->Internal;
}

void vtkSMEnsembleDataReaderProxy::SetPropertyModifiedFlag(const char *name,
                                                           int flag)
{
  this->Superclass::SetPropertyModifiedFlag(name, flag);

  if (strcmp(name, "FileName") == 0)
    {
    vtkSMProperty *property = this->GetProperty("FileName");
    if (property->IsA("vtkSMStringVectorProperty"))
      {
      vtkSMStringVectorProperty *pv =
        vtkSMStringVectorProperty::SafeDownCast(property);
      if (pv->GetNumberOfElements() > 0)
        {
        const char *fileName = pv->GetElement(0);
        if (fileName && fileName[0])
          {
            this->Internal->FileNamePotentiallyModified = true;
          }
        }
      }
    }
}

void vtkSMEnsembleDataReaderProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();

  if (this->Internal->FileNamePotentiallyModified)
    {
    this->FetchFileNames();
    this->Internal->FileNamePotentiallyModified = false;
    }
}

bool vtkSMEnsembleDataReaderProxy::FetchFileNames()
{
  // Gather information
  vtkNew<vtkPVEnsembleDataReaderInformation> info;
  assert(info.GetPointer());
  this->GatherInformation(info.GetPointer());

  // Create reader factory
  vtkSMReaderFactory *readerFactory =
    vtkSMProxyManager::GetProxyManager()->GetReaderFactory();
  assert(readerFactory &&
         readerFactory->GetNumberOfRegisteredPrototypes() > 0);

  // Get session and session proxy manager
  vtkSMSession *session = this->GetSession();
  assert(session);
  vtkSMSessionProxyManager *spm = session->GetSessionProxyManager();
  assert(spm);

  // Stream reader proxies to VTK object
  vtkClientServerStream stream;
  std::vector<vtkSmartPointer<vtkSMProxy> > proxies;
  for (int i = 0; i < info->GetFileCount(); ++i)
    {
    vtkStdString filePath = info->GetFilePath(i);
    if (readerFactory->CanReadFile(filePath.c_str(), session))
      {
      // Create reader proxy
      const char *readerGroup = readerFactory->GetReaderGroup();
      const char *readerName = readerFactory->GetReaderName();
      vtkSMProxy *proxy = spm->NewProxy(readerGroup, readerName);
      if (!proxy)
        {
        return false;
        }
      const char *fileNameProperty = vtkSMCoreUtilities::GetFileNameProperty(proxy);
      vtkSMPropertyHelper(proxy, fileNameProperty).Set(filePath);
      proxy->UpdateVTKObjects();

      // Push to stream
      stream << vtkClientServerStream::Invoke
             << VTKOBJECT(this) << "SetReader" << i << VTKOBJECT(proxy)
             << vtkClientServerStream::End;

      proxies.push_back(proxy);
      proxy->Delete();
      }
    }
  this->ExecuteStream(stream);
  return true;
}

void vtkSMEnsembleDataReaderProxy::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  // FileNamePotentiallyModified
  os << indent << "FileNamePotentiallyModified: ";
  if (this->Internal)
    {
    os << this->Internal->FileNamePotentiallyModified << endl;
    }
  else
    {
    os << "(NULL)" << endl;
    }
}
