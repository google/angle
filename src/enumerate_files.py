import fnmatch
import os
import sys

rootdirs = [ ]
filetypes = [ ]

foundTypesArg = False
for i in range(1, len(sys.argv)):
    arg = sys.argv[i]
    if arg == "-types":
        foundTypesArg = True
        continue
    
    if foundTypesArg:
        filetypes.append(arg)
    else:
        rootdirs.append(arg)

for rootdir in rootdirs:
    for root, dirnames, filenames in os.walk(rootdir):
        for file in filenames:
            for type in filetypes:
                if fnmatch.fnmatchcase(file, type):
                    print os.path.join(root, file).replace("\\", "/")
                    break
