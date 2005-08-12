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
#include "vtkObjectFactory.h"

#include <vtksys/stl/list>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWHistogramSet);
vtkCxxRevisionMacro(vtkKWHistogramSet, "1.6");

//----------------------------------------------------------------------------
class vtkKWHistogramSetInternals
{
public:

  class HistogramSlot
  {
  public:

    vtksys_stl::string Name;
    vtkKWHistogram *Histogram;
  };

  typedef vtksys_stl::list<HistogramSlot> HistogramsContainer;
  typedef vtksys_stl::list<HistogramSlot>::iterator HistogramsContainerIterator;

  HistogramsContainer Histograms;
};

//----------------------------------------------------------------------------
class vtkKWHistogramCallback : public vtkCommand
{
public:
  static vtkKWHistogramCallback *New()
    { return new vtkKWHistogramCallback; }
  virtual void Execute(vtkObject *caller, unsigned long event, void *calldata);
  
  vtkKWHistogramSet *Self;
  double StartProgressValue;
  double SpanProgressValue;

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
  double progress = 0.0;
  switch (event)
    {
    case vtkCommand::ProgressEvent:
      progress = *(static_cast<double *>(calldata)) * this->SpanProgressValue +
        this->StartProgressValue;
      this->Self->InvokeEvent(event, &progress);
      break;
    default:
      this->Self->InvokeEvent(event, calldata);
    }
}

//----------------------------------------------------------------------------
vtkKWHistogramSet::vtkKWHistogramSet()
{
  this->Internals = new vtkKWHistogramSetInternals;
}

//----------------------------------------------------------------------------
vtkKWHistogramSet::~vtkKWHistogramSet()
{
  // Remove histograms and delete the container

  this->RemoveAllHistograms();

  if (this->Internals)
    {
    delete this->Internals;
    this->Internals = NULL;
    }
}

//----------------------------------------------------------------------------
int vtkKWHistogramSet::GetNumberOfHistograms()
{
  return this->Internals ? this->Internals->Histograms.size() : 0;
}

