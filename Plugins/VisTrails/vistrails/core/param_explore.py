
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
##
############################################################################
"""
This module handles Parameter Exploration in VisTrails
"""
from core.vistrail.module_function import ModuleFunction
from core.vistrail.module_param import ModuleParam
import copy

################################################################################

class ParameterExploration(object):
    """
    ParameterExploration is a class that will take a pipeline and a
    set of parameter values and executes all of them

    """
    
    def __init__(self, specs):
        """ ParameterExploration(specs: list) -> ParameterExploration
        Takes a list of interpolator list. The number of items in the
        list is also the number of dimensions this parameter
        exploration is going to explore on
        
        """
        self.specs = specs

    def explore(self, pipeline):
        """ explore(pipeline: VisPipeline) -> list[VisPipeline]        
        Apply parameter exploration on multiple dimensions using the
        values in self.specs
        
        """
        pipelineList = [pipeline]
        for i in xrange(len(self.specs)):
            pipelineList = self.interpolateList(pipelineList, self.specs[i])
        return pipelineList

    def interpolateList(self, pipelineList, interpList):
        """ interpolateList(pipeline: list[VisPipeLine],
                            interpList: InterpolateDiscreteParam)
                            -> list[VisPipeline]                            
        Returns a list of interpolated pipelines, by applying the
        interpolators in a multiplication manner
        
        """
        if len(interpList)<1: return pipelineList
        stepCount = interpList[0].stepCount
        result = []
        for step in xrange(stepCount):
            for pipeline in pipelineList:
                newp = copy.copy(pipeline)
                for interp in interpList:
                    interp.perform(newp, step)
                result.append(newp)
        return result
    
class InterpolateDiscreteParam(object):
    """
    InterpolateDiscreteParam takes in ranges and a number of
    steps. Then given a pipeline, it will interpolate and generate a
    number of steps pipelines with values interpolated between the
    ranges
    
    """

    def __init__(self, module, function, ranges, stepCount):
        """ InterpolateDiscreteParam(module: Module,
                                     function: str,
                                     ranges: list(tuple),
                                     stepCount: int)
                                     -> InterpolateDiscreteParam
                                     
        Initialize the interpolator with a specific module function
        given a list of parameter ranges

        Keyword arguments:
        module    --- module id in a pipeline        
        function  --- a string express the function name that belongs
                      to module
                      
        ranges    --- [tupe(min,max) or tuple(s1,s2..,s{stepCount}],
                      ranges specified for each argument of function
                      where s{i} is of type 'str'
                      
        stepCount --- the number of step for the interpolation
                                     
        """
        self.module = module
        self.function = function
        self.stepCount = stepCount
        self.values = self.interpolate(ranges, stepCount)

    def interpolate(self, ranges, stepCount):
        """ interpolate(ranges: tuple, stepCount: int) -> list
        
        This function takes a number of (min,max) or (s1,...sn) to
        interpolate exact stepCount number of step. The output will be
        a list of stepCount elements where each of them is (a1,...,an).
        a{i} is either int or string and n is the number of arguments.
        
        """
        params = []
        for r in ranges:
            interpolatedValues = []
            argumentType = type(r[0])
            if argumentType in [int, float]:
                for i in xrange(stepCount):
                    if stepCount>1: t = i/float(stepCount-1)
                    else: t = 0
                    interpolatedValues.append(argumentType(r[0]+t*(r[1]-r[0])))
            elif argumentType==str:
                interpolatedValues = list(r)
            else:
                print 'Cannot interpolate non-cardinal types'
                assert False
            params.append(interpolatedValues)
        return zip(*params)
        
        
    def perform(self, pipeline, step):
        """ perform(pipeline: VisPipeline, step: int) -> None        
        This will takes a pipeline and apply the interpolated values
        at step 'step' to the pipeline. Then return the updated
        pipeline

        """
        m = pipeline.modules[self.module.id]
        f = ModuleFunction()
        f.name = self.function
        f.returnType = 'void'
        value = self.values[step]
        for v in value:
            p = ModuleParam()
            convert = {'int':'Integer', 'str':'String',
                       'float':'Float', 'double':'Float'}
            p.type = convert[type(v).__name__]
            p.strValue = str(v)
            f.params.append(p)
        m.functions.append(f)

class ActionBasedParameterExploration(object):
    """
    ActionBasedParameterExploration is going to replace
    ParameterExploration class to perform parameter exploration. It is
    an action based parameter exploration
    
    """
    def explore(self, pipeline, actions):
        """ explore(pipeline: Pipeline, actions: [action set]) -> [(pipeline, actions)]
        Perform the parameter exploration on the pipeline with a set
        of actions. Each 'action set' represent a tuple of actions that we
        need to apply on the pipeline at a step in a dimension. For example:
        actions = [[(action1a, action2a), (action1b, action2b)],
                   [(action3a), (action3b)]]
                   
        which means this is a 2 x 1 parameter exploration resulting of
        executing 2 pipelines. The first one is
        pipeline.action1a.action2a.action3a and the second one is
        pipeline.action1b.action2b.action3b.
        
        The return values are a list of tuples containing interpolated
        pipeline and a set of actions leading to that interpolated
        pipeline. This is useful for update the parameter exploration
        back to the builder.
        
        """
        results = []
        resultActions = []
        
        def exploreDimension(pipeline, performedActions, dim):
            """ exploreDimension(pipeline: Pipeline, performedActions: [actions],
                                 dim: int) -> None
            Start applying actions to the pipeline at dimension
            dim. 'pipeline' will not be modified in the function
            
            """
            if dim<0:
                results.append(pipeline)
                resultActions.append(performedActions)
                return
            currentActions = actions[dim]
            if len(currentActions)==0:
                # Ignore empty dimension
                exploreDimension(pipeline, performedActions, dim-1)
                return
            for actionSet in currentActions:
                currentPipeline = copy.copy(pipeline)
                currentPeformedActions = copy.copy(performedActions)
                for action in actionSet:
                    currentPipeline.perform_action(action)
                    currentPeformedActions.append(action)
                exploreDimension(currentPipeline, currentPeformedActions, dim-1)
                
        exploreDimension(pipeline, [], len(actions)-1)
        return (results, resultActions)

################################################################################
        
import unittest

class TestParameterExploration(unittest.TestCase):
    """
    Test if ParameterExploration is executing correctly. For now it is a very
    simple test to test more of the interpolated values
    
    """
    def testInterpolator(self):
        interpolator = InterpolateDiscreteParam(0, 'testing',
                                                [(0,10),
                                                 (0.0,10.0),
                                                 ('one', 'two', 'three')],
                                                3)
        self.assertEqual(interpolator.values,
                         [(0, 0.0, 'one'),
                          (5, 5.0, 'two'),
                          (10, 10.0, 'three')])

if __name__ == '__main__':
    unittest.main()
