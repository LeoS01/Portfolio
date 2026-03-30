HELPMSG = """
Execute an engine command (python script)
Or run any arbitrary command (will be executed on a seperate thread and piped to the terminal)
Usage: P_Ex <Target> [args]
""".strip()

import sys
from pathlib import Path
import subprocess
import threading

#region Dependencies
try:
    import colorama
    colorama.init()
except Exception as e:
    dependencies = ["colorama"]
    for d in dependencies:
        print(d)
    print("Error: failed to import these dependencies. Do you wish to install them (Y/N)?")
    rslt:str = input()

    if "n" in rslt or "N" in rslt:
        print("Not installing them!")
        exit(0)
    elif "y" in rslt or "Y" in rslt:
        for d in dependencies:
            subprocess.run(f"pip install {d}")
        print("Done!")
        exit(0)
    else:
        print("Unknown answer: " + rslt)
        exit(-1)
#endregion

#region Utils
#https://gist.github.com/JBlond/2fea43a3049b38287e5e9cefc87b2124
ANSI_RED    =   "\x1b[0;31m"
ANSI_GREEN  =   "\x1b[0;32m"
ANSI_YELLOW =   "\x1b[0;33m"
ANSI_PURPLE =   "\x1b[0;35m"
ANSI_CYAN   =   "\x1b[0;36m"
ANSI_WHITE   =  "\x1b[0;37m"

IGNORE_FILES = ["__init__.py"]

def LogError(msg):
    print(ANSI_RED + msg + ANSI_WHITE, file=sys.stderr)

def LogInfo(msg):
    print(ANSI_CYAN + msg + ANSI_WHITE, file=sys.stderr)

def LogHead(msg):
    print(ANSI_PURPLE + "\n" + msg + ANSI_WHITE, file=sys.stderr)

def LogHelpCheck(msg):
    if "-h" or "-help" in sys.argv[1:]:
        LogInfo(msg)
#endregion


def RunParrallel(command: str) -> int:
    LogHead(f"Running '{Path(command).stem}'")

    #Vars
    LINE_CAPACITY = 1000
    lineBuffer: list[str] = []
    processActive: bool = True

    #Base
    def AsyncStdOut(stdOut):
        lineCounter = 0
        threadLock = threading.Lock()
        try:
            for line in iter(stdOut.readline, ''):
                if not processActive:
                    return
                
                #Get
                ln = line.strip()
                print(ln)

                #Save
                threadLock.acquire()
                if len(lineBuffer) < LINE_CAPACITY:
                    lineBuffer.append(ln)
                else:
                    lineBuffer[ lineCounter % LINE_CAPACITY] = ln

                lineCounter += 1
                threadLock.release()
        except Exception as e:
            print(str(e))

    def AsyncStdIn(stdIn):
        try:
            while processActive:
                inpt = input()
                stdIn.write(inpt + "\n")
                stdIn.flush()
        except Exception as e:
            pass

    #Open Process
    process = subprocess.Popen(command, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

    #Create Threads
    stdInthread = threading.Thread(target=AsyncStdIn, args=(process.stdin, ))
    stdInthread.daemon = True
    stdInthread.start()

    stdOutThread = threading.Thread(target=AsyncStdOut, args=(process.stdout, ))
    stdOutThread.daemon = True
    stdOutThread.start()

    #Run
    try:
        process.wait()
    except KeyboardInterrupt:
        LogInfo("\nKeyboard interrupt")
        
    except Exception as e:
        LogError(str(e))

    processActive = False
    
    #Close
    try:
        stdInthread.join()
        stdOutThread.join()
    except KeyboardInterrupt:
        LogInfo("\nKeyboard interrupt")
        
    except Exception as e:
        LogError(str(e))

    #Search
    try:
        lineLen: int = len(lineBuffer)
        LogInfo(f"Got {lineLen} lines, max capacity: {LINE_CAPACITY}")
        if lineLen >= LINE_CAPACITY:
            LogInfo("(Buffer starts looping on capacity overshoot)")

        while True:
            pattern = input("\nPattern to search for (keyboard intterupt to close): ")
            for line in lineBuffer:
                if pattern in line:
                    print(line)

    except KeyboardInterrupt:
        LogInfo("\nKeyboard interrupt")
        
    except Exception as e:
        LogError(str(e))

    return process.returncode

#Note: May have some redundancy~ but this phrasing is clean and the difference in performance should be negligable
if __name__ == "__main__":
    #Alloc
    stdin = None if sys.stdin.isatty() else str(sys.stdin.read().replace("\n", " "))
    thisFilePath = Path(__file__)
    thisDirectory: str = str( thisFilePath.resolve().parent )
    justLogHelp:bool = len(sys.argv) <= 1

    #Get engine candidates
    engineCandidates: list[str] = []
    if justLogHelp:
        LogInfo(HELPMSG + "\navailable commands:")
    for file in Path(thisDirectory).rglob("*.py"):
        if file.is_dir():
            continue
        fName:str = file.stem.lower()

        #validate
        if fName not in IGNORE_FILES and fName != thisFilePath.stem:
            engineCandidates.append(file.resolve())
            if justLogHelp:
                LogInfo(file.stem)
    if justLogHelp:
        exit(0)


    #Engine CLI?
    target: str = stdin if len(sys.argv) < 1 else sys.argv[1]

    for file in engineCandidates:
        fPath: Path = Path(file)
        fName:str = fPath.stem.lower()

        #Run?
        if fName in target.lower() and fName not in IGNORE_FILES and fName != thisFilePath.stem:
            args:str = " ".join(sys.argv[2:])
            LogHead(f"Running engine-cmd '{target}'")
            exit( subprocess.run(f'python "{file.resolve()}" {args}').returncode )
    
    #Arbitrary Command?
    exit( RunParrallel(target) )