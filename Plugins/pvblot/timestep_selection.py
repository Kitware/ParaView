"""

4.1.3   Time Step Selection (any subprogram)

These commands control the time step selection as explained in Section
[Ref:intro:timemode].  The following are the time step selection parameters:

     * tmin is the minimum selected time,
     * tmax is the maximum selected time,
     * nintv is the number of selected time intervals, and
     * delt is the selected time interval.
                                            
In the interval-times mode, up to nintv time steps at interval delt between tmin and tmax are
selected. The mode may have a delta offset or a zero offset. With a delta offset, the first
selected time is tmin+delt; with a zero offset, it is tmin.
In the interval-times mode with a delta offset, the number of selected time intervals nintv
and the selected time interval delt are related mathematically by the equations:

delt = (tmax-tmin) / nintv (1)
nintv = int ((tmin-tmax) / delt) (2)

With a zero offset, nintv and delt are related mathematically by the equations:

delt = (tmax-tmin) / (nintv-1) (1)
nintv = int ((tmin-tmax) / delt) + 1 (2)

The user specifies either nintv or delt. If nintv is specified, delt is calculated using equation
1. If delt is specified, nintv is calculated using equation 2.
In the all-available-times mode, all database time steps between tmin and tmax are selected
(parameters nintv and delt are ignored). In the user-selected-times mode, the specified
times are selected (all parameters are ignored).
The initial mode is the interval-times mode with a delta offset. Parameters tmin, tmax, and
nintv are set to their default values and delt is calculated.

TMIN tmin <minimum database time>
      TMIN sets the minimum selected time tmin to the specified parameter value. If the
      user-selected-times mode is in effect, the mode is changed to the all-available-
      times mode.
      In interval-times mode, if nintv is selected (by a NINTV or ZINTV command), delt
      is calculated. If delt is selected (by a DELTIME command), nintv is calculated.

TMAX tmax <maximum database time>
      TMAX sets the maximum selected time tmax to the specified parameter value. If
      the user-selected-times mode is in effect, the mode is changed to the all-available-
      times mode.
      In interval-times mode, if nintv is selected (by a NINTV or ZINTV command), delt
      is calculated. If delt is selected (by a DELTIME command), nintv is calculated.

NINTV nintv <10 or the number of database time steps - 1,> whichever is smaller
      NINTV sets the number of selected time intervals nintv to the specified parameter
      value and changes the mode to the interval-times mode with a delta offset. The
      selected time interval delt is calculated.
                                                22
ZINTV nintv <10 or the number of database time steps,> whichever is smaller
    ZINTV sets the number of selected time intervals nintv to the specified parameter
    value and changes the mode to the interval-times mode with a zero offset. The
    selected time interval delt is calculated.

DELTIME delt <(tmax-tmin) / (nintv-1), where nintv is 10> or the number of
    database time steps, whichever is smaller
    DELTIME sets the selected time interval delt to the specified parameter value and
    changes the mode to the interval-times mode with a zero offset. The number of
    selected time intervals nintv is calculated.

ALLTIMES
    ALLTIMES changes the mode to the all-available-times mode.

TIMES [ADD,] t1, t2, ... <no times selected>
    TIMES changes the mode to the user-selected-times mode and selects times t1, t2,
    etc. The closest time step from the database is selected for each specified time.
    Normally, a TIMES command selects only the listed time steps. If ADD is the first
    parameter, the listed steps are added to the current selected times. Any other time
    step selection command clears all TIMES selected times.
    Up to the maximum number of time steps in the database may be specified. Times
    are selected in the order encountered on the database, regardless of the order the
    times are specified in the command. Duplicate references to a time step are
    ignored.

STEPS [ADD,] n1, n2, ... <no steps selected>
    The STEPS command is equivalent to the TIMES command except that it selects
    time steps by the step number, not by the step time.

HISTORY *skipped*


"""
class Error(Exception): pass

