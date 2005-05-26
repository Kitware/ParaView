/*=========================================================================

  Module:    vtkKWHistogramSet.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWHistogramSet - a set of histograms
// .SECTION Description
// A set of histograms.

#ifndef __vtkKWHistogramSet_h
#define __vtkKWHistogramSet_h

#include "vtkObject.h"
#include "vtkKWWidgets.h" // Needed for export symbols directives

class vtkDataArray;
class vtkKWHistogram;
class vtkKWHistogramCallback;
class vtkKWHistogramSetInternals;

class KWWIDGETS_EXPORT vtkKWHistogramSet : public vtkObject
{
public:
  static vtkKWHistogramSet* New();
  vtkTypeRevisionMacro(vtkKWHistogramSet,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add an histogram. 
  // Return 1 on success, 0 otherwise.
  int AddHistogram(const char *name);
  vtkIdType GetNumberOfHistograms();

  // Description:
  // Get/query an histogram, given its name.
  vtkKWHistogram* GetHistogram(const char *name);
  const char* GetHistogramName(vtkKWHistogram *hist);
  int HasHistogram(const char *name);
  int HasHistogram(vtkKWHistogram *hist);

  // Description:
  // Get the n-th histogram.
  vtkKWHistogram* GetNthHistogram(vtkIdType index);

  // Description:
  // Add and build histograms for all components of a scalar array.
  // Each histogram name is built by appending the component index to
  // the array name or 'prefix' if not NULL.
  // If 'independent' == 1, all components are considered independent
  // and an histogram will be built for each one of them.
  // If independent == 0, components 0 to 3 will be ignored if the number of
  // components == 3 or 4, as we do not need the histogram for the RGB
  // channels most of the time (if always needed, then set the param to 1)
  // Return 1 on success, 0 otherwise.
  int AddHistograms(
    vtkDataArray *array, int independent = 1, const char *prefix = NULL);

  // Description:
  // Remove one or all histograms. 
  // Return 1 on success, 0 otherwise.
  int RemoveHistogram(const char *name);
  void RemoveAllHistograms();

protected:
  vtkKWHistogramSet();
  ~vtkKWHistogramSet();

  //BTX

  // An histogram slot.
 
  class HistogramSlot
  {
  public:
    
    HistogramSlot();
    virtual ~HistogramSlot();

    virtual void SetName(const char *name);
    virtual char* GetName() { return this->Name; }
    virtual vtkKWHistogram * GetHistogram()  { return this->Histogram; };
    
  protected:

    char *Name;
    vtkKWHistogram *Histogram;
  };

  // PIMPL Encapsulation for STL containers

  vtkKWHistogramSetInternals *Internals;
  friend class vtkKWHistogramSetInternals;

  // Helper methods

  HistogramSlot* GetHistogramSlot(const char *name);
  HistogramSlot* GetHistogramSlot(vtkKWHistogram *hist);

  //ETX

private:
  vtkKWHistogramSet(const vtkKWHistogramSet&); // Not implemented
  void operator=(const vtkKWHistogramSet&); // Not implemented
};

#endif

