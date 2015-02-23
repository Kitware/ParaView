import cinema_store
import itertools

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
            e.execute(doc)
        self.insert(doc)

    def explore(self, fixedargs=None):
        """Explore the problem space to populate the store"""
        self.prepare()

        ordered = self.list_parameters()
        args = []
        values = []
        for name in ordered:
            vals = self.cinema_store.get_parameter(name)['values']
            args.append(name)
            values.append(vals)

        for element in itertools.product(*values):
            desc = dict(itertools.izip(args, element))
            if fixedargs != None:
                desc.update(fixedargs)
            self.execute(desc)

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
