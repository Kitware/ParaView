#ifndef __nektarObject_h

#include "vtkUnstructuredGrid.h"

class VTK_EXPORT nektarObject
{
 public:
//    static nektarObject *New();
//    static nektarObject *Delete();

    vtkUnstructuredGrid* ugrid;
    bool pressure;
    bool velocity;
    bool vorticity;
    bool lambda_2;
    int index;
    int resolution;
    nektarObject *prev;
    nektarObject *next;

    nektarObject *find_obj(int id);
    void reset();

// protected:
    nektarObject();
    ~nektarObject();

};


class nektarList
{
 public:
//    static nektarList *New();
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
