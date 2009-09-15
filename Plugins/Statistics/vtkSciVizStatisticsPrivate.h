#ifndef __vtkSciVizStatisticsPrivate_h
#define __vtkSciVizStatisticsPrivate_h

#include "vtkStatisticsAlgorithmPrivate.h"

class vtkSciVizStatisticsP : public vtkStatisticsAlgorithmPrivate
{
public:
  bool Has( vtkStdString arrName )
    {
    return this->Buffer.find( arrName ) != this->Buffer.end();
    }
};


#endif // __vtkSciVizStatisticsPrivate_h
