import paraview.benchmark as bm
import math
import sys
import re


class FrameLogEntry:
    '''A basic container for holding timing information for a given filter'''

    _timere = '([-+]?\d*\.?\d+([eE][-+]?\d+)?) +seconds'
    _match_filter = re.compile('Execute (\w+) id: +(\d+), +' + _timere)
    _match_vfilter = re.compile('Execute (\w+) *, +' + _timere)
    _match_comp_comp = re.compile('TreeComp composite, *' + _timere)
    _match_comp_xmit = re.compile('TreeComp (Send|Receive) (\d+) (to|from) (\d+) uchar (\d+), +' + _timere)
    _match_composite = re.compile('Compositing, +' + _timere)
    _match_send = re.compile('Sending, +' + _timere)
    _match_receive = re.compile('Receiving, +' + _timere)
    _match_still_render = re.compile('(Still) Render, +' + _timere)
    _match_interactive_render = re.compile('(Interactive) Render, +' + _timere)
    _match_render = re.compile('(\w+|\w+ Dev) Render, +' + _timere)
    _match_timeonly = re.compile('([^,]*), +' + _timere)

    def __init__(self, log_msg):
        ls = log_msg.strip()
        self.Indent = log_msg.find(ls)
        if not ls:
            self.Id ='-1'
            return
        self.Id, self.Name, self.Duration = FrameLogEntry._parse_message(ls)

    @classmethod
    def _parse_message(cls, msg):
        match = cls._match_filter.match(msg)
        if match:
            return match.group(2), match.group(1), float(match.group(3))
        match = cls._match_vfilter.match(msg)
        if match:
            return match.group(1), match.group(1), float(match.group(2))
        match = cls._match_comp_comp.match(msg)
        if match:
            return 'tree comp', 'tree comp', float(match.group(1))
        match = cls._match_comp_xmit.match(msg)
        if match:
            return name, match.group(1), float(match.group(6))
        match = cls._match_composite.match(msg)
        if match:
            return 'comp', 'composite', float(match.group(1))
        match = cls._match_send.match(msg)
        if match:
            return 'send', 'send', float(match.group(1))
        match = cls._match_receive.match(msg)
        if match:
            return 'recv', 'receive', float(match.group(1))
        match = cls._match_still_render.match(msg)
        if match:
            return 'still', match.group(1), float(match.group(2))
        match = cls._match_interactive_render.match(msg)
        if match:
            return 'inter', match.group(1), float(match.group(2))
        match = cls._match_render.match(msg)
        if match:
            return 'render', match.group(1), float(match.group(2))
        match = cls._match_timeonly.match(msg)
        if match:
            return '', match.group(1), float(match.group(2))
        return None, None, None

    def __repr__(self):
        return '%(ind)s%(id)s %(name)s %(t)f' % {'ind': ' ' * self.Indent, 'id': self.Id, 'name': self.Name, 't': float('NaN') if self.Duration is None else self.Duration}

    def __eq__(self, other):
        return self.Indent == other.Indent and self.Id == other.Id and self.Name == other.Name

class FrameLog:
    '''Hold the log entries for an entire (sub-)frame'''
    def __init__(self, parent=None, indent=0):
        self.Parent = parent
        self.Indent = indent
        self.Logs = []

    def __contains__(self, msg):
        return msg in [e for e in self.Logs if isinstance(e, FrameLogEntry)]

    def __str__(self):
        return '\n'.join(map(str, self.Logs))


def _parse_a_log(log, merge_before_nframes=0):
    '''Parse the raw logs for a rank into FrameLog and FrameLogEntry objects

    Keyword arguments:
    merge_before_nframes -- All entries before this many frames will be merged
    '''
    all_frames = []
    f = FrameLog()
    for l in log.lines:
        entry = FrameLogEntry(l)
        if entry.Id is None:
            continue

        # Start a new child entry
        if entry.Indent > f.Indent:
            fnew = FrameLog(parent=f, indent=entry.Indent)
            f.Logs.append(fnew)
            f = fnew

        # Fall back to parent entry
        elif entry.Indent < f.Indent:
            while entry.Indent < f.Indent:
                f = f.Parent

        # Check for a new frame
        if entry.Id == '-1' or (entry.Indent == 0 and entry in f):
            all_frames.append(f)
            f = FrameLog()

        if entry.Id != '-1':
            f.Logs.append(entry)

    # Combine the initial entries into a single 'Frame 0' entry
    f0 = all_frames[0]
    map(lambda f: f0.Logs.extend(f.Logs), all_frames[1:-merge_before_nframes])

    return [f0] + all_frames[-merge_before_nframes:]