//----------------------------------------------------------------------------
vtkKWHistogram* vtkKWHistogramSet::GetHistogramWithName(const char *name)
{
  if (name && *name && this->Internals)
    {
    vtkKWHistogramSetInternals::HistogramsContainerIterator it = 
      this->Internals->Histograms.begin();
    vtkKWHistogramSetInternals::HistogramsContainerIterator end = 
      this->Internals->Histograms.end();
    for (; it != end; ++it)
      {
      if (!strcmp(it->Name.c_str(), name))
        {
        return it->Histogram;
        }
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
const char* vtkKWHistogramSet::GetHistogramName(vtkKWHistogram *hist)
{
  if (hist && this->Internals)
    {
    vtkKWHistogramSetInternals::HistogramsContainerIterator it = 
      this->Internals->Histograms.begin();
    vtkKWHistogramSetInternals::HistogramsContainerIterator end = 
      this->Internals->Histograms.end();
    for (; it != end; ++it)
      {
      if (it->Histogram == hist)
        {
        return it->Name.c_str();
        }
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
int vtkKWHistogramSet::HasHistogramWithName(const char *name)
{
  return this->GetHistogramWithName(name) ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkKWHistogramSet::HasHistogram(vtkKWHistogram *hist)
{
  return this->GetHistogramName(hist) ? 1 : 0;
}

//----------------------------------------------------------------------------
vtkKWHistogram* vtkKWHistogramSet::GetNthHistogram(int index)
{
  if (this->Internals && index >= 0 && index < this->GetNumberOfHistograms())
    {
    vtkKWHistogramSetInternals::HistogramsContainerIterator it = 
      this->Internals->Histograms.begin();
    vtkKWHistogramSetInternals::HistogramsContainerIterator end = 
      this->Internals->Histograms.end();
    for (; it != end; ++it)
      {
      if (!index--)
        {
        return it->Histogram;
        }
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
int vtkKWHistogramSet::AddHistogram(vtkKWHistogram *hist, const char *name)
{
  if (!hist)
    {
    vtkErrorMacro("Can not add a NULL histogram.");
    return 0;
    }

  if (!name || !*name)
    {
    vtkErrorMacro("Can not add an histogram with a NULL or empty name.");
    return 0;
    }

  // Check if we have an histogram with that name already

  if (this->HasHistogramWithName(name))
    {
    vtkErrorMacro("An histogram with that name (" << name << ") already "
                  "exists in the histogram set.");
    return 0;
    }

  // Add the histogram slot to the manager

  vtkKWHistogramSetInternals::HistogramSlot histogram_slot;
  histogram_slot.Histogram = hist;
  histogram_slot.Histogram->Register(this);
  histogram_slot.Name = name;
  this->Internals->Histograms.push_back(histogram_slot);
  
  return 1;
}

//----------------------------------------------------------------------------
vtkKWHistogram* vtkKWHistogramSet::AllocateAndAddHistogram(const char *name)
{
  vtkKWHistogram *hist = vtkKWHistogram::New();
  int res = this->AddHistogram(hist, name);
  hist->Delete();
  return res ? hist : NULL;
}

//----------------------------------------------------------------------------
void vtkKWHistogramSet::RemoveAllHistograms()
{
  // Remove/delete all histograms

  if (this->Internals)
    {
    vtkKWHistogramSetInternals::HistogramsContainerIterator it = 
      this->Internals->Histograms.begin();
    vtkKWHistogramSetInternals::HistogramsContainerIterator end = 
      this->Internals->Histograms.end();
    for (; it != end; ++it)
      {
      if (it->Histogram)
        {
        it->Histogram->UnRegister(this);
        }
      }
    this->Internals->Histograms.clear();
    }
}

//----------------------------------------------------------------------------
int vtkKWHistogramSet::RemoveHistogram(vtkKWHistogram *hist)
{
  if (!hist)
    {
    vtkErrorMacro("Can not remove a NULL histogram.");
    return 0;
    }

  vtkKWHistogramSetInternals::HistogramsContainerIterator it = 
    this->Internals->Histograms.begin();
  vtkKWHistogramSetInternals::HistogramsContainerIterator end = 
    this->Internals->Histograms.end();
  for (; it != end; ++it)
    {
    if (it->Histogram == hist)
      {
      it->Histogram->UnRegister(this);
      this->Internals->Histograms.erase(it);
      return 1;
      }
    }

  return 0;
}


//----------------------------------------------------------------------------
int vtkKWHistogramSet::RemoveHistogramWithName(const char *name)
{
  return this->RemoveHistogram(this->GetHistogramWithName(name));
}

//----------------------------------------------------------------------------
int vtkKWHistogramSet::ComputeHistogramName(const char *array_name, 
                                            int component,
                                            const char *tag,
                                            char *buffer)
{
  if (!buffer)
    {
    return 0;
    }

  sprintf(buffer, 
          "%s%d%s", 
          (array_name ? array_name : ""), component, (tag ? tag : ""));

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWHistogramSet::AddHistograms(vtkDataArray *array, 
                                     const char *tag, 
                                     int skip_components_mask)
{
  if (!array)
    {
    vtkErrorMacro("Can not add histograms from a NULL data array.");
    return 0;
    }

  // Add an histogram for each component

  int nb_components = array->GetNumberOfComponents();

  char *hist_name = new char [1024 + (tag ? strlen(tag) : 0)];

  int nb_histograms = 0;
  int component;
  for (component = 0; component < nb_components; component++)
    {
    if (!(skip_components_mask & (1 << component)))
      {
      nb_histograms++;
      }
    }

  int nb_components_processed = 0;
  for (component = 0; component < nb_components; component++)
    {
    if (skip_components_mask & (1 << component))
      {
      continue;
      }
    nb_components_processed++;

    if (!this->ComputeHistogramName(
          array->GetName(), component, tag, hist_name))
      {
      vtkErrorMacro("Can not compute histogram name for component " 
                    << component);
      continue;
      }

    vtkKWHistogram *hist = this->GetHistogramWithName(hist_name);
    if (!hist)
      {
      hist = this->AllocateAndAddHistogram(hist_name);
      }
    
    if (!hist)
      {
      vtkErrorMacro("Can not retrieve histogram for component " << component);
      continue;
      }
    
    // Monitor histogram progress

    vtkKWHistogramCallback *callback = vtkKWHistogramCallback::New();
    callback->Self = this;
    callback->StartProgressValue = 
      (double)(nb_components_processed - 1) / (double)nb_histograms;
    callback->SpanProgressValue = 1.0 / (double)nb_histograms;
    
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
    }

  delete [] hist_name;

  return 1;
}

//----------------------------------------------------------------------------
void vtkKWHistogramSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
