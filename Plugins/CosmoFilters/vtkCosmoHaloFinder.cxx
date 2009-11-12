/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCosmoHaloFinder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkCosmoHaloFinder.cxx

Copyright (c) 2007, 2009, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007, 2009. Los Alamos National Security, LLC. 
This software was produced under U.S. Government contract DE-AC52-06NA25396 
for Los Alamos National Laboratory (LANL), which is operated by 
Los Alamos National Security, LLC for the U.S. Department of Energy. 
The U.S. Government has rights to use, reproduce, and distribute this software. 
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.  
If software is modified to produce derivative works, such modified software 
should be clearly marked, so as not to confuse it with the version available 
from LANL.
 
Additionally, redistribution and use in source and binary forms, with or 
without modification, are permitted provided that the following conditions 
are met:
-   Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer. 
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software 
    without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkCosmoHaloFinder.h"

#include "vtkCellType.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkPointData.h"
#include "vtkSortDataArray.h"
#include "vtkUnstructuredGrid.h"

#include "vtkIntArray.h"
#include "vtkLongArray.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkTimerLog.h"
#include "vtkMath.h"
#include "vtkDirectory.h"
#include "vtkXMLUnstructuredGridWriter.h"

#include "vtksys/ios/fstream"
#include "vtkstd/algorithm"
#include "string.h"

#define NUM_DATA_DIMS 3
#define DATA_X 0
#define DATA_Y 1
#define DATA_Z 2

vtkCxxRevisionMacro(vtkCosmoHaloFinder, "1.6");
vtkStandardNewMacro(vtkCosmoHaloFinder);

//----------------------------------------------------------------------------
class ValueIdPairLT
{
public:
  bool operator() (const ValueIdPair& p, const ValueIdPair& q) const
  {
    return p.value < q.value;
  }
};

/****************************************************************************/
vtkCosmoHaloFinder::vtkCosmoHaloFinder()
{
  this->CurrentTimeIndex = 0;
  this->NumberOfTimeSteps = 0;
  this->BatchMode = false;
  this->outputDir = new char[strlen("./halo/") + 1];
  strcpy(this->outputDir, "./halo/");
}

/****************************************************************************/
vtkCosmoHaloFinder::~vtkCosmoHaloFinder()
{
  delete [] this->outputDir;
}

void vtkCosmoHaloFinder::SetOutputDirectory(const char *dir)
{
  delete [] this->outputDir;
  this->outputDir = new char[strlen(dir) + 1];
  strcpy(this->outputDir, dir);
}

