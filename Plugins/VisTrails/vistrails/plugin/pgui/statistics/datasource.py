
############################################################################
##
## This file is part of the Vistrails ParaView Plugin.
##
## This file may be used under the terms of the GNU General Public
## License version 2.0 as published by the Free Software Foundation
## and appearing in the file LICENSE.GPL included in the packaging of
## this file.  Please review the following to ensure GNU General Public
## Licensing requirements will be met:
## http://www.opensource.org/licenses/gpl-2.0.php
##
## This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
## WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
##
############################################################################

############################################################################
##
## Copyright (C) 2006, 2007, 2008 University of Utah. All rights reserved.
## Copyright (C) 2008, 2009 VisTrails, Inc. All rights reserved.
##
############################################################################
"""
DataSource
ScrollableDataSource
TimeDataSource
TimeDiffDataSource
"""


#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------
from gui.utils import getBuilderWindow
import math,time,datetime


#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------
class DataSource(object):
    """ DataSource is the generic data structure to implement a source.
    """
  
    def __init__(self):
        """ __init__() -> None
        Do nothing.
        """
        pass
        
    def data(self):
        """ data() -> ???
        Overwrite this function to return the content of the source.
        """
        return None
        
    def isEmpty(self):
        """ isEmpty() -> bool
        Overwrite this function to provide information, if the source is empty.
        """
        return True
        

#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------
class ScrollableDataSource(DataSource):
    """ ScrollableDataSource implements a source where the data can be scaled
    in one variable.
    """

    def defaultLeftBound(self):
        """ defaultLeftBound() -> float/None 
        """
        return None
        
    def defaultRightBound(self):
        """ defaultRightBound() -> float/None
        """
        return None
    
    def defaultPageSize(self):
        """ defaultPageSize() -> float/None
        Return the initial page size for scrollable statistics views
        like histograms.
        """
        return None
        

#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------
class TimeDataSource(ScrollableDataSource):
    """ TimeDataSource implements a source that provides the node creation
    dates in an 1D-array.
    """
  
    def __init__(self):
        """ __init__() -> None
        """
        
        """ extract the vistrail dates """
        self.dates=[]
        for action in getBuilderWindow().viewManager.currentWidget().controller.vistrail.actions:
            dt=datetime.datetime.strptime(action.date,'%d %b %Y %H:%M:%S')
            self.dates.append(time.mktime(dt.timetuple())+dt.microsecond/1000000.0)
                
    def data(self):
        """ data() -> array(int)
        See DataSource.data().
        """
        return self.dates
        
    def isEmpty(self):
        """ isEmpty() -> bool
        Self-explanatory; see DataSource.isEmpty().
        """
        return len(self.dates)==0
    
    def defaultLeftBound(self):
        """ defaultLeftBound() -> float/None
        See ScrollableDataSource.defaultLeftBound().
        """
        if len(self.dates)>0:
            return min(self.dates)
        else:
            return None
            
    def defaultRightBound(self):
        """ defaultRightBound() -> float/None
        See ScrollableDataSource.defaultRightBound().
        """
        if len(self.dates)>0:
            return max(self.dates)
        else:
            return None
        
    def defaultPageSize(self):
        """ defaultPageSize() -> float/None
        See ScrollableDataSource.defaultPageSize().
        """
        if len(self.dates)>0:
            defaultLeft=min(self.dates)
            defaultRight=max(self.dates)
            return defaultRight-defaultLeft
        else:
            return None