def process_logs(merge_before_nframes=0):
    '''Collect and parse logs for all ranks

    Keyword arguments:
    merge_before_nframes -- All entries before this many frames will be merged
    '''
    bm.get_logs()

    # We can't guarantee the order the logs will be iterated in
    all_frames = [None] * len(bm.logs)
    for log in bm.logs:
        all_frames[log.rank] = _parse_a_log(log, merge_before_nframes)
    return all_frames


def _init_log_collection(logs):
    '''Initialize a collection of logs for statistics purposes'''
    col = []
    for l in logs:
        if isinstance(l, FrameLogEntry):
            l = l.__dict__
        elif isinstance(l, FrameLog):
            l = l.Logs

        if isinstance(l, dict):
            col.append({'Id': l['Id'], 'Name': l['Name'], 'Duration': [l['Duration']]})
        else:
            assert isinstance(l, list)
            col.append(_init_log_collection(l))
    return col


def _append_log_collection(col_entry, log):
    if isinstance(log, FrameLogEntry):
        log = log.__dict__
    elif isinstance(log, FrameLog):
        log = log.Logs

    if isinstance(log, dict):
        assert isinstance(col_entry, dict)
        assert col_entry['Id'] == log['Id']
        assert col_entry['Name'] == log['Name']
        col_entry['Duration'].append(log['Duration'])
    else:
        assert isinstance(log, list)
        assert isinstance(col_entry, list)
        for c, l in zip(col_entry, log):
            _append_log_collection(c, l)


class BasicStats:
    def __init__(self, samples=[]):
        self.K = 0
        self.N = 0
        self.Ex = 0
        self.Ex2 = 0
        self.Min = None
        self.Max = None
        self._Mean = None
        self._StdDev = None
        map(self.add_sample, samples)

    def add_sample(self, x):
        if self.N == 0:
            self.K = x
        self.N += 1
        self.Ex += x - self.K
        self.Ex2 += (x - self.K) * (x - self.K)
        if self.Min is None or x < self.Min:
            self.Min = x
        if self.Max is None or x > self.Max:
            self.Max = x
        self._Mean = None
        self._StdDev = None

    @property
    def Mean(self):
        if self.N == 0:
            return None
        if self._Mean is None:
            self._Mean = self.K + self.Ex / self.N
        return self._Mean

    @property
    def StdDev(self):
        if self.N == 0:
            return None
        if self._StdDev is None:
            if self.N == 1:
                self._StdDev = 0
            else:
                self._StdDev = math.sqrt((self.Ex2 - (self.Ex * self.Ex) / self.N) / (self.N - 1))
        return self._StdDev

    def __repr__(self):
        return 'Count: %(count)d, Mean: %(mean)f, StdDev: %(stddev)f, Min: %(min)f, Max: %(max)f' % {'count': self.N, 'min': self.Min, 'max': self.Max, 'mean': self.Mean, 'stddev': self.StdDev}


def _collect_duration_stats(logs):
    '''Copmpute statistics Group a set of logs as a list of durrations'''
    stats = []
    for l in logs:
        if isinstance(l, dict):
            b = BasicStats(l['Duration'])
            stats.append({'Id': l['Id'], 'Name': l['Name'], 'Stats': b, 'Duration': b.Mean})
        else:
            assert isinstance(l, list)
            stats.append(_collect_duration_stats(l))
    return stats


def collect_duration_stats(frame_logs):
    '''Collect statistics on the 'Duration' key in a set of logs'''
    if isinstance(frame_logs[0], FrameLog):
        frame_logs = [x.Logs for x in frame_logs]
    log_collection = _init_log_collection(frame_logs[0])
    for f in frame_logs[1:]:
        assert len(log_collection) == len(f)
        for log_collection_entry, l in zip(log_collection, f):
            _append_log_collection(log_collection_entry, l)
    return _collect_duration_stats(log_collection)


def process_stats_across_ranks(rank_frame_logs):
    '''Calculate stats across all ranks for each frame'''
    return [collect_duration_stats([rank_frame_logs[r][f] for r in range(0, len(rank_frame_logs))]) for f in range(1, len(rank_frame_logs[0]))]


def summarize_all_logs(rank_frame_logs):
    '''Summarize statistics across ranks, and then across frames'''
    try:
        frame_stats = process_stats_across_ranks(rank_frame_logs)
        try:
            summary_stats = collect_duration_stats(frame_stats)
        except:
            return frame_stats, None
    except:
        return None, None

    return frame_stats, summary_stats


def write_stats_to_file(stats, indent=0, outfile=sys.stdout):
    '''Print the statics for a given frame'''
    for s in stats:
        if isinstance(s, dict):
            outfile.write(' ' * indent + s['Id'] + ' ' + s['Name'] + ', ' + str(s['Stats']) + '\n')
        else:
            assert isinstance(s, list)
            write_stats_to_file(s, indent + 4, outfile)
