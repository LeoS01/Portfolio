HELPMSG = """
Merges a directory into an engine-ready .p_assets archive.
""".strip()

from pathlib import Path
from typing import List
import argparse
import sys
from P_Build import Files, Log, Config

#Config
MAX_ARCHIVE_BYTES = 1024 * 1024    #max. 1MB size per Archive

def IsDirArchivable(inputDir:str, recursive: bool, illegalSuffixes:list[str] = []) -> bool:
    #Loop
    globFunc = Path(inputDir).rglob if recursive else Path(inputDir).glob
    for file in globFunc("*"):
        #Validate
        if Files.SUFFIX_ARCHIVE in file.name:
            continue

        if file.is_dir():
            continue

        if file.suffix in illegalSuffixes:
            continue

        return True
    
    #Finalize
    return False

#Returns actually written output-file as string ("" on failure)
def WriteAArchive(inputDir:str, recursive:bool, verbose:bool, illegalSuffixes:list[str] = []) -> str:
    #Init
    Log(False).Head( f"assets to {Files.SUFFIX_ARCHIVE}" )

    outDir = Config.outputDirectory + "\\assets"
    Files.EnsurePathExists(outDir)

    #Utils
    class ArchiveFile:
        def __init__(self, name_AsciiNullTerm: bytes, content: bytes):
            #Utils
            def ENCODE(byteInput: bytes) -> bytes:

                #(Conventiently left out)

                return byteInput
    
            #Init
            self.nameSize = len(name_AsciiNullTerm)
            self.name_AsciiNullTerm = ENCODE(name_AsciiNullTerm)
            self.contentSize = len(content)
            self.content = ENCODE(content)
    
    #Alloc
    log = Log(verbose)
    filePaths: List[Path] = []
    fileSet: List[ArchiveFile] = []
    fileArchive: bytearray = bytearray([])
    finalOutput:str = outDir + "\\" + Path(inputDir).name + Files.SUFFIX_ARCHIVE        

    #Validate directory
    Files.EnsurePathExists(outDir) 

    #No validation via method above, as we usually want to print empty directories

#region READ ALL PATHS
    log.VMessage(f"Reading '{inputDir}'...")

    #Get all files
    #Loop
    globFunc = Path(inputDir).rglob if recursive else Path(inputDir).glob
    for file in globFunc("*"):
        #Validate
        if Files.SUFFIX_ARCHIVE in file.name:
            log.VMessage(f"Skipping {file.name} as it seems to be an archive!")
            continue

        if file.is_dir():
            log.VMessage(f"Skipping {file.name} as it seems to be a folder!")
            continue

        if file.suffix in illegalSuffixes:
            log.VMessage(f"Skipping, as {file.name} has illegal suffix!")
            continue

        #Add
        filePaths.append( str(file.resolve()) )
        log.VMessage(f"found file: {file.name}")

    if len(filePaths) <= 0:
        log.Error(f"It seems like no files where found using {inputDir}")
        raise Exception("Empty input directory")
#endregion

#region CHECK NECESSITY
    log.VMessage("\nValidating...")

    #Returns at end or breaks earlier, if a remake is required
    while True:
        oPath = Path(finalOutput)
        
        if not Files.IsOutputUpToDate(finalOutput, filePaths):
            log.VMessage("Output not up to date, rebuilding!")
            break

        #Invalid size?
        expectedSize:int = 0
        for file in filePaths:
            fPath = Path(file)
            expectedSize += fPath.stat().st_size + len(fPath.name) + 4 + 4

        realOSize: int = oPath.stat().st_size
        if realOSize != expectedSize:
            log.VMessage(f"Archive bytes: {realOSize}/{expectedSize}")
            log.Info("Rebuilding archive, as it does not have the expected size!")
            break

        #Cancel
        log.Info(f"Aborting, as '{Path(finalOutput).name}' is up to date!")
        return oPath.name 
#endregion

