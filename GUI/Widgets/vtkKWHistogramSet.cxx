/*=========================================================================

  Module:    vtkKWHistogramSet.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWHistogramSet.h"

#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkKWHistogram.h"
#include "vtkLinkedList.txx"
#include "vtkLinkedListIterator.txx"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkKWHistogramSet);
vtkCxxRevisionMacro(vtkKWHistogramSet, "1.1");

//----------------------------------------------------------------------------
class vtkKWHistogramCallback : public vtkCommand
{
public:
  static vtkKWHistogramCallback *New()
    { return new vtkKWHistogramCallback; }
  virtual void Execute(vtkObject *caller, unsigned long event, void *calldata);
  
  vtkKWHistogramSet *Self;
  float StartProgressValue;
  float SpanProgressValue;

protected:
  vtkKWHistogramCallback();
};

vtkKWHistogramCallback::vtkKWHistogramCallback()
{
  this->Self = 0;
  this->StartProgressValue = 0.0;
  this->SpanProgressValue = 1.0;
}

void vtkKWHistogramCallback::Execute(
  vtkObject *vtkNotUsed(caller), unsigned long event, void *calldata)
{
  float progress = 0.0;
  switch (event)
    {
    case vtkCommand::ProgressEvent:
      progress = *(static_cast<float *>(calldata)) * this->SpanProgressValue +
        this->StartProgressValue;
      this->Self->InvokeEvent(event, &progress);
      break;
    default:
      this->Self->InvokeEvent(event, calldata);
    }
}

//----------------------------------------------------------------------------
vtkKWHistogramSet::HistogramSlot::HistogramSlot()
{
  this->Histogram = vtkKWHistogram::New();
  this->Name = NULL;
}

//----------------------------------------------------------------------------
vtkKWHistogramSet::HistogramSlot::~HistogramSlot()
{
  if (this->Histogram)
    {
    this->Histogram->Delete();
    this->Histogram = NULL;
    }

  this->SetName(NULL);
}

//----------------------------------------------------------------------------
char* vtkKWHistogramSet::HistogramSlot::GetName()
{
  return this->Name;
}

//----------------------------------------------------------------------------
void vtkKWHistogramSet::HistogramSlot::SetName(const char *_arg)
{
  if (this->Name == NULL && _arg == NULL) 
    { 
    return;
    }

  if (this->Name && _arg && (!strcmp(this->Name, _arg))) 
    {
    return;
    }

  if (this->Name) 
    { 
    delete [] this->Name; 
    }

  if (_arg)
    {
    this->Name = new char[strlen(_arg)+1];
    strcpy(this->Name, _arg);
    }
  else
    {
    this->Name = NULL;
    }
}

//----------------------------------------------------------------------------
vtkKWHistogram* vtkKWHistogramSet::HistogramSlot::GetHistogram()
{
  return this->Histogram;
}

//----------------------------------------------------------------------------
vtkKWHistogramSet::vtkKWHistogramSet()
{
  this->Histograms = vtkKWHistogramSet::HistogramsContainer::New();
}

//----------------------------------------------------------------------------
vtkKWHistogramSet::~vtkKWHistogramSet()
{
  // Remove histograms and delete the container

  this->RemoveAllHistograms();
  this->Histograms->Delete();
}

//----------------------------------------------------------------------------
void vtkKWHistogramSet::RemoveAllHistograms()
{
  // Remove/delete all histograms

  vtkKWHistogramSet::HistogramSlot *histogram_slot = NULL;
  vtkKWHistogramSet::HistogramsContainerIterator *it = 
    this->Histograms->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(histogram_slot) == VTK_OK)
      {
      delete histogram_slot;
      }
    it->GoToNextItem();
    }
  it->Delete();

  this->Histograms->RemoveAllItems();
}

//----------------------------------------------------------------------------
vtkKWHistogramSet::HistogramSlot* 
vtkKWHistogramSet::GetHistogramSlot(const char *name)
{
  if (!name)
    {
    return NULL;
    }

  vtkKWHistogramSet::HistogramSlot *histogram_slot = NULL;
  vtkKWHistogramSet::HistogramSlot *found = NULL;
  vtkKWHistogramSet::HistogramsContainerIterator *it = 
    this->Histograms->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(histogram_slot) == VTK_OK && 
        !strcmp(histogram_slot->GetName(), name))
      {
      found = histogram_slot;
      break;
      }
    it->GoToNextItem();
    }
  it->Delete();

  return found;
}

//----------------------------------------------------------------------------
vtkKWHistogramSet::HistogramSlot* 
vtkKWHistogramSet::GetHistogramSlot(vtkKWHistogram *hist)
{
  if (!hist)
    {
    return NULL;
    }

  vtkKWHistogramSet::HistogramSlot *histogram_slot = NULL;
  vtkKWHistogramSet::HistogramSlot *found = NULL;
  vtkKWHistogramSet::HistogramsContainerIterator *it = 
    this->Histograms->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(histogram_slot) == VTK_OK && 
        histogram_slot->GetHistogram() == hist)
      {
      found = histogram_slot;
      break;
      }
    it->GoToNextItem();
    }
  it->Delete();

  return found;
}

//----------------------------------------------------------------------------
vtkIdType vtkKWHistogramSet::GetNumberOfHistograms()
{
  return this->Histograms->GetNumberOfItems();
}

//----------------------------------------------------------------------------
vtkKWHistogram* vtkKWHistogramSet::GetHistogram(const char *name)
{
  vtkKWHistogramSet::HistogramSlot *histogram_slot = 
    this->GetHistogramSlot(name);
  if (!histogram_slot)
    {
    return NULL;
    }

  return histogram_slot->GetHistogram();
}

//----------------------------------------------------------------------------
const char* vtkKWHistogramSet::GetHistogramName(vtkKWHistogram *hist)
{
  vtkKWHistogramSet::HistogramSlot *histogram_slot = 
    this->GetHistogramSlot(hist);
  if (!histogram_slot)
    {
    return NULL;
    }

  return histogram_slot->GetName();
}

//----------------------------------------------------------------------------
int vtkKWHistogramSet::HasHistogram(const char *name)
{
  return this->GetHistogramSlot(name) ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkKWHistogramSet::HasHistogram(vtkKWHistogram *hist)
{
  return this->GetHistogramSlot(hist) ? 1 : 0;
}

//----------------------------------------------------------------------------
vtkKWHistogram* vtkKWHistogramSet::GetNthHistogram(vtkIdType index)
{
  if (index < 0 || index >= this->GetNumberOfHistograms())
    {
    return NULL;
    }

  vtkKWHistogramSet::HistogramSlot *histogram_slot = NULL;
  if (this->Histograms->GetItem(index, histogram_slot) != VTK_OK)
    {
    return NULL;
    }

  return histogram_slot->GetHistogram();
}

//----------------------------------------------------------------------------
int vtkKWHistogramSet::AddHistogram(const char *name)
{
  // NULL name ?

  if (!name)
    {
    vtkErrorMacro("Can not add an histogram with a NULL name.");
    return 0;
    }

  // Check if we have an histogram with that name already

  if (this->HasHistogram(name))
    {
    vtkErrorMacro("An histogram with that name (" << name << ") already "
                  "exists in the histogram set.");
    return 0;
    }

  // Add the histogram slot to the manager

  vtkKWHistogramSet::HistogramSlot *histogram_slot = 
    new vtkKWHistogramSet::HistogramSlot;
  
  if (this->Histograms->AppendItem(histogram_slot) != VTK_OK)
    {
    vtkErrorMacro("Error while adding an histogram to the set.");
    delete histogram_slot;
    return 0;
    }
  
  // Assign the name

  histogram_slot->SetName(name);

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWHistogramSet::RemoveHistogram(const char *name)
{
  vtkKWHistogramSet::HistogramSlot *histogram_slot = 
    this->GetHistogramSlot(name);
  if (!histogram_slot)
    {
    return 0;
    }

  vtkIdType pos = 0;
  if (this->Histograms->FindItem(histogram_slot, pos) != VTK_OK)
    {
    vtkErrorMacro("Error while removing an histogram from the set "
                  "(can not find the histogram).");
    return 0;
    }

  // Remove the histogram from the container

  if (this->Histograms->RemoveItem(pos) != VTK_OK)
    {
    vtkErrorMacro("Error while removing the histogram from the container.");
    return 0;
    }

  // Delete the slot

  delete histogram_slot;
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWHistogramSet::AddHistograms(
  vtkDataArray *array, int independent, const char *prefix)
{
  // NULL name ?

  if (!array)
    {
    vtkErrorMacro("Can not add histograms from a NULL data array.");
    return 0;
    }

  // Add an histogram for each component

  int nb_components = array->GetNumberOfComponents();
  int skip_rgb_components = (!independent && nb_components >= 3);
  int nb_histograms = skip_rgb_components ? nb_components - 3 : nb_components;

  int component, nb_components_processed = 0;
  for (component = 0; component < nb_components; component++)
    {
    if (skip_rgb_components && component < 3)
      {
      continue;
      }
    nb_components_processed++;

    ostrstream name;
    name << (prefix ? prefix : (array->GetName() ? array->GetName() : "")) 
         << component << ends;

    if (!this->HasHistogram(name.str()) &&
        !this->AddHistogram(name.str()))
      {
      vtkErrorMacro("Can not add histogram for component " << component);
      continue;
      }

    vtkKWHistogram *hist = this->GetHistogram(name.str());
    if (!hist)
      {
      vtkErrorMacro("Can not retrieve histogram for component " << component);
      continue;
      }
    
    // Monitor histogram progress

    vtkKWHistogramCallback *callback = vtkKWHistogramCallback::New();
    callback->Self = this;
    callback->StartProgressValue = 
      (float)(nb_components_processed - 1) / (float)nb_histograms;
    callback->SpanProgressValue = 1.0 / (float)nb_histograms;
    
    if (nb_components_processed == 1)
      {
      hist->AddObserver(vtkCommand::StartEvent, callback);
      }
    if (nb_components_processed == nb_histograms)
      {
      hist->AddObserver(vtkCommand::EndEvent, callback);
      }
    hist->AddObserver(vtkCommand::ProgressEvent, callback);

    // Build the histogram *right now* (it's not a pipeline)

    hist->BuildHistogram(array, component);
    
    // Stop monitoring histogram progress

    hist->RemoveObserver(callback);
    callback->Delete();

    name.rdbuf()->freeze(0);
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkKWHistogramSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
