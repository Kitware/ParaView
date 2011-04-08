#include "vtkSpyPlotReaderMap.h"
#include "vtkSpyPlotReader.h"
#include "vtkSpyPlotUniReader.h"

void vtkSpyPlotReaderMap::Clean(vtkSpyPlotUniReader* save)
{
  MapOfStringToSPCTH::iterator it;
  MapOfStringToSPCTH::iterator end=this->Files.end();
  for (it=this->Files.begin();it!=end; ++it)
    {
    if ( it->second && it->second != save )
      {
      it->second->Delete();
      it->second = 0;
      }
    }
  this->Files.erase(this->Files.begin(),end);
}

void vtkSpyPlotReaderMap::Initialize(const char *file)
{
  if ( !file || file != this->MasterFileName )
    {
    this->Clean(0);
    }
}

vtkSpyPlotUniReader* 
vtkSpyPlotReaderMap::GetReader(MapOfStringToSPCTH::iterator& it, 
                               vtkSpyPlotReader* parent)
{
  if ( !it->second )
    {
    it->second = vtkSpyPlotUniReader::New();
    it->second->SetCellArraySelection(parent->GetCellDataArraySelection());
    it->second->SetFileName(it->first.c_str());
    //cout << parent->GetController()->GetLocalProcessId() 
    // << "Create reader: " << it->second << endl;
    }
  return it->second;
}

void vtkSpyPlotReaderMap::TellReadersToCheck(vtkSpyPlotReader *parent)
{
  MapOfStringToSPCTH::iterator it;
  MapOfStringToSPCTH::iterator end=this->Files.end();
  for (it=this->Files.begin();it!=end; ++it)
    {
    this->GetReader(it, parent)->SetNeedToCheck(1);
    }
}
