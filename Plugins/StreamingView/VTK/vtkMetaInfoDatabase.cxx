/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMetaInfoDatabase.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMetaInfoDatabase.h"

#include "vtkObjectFactory.h"

#include <vtkstd/map>
#include <vtkstd/string>
#include <vtkstd/vector>

#define DEBUGPRINT_METAINFORMATION(arg) ;

vtkStandardNewMacro(vtkMetaInfoDatabase);

struct RangeRecord2
{
  //min and max values found in one array within a particular piece
  int p;
  int np;
  int ext[6];
  double range[2];
  double resolution;
};

class ArrayRecord {
public:
  //all of the min and max values ever found for one array
  vtkstd::vector<RangeRecord2 *> ranges;

  ~ArrayRecord()
  {
    vtkstd::vector<RangeRecord2 *>::iterator rit;
    for (rit = ranges.begin(); rit < ranges.end(); rit++)
      {
      delete *rit;
      }
    ranges.clear();
  }

  void Insert(int p, int np, int ext[6], double resolution, double range[2])
  {
    if (range[1] < range[0])
      {
      //caller didn't know yet
      return;
      }

    DEBUGPRINT_METAINFORMATION
      (cerr << "Insert range of "
       << range[0] << " .. " << range[1] << " for "
       << p << "/" << np << " "
       << ext[0] << "," << ext[1] << ","
       << ext[2] << "," << ext[3] << ","
       << ext[4] << "," << ext[5] << "@"
       << resolution << endl;);

    //go through the list and find pieces that entirely contain this piece
    //for all of those, update their information if necessary with the new
    //information from this new piece
    vtkstd::vector<RangeRecord2 *>::iterator rit;
    for (rit = this->ranges.begin();
         rit < this->ranges.end();
         rit++)
      {
      RangeRecord2 *rr = *rit;
      if (rr->p == p &&
          rr->np == np)
        {
        DEBUGPRINT_METAINFORMATION(cerr << "Found match!" << endl;);
        //what is there already should be at least as correct as what
        //we've just produced (in case a child refined it) so leave it alone
        return;
        }

      if (rr->np < np) //potentially an ancestor
        {
        if ((p/(np/rr->np)) == rr->p) //it is an ancestor
          {
          DEBUGPRINT_METAINFORMATION
            (cerr << p << "/" << np
             << " descends from "
             << rr->p << "/" << rr->np << endl;);
          if (rr->range[0] > range[0])
            {
            DEBUGPRINT_METAINFORMATION
              (cerr << "updated L range from "
               << rr->range[0] << " to " << range[0] << endl;);
            rr->range[0] = range[0];
            }
          if (rr->range[1] < range[1])
            {
            DEBUGPRINT_METAINFORMATION
              (cerr << "updated U range from "
               << rr->range[1] << " to " << range[1] << endl;);
            rr->range[1] = range[1];
            }
          }
        }
      }

    DEBUGPRINT_METAINFORMATION(cerr << "Inserting new record" << endl;);
    RangeRecord2 *rr = new RangeRecord2();
    rr->p = p;
    rr->np = np;
    rr->ext[0] = ext[0];
    rr->ext[1] = ext[1];
    rr->ext[2] = ext[2];
    rr->ext[3] = ext[3];
    rr->ext[4] = ext[4];
    rr->ext[5] = ext[5];
    rr->range[0] = range[0];
    rr->range[1] = range[1];
    rr->resolution = resolution;
    this->ranges.push_back(rr);
  }

  int Search(int p, int np, int vtkNotUsed(ext)[6], double *range)
  {
    DEBUGPRINT_METAINFORMATION(cerr << "Search " <<p << "/" << np << endl;);
    //go through list and use the ranges of all my ancestors
    //use the most refined of them as an estimate for my range
    RangeRecord2 *best = NULL;
    vtkstd::vector<RangeRecord2 *>::iterator rit;
    for (rit = this->ranges.begin();
         rit < this->ranges.end();
         rit++)
      {
      RangeRecord2 *rr = *rit;
      if (rr->p == p &&
          rr->np == np)
        {
        DEBUGPRINT_METAINFORMATION(cerr << "Found match!" << endl;);
        range[0] = rr->range[0];
        range[1] = rr->range[1];
        return 1;
        }

      if (rr->np < np) //potentially an ancestor
        {
        if ((p/(np/rr->np)) == rr->p) //it is an ancestor
          {
          if (!best)
            {
            DEBUGPRINT_METAINFORMATION
              (cerr << "Found first candidate "
               << rr->p << "/" << rr->np << endl;);
            best = rr;
            }
          else
            {
            if (rr->np > best->np)
              {
              DEBUGPRINT_METAINFORMATION
                (cerr << rr->p << "/" << rr->np << " is better than "
                 << best->p << "/" << best->np << endl;);
              best = rr;
              }
            else
              {
              DEBUGPRINT_METAINFORMATION
                (cerr << best->p << "/" << best->np << " STILL better than "
                 << rr->p << "/" << rr->np << endl;);
              }
            }
          }
        }
      }
    if (best)
      {
      range[0] = best->range[0];
      range[1] = best->range[1];
      DEBUGPRINT_METAINFORMATION
        (cerr << "found close match of " << best->p << "/" << best->np
         << " with: " << range[0] << "," << range[1] << endl;);
      return 1;
      }
    DEBUGPRINT_METAINFORMATION(cerr << "No match" << endl;);
    return 0;
  }

};

class vtkMetaInfoDatabase::ArrayRecords {
public:
  //ranges known known for all arrays
  vtkstd::map<vtkstd::string, ArrayRecord* > records;

  ArrayRecord *GetRecord(int field, const char *ArrayName, int component)
  {
    //looks up a record holder for the given array
    //if none found, makes one, adds it to the internal list and returns it
    size_t len = (ArrayName?strlen(ArrayName):20) + 11;
    char *key = new char[len];
    sprintf(key, "%2d_%s_%6d", field, ArrayName, component);
    ArrayRecord *record = records[key];
    if (!record)
      {
      record = new ArrayRecord;
      records[key] = record;
      }
    delete[] key;
    return record;
  }

  ~ArrayRecords()
  {
    vtkstd::map<vtkstd::string, ArrayRecord *>::iterator rit;
    for (rit = records.begin(); rit != records.end(); ++rit)
      {
      delete rit->second;
      }
    records.clear();
  }
};

//----------------------------------------------------------------------------
vtkMetaInfoDatabase::vtkMetaInfoDatabase()
{
  this->Records = new ArrayRecords;
}

//----------------------------------------------------------------------------
vtkMetaInfoDatabase::~vtkMetaInfoDatabase()
{
  delete this->Records;
}

//----------------------------------------------------------------------------
void vtkMetaInfoDatabase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkMetaInfoDatabase::Insert(int p, int np, int ext[6], double resolution,
                                 int field_association,
                                 const char *ArrayName, int component,
                                 double range[2])
{
  ArrayRecord *record = this->Records->GetRecord
    (field_association, ArrayName, component);
  record->Insert(p,np,ext,resolution,range);
}

//------------------------------------------------------------------------------
int vtkMetaInfoDatabase::Search(int p, int np, int ext[6],
                                int field_association,
                                const char *ArrayName, int component,
                                double *range)
{
  ArrayRecord *record = this->Records->GetRecord
    (field_association, ArrayName, component);
  return record->Search(p, np, ext, range);
}