#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------
class TimeDiffDataSource(ScrollableDataSource):
    """ TimeDiffDataSource implements a source that provides the differences
    between consecutive created nodes in the vistrails.
    """
  
    """ maximum time difference in seconds """
    timeDiffThreshold=60*10

    def __init__(self):
        """ __init__() -> None
        """
    
        """ extract the vistrail dates """
        dates=[]
        for action in getBuilderWindow().viewManager.currentWidget().controller.vistrail.actions:
            dt=datetime.datetime.strptime(action.date,'%d %b %Y %H:%M:%S')
            dates.append(time.mktime(dt.timetuple())+dt.microsecond/1000000.0)

        """ compute the time differences """
        self.diffs=[]
        if len(dates)>0:
            dates=sorted(dates)
            for i in range(len(dates)-1):
                diff=dates[i+1]-dates[i]
                if diff<=self.timeDiffThreshold:
                    self.diffs.append(diff)
              
    def data(self):
        """ data() -> array(float)
        See DataSource.data().
        """
        return self.diffs
    
    def isEmpty(self):
        """ isEmpty() -> bool
        Self-explanatory; see DataSource.isEmpty()
        """
        return len(self.diffs)==0
    
    def defaultLeftBound(self):
        """ defaultLeftBound() -> float/None
        See ScrollableDataSource.defaultLeftBound().
        """
        if len(self.diffs)>0:
            return 0
        else:
            return None
            
    def defaultRightBound(self):
        """ defaultRightBound() -> float/None
        See ScrollableDataSource.defaultRightBound().
        """
        if len(self.diffs)>0:
            return max(self.diffs)
        else:
            return None

    def defaultPageSize(self):
        """ defaultPageSize() -> float/None
        Use twice the standard deviation of the signal for the default page size;
        see ScrollableDataSource.defaultPageSize().
        """
        if len(self.diffs)>0:
            defaultLeft=0
            defaultRight=max(self.diffs)

            """ mean and stddev for initial view estimate """
            mean=0
            for diff in self.diffs:
                mean+=diff
            mean/=len(self.diffs)
            stddev=0
            for diff in self.diffs:
                stddev+=(diff-mean)*(diff-mean)
            stddev=math.sqrt(stddev/len(self.diffs))
            return 2*stddev
        else:
            return None


#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------
class TimeActionDataSource(DataSource):
    """ TimeActionDataSource 
    """
  
    def __init__(self):
        """ __init__() -> None
        """
    
        pass
              
    def data(self):
        """ data() -> array(float)
        See DataSource.data().
        """
        return self.diffs
    
    def isEmpty(self):
        """ isEmpty() -> bool
        Self-explanatory; see DataSource.isEmpty()
        """
        return len(self.diffs)==0
    
    def defaultLeftBound(self):
        """ defaultLeftBound() -> float/None
        See ScrollableDataSource.defaultLeftBound().
        """
        if len(self.diffs)>0:
            return 0
        else:
            return None
            
    def defaultRightBound(self):
        """ defaultRightBound() -> float/None
        See ScrollableDataSource.defaultRightBound().
        """
        if len(self.diffs)>0:
            return max(self.diffs)
        else:
            return None

    def defaultPageSize(self):
        """ defaultPageSize() -> float/None
        Use twice the standard deviation of the signal for the default page size;
        see ScrollableDataSource.defaultPageSize().
        """
        if len(self.diffs)>0:
            defaultLeft=0
            defaultRight=max(self.diffs)

            """ mean and stddev for initial view estimate """
            mean=0
            for diff in self.diffs:
                mean+=diff
            mean/=len(self.diffs)
            stddev=0
            for diff in self.diffs:
                stddev+=(diff-mean)*(diff-mean)
            stddev=math.sqrt(stddev/len(self.diffs))
            return 2*stddev
        else:
            return None

    def getInfoFromPipeline(self):
        """ getInfoFromPipeline() -> (string array,string array)
        get operations and descriptions from pipeline """
        controller=getBuilderWindow().viewManager.currentWidget().controller
        ops=[]
        descriptions=controller.vistrail.descriptionMap.values()
        moduleIds=sorted(controller.current_pipeline.modules.keys())
        for mId in moduleIds:
            module=controller.current_pipeline.modules[mId]
            localOps=[]
            descritpion=''
            for function in module.functions:
                if function.name=='value':
                    o=urllib.unquote(module.functions[function.id].params[0].strValue)
                    localOps.append(unpickleOperations(o))
            if len(localOps)>0:
                ops.append(localOps[0][0])
        return (descriptions,ops)
