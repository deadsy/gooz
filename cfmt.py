#!/usr/bin/python3
# Format the project source code per the standard (linux style)

import glob
import subprocess
import os

src_dirs = (
  'ggm/src/core',
  'ggm/src/inc',
  'ggm/src/module',
  'ggm/src/module/delay',
  'ggm/src/module/env',
  'ggm/src/module/filter',
  'ggm/src/module/midi',
  'ggm/src/module/mix',
  'ggm/src/module/osc',
  'ggm/src/module/pm',
  'ggm/src/module/root',
  'ggm/src/module/seq',
  'ggm/src/module/view',
  'ggm/src/module/voice',
  'ggm/src/os/linux',
  'ggm/src/os/zephyr',
  'files/zephyr/drivers/audio',
)

src_filter_out = (
)

indent_exec = '/usr/bin/indent'

def exec_cmd(cmd):
  """execute a command, return the output and return code"""
  output = ''
  rc = 0
  try:
    output = subprocess.check_output(cmd, shell=True)
  except subprocess.CalledProcessError as x:
    rc = x.returncode
  return output, rc

def get_files(dirs, filter_out):
  files = []
  for d in dirs:
    files.extend(glob.glob('%s/*.h' % d))
    files.extend(glob.glob('%s/*.c' % d))
  return [f for f in files if f not in filter_out]

def format(f):
  exec_cmd('%s -brf -linux -l10000 %s' % (indent_exec, f))
  os.unlink('%s~' % f)

def main():
  files = get_files(src_dirs, src_filter_out)
  for f in files:
    format(f)

main()


