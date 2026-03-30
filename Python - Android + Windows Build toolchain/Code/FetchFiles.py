HELPMSG = """
returns/outputs all files that are newer than the comparison file OR include a file that is newer than the comparison file
TLDR: Returns all files that need to be 'recompiled'
""".strip()

from pathlib import Path
import os
import sys
import argparse
import copy
from P_Build import Log, Files, Config

#Check if any header is in sourceFil
def IsAnyIncludedIn(sourceFile:str, headerSet:list[str]) -> bool:
    #Alloc
    src: Path = Path(sourceFile)

    #Loop
    for header in headerSet:
        #Contains?
        hdr = Path(header)
        if not hdr.exists():
            log.Error(f"Error: Asking to read inexsitent file: {hdr.resolve()}")
            raise Exception("Cannot read: " + header)

        if hdr.name not in src.read_text():
            continue

        #Self-include?
        if header.name == src.name:
            continue

        #Found
        return True

    #Not found
    return False

#Add all files that contain an item from includeTarget recursively. Returns includeTarget but with the found chain.
def SolveIncludeChainFor(files:list[str], includeTarget:list[str]) -> list[str]:
    #Alloc
    openList = copy.copy(files)
    newerFiles = copy.copy(includeTarget)

    #Recursively step through files
    equilibriumReached = False
    while not equilibriumReached:
        equilibriumReached = True

        for file in openList:
            if IsAnyIncludedIn(file, newerFiles):
                
                #Include-chain still not ended
                newerFiles.append(file)
                openList.remove(file)
                equilibriumReached = False

    return newerFiles


#Returns list of all files that need an update (recursive)
def FetchFiles(fileForDateComparison: str, searchPattern:str, includeSuffix:str = None, verbose: bool = False) -> list[str]:  
    #Init
    Log(False).Head("Fetching files...")
    log = Log(verbose)
    log.VMessage(f'Looping "{searchPattern}"')

    #Util
    def GetAffectedIncludesFrom(dir:str) -> list[str]:
        log.VMessage(f"\nSearching '{Path(dir).name}' for affected header-files, checking against '{Path(fileForDateComparison).name}'...")

        #Alloc
        fDir = Path(fileForDateComparison)
        fResolved:str = fDir.resolve()
        allIncludes: list[str] = []
        newFiles: list[str] = []

        #Get all includes
        for file in Path(dir).rglob("*" + includeSuffix):
            if file.is_dir():
                continue
            allIncludes.append(file.resolve())

        log.VMessage(f"Found {len(allIncludes)}x potential files!")

        #No further validation
        if not fDir.exists():
            log.VMessage(f'"{fileForDateComparison}" does not exist, returning all files')
            return allIncludes

        #Filter for newer files only
        for file in allIncludes:
            if os.path.getmtime(file) > os.path.getmtime(fResolved):
                newFiles.append(file)

        log.VMessage(f"Found {len(newFiles)}x newer files!")

        result:list[str] = SolveIncludeChainFor(allIncludes, newFiles)
        log.VMessage(f"solved for {len(result)}x affected files")
        return result

    #Alloc
    searchDir, searchFile = Files.SplitDirPattern(searchPattern)
    fdName: str = Path(fileForDateComparison).name
    targetFiles: list[str] = []
    affectedIncludes: list[str] = []

    #Get include chain
    if includeSuffix != None:
        affectedIncludes: list[str] = GetAffectedIncludesFrom(Config.GetEngineIncl()) + GetAffectedIncludesFrom(searchDir)
        affectedIncludes = SolveIncludeChainFor(list(Path(searchDir).rglob("*" + includeSuffix)), affectedIncludes)
        log.VMessage(f"\nfinally solved for {len(affectedIncludes)}x affected headers!")

    #Loop
    for file in Path(searchDir).rglob(searchFile):
        log.VMessage(f"\nChecking {file.name} ...")

        #Validate
        if file.is_dir():
            continue

        if Files.IsPathHidden(file.parent.resolve()):
            if verbose:
                log.Info(f"Warning: skipping file {file}, as it seems to be in a hidden folder.")
            continue

        #Get
        thisFileDir = str(file.resolve())

        #Output exists?
        if not Path(fileForDateComparison).exists():
            log.Success(f"Selecting '{file.name}', as comparison does not exist...")
            targetFiles.append(thisFileDir)
            continue
        else: log.VMessage(f"File '{fdName}' exists")

        #Is this file newer?
        if os.path.getmtime(thisFileDir) > os.path.getmtime(fileForDateComparison):
            log.Success(f"Selecting '{file.name}' - due to it being NEWER than the reference output '{fdName}'")
            targetFiles.append(thisFileDir)  
            continue
        else: log.VMessage(f"'{file.name}' is older than '{fdName}'")

        #Check
        if IsAnyIncludedIn(file, affectedIncludes) and includeSuffix != None:
            log.Success(f"Selecting '{file.name}' - due to it including an affected header")
            targetFiles.append(thisFileDir)  
            continue
        else: log.VMessage(f"'{file.name}' does not contain any affected includes")

        log.Success(f"no need to recompile '{file.name}'!")
            
    return targetFiles

#Main
if __name__ == "__main__":
    #Arg Config
    argprs = argparse.ArgumentParser(
        prog="FetchFiles",
        description=HELPMSG
    )

    argprs.add_argument("-v", "--verbose", action="store_true", help=+"Log detailed output")
    argprs.add_argument("-d", "--dateFile", action="store", help="File to compare date against")
    argprs.add_argument("-i", "--inputDir", action="store", help="Directory to input pattern (e.G. code/*.cpp)")
    argprs.add_argument("-icl", "--includeSuffix", action="store", help="Directory to include suffix (e.G. '.h')")

    #Get args
    args = argprs.parse_args(sys.argv[1:])
    verbose: bool = args.verbose
    inputPattern: str = str(args.inputDir) if (args.inputDir != None) else "*"
    includeSuffix: str = args.includeSuffix
    dateFile: str = str(args.dateFile)
    log = Log(verbose)

    #Log
    log.VHead("Arguments:")
    log.VMessage(f"verbose = {verbose}")
    log.VMessage(f"inputPattern = {inputPattern}")
    log.VMessage(f"includeSuffix = {includeSuffix}")
    log.VMessage(f"dateFile = {dateFile}")

    if not args.dateFile:
        log.Error("Please ensure its set with -d 'target.file'!")
        exit(-1)

    targetFiles: list[str] = FetchFiles(dateFile, inputPattern, includeSuffix, verbose)

    log.VMessage("\nfinal stdout")
    if len(targetFiles) <= 0:
        log.VMessage("(empty)")
    for file in targetFiles:
        print(f'"{file}"')