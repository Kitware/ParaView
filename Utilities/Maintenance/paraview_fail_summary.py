#!/usr/bin/python
"""
This script asks cdash to give it a summary of all of the failing tests
on the Nightly (master) and Nightly (next) section.
It presents the tests ranked by the number
of failing machines. From this view you can more easily see what is in
the greatest need of fixing.
The script also scrapes the dashboard with a list of expected submissions and
generates a list of missing ones.
"""

import argparse
import datetime
import sys
import time
import urllib

#-----------------------------------------------------------------------------
def PrintReport(dashDate, branch):

  if csvOutput:
    print ""
    print "-"*10, "Dashboard status for", branch, "-"*10
  elif wikiOutput:
    print "===Dashboard status for " + branch + "==="
  elif htmlOutput:
    print '<p><u><b>Dashboard status for', branch, '</b></u></p><p>'

  branch = branch.replace(" ", "%20")

  url =\
  'http://open.cdash.org/api/?method=build&task=sitetestfailures&project=ParaView&group='\
  + branch
  page = urllib.urlopen(url)
  data = page.readlines()
  if len(data[0]) == 2: #"[]"
    print "Cdash returned nothing useful as of this report."
    return
#    raise SystemExit

  submissions = eval(data[0])
  tfails = dict()
  if csvOutput or htmlOutput:
    print "-"*20, "ANALYZING", "-"*20
  elif wikiOutput:
    print "====Builds for " + dashDate + "===="
    print r'{| class="wikitable sortable" border="1" cellpadding="5" cellspacing="0"'
    print r'|-'
    print r'| Build Name'
    print r'| Failing'

  if htmlOutput:
    print '<br>'

  for skey in submissions.keys():
    submission = submissions[skey]
    bname = submission['buildname']
    bfails = submission['tests']
    if len(bfails) > 100:
      continue
    if csvOutput:
      print bname
      print len(bfails)
    elif wikiOutput:
      print r'|-'
      print r'| ',
      print r'[http://open.cdash.org/index.php?project=ParaView' + '&date=' + dashDate + r'&filtercount=1' + r'&field1=buildname/string&compare1=61&value1=' + bname + " " + bname + "]"
      print r'|'
      print len(bfails)
    elif htmlOutput:
      print '<a href=http://open.cdash.org/index.php?project=ParaView&date=' +\
        dashDate + '&filtercount=1' +\
        '&field1=buildname/string&compare1=61&value1=' + bname + '>' + bname +\
        '</a>'
      print len(bfails), '<br>'
    for tnum in range(0, len(bfails)):
      test = bfails[tnum]
      tname = test['name']
      if not tname in tfails:
         tfails[tname] = list()
      tfails[tname].append(bname)
  if wikiOutput:
    print r'|}'
  elif htmlOutput:
    print '<br>'

  if csvOutput:
    print "-"*20, "REPORT", "-"*20
    print len(tfails)," FAILURES"
  elif wikiOutput:
    print "====Tests for " + dashDate + "===="
    print r'{| class="wikitable sortable" border="1" cellpadding="5" cellspacing="0"'
    print r'|-'
    print r'| Test'
    print r'| Failing'
    print r'| Platforms'

  if htmlOutput:
    print "-"*20, "REPORT", "-"*20,"<br>"
    print len(tfails)," FAILURES<br>"

  failcounts = map(lambda x: (x,len(tfails[x])), tfails.keys())
  sortedfails = sorted(failcounts, key=lambda fail: fail[1])
  for test in sortedfails:
    tname = test[0]
    if csvOutput:
      print tname, ",", len(tfails[tname]), ",", tfails[tname]
    elif wikiOutput:
      print r'|-'
      print r'| '
      print r'[http://open.cdash.org/testSummary.php?' + r'project=9' + r'&date=' + dashDate + r'&name=' + tname + ' ' + tname + ']'
      print r'|',
      print len(tfails[tname])
      print r'|',
      print tfails[tname]
    elif htmlOutput:
      print '<a href=http://open.cdash.org/testSummary.php?' + 'project=9' +\
        '&date=' + dashDate + '&name=' + tname + '>' + tname + '</a>', ',',\
        len(tfails[tname]), ',', tfails[tname], '<br>'

  if wikiOutput:
    print r'|}'
  elif htmlOutput:
    print '</p>'

def PrintSubmitterIssues(dashDate):
  global csvOutput, wikiOutput, htmlOutput

  if csvOutput:
    print ""
    print "-"*10, "Issues with expected submitters", "-"*10
  elif wikiOutput:
    print "===Issues with submitters==="
    print r'{| class="wikitable sortable" border="1" cellpadding="5" cellspacing="0"'
    print r'|-'
    print r'| Submitter'
    print r'| Build Name'
    print r'| Branch'
  elif htmlOutput:
    print "<p><u><b>Issues with submitters</b></u></p>"
#    print "<table border=\"1\" style=\"width:500px\">"

  url = "http://open.cdash.org/index.php?project=ParaView"
  url = url + "&date=" + dashDate
  url = url + "&filtercount=1&field1=buildname/string&compare1=61&value1="

### To add more expected submissions, simply append to the following list
  expectedSubmissions =\
      {
        "Blight.kitware" :\
          [\
            "ubuntu-x64-nightlymaster",\
            "ubuntu-x64-nightlymaster-static",\
            "ubuntu-x64-coverage",\
            "ubuntu-x64-nightlynext",\
            "ubuntu-x64-nightlynext-static",\
            "ubuntu-x64-nightlynext-nogui"\
          ],\
        "amber12.kitware" :\
          [\
            "Master-Win64-vs9-shared-release",
            "Master-Win64-vs9-static-release",\
            "Win32-vs9-shared-release",\
            "Win64-vs9-shared-debug-nocollab"\
          ],\
        "miranda.kitware":\
          [\
            "Win7x64-ninja-nightlymaster-static-release",\
            "Win7x64-vs9-nightlymaster-shared-release"\
          ],\
        "tarvalon.kitware":\
          [\
            "Win7x64-vs10-shared-release",\
            "Win7x64-vs10-static-release",\
            "Win7x64-vs10-shared-release",\
            "Win7x64-vs10-static-release"\
          ],\
        "amber8.kitware":\
          [\
            "ubuntu-x64-nightlymaster",\
            "ubuntu-x64-nightlynext"\
          ],\
        "KargadMac.kitware":\
          [\
            "Lion-gcc-pvVTK"\
          ],\
        "kamino.kitware":\
          [\
            "Mac10.7.5-gcc-nightlymaster-release",\
            "Mac10.7.5-clang-nightlymaster-release"\
          ]
      }

  for submitter in expectedSubmissions:
    for build in expectedSubmissions[submitter]:
      searchUrl = url + build
      page = urllib.urlopen(searchUrl)
      data = page.readlines()
      if not any(submitter.upper() in s.upper() for s in data):
        if csvOutput:
          print submitter, ",",  build
        elif wikiOutput:
          print r'|-'
          print r'| '
          print submitter
          print r'| '
          print build
        elif htmlOutput:
          print "<b>",submitter, "</b>", ",", "<i>", build, "</i>", "<br>"
#          print "<tr><td>" + submitter + "</td>"
#          print "<td>" + build + "</td>"
#          print "<td>" + branch.replace("%20", " ") + "</td></tr>"

  if wikiOutput:
    print r'|}'
#  elif htmlOutput:
#    print "</table>"

#-----------------------------------------------------------------------------
def analyze():
  global csvOutput, wikiOutput, htmlOutput

  dt = datetime.date.today()
  dashDate = str(dt)

  # Date for printing purposes
  dty = dt - datetime.timedelta(1)
  dashDay = str(dty.strftime("%A"))
  dashMon = str(dty.strftime("%B"))
  dashYear = str(dty.strftime("%Y"))
  dashDt = str(dty.strftime("%d"))

  if csvOutput:
    print "="*10, "Dashboard for " + dashDay + ',', dashMon, dashDt, dashYear,\
      "="*10
  elif wikiOutput:
    print "==Dashboard for " + dashDate + "=="
  elif htmlOutput:
    print '<html>'
    print '<body>'
    print '<p>Dashboard for <b>' + dashDay + ',', dashMon, dashDt, dashYear, '</b></p>'

  PrintReport(dashDate, "Nightly (master)")
  PrintReport(dashDate, "Nightly (next)")

  PrintSubmitterIssues(dashDate.replace("-",""))
  if htmlOutput:
    print '</body>'
    print '</html>'

#-----------------------------------------------------------------------------
if __name__ == "__main__":
  parser = argparse.ArgumentParser(description="Analyze last night\'s"\
      " dashboard  results")
  parser.add_argument("--wiki", action="store_true", default=False,
      help="Wiki output")
  parser.add_argument("--html", action="store_true", default=False,
      help="HTML output")
  args = parser.parse_args()

  csvOutput = wikiOutput = htmlOutput = False

  if args.wiki:
    wikiOutput = True
  elif args.html:
    htmlOutput = True
  else:
    csvOutput = True

  analyze()

# vim: shiftwidth=2 softtabstop=2 expandtab
