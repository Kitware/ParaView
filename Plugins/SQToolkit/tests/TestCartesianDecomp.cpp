
#include <mpi.h>

#include <iostream>
#include <cstdlib>
#include <time.h>
using std::cerr;
using std::endl;

#include "vtkImageData.h"

#include "Tuple.hxx"
#include "CartesianDecomp.h"
#include "CartesianDataBlock.h"
#include "PriorityQueue.hxx"

int main(int argc, char **argv)
{
  MPI_Init(&argc,&argv);

  int nBlocks=atoi(argv[1]);

  CartesianDecomp *D=CartesianDecomp::New();
  D->SetOrigin(0.0,0.0,0.0);
  D->SetSpacing(1.0,1.0,1.0);
  D->SetExtent(0,100,0,100,0,100);
  D->ComputeBounds();
  D->SetDecompDims(nBlocks);
  D->DecomposeDomain();

  int cacheSize=10;
  int accessTime=0;
  PriorityQueue<int> cache;
  cache.Initialize(
        cacheSize,
        D->GetNumberOfBlocks());

  int nIts=100;//*nBlocks;
  double pt[3];
  //srand(time(0));
  for (int i=0; i<nIts; ++i)
    {
    pt[0]=100.0*rand()/(double)RAND_MAX;
    pt[1]=100.0*rand()/(double)RAND_MAX;
    pt[2]=100.0*rand()/(double)RAND_MAX;

    CartesianDataBlock *b=D->GetBlock(pt);

    // cerr
    //   << Tuple<double>(pt,3) 
    //   << " is in "  << Tuple<double>(b->GetBounds(),6) 
    //   << " " << Tuple<int>(b->GetId(),4) 
    //   << endl;

    cerr << "Accessing " << Tuple<int>(b->GetId(),4);

    vtkImageData *data=b->GetData();
    if (data==0)
      {
      // cache miss
      cerr << "\tCache miss";

      // If the cache is full pop least-recently-used block.
      if (cache.Full())
        {
        CartesianDataBlock *ob=D->GetBlock(cache.Pop());
        ob->SetData(0);

        cerr << "\tRemoved " << Tuple<int>(ob->GetId(),4);
        }

      // allocate data
      vtkImageData *id=vtkImageData::New();
      b->SetData(id);
      id->Delete();

      // insert the block into the cache
      cache.Push(b->GetIndex(),++accessTime);
      cerr << "\tInserted " << Tuple<int>(b->GetId(),4) << endl;
      }
    else
      {
      cerr << "\tCache hit" << endl;
      cache.Update(b->GetIndex(),++accessTime);
      }
    }

  // clear the cache
  while (!cache.Empty())
    {
    CartesianDataBlock *b=D->GetBlock(cache.Pop());
    b->SetData(0);
    }

  D->Delete();

  MPI_Finalize();

  return 0;
}

