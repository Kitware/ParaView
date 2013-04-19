#ifndef __nektarObject_h

#include "vtkUnstructuredGrid.h"

#define MAX_EXTRA_VARS 16

class nektarObject 
{
 public:
    vtkUnstructuredGrid* ugrid;
    vtkUnstructuredGrid* wss_ugrid;
    bool pressure;
    bool velocity;
    bool vorticity;
    bool lambda_2;
    bool wss;
    bool extra_vars[MAX_EXTRA_VARS];
    int index;
    int resolution;
    int wss_resolution;
    bool use_projection;
    nektarObject *prev;
    nektarObject *next;
    char * dataFilename;

    nektarObject *find_obj(int id);
    void setDataFilename(char* filename);
    void reset();
   
// protected:
    nektarObject();
    ~nektarObject();

};


class nektarList
{
 public:
    nektarObject* head;
    nektarObject* tail;
    int max_count;
    int cur_count;
    nektarObject* getObject(int);

// protected:
    nektarList();
    ~nektarList();
};


#endif
