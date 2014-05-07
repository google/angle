import os
import re
import sys

def ReadFileAsLines(filename):
    """Reads a file, removing blank lines and lines that start with #"""
    file = open(filename, "r")
    raw_lines = file.readlines()
    file.close()
    lines = []
    for line in raw_lines:
        line = line.strip()
        if len(line) > 0 and not line.startswith("#"):
            lines.append(line)
    return lines

def GetSuiteName(testName):
    return testName[:testName.find("/")]

def GetTestName(testName):
    replacements = { ".test": "", ".": "_" }
    splitTestName = testName.split("/")
    cleanName = splitTestName[-2] + "_" + splitTestName[-1]
    for replaceKey in replacements:
        cleanName = cleanName.replace(replaceKey, replacements[replaceKey])
    return cleanName

def GenerateTests(outFile, testNames):
    # Remove duplicate tests
    testNames = list(set(testNames))

    outFile.write("#include \"gles_conformance_tests.h\"\n\n")

    for test in testNames:
        outFile.write("TEST(" + GetSuiteName(test) + ", " + GetTestName(test) + ")\n")
        outFile.write("{\n")
        outFile.write("    RunConformanceTest(\"" + test + "\", GetCurrentConfig());\n")
        outFile.write("}\n\n")

def GenerateTestList(sourceFile, rootDir):
    tests = [ ]
    fileName, fileExtension = os.path.splitext(sourceFile)
    if fileExtension == ".run":
        lines = ReadFileAsLines(sourceFile)
        for line in lines:
            tests += GenerateTestList(os.path.join(os.path.dirname(sourceFile), line), rootDir)
    elif fileExtension == ".test":
        tests.append(os.path.relpath(os.path.realpath(sourceFile), rootDir).replace("\\", "/"))
    return tests;

def main(argv):
    tests = GenerateTestList(argv[0], argv[1])
    tests.sort()

    output = open(argv[2], 'wb')
    GenerateTests(output, tests)
    output.close()

    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
