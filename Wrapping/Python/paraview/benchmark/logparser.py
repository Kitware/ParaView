from __future__ import absolute_import

import sys
from . import logbase


class FrameLogEntry:
    '''A basic container for holding timing information for a given filter'''

    import re
    _timere = '([-+]?\d*\.?\d+([eE][-+]?\d+)?) +seconds'
    _match_filter = re.compile('Execute (\w+) id: +(\d+), +' + _timere)
    _match_vfilter = re.compile('Execute (\w+) *, +' + _timere)
    _match_comp_comp = re.compile('TreeComp composite, *' + _timere)
    _match_comp_xmit = re.compile(
        'TreeComp (Send|Receive) (\d+) (to|from) (\d+) uchar (\d+), +' + _timere)
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
            self.Id = '-1'
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
        return '%(ind)s%(id)s %(name)s %(t)f' % \
            {'ind': ' ' * self.Indent, 'id': self.Id, 'name': self.Name,
             't': float('NaN') if self.Duration is None else self.Duration}

    def __eq__(self, other):
        return self.Indent == other.Indent and self.Id == other.Id and \
            self.Name == other.Name


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
    if merge_before_nframes > 0:
        f0 = all_frames[0]
        list(map(lambda f: f0.Logs.extend(f.Logs),
            all_frames[1:-merge_before_nframes]))
        return [f0] + all_frames[-merge_before_nframes:]
    return all_frames


def process_logs(merge_before_nframes=0):
    '''Collect and parse logs for all ranks

    Keyword arguments:
    merge_before_nframes -- All entries before this many frames will be merged
    '''
    logbase.get_logs()

    comp_rank_frame_logs = {}
    for log in logbase.logs:
        rank_frame_logs = comp_rank_frame_logs.setdefault(log.component, [])
        if len(rank_frame_logs) < log.rank + 1:
            rank_frame_logs.extend(
                [None] * (log.rank + 1 - len(rank_frame_logs)))
        rank_frame_logs[log.rank] = _parse_a_log(log, merge_before_nframes)
    return comp_rank_frame_logs


def _init_log_collection(logs):
    '''Initialize a collection of logs for statistics purposes'''
    col = []
    for l in logs:
        if isinstance(l, FrameLogEntry):
            l = l.__dict__
        elif isinstance(l, FrameLog):
            l = l.Logs

        if isinstance(l, dict):
            col.append({'Id': l['Id'], 'Name': l['Name'],
                        'Duration': [l['Duration']]})
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
        list(map(self.add_sample, samples))

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
                import math
                self._StdDev = math.sqrt(
                    (self.Ex2 - (self.Ex * self.Ex) / self.N) / (self.N - 1))
        return self._StdDev

    def __repr__(self):
        return 'Count: %(count)d, Mean: %(mean)f, StdDev: %(stddev)f, Min: %(min)f, Max: %(max)f' % {'count': self.N, 'min': self.Min, 'max': self.Max, 'mean': self.Mean, 'stddev': self.StdDev}


def _collect_stats(logs, stat_summary):
    '''Compute statistics and group a set of logs as a list of durations'''
    stats = []
    for l in logs:
        if isinstance(l, dict):
            b = BasicStats(l['Duration'])
            stats.append({'Id': l['Id'], 'Name': l[
                         'Name'], 'Stats': b, 'Duration': getattr(b, stat_summary)})
        else:
            assert isinstance(l, list)
            stats.append(_collect_stats(l, stat_summary))
    return stats


def collect_stats(frame_logs, stat_summary='Mean'):
    '''Collect statistics on the specified key in a set of logs'''
    if isinstance(frame_logs[0], FrameLog):
        frame_logs = [x.Logs for x in frame_logs]
    log_collection = _init_log_collection(frame_logs[0])
    for f in frame_logs[1:]:
        for log_collection_entry, l in zip(log_collection, f):
            _append_log_collection(log_collection_entry, l)
    return _collect_stats(log_collection, stat_summary)