#region PATHS -> ARCHFILES
    log.VMessage("\nPreparing files for archive-insertion...")

    #Loop through all
    fullSize:int = 0
    for file in filePaths:
        fPath = Path(file)
        if fPath.is_dir():
            continue

        with open(file, "rb") as file:
            content: bytes = file.read()
            fileSet.append( ArchiveFile( fPath.name.encode("ascii"), content ))
            fullSize += len(content) + len(fPath.name) + 4 + 4

        if fullSize > MAX_ARCHIVE_BYTES:
            log.Error(f"ERROR: Archive cannot exceed {MAX_ARCHIVE_BYTES}x bytes. Current size: {fullSize}!")
            raise Exception("Invalid output size")
            


    #Validate
    if len(fileSet) == 0:
        log.Error("No valid files found!")
        raise Exception("No valid files found")
    
    #Fin
    fileTerm:str = "files" if len(fileSet) != 1 else "file"
    log.VMessage(f"Found {len(fileSet)}x {fileTerm}!")
#endregion
    
#region ARCHFILES -> ARCHIVE
    log.VMessage(f"\n{len(fileSet)} files to merge as asset-archive...")

    #Alloc
    int32Size = 4
    fileCount: int = len(fileSet)

    #Validate in
    if fileCount <= 0:
        log.Error("No files given to create archive from!")
        raise Exception("Empty archive fileset")

    #Run
    for aFile in fileSet:
        fileArchive.extend(aFile.nameSize.to_bytes(int32Size, "little"))
        fileArchive.extend(aFile.name_AsciiNullTerm)

        fileArchive.extend(aFile.contentSize.to_bytes(int32Size, "little"))
        fileArchive.extend(aFile.content)

        log.VMessage(f"Merged a file with {aFile.nameSize} name-bytes and {aFile.contentSize} content-bytes!")

    #Validate out
    if len(fileArchive) <= 0:
        log.Error("Seems like no bytes where actually archived")
        raise Exception("Invalid conversion")
#endregion

#region WRITE ARCHIVE   
    log.VMessage(f"\nWriting archive to disk...")
 
    #Alloc
    byteCount: int = len(fileArchive)

    #Validate
    if byteCount > MAX_ARCHIVE_BYTES or byteCount <= 0:
        log.Error(f"Error: Archive size invalid! {len(fileArchive)}/{MAX_ARCHIVE_BYTES} bytes!")
        raise Exception("Invalid conversion size")
    
    Files.EnsurePathExists( str(Path(finalOutput).parent.resolve()) )

    #Do
    log.VMessage(f"Saving {byteCount}x bytes as {Path(finalOutput).name}")
    if Path(finalOutput).exists():
        log.VMessage("output already exists, replacing with new!")

        #Rewrite old file
        tempPath = Path(finalOutput + ".tmp")
        with open(tempPath, "wb") as file:
            file.write(fileArchive)

        tempPath.replace(finalOutput)
    else:
        #Create new
        with open(finalOutput, "wb") as file:
            file.write(fileArchive)
      
#endregion

    #Fin
    log.Success(f"Wrote {Path(finalOutput).name} to disk successfully!")
    return finalOutput

#Main
if __name__ == "__main__":

    #Config args
    argprs = argparse.ArgumentParser(
        prog="AArchive",
        description=HELPMSG
    )

    argprs.add_argument("-r", "--recursive", action="store_true", help="Search INPUT recursively")
    argprs.add_argument("-v", "--verbose", action="store_true", help="Log detailed output")
    argprs.add_argument("-i", "--input", action="store", help="Set the input DIRECTORY")
    argprs.add_argument("-o", "--output", action="store", help="Set the output DIRECTORY")

    #Get args
    args = argprs.parse_args(sys.argv[1:])
    recursive: bool = args.recursive
    inputDir: str = args.input if (args.input != None) else "."
    outDir: str = args.output if (args.output != None) else "."
    log = Log(args.verbose)

    #Print config (verbose only)
    log.VMessage(f"Current dir: {Path.cwd()}")
    log.VMessage("using input directory (local): " + inputDir)
    log.VMessage("using output directory: " + outDir)
    log.VMessage("getting files recursively: " + ("yes" if recursive else "no"))

    #Do
    Config.SetOutputDirectory(outDir)
    print(WriteAArchive(inputDir, recursive, args.verbose))