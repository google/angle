import fnmatch
import os
import sys

dirs = [ ]
types = [ ]
excludes = [ ]
files = [ ]

# Default to accepting a list of directories first
curArray = dirs

# Iterate over the arguments and add them to the arrays
for i in range(1, len(sys.argv)):
    arg = sys.argv[i]

    if arg == "-dirs":
        curArray = dirs
        continue

    if arg == "-types":
        curArray = types
        continue

    if arg == "-excludes":
        curArray = excludes
        continue

    curArray.append(arg)

# If no directories were specified, use the current directory
if len(dirs) == 0:
    dirs.append(".")

# If no types were specified, accept all types
if len(types) == 0:
    types.append("*")

# Walk the directories listed and compare with type and exclude lists
for rootdir in dirs:
    for root, dirnames, filenames in os.walk(rootdir):
        for file in filenames:
            # Skip files that are "hidden"
            if file.startswith("."):
                continue;

            fullPath = os.path.join(root, file).replace("\\", "/")
            for type in types:
                if fnmatch.fnmatchcase(fullPath, type):
                    excluded = False
                    for exclude in excludes:
                        if fnmatch.fnmatchcase(fullPath, exclude):
                            excluded = True
                            break

                    if not excluded:
                        files.append(fullPath)
                        break

files.sort()
for file in files:
    print file
