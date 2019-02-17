/*=========================================================================

Copyright (c) 2017, Los Alamos National Security, LLC

All rights reserved.

Copyright 2017. Los Alamos National Security, LLC.
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

#ifndef _GIO_PV_OCTREE_H_
#define _GIO_PV_OCTREE_H_

#include <list>
#include <math.h>
#include <stdint.h>
#include <vector>

namespace GIOPvPlugin
{

struct extent
{
  float extents[6]; // minX, maxX,  minY, maxY  minZ, maxZ
};

struct octreeMeta
{
  std::string filename;
  float extents[6];
  int numLevels;
  int numMPIranks;
  int numOctreeLeaves;
  std::vector<int> leaf;
  std::vector<extent> coord;
  std::vector<size_t> numPoints;
  int MPIrank;
  int fileOffset;
};

struct PartitionExtents
{
  float extents[6];

  PartitionExtents(){};
  PartitionExtents(float _extents[6])
  {
    for (int i = 0; i < 6; i++)
      extents[i] = _extents[i];
  }
};

class Octree
{
  float extents[6];
  int numLevels;

  std::vector<uintptr_t> octreeLevels; // stores the real nodes

  void chopVolume(float rootExtents[6], int _numLevels, std::list<PartitionExtents>& partitions);

public:
  std::list<PartitionExtents> octreePartitions; // stores only extents of all leaves

  Octree(){};
  Octree(int _numLevels, float _extents[6])
    : numLevels(_numLevels)
  {
    for (int i = 0; i < 6; i++)
      extents[i] = _extents[i];
  };
  ~Octree(){};

  void init(int _numLevels, float _extents[6]);
  void buildOctree();

  int getNumNodes() { return (int)(pow(8.0f, numLevels)); }
  int getLeafIndex(float pos[3]);

  void writeOctFile(std::string filename, int numMPIRanks, int rowsPerLeaf[]);

  void displayPartitions();
};

inline void Octree::init(int _numLevels, float _extents[6])
{
  numLevels = _numLevels;

  for (int i = 0; i < 6; i++)
    extents[i] = _extents[i];
}

inline void Octree::buildOctree()
{
  chopVolume(extents, numLevels, octreePartitions);
}

inline void Octree::chopVolume(
  float rootExtents[6], int _numLevels, std::list<PartitionExtents>& partitionList)
{
  // std::cout << "rootExtents: " << rootExtents[0] << ", " << rootExtents[1] << "   " <<
  // rootExtents[2] << ", " << rootExtents[3] << "   "  << rootExtents[4] << ", " << rootExtents[5]
  // << std::endl;
  PartitionExtents temp(rootExtents); // Set the first partition as root
  partitionList.push_back(temp);

  PartitionExtents firstHalf, secondHalf;

  int splittingAxis = 0; // start with x-axis
  int numDesiredBlocks =
    (int)pow(8.0f, _numLevels); // Compute number of splits needed based on levels
  int numBlocks = 1;

  while (numBlocks < numDesiredBlocks)
  {
    int numCurrentBlocks = partitionList.size();

    for (int i = 0; i < numCurrentBlocks; i++)
    {
      temp = partitionList.front();
      partitionList.pop_front();

      // std::cout << "\t" << temp.extents[0] << ", " <<  temp.extents[1] << "   "
      //      << temp.extents[2] << ", " <<  temp.extents[3] << "   "
      //      << temp.extents[4] << ", " <<  temp.extents[5] << std::endl;

      if (splittingAxis == 0) // x-axis
      {
        firstHalf.extents[0] = temp.extents[0];
        firstHalf.extents[1] = (temp.extents[0] + temp.extents[1]) / 2;

        secondHalf.extents[0] = (temp.extents[0] + temp.extents[1]) / 2;
        secondHalf.extents[1] = temp.extents[1];

        firstHalf.extents[2] = secondHalf.extents[2] = temp.extents[2];
        firstHalf.extents[3] = secondHalf.extents[3] = temp.extents[3];

        firstHalf.extents[4] = secondHalf.extents[4] = temp.extents[4];
        firstHalf.extents[5] = secondHalf.extents[5] = temp.extents[5];
      }
      else if (splittingAxis == 1) // y-axis
      {
        firstHalf.extents[0] = secondHalf.extents[0] = temp.extents[0];
        firstHalf.extents[1] = secondHalf.extents[1] = temp.extents[1];

        firstHalf.extents[2] = temp.extents[2];
        firstHalf.extents[3] = (temp.extents[2] + temp.extents[3]) / 2;

        secondHalf.extents[2] = (temp.extents[2] + temp.extents[3]) / 2;
        secondHalf.extents[3] = temp.extents[3];

        firstHalf.extents[4] = secondHalf.extents[4] = temp.extents[4];
        firstHalf.extents[5] = secondHalf.extents[5] = temp.extents[5];
      }
      else if (splittingAxis == 2) // z-axis
      {
        firstHalf.extents[0] = secondHalf.extents[0] = temp.extents[0];
        firstHalf.extents[1] = secondHalf.extents[1] = temp.extents[1];

        firstHalf.extents[2] = secondHalf.extents[2] = temp.extents[2];
        firstHalf.extents[3] = secondHalf.extents[3] = temp.extents[3];

        firstHalf.extents[4] = temp.extents[4];
        firstHalf.extents[5] = (temp.extents[4] + temp.extents[5]) / 2;

        secondHalf.extents[4] = (temp.extents[4] + temp.extents[5]) / 2;
        secondHalf.extents[5] = temp.extents[5];
      }

      partitionList.push_back(firstHalf);
      partitionList.push_back(secondHalf);

      // std::cout << "\t\t" << firstHalf.extents[0] << ", " <<  firstHalf.extents[1] << "   "
      //      << firstHalf.extents[2] << ", " <<  firstHalf.extents[3] << "   "
      //      << firstHalf.extents[4] << ", " <<  firstHalf.extents[5] << std::endl;

      // std::cout << "\t\t" << secondHalf.extents[0] << ", " <<  secondHalf.extents[1] << "   "
      //      << secondHalf.extents[2] << ", " <<  secondHalf.extents[3] << "   "
      //      << secondHalf.extents[4] << ", " <<  secondHalf.extents[5] << std::endl << std::endl;
    }

    // cycle axis
    splittingAxis++;
    if (splittingAxis == 3)
      splittingAxis = 0;

    numBlocks = partitionList.size();
  }
}

inline void Octree::writeOctFile(std::string filename, int numMPIRanks, int rowsPerLeaf[])
{
  std::ofstream outputFile((filename + ".oct").c_str(), std::ios::out);

  outputFile << extents[0] << " " << extents[1] << " " << extents[2] << " " << extents[3] << " "
             << extents[4] << " " << extents[5] << "\n";
  outputFile << numLevels << std::endl;
  outputFile << numMPIRanks << std::endl;
  outputFile << (int)(pow(8.0f, numLevels));

  int count = 0;
  for (auto it = octreePartitions.begin(); it != octreePartitions.end(); it++, count++)
    outputFile << "\n"
               << count << "  " << (*it).extents[0] << " " << (*it).extents[1] << " "
               << (*it).extents[2] << " " << (*it).extents[3] << " " << (*it).extents[4] << " "
               << (*it).extents[5] << "  " << rowsPerLeaf[count];

  outputFile.close();
}

inline void Octree::displayPartitions()
{
  int count = 0;
  for (auto it = octreePartitions.begin(); it != octreePartitions.end(); it++, count++)
    std::cout << count << " : " << (*it).extents[0] << ", " << (*it).extents[1] << "   "
              << (*it).extents[2] << ", " << (*it).extents[3] << "   " << (*it).extents[4] << ", "
              << (*it).extents[5] << std::endl;
}

inline int Octree::getLeafIndex(float pos[3])
{
  // extents
  float xDiv = (pos[0] - extents[0]) / (extents[1] - extents[0]);
  float yDiv = (pos[1] - extents[2]) / (extents[3] - extents[2]);
  float zDiv = (pos[2] - extents[4]) / (extents[5] - extents[4]);

  std::vector<int> bitPosition;
  float halfX, halfY, halfZ;
  float sizeX, sizeY, sizeZ;

  sizeX = sizeY = sizeZ = 0.25;
  halfX = halfY = halfZ = 0.5;

  // std::cout << " pos " << xDiv << ", " << yDiv << ", " << zDiv << std::endl;
  for (int i = 0; i < numLevels; i++)
  {
    // std::cout << i << " : " << halfX << ", " << halfY << ", " << halfZ << std::endl;

    // x-axis
    if (xDiv < halfX)
    {
      halfX -= sizeX;
      bitPosition.push_back(0);
    }
    else
    {
      halfX += sizeX;
      bitPosition.push_back(1);
    }

    // y-axis
    if (yDiv < halfY)
    {
      halfY -= sizeY;
      bitPosition.push_back(0);
    }
    else
    {
      halfY += sizeY;
      bitPosition.push_back(1);
    }

    // z-axis
    if (zDiv < halfZ)
    {
      halfZ -= sizeZ;
      bitPosition.push_back(0);
    }
    else
    {
      halfZ += sizeZ;
      bitPosition.push_back(1);
    }
    sizeX /= 2;
    sizeY /= 2;
    sizeZ /= 2;
  }

  // for (int i=0; i<bitPosition.size(); i++)
  //  std::cout << bitPosition[i] << " ";
  // std::cout << "\n bitPosition.size(): " << bitPosition.size()  <<std::endl;

  int index = 0;
  for (int i = 0; i < bitPosition.size(); i++)
  {
    index += pow(2, bitPosition.size() - 1 - i) * bitPosition[i];
    // std::cout << "pow(2, bitPosition.size()-i): " << pow(2, bitPosition.size()-1-i) <<  "
    // bitPosition[i]: " << bitPosition[i] << "  val: " << val << std::endl;
  }
  // std::cout << "\n index: " << index << std::endl;

  return index;
}

///////////////////////////////////////////////////////////////////////////////////
///////////// Read octree file

inline int readOctFile(std::string filename, octreeMeta& octreeInfo)
{
  std::ifstream metaFile(filename.c_str());

  if (metaFile.is_open())
  {
    octreeInfo.filename = filename;

    while (metaFile)
    {
      metaFile >> octreeInfo.extents[0] >> octreeInfo.extents[1];
      metaFile >> octreeInfo.extents[2] >> octreeInfo.extents[3];
      metaFile >> octreeInfo.extents[4] >> octreeInfo.extents[5];

      metaFile >> octreeInfo.numLevels;
      metaFile >> octreeInfo.numMPIranks;
      metaFile >> octreeInfo.numOctreeLeaves;

      // std::cout << octreeInfo.numLevels << std::endl;
      // std::cout << octreeInfo.numMPIranks << std::endl;
      // std::cout << octreeInfo.numOctreeLeaves << std::endl;

      // Allocate space for entries
      octreeInfo.leaf.resize(octreeInfo.numOctreeLeaves);
      octreeInfo.coord.resize(octreeInfo.numOctreeLeaves);
      octreeInfo.numPoints.resize(octreeInfo.numOctreeLeaves);

      for (int i = 0; i < octreeInfo.numOctreeLeaves; i++)
      {
        metaFile >> octreeInfo.leaf[i];
        metaFile >> octreeInfo.coord[i].extents[0];
        metaFile >> octreeInfo.coord[i].extents[1];
        metaFile >> octreeInfo.coord[i].extents[2];
        metaFile >> octreeInfo.coord[i].extents[3];
        metaFile >> octreeInfo.coord[i].extents[4];
        metaFile >> octreeInfo.coord[i].extents[5];
        metaFile >> octreeInfo.numPoints[i];
        metaFile >> octreeInfo.MPIrank;
        metaFile >> octreeInfo.fileOffset;
      }
    }
    metaFile.close();
  }
  else
  {
    // Could not open file!!!
    return 0;
  }

  return 1;
}

} // GIOPvPlugin namespace

#endif