def process_stats_across_ranks(rank_frame_logs):
    '''Calculate stats across all ranks for each frame'''
    frame_stats = []
    for f in range(1, len(rank_frame_logs[0])):
        rank_logs = [r[f] for r in rank_frame_logs]
        frame_stats.append(collect_stats(rank_logs, 'Max'))
    return frame_stats


def summarize_stats(rank_frame_logs):
    '''Summarize statistics across ranks, and then across frames'''
    try:
        frame_stats = process_stats_across_ranks(rank_frame_logs)
        try:
            summary_stats = collect_stats(frame_stats)
        except:
            return frame_stats, None
    except:
        return None, None

    return frame_stats, summary_stats


def write_stats_to_file(stats, indent=0, outfile=sys.stdout):
    '''Print the statics for a given frame'''
    for s in stats:
        if isinstance(s, dict):
            outfile.write(
                ' ' * indent + s['Id'] + ' ' + s['Name'] + ', ' + str(s['Stats']) + '\n')
        else:
            assert isinstance(s, list)
            write_stats_to_file(s, indent + 4, outfile)


def summarize_results(num_frames, num_seconds_m0, items_per_frame, item_label,
                      save_logs=False, output_basename=None):
    '''Process the timing logs to display, save, and gather stats

    Keyword arguments:
    num_frames      -- Number of frames to process
    num_seconds_m0  -- Total number of seconds, excluding the first frame
    items_per_frame -- Number of items per frame getting processed
    item_label      -- Output label for associated items_per_frame
    save_logs       -- Whether or not to write the logs to a file
    output_basename -- Basename to use for output files
    '''

    comp_rank_frame_logs = process_logs(num_frames - 1)
    if save_logs:
        logbase.dump_logs(output_basename + '.logs.raw.bin')
        with open(output_basename + '.logs.parsed.bin', 'wb') as ofile:
            import pickle
            pickle.dump(comp_rank_frame_logs, ofile)

    # Only deal with the server logs
    if 'Servers' in comp_rank_frame_logs.keys():
        rank_frame_logs = comp_rank_frame_logs['Servers']
    elif 'ClientAndServers' in comp_rank_frame_logs.keys():
        rank_frame_logs = comp_rank_frame_logs['ClientAndServers']
    else:
        rank_frame_logs = None

    print ('\nStatistics:\n' + '=' * 40 + '\n')
    if rank_frame_logs:
        print ('Rank 0 Frame 0\n' + '-' * 40)
        print (rank_frame_logs[0][0])
        print ('')
        if save_logs:
            with open(output_basename + '.stats.r0f0.txt', 'w') as ofile:
                ofile.write(str(rank_frame_logs[0][0]))

        frame_stats, summary_stats = summarize_stats(rank_frame_logs)
        if frame_stats:
            for f in range(0, len(frame_stats)):
                print ('Frame ' + str(f + 1) + '\n' + '-' * 40)
                write_stats_to_file(frame_stats[f], outfile=sys.stdout)
                print ('')
                with open(output_basename + '.stats.frame.txt', 'w') as ofile:
                    for f in range(0, len(frame_stats)):
                        ofile.write('Frame ' + str(f + 1) + '\n' + '-' * 40 + '\n')
                        write_stats_to_file(frame_stats[f], outfile=ofile)
                        ofile.write('\n')

        if summary_stats:
            print ('Frame Summary\n' + '-' * 40)
            write_stats_to_file(summary_stats, outfile=sys.stdout)
            if save_logs:
                with open(output_basename + '.stats.summary.txt', 'w') as ofile:
                    write_stats_to_file(summary_stats, outfile=ofile)

    fps = (num_frames - 1) / num_seconds_m0
    ips = fps * items_per_frame
    print ('')
    print ('Frames / Sec: %(fps).2f' % {'fps': fps})
    print ('%(ilabel)s / Frame: %(ipf)d' % {'ilabel': item_label, 'ipf': items_per_frame})
    print ('Mi%(ilabel)s / Sec: %(ips).3f' % {'ilabel': item_label, 'ips': ips / (1024.0 * 1024.0)})
