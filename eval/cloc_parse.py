#!/usr/bin/env python

import subprocess
import sys
import re
from collections import defaultdict

name = sys.argv[1] if len(sys.argv) >= 2 else "<No Name>"

state = 0
files = []
loc = defaultdict(int)

for line in sys.stdin:
  line = line.rstrip()

  # State 0: Looking for breakdown by file
  if state == 0:
    m = re.match('^File\s+blank\s+comment\s+code$', line)
    if m:
      next(sys.stdin)
      state = 1
  # State 1: Keeping track of all files processed by cloc
  elif state == 1:
    m = re.match('^-+$', line)
    if m:
      state = 2
    else:
      m = re.match('^(.+)\s+\d+\s+\d+\s+\d+$', line)
      if m:
        files.append(m.groups()[0].rstrip())
      else:
        print 'Error: Could not parse line "{}"'.format(line)
  # State 2: Looking for breakdown by language
  elif state == 2:
    m = re.match('^Language\s+files\s+blank\s+comment\s+code$', line)
    if m:
      next(sys.stdin)
      state = 3
  # State 3: Keeping track of per-language code totals
  elif state == 3:
    m = re.match('^(.+)\s+\d+\s+\d+\s+\d+\s+(\d+)$', line)
    if m:
      g = m.groups()
      loc[g[0].rstrip()] = int(g[1])

locStmt = 0
for f in files:
  with open(f, 'r') as fd:
    locStmt += fd.read().count(';')

print("{}: {} {} {} {} {}".format(name, loc["C"], loc["C/C++ Header"], loc["Assembly"], loc["SUM:"], locStmt))


