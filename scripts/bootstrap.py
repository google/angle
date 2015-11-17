# Copyright 2015 Google Inc.  All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generate .gclient file for Angle.

Because gclient won't accept "--name ." use a different name then edit.
"""

import subprocess
import sys


def main():
  gclient_cmd = [
      'gclient', 'config',
      '--name', 'change2dot',
      '--unmanaged',
      'https://chromium.googlesource.com/angle/angle.git'
  ]
  cmd_str = ' '.join(gclient_cmd)
  try:
    rc = subprocess.call(gclient_cmd)
  except OSError:
    print 'could not run "%s" - is gclient installed?' % cmd_str
    sys.exit(1)

  if rc:
    print 'failed command: "%s"' % cmd_str
    sys.exit(1)

  with open('.gclient') as gclient_file:
    content = gclient_file.read()

  with open('.gclient', 'w') as gclient_file:
    gclient_file.write(content.replace('change2dot', '.'))

  print 'created .gclient'

if __name__ == '__main__':
  main()