//----------------------------------------------------------------------------
int vtkCosmoHaloFinder::RequestInformation(vtkInformation* request,
                                           vtkInformationVector** inputVector,
                                           vtkInformationVector* outputVector)
{
  // if not in batch mode, time steps information is not needed
  if (!this->BatchMode)
    {
    return this->Superclass::RequestInformation(request, inputVector, outputVector);
    }

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  if ( inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) )
    {
    this->NumberOfTimeSteps = 
      inInfo->Length( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
    }
  else
    {
    this->NumberOfTimeSteps = 0;
    }
  // The output of this filter does not contain a specific time, rather 
  // it contains a collection of time steps. Also, this filter does not
  // respond to time requests. Therefore, we remove all time information
  // from the output.
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
    {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    }
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()))
    {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkCosmoHaloFinder::RequestUpdateExtent(vtkInformation* request,
                                            vtkInformationVector** inputVector,
                                            vtkInformationVector* outputVector)
{
  // get the requested update extent
  double *inTimes = inputVector[0]->GetInformationObject(0)
    ->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (inTimes && this->BatchMode)
    {
    double timeReq[1];
    timeReq[0] = inTimes[this->CurrentTimeIndex];
    inputVector[0]->GetInformationObject(0)->Set
      ( vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS(), 
        timeReq, 1);
    return 1;
    }
  else
    {
    return this->Superclass::RequestUpdateExtent(request, inputVector, outputVector);
    }

}

/****************************************************************************/
int vtkCosmoHaloFinder::RequestData(vtkInformation* request,
                                    vtkInformationVector** inputVector,
                                    vtkInformationVector* outputVector)
{
  //vtkDebugMacro(<< "np:       " << this->np << "\n");
  vtkDebugMacro(<< "rL:       " << this->rL << "\n");
  vtkDebugMacro(<< "bb:       " << this->bb << "\n");
  vtkDebugMacro(<< "pmin:     " << this->pmin << "\n");
  vtkDebugMacro(<< "periodic: " << this->Periodic << "\n");

  if (this->BatchMode && this->NumberOfTimeSteps == 0)
    {
    vtkErrorMacro("No time steps in input data!");
    return 0;
    }

  vtkDataSet *input = vtkDataSet::GetData(inputVector[0]);
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::GetData(outputVector);

  // temporarily save the output dataset in this variable
  // if we operate on the output directly, when it's passed to the writer, it will cause
  // infinite recursive update of the pipeline
  vtkUnstructuredGrid *tmp_output = vtkUnstructuredGrid::New();

  if (this->BatchMode && !this->CurrentTimeIndex)
    {
    // check the existence of the output directory
    vtkDirectory *directory = vtkDirectory::New();
    if (!directory->Open(this->outputDir))
      {
      if (!vtkDirectory::MakeDirectory(this->outputDir))
        {
        vtkErrorMacro(<< "Invalid output directory: " << 
                      this->outputDir << "\n");
        return 0;
        }
      }

    directory->Delete();
    this->WritePVDFile(inputVector);
    // Tell the pipeline to start looping.
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    }

  // copy over the existing fields from input data
  tmp_output->ShallowCopy(input);

  this->npart = input->GetNumberOfPoints();
  vtkDebugMacro(<< "npart = " << this->npart);

  vtkDataArray* tag = input->GetPointData()->GetArray("tag");
  if (tag == NULL)
    {
    vtkErrorMacro("The input data set doesn't have tag field!");
    return 0;
    }

  // calculate np from data
  this->np = (int)(pow(this->npart, 1.0 / 3.0) + .5);
  vtkDebugMacro(<< "np = " << this->np);

  // normalize
  float xscal = this->rL / (1.0 * this->np);

  // create workspace for halo finding.  
  this->ht = new int[this->npart];
  this->halo = new int[this->npart];
  this->nextp = new int[this->npart];
  this->pt = new int[this->npart];

  for (int i = 0; i < this->npart; i++)
    {
    this->ht[i] = i;
    this->halo[i] = i;
    this->nextp[i] = -1;
    this->pt[i] = (int)tag->GetComponent(i, 0);
    }

  // preprocess
  typedef float* floatptr;
  this->data = new floatptr[NUM_DATA_DIMS];
  this->lb = new floatptr[NUM_DATA_DIMS];
  this->ub = new floatptr[NUM_DATA_DIMS];
  for (int i=0; i<NUM_DATA_DIMS; i++)
    {
    this->data[i] = new float[this->npart];
    this->lb[i] = new float[this->npart];
    this->ub[i] = new float[this->npart];
    }

  for (int i=0; i<this->npart; i++)
    {
    double* point = input->GetPoint(i);
    float xx = (float) point[0];
    float yy = (float) point[1];
    float zz = (float) point[2];

    // FIXME: a quick fix
    if (xx == rL)
      {
      xx = 0.0;
      }
    if (yy == rL)
      {
      yy = 0.0;
      }
    if (zz == rL)
      {
      zz = 0.0;
      }

    this->data[DATA_X][i] = xx / xscal;
    this->data[DATA_Y][i] = yy / xscal;
    this->data[DATA_Z][i] = zz / xscal;
    }

  this->v = new ValueIdPair[this->npart];
  for (int i=0; i<this->npart; i++)
    {
    this->v[i].value = this->data[DATA_X][i];
    this->v[i].id = i;
    }

  // reorder
  Reorder(0, this->npart, DATA_X);

  // recording
  this->seq = new int[this->npart];
  for (int i=0; i<this->npart; i++)
    {
    this->seq[i] = this->v[i].id;
    }

  // finding halos
  myFOF(0, this->npart, DATA_X);

  // compute halos statistics
  int *hsize = new int[this->npart];
  for (int h=0; h<this->npart; h++)
    {
    hsize[h] = 0;
    }

  for (int i=0; i<this->npart; i++)
    {
    hsize[this->ht[i]] += 1;
    }

  this->nhalo = 0;
  for (int h=0; h<this->npart; h++)
    {
    if (hsize[h] >= this->pmin)
      {
      this->nhalo++;
      }
    }

  vtkDebugMacro(<< nhalo << " halos\n");

  this->nhalopart = 0;
  for (int i=0; i<this->npart; i++)
    {
    if (hsize[this->ht[i]] >= this->pmin)
      {
      this->nhalopart++;
      }
    }

  vtkDebugMacro(<< this->nhalopart << " halo particles\n");

  vtkIntArray* hID = vtkIntArray::New();
  hID->SetName("hID");
  hID->SetNumberOfComponents(1);
  hID->SetNumberOfTuples(npart);

  vtkIntArray* haloSize = vtkIntArray::New();
  haloSize->SetName("haloSize");
  haloSize->SetNumberOfValues(this->npart);

  for (int i = 0; i < this->npart; i++)
    {
    hID->SetValue(i, 
                  ((hsize[this->ht[i]] < this->pmin) 
                   ? -1 : pt[ht[i]]));
    haloSize->SetValue(i, 
                       (hsize[this->ht[i]] < this->pmin) 
                       ? 0 : hsize[this->ht[i]]);
    }

  tmp_output->GetPointData()->AddArray(hID);
  tmp_output->GetPointData()->AddArray(haloSize);
  hID->Delete();
  haloSize->Delete();

  // clean up
  delete[] this->pt;
  delete[] this->ht;
  delete[] this->halo;
  delete[] this->nextp;

  for (int i = 0; i < NUM_DATA_DIMS; i++)
    {
    delete[] this->data[i];
    delete[] this->lb[i];
    delete[] this->ub[i];
    }
  delete[] this->data;
  delete[] this->lb;
  delete[] this->ub;
  delete[] this->v;
  delete[] this->seq;
  delete[] hsize;

  // if in batch mode, write out the new dataset to file
  if (this->BatchMode) 
    {
    double *inTimes = inputVector[0]->GetInformationObject(0)
      ->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    char* buffer = new char[strlen(this->outputDir) + 64];
    sprintf(buffer, "%s/part_%08.4f.vtu", this->outputDir,
            fabs(inTimes[this->CurrentTimeIndex]));

    vtkXMLUnstructuredGridWriter* writer = vtkXMLUnstructuredGridWriter::New();
    writer->SetInput(tmp_output);
    writer->SetDataModeToBinary();
    writer->SetFileName(buffer);
    writer->Write();
    writer->Delete();

    this->CurrentTimeIndex++;
    if (this->CurrentTimeIndex == this->NumberOfTimeSteps)
      {
      request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      this->CurrentTimeIndex = 0;
      }

    delete [] buffer;
    }

  // copy the data from temorary variable to the real output
  output->ShallowCopy(tmp_output);
  tmp_output->Delete();

  return 1;
}

/****************************************************************************/
void vtkCosmoHaloFinder::Reorder(int first, int last, 
                                 int dataFlag)
{
  int len = (last - first);

  // base case
  if (len <= 1)
    {
    double progress = .5 * (double)last / (double)this->npart;
    if (int(100 * progress) % 1 == 0)
      {
      this->UpdateProgress(progress);
      }

    return;
    }

  // non-base cases
  // preprocessing
  for (int i = first; i < last; i = i + 1)
    {
    this->v[i].value = this->data[dataFlag][this->v[i].id];
    }

  // divide
  int half = len / 2;
  vtkstd::nth_element(&this->v[first], 
                      &this->v[first+half], 
                      &this->v[last], ValueIdPairLT());

  Reorder(first, first+half, (dataFlag+1)%3);
  Reorder(first+half, last, (dataFlag+1)%3);

  // book-keeping
  int middle  = first + len / 2;
  int middle1 = first + len / 4;
  int middle2 = first + 3 * len / 4;

  // base case
  if (len == 2)
    {
    int ii = this->v[first+0].id;
    int jj = this->v[first+1].id;

    this->lb[DATA_X][middle] = 
      vtkstd::min(this->data[DATA_X][ii], this->data[DATA_X][jj]);
    this->lb[DATA_Y][middle] = 
      vtkstd::min(this->data[DATA_Y][ii], this->data[DATA_Y][jj]);
    this->lb[DATA_Z][middle] = 
      vtkstd::min(this->data[DATA_Z][ii], this->data[DATA_Z][jj]);

    this->ub[DATA_X][middle] = 
      vtkstd::max(this->data[DATA_X][ii], this->data[DATA_X][jj]);
    this->ub[DATA_Y][middle] = 
      vtkstd::max(this->data[DATA_Y][ii], this->data[DATA_Y][jj]);
    this->ub[DATA_Z][middle] = 
      vtkstd::max(this->data[DATA_Z][ii], this->data[DATA_Z][jj]);

    return;
    }

  // base case
  // this case is needed when len is a non-power-of-two
  if (len == 3)
    {
    int ii = this->v[first+0].id;

    this->lb[DATA_X][middle] = 
      vtkstd::min(data[DATA_X][ii], this->lb[DATA_X][middle2]);
    this->lb[DATA_Y][middle] = 
      vtkstd::min(data[DATA_Y][ii], this->lb[DATA_Y][middle2]);
    this->lb[DATA_Z][middle] = 
      vtkstd::min(data[DATA_Z][ii], this->lb[DATA_Z][middle2]);

    this->ub[DATA_X][middle] = 
      vtkstd::max(data[DATA_X][ii], this->ub[DATA_X][middle2]);
    this->ub[DATA_Y][middle] = 
      vtkstd::max(data[DATA_Y][ii], this->ub[DATA_Y][middle2]);
    this->ub[DATA_Z][middle] = 
      vtkstd::max(data[DATA_Z][ii], this->ub[DATA_Z][middle2]);

    return;
    }

  // non-base case
  this->lb[DATA_X][middle] = 
    vtkstd::min(this->lb[DATA_X][middle1], this->lb[DATA_X][middle2]);
  this->lb[DATA_Y][middle] = 
    vtkstd::min(this->lb[DATA_Y][middle1], this->lb[DATA_Y][middle2]);
  this->lb[DATA_Z][middle] = 
    vtkstd::min(this->lb[DATA_Z][middle1], this->lb[DATA_Z][middle2]);

  this->ub[DATA_X][middle] = 
    vtkstd::max(this->ub[DATA_X][middle1], this->ub[DATA_X][middle2]);
  this->ub[DATA_Y][middle] = 
    vtkstd::max(this->ub[DATA_Y][middle1], this->ub[DATA_Y][middle2]);
  this->ub[DATA_Z][middle] = 
    vtkstd::max(this->ub[DATA_Z][middle1], this->ub[DATA_Z][middle2]);

  return;
}


/****************************************************************************************/
void vtkCosmoHaloFinder::myFOF(int first, int last, int dataFlag)
{

  //cout << "myFOF [" << first << "," << last << ")" << " " << dataFlag << endl;

  int len = last - first;

  // base case
  if (len == 1)
    {
    double progress = .5 * (double)last / (double)this->npart + .5;
    if (int(100 * progress) % 1 == 0)
      {
      this->UpdateProgress(progress);
      }

    return;
    }

  // non-base cases

  // divide
  int half = len / 2;

  // continue FOF at the next level
  myFOF(first, first+half, (dataFlag+1)%3);
  myFOF(first+half,  last, (dataFlag+1)%3);

  // recursive merge
  Merge(first, first+half, first+half, last, dataFlag);

  return;
}


/****************************************************************************/
void vtkCosmoHaloFinder::Merge(int first1, int last1, int first2, int last2, int dataFlag)
{
  // cout << "Merge [" << first1 << "," << last1 << ")[" << first2 << "," << last2 << ") " << dataFlag << endl;

  int len1 = last1 - first1;
  int len2 = last2 - first2;

  // three base cases
  // len1 == 1 && len2 == 1
  // len1 == 1 && len2 == 2
  // len1 == 2 && len2 == 1
  // the latter two are for n being a non-power of 2.

  // base cases
  if (len1 == 1 && len2 == 1)
    {
    basicMerge(this->seq[first1], this->seq[first2]);
    return;
    }

  if (len1 == 1 && len2 == 2) 
    {
    basicMerge(this->seq[first1], this->seq[first2]);
    basicMerge(this->seq[first1], this->seq[first2+1]);
    basicMerge(this->seq[first2], this->seq[first2+1]);
    return;
    }

  if (len1 == 2 && len2 == 1)
    {
    basicMerge(this->seq[first1], this->seq[first2]);
    basicMerge(this->seq[first1+1], this->seq[first2]);
    basicMerge(this->seq[first1], this->seq[first1+1]);
    return;
    }

  // non-base case

  // pruning?
  int middle1 = first1 + len1 / 2;
  int middle2 = first2 + len2 / 2;

  float lL = this->lb[dataFlag][middle1];
  float uL = this->ub[dataFlag][middle1];
  float lR = this->lb[dataFlag][middle2];
  float uR = this->ub[dataFlag][middle2];

  float dL = uL - lL;
  float dR = uR - lR;
  float dc = vtkstd::max(uL,uR) - vtkstd::min(lL,lR);

  float dist = dc - dL - dR;
  if (this->Periodic) 
    {
    dist = vtkstd::min(dist, np-dc);
    }

  if (dist >= this->bb)
    {
    return;
    }

  // continue merging at the next level
  Merge(first1, middle1,  first2, middle2, (dataFlag+1)%3);
  Merge(first1, middle1, middle2,   last2, (dataFlag+1)%3);
  Merge(middle1,  last1,  first2, middle2, (dataFlag+1)%3);
  Merge(middle1,  last1, middle2,   last2, (dataFlag+1)%3);

  return;
}

/****************************************************************************/
void vtkCosmoHaloFinder::basicMerge(int ii, int jj)
{

  // fast exit
  if (ht[ii] == ht[jj])
    {
    return;
    }

  // ht[ii] != ht[jj]
  float xdist = fabs(this->data[DATA_X][jj] - this->data[DATA_X][ii]);
  float ydist = fabs(this->data[DATA_Y][jj] - this->data[DATA_Y][ii]);
  float zdist = fabs(this->data[DATA_Z][jj] - this->data[DATA_Z][ii]);

  if (this->Periodic)
    {
    xdist = vtkstd::min(xdist, np-xdist);
    ydist = vtkstd::min(ydist, np-ydist);
    zdist = vtkstd::min(zdist, np-zdist);
    }

  if ((xdist<this->bb) && (ydist<this->bb) && (zdist<this->bb))
    {

    float dist = xdist*xdist + ydist*ydist + zdist*zdist;
    if (dist < bb*bb)
      {

      // union two halos to one
      int newHaloId = (this->ht[ii] < this->ht[jj]) 
        ? this->ht[ii] : this->ht[jj];
      int oldHaloId = (this->ht[ii] < this->ht[jj]) 
        ? this->ht[jj] : this->ht[ii];

      // update particles with oldHaloId
      int last = -1;
      int ith = this->halo[oldHaloId];
      while (ith != -1)
        {
        this->ht[ith] = newHaloId;
        last = ith;
        ith = this->nextp[ith];
        }

      // update halo's linked list
      this->nextp[last] = this->halo[newHaloId];
      this->halo[newHaloId] = this->halo[oldHaloId];
      this->halo[oldHaloId] = -1;
      }
    }

  return;
}

/****************************************************************************/

void vtkCosmoHaloFinder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  //os << indent << "np: " << this->np << endl;
  os << indent << "bb: " << this->bb << endl;
  os << indent << "pmin: " << this->pmin << endl;
  os << indent << "rL: " << this->rL << endl;
  os << indent << "Periodic: " << (this->Periodic?"ON":"OFF") << endl;
  os << indent << "BatchMode: " << (this->BatchMode?"ON":"OFF") << endl;
}

int vtkCosmoHaloFinder::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

// write the pvd file of the time series data
void vtkCosmoHaloFinder::WritePVDFile(vtkInformationVector** inputVector)
{
  double *inTimes = inputVector[0]->GetInformationObject(0)
    ->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  char* buffer = new char[strlen(this->outputDir) + 64];
  sprintf(buffer, "%s/halo.pvd", this->outputDir);
  ofstream ofile(buffer);
  if (!ofile)
    {
    vtkErrorMacro("Failed to open pvd file for writing!");
    return;
    }

  ofile <<"<?xml version=\"1.0\"?>\n"
        <<"<VTKFile type=\"Collection\" version=\"0.1\" byte_order=\"LittleEndian\">\n"
        <<"<Collection>\n";

  for (int i = 0; i < this->NumberOfTimeSteps; i++)
    {
    sprintf(buffer, "part_%08.4f.vtu", fabs(inTimes[i])); 
    ofile << "<DataSet timestep=\"" << inTimes[i] << "\" file=\"" << buffer << "\"/>\n";
    }

  ofile << "</Collection>\n</VTKFile>";

  ofile.close();

  delete [] buffer;
}