class TimestepSelection(object):
    """Public api:

    selected_indices
    selected_times

    reset()
    set_tmin(value=None)
    set_tmax(value=None)
    set_nintv(value=None)
    set_zintv(value=None)
    set_deltime(value=None)
    set_alltimes()

    parse_times(string)
    parse_steps(string)

    """
  
    # mode:
    all_available_times = 1
    interval_times = 2
    user_selected_times = 3

    # interval_select_mode:
    nintv_select = 1
    delt_select = 2

    # offset_mode:
    delt_offset = 1
    zero_offset = 2

    def __init__(self, available_times):
        """Given available_times, a list of all the available timesteps.
        Assumes the available_times is list sorted in ascending order and
        all values are positive."""

        # Make a copy of the list and make sure it has at least 1 value
        self.available_times = list(available_times)
        if not self.available_times: self.available_times = [0]
        self.reset()

    def reset(self):
        self.selected_indices = []
        self.selected_times = []
        self.tmin = self.available_times[0]
        self.tmax = self.available_times[-1]
        self.mode = self.interval_times
        self.interval_select_mode = self.nintv_select
        self.offset_mode = self.delt_offset
        self.nintv = self._get_default_nintv()
        self._update_delt_and_nintv()
        self._update_selected_timesteps()

    def _update_delt_and_nintv(self):
        # For nintv_select mode, update delt
        if self.interval_select_mode == self.nintv_select:
            if self.offset_mode == self.delt_offset:
                try: self.delt = (self.tmax - self.tmin) / self.nintv
                except ZeroDivisionError: self.delt = self.tmax - self.tmin
            elif self.offset_mode == self.zero_offset:
                try: self.delt = (self.tmax - self.tmin) / (self.nintv - 1)
                except ZeroDivisionError: self.delt = self.tmax - self.tmin

        # For delt_select mode, update nintv
        if self.interval_select_mode == self.delt_select:
            if self.offset_mode == self.delt_offset:
                try: self.nintv = int( (self.tmax - self.tmin) / self.delt )
                except ZeroDivisionError: self.nintv = int(self.tmax - self.tmin) 
            elif self.offset_mode == self.zero_offset:
                try: self.nintv = int( (self.tmax - self.tmin) / self.delt ) + 1
                except ZeroDivisionError: self.nintv = int(self.tmax - self.tmin) + 1


    def print_show(self):
        if self.mode == self.all_available_times:
            print " Select all whole times from %f to %f" % (self.tmin, self.tmax)
        elif self.mode == self.interval_times:
            if self.interval_select_mode == self.nintv_select:
                min_max_nintv = (self.tmin, self.tmax, self.nintv)
                if self.offset_mode == self.delt_offset:
                    print " Select whole times %f to %f in %d intervals with delta offset" % min_max_nintv
                elif self.offset_mode == self.zero_offset:
                    print " Select whole times %f to %f in %d intervals with zero offset" % min_max_nintv
            elif self.interval_select_mode == self.delt_select:
                print " Select whole times %f to %f by %f" %  (self.tmin, self.tmax, self.delt)
        elif self.mode == self.user_selected_times:
            print " Select specified whole times"   
        print "    Number of selected times = %d" % len(self.selected_times)
        print    


    def _update_selected_timesteps(self):
        if self.mode == self.all_available_times:
            lo = self._get_closest_index_for_time(self.tmin)
            hi = self._get_closest_index_for_time(self.tmax)
            self.selected_indices = range(lo, hi+1)
        elif self.mode == self.interval_times:
            self.selected_indices = []
            index_offset = 0
            if self.offset_mode == self.delt_offset: index_offset = 1
            indices_set = set()
            for i in range(index_offset, self.nintv + index_offset):
                t = self.tmin + i * self.delt
                indices_set.add(self._get_closest_index_for_time(t))
                #print "loop",i,self.selected_indices[-1]
            self.selected_indices = list(indices_set)
            self.selected_indices.sort()
        elif self.mode == self.user_selected_times:
            pass

        self.selected_times = [self.available_times[i] for i in self.selected_indices]

    def _on_tmin_tmax_changed(self):
        if self.mode == self.user_selected_times:
            self.mode = self.all_available_times
        self._update_delt_and_nintv()
        self._update_selected_timesteps()

    def set_tmin(self, value=None):
        if value == None: self.tmin = self.available_times[0]
        else: self.tmin = value
        self._on_tmin_tmax_changed()

    def set_tmax(self, value=None):
        if value == None: self.tmax = self.available_times[-1]
        else: self.tmax = value
        self._on_tmin_tmax_changed()

    def _set_nintv_internal(self, value=None):
        nintv_max = len(self.available_times)-1
        if value > nintv_max: value = nintv_max
        self.mode = self.interval_times
        self.interval_select_mode = self.nintv_select
        if value is None or value == 0:
            value = self._get_default_nintv()
        self.nintv = value
        self._update_delt_and_nintv()
        self._update_selected_timesteps()
        
    def set_nintv(self, value=None):
        self.offset_mode = self.delt_offset
        self._set_nintv_internal(value)

    def set_zintv(self, value=None):
        self.offset_mode = self.zero_offset
        self._set_nintv_internal(value)

    def set_deltime(self, value=None):
        self.mode = self.interval_times
        self.offset_mode = self.zero_offset
        self.interval_select_mode = self.delt_select
        if value is None:
            default_nintv = self._get_default_nintv()
            try: value = (self.tmax - self.tmin) / (default_nintv - 1)
            except ZeroDivisionError: value = self.tmax - self.tmin
        if value <= 0:
            self.set_alltimes()
            return
        self.delt = value
        self._update_delt_and_nintv()
        self._update_selected_timesteps()

    def set_alltimes(self):
        self.mode = self.all_available_times
        self._update_selected_timesteps()

    def _set_selected_indices_from_set(self, indices_set):
        self.selected_indices = list(indices_set)
        self.selected_indices.sort()
        self._update_selected_timesteps()

    def parse_times(self, string_arg):
        """If there is an error in parsing raises an Error exception."""
        self.mode = self.user_selected_times
        tokens = string_arg.replace(",", "").lower().split()
        if len(tokens) == 0: tokens = [0]
        if tokens[0] == "add":
            indices_set = set(self.selected_indices)
            tokens.pop(0)
        else: indices_set = set()
        for token in tokens:
            try: t = float(token)
            except: raise Error("Expected number of type float not '%s'" % token)
            indices_set.add(self._get_closest_index_for_time(t))
        self._set_selected_indices_from_set(indices_set)

    def parse_steps(self, string_arg):
        """If there is an error in parsing raises an Error exception.
        Note, steps are specified with starting index equal to 1 (not 0),
        but steps indices are stored (self.selected_indices) with starting
        index equal to 0."""
        self.mode = self.user_selected_times
        tokens = string_arg.replace(",", "").lower().split()
        if len(tokens) == 0: tokens = [0]
        if tokens[0] == "add":
            indices_set = set(self.selected_indices)
            tokens.pop(0)
        else: indices_set = set()
        idx_max = len(self.available_times)
        for token in tokens:
            try: idx = int(token)
            except: raise Error("Expected number of type int not '%s'" % token)
            if idx <= 0 or idx > idx_max:
                print " *** ERROR - Step %d does not exist, ignored" % idx
                continue
            indices_set.add(idx-1)
        self._set_selected_indices_from_set(indices_set)

    def _get_default_nintv(self):
        default_nintv = 10
        nSteps = len(self.available_times)
        if nSteps-1 < default_nintv: return nSteps-1
        else: return default_nintv


    def _test_closest(self, time):
        idx = self._get_closest_index_for_time(time)
        print "idx, time", idx, self.available_times[idx]

    def _get_closest_time(self, time):
        return self.available_times[self._get_closest_index_for_time(time)]

    def _get_closest_index_for_time(self, time):
        import bisect
        # Find indices hi and lo
        hi = bisect.bisect_right(self.available_times, time)
        lo = hi - 1

        # Check if hi index is on list boundary
        if hi == 0: return hi
        if hi == len(self.available_times): return lo

        # Find value closest to given time and return the index
        dxLo = abs(self.available_times[lo]-time)
        dxHi = abs(self.available_times[hi]-time)
        if dxLo < dxHi: return lo
        else: return hi

