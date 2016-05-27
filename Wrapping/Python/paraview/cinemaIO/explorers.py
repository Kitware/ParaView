#==============================================================================
# Copyright (c) 2015,  Kitware Inc., Los Alamos National Laboratory
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this
# list of conditions and the following disclaimer in the documentation and/or other
# materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors may
# be used to endorse or promote products derived from this software without specific
# prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
# OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#==============================================================================
import cinema_store
import itertools
import json

class Explorer(object):
    """
    Middleman that connects an arbitrary producing codes to the CinemaStore.
    The purpose of this class is to run through the parameter sets, and tell a
    set of tracks (in order) to do something with the parameter values
    it cares about.
    """

    def __init__(self,
        cinema_store,
        parameters, #these are the things that this explorer is responsible for and their ranges
        tracks #the things we pass off values to in order to do the work
        ):

        self.__cinema_store = cinema_store
        self.parameters = parameters
        self.tracks = tracks

    @property
    def cinema_store(self):
        return self.__cinema_store

    def list_parameters(self):
        """
        parameters is an ordered list of parameters that the Explorer varies over
        """
        return self.parameters

    def prepare(self):
        """ Give tracks a chance to get ready for a run """
        if self.tracks:
            for e in self.tracks:
                res = e.prepare(self)

    def execute(self, desc):
        # Create the document/data product for this sample.
        doc = cinema_store.Document(desc)
        for e in self.tracks:
            #print "EXECUTING track ", e, doc.descriptor
            e.execute(doc)
        self.insert(doc)

    def explore(self, fixedargs=None):
        """
        Explore the problem space to populate the store being careful not to hit combinations
        where dependencies are not satisfied.
        Fixed arguments are the parameters that we want to hold constant in the exploration.
        """
        self.prepare()

        for descriptor in self.cinema_store.iterate(self.list_parameters(), fixedargs):
            self.execute(descriptor)

        self.finish()

    def finish(self):
        """ Give tracks a chance to clean up after a run """
        if self.tracks:
            for e in self.tracks:
                res = e.finish()

    def insert(self, doc):
        self.cinema_store.insert(doc)

class Track(object):
    """
    abstract interface for things that can produce data

    to use this:
    caller should set up some visualization
    then tie a particular set of parameters to an action with a track
    """

    def __init__(self):
        pass

    def prepare(self, explorer):
        """ subclasses get ready to run here """
        pass

    def finish(self):
        """ subclasses cleanup after running here """
        pass

    def execute(self, document):
        """ subclasses operate on parameters here"""
        pass

class LayerControl(object):
    """
    Prototype for something that Layer track can control
    """
    def __init__(self, name, showFunc, hideFunc):
        self.name = name
        self.callShow = showFunc  #todo, determine if function now and convert instead of try/except below
        self.callHide = hideFunc

class Layer(Track):
    """
    A track that connects a layer to the set of objects in the scene that it controls.
    """
    def __init__(self, layer, objectlist):
        super(Layer, self).__init__()
        self.parameter = layer
        # objlist is an array of class instances, they must have a name and
        #show and hide method, use LayerControl to make them.
        self.objectlist = objectlist

    def execute(self, doc):
        o = None
        if self.parameter in doc.descriptor:
            o = doc.descriptor[self.parameter]
        for obj in self.objectlist:
            if obj.name == o:
                try:
                    obj.callShow() #method
                except TypeError:
                    obj.callShow(obj) #function
            else:
                try:
                    obj.callHide()
                except TypeError:
                    obj.callHide(obj)
