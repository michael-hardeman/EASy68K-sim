
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>

#ifdef WIN32
  #include <windows.h>
#else
  #include <unistd.h>
#endif

#include "def.h"
#include "proto.h"
#include "var.h"

//---------------------------------------------------------------------------
// Open program file
int loadProgramFromFile(char* name) {
  int addr;
  initSim();            // initialize simulator
  for (int i=0; i<MEMSIZE; i++)
    memory[i] = 0xFF;        // erase 68000 memory to $FF

  simhalt_on = true;            // default to SIMHALT enabled
  try {
    // load S-Record file
    if(loadSrec(name) == false) {  // if S-Record loads with no errors
      startPC = PC;               // save PC starting address for Reset
    }
  } catch(...) {
    return 1;
  }
  return 0;
}

void runLoop() {
  trace = false;
  sstep = false;
  runMode = true;
  try {
    while(runMode){
      runprog(); // run next 68000 instruction
    }
  }
  catch( ... ) {
    printf("Unexpected error in runLoop\n");
    exit(1);
  }
}


void help(void)
{
  fprintf(stderr, "asy68k cli 68000 simulator Version %#010x based on source code from http://www.easy68k.com/\n", VERSION);
  fprintf(stderr, "Distributed under the GNU General Public Use License. The software is not warranted in any way. Use at your own risk.\n");
  fprintf(stderr, "\nPorted to the *nix CLI so it can be used in a Makefile workflow w/o WINE by Michael Hardeman December 2024\n\n");
  fprintf(stderr, "Usage:\n   asy68ksim {options} file1.S68 {file2.S68} ... {fileN.S68}\n\n");
  fprintf(stderr, "(Options with \"default:\" are enabled, use --no-{option} to turn off, i.e. --no-list)\n");
  fprintf(stderr, "--print-registers       default: log registers to stdout after program is run\n");
  fprintf(stderr, "--{register}={value}             test register for the supplied value. exit with error code if test failed.\n");
  fprintf(stderr, "                                 Valid registers are D0-8, A0-9, PC, and SR\n");
  fprintf(stderr, "\n");
}

struct RegisterTest {
  bool enabled = false;
  int expected = 0;
};

// stolen from mainS.cpp
int main(int argc, char *argv[])
{
  bool printRegistersFlag = true; // prints the register contents to stdout
  RegisterTest D_Tests[D_REGS];
  RegisterTest A_Tests[A_REGS];
  RegisterTest PC_Test;
  RegisterTest SR_Test;
  bool testFailure = false;

  if (argc == 1)  {help(); exit(0);}

  for (int i=1; i<argc; i++)
  {
    if (strncmp(argv[i],"--print-registers",17)==0) {printRegistersFlag = true; continue;}
    if (strncmp(argv[i],"--no-print-registers",20)==0) {printRegistersFlag = false; continue;}

    if (strncmp(argv[i], "--D", 3) == 0) {
      char *equalsPos = strchr(argv[i], '=');
      if (equalsPos && equalsPos[1] >= '0' && equalsPos[1] <= '7') {
        int regIndex = argv[i][3] - '0';
        D_Tests[regIndex].enabled = true;
        D_Tests[regIndex].expected = atoi(equalsPos + 1);
        continue;
      } else {
        fprintf(stderr, "Invalid --D argument. Usage: --D<number>=<value> where <number> is 0-7.\n");
        exit(1);
      }
    }

    if (strncmp(argv[i], "--A", 3) == 0) {
      char *equalsPos = strchr(argv[i], '=');
      if (equalsPos && equalsPos[1] >= '0' && equalsPos[1] <= '7') {
        int regIndex = argv[i][3] - '0';
        A_Tests[regIndex].enabled = true;
        A_Tests[regIndex].expected = atoi(equalsPos + 1);
        continue;
      } else {
        fprintf(stderr, "Invalid --A argument. Usage: --A<number>=<value> where <number> is 0-7.\n");
        exit(1);
      }
    }

    if (strncmp(argv[i], "--PC=", 5) == 0) {
      char *equalsPos = strchr(argv[i], '=');
      PC_Test.enabled = true;
      PC_Test.expected = atoi(equalsPos + 1);
    }

    if (strncmp(argv[i], "--SR=", 5) == 0) {
      char *equalsPos = strchr(argv[i], '=');
      SR_Test.enabled = true;
      SR_Test.expected = atoi(equalsPos + 1);
    }

    if (argv[i][0]=='-') {help(); fprintf(stderr,"\n\nUnknown option \"%s\"",argv[i]); exit(1);}

    fprintf(stderr,"Simulating %s\n",argv[i]);
    memory = new char[MEMSIZE];      // reserve 68000 memory space
    if(loadProgramFromFile(argv[i])) {
      fprintf(stderr,"Unexpected error in OpenFile\n");
    } else {
      runLoop();

      if (printRegistersFlag) {
        for (int j=0; j<D_REGS; j++){
          printf("D%d=%d\n", j, D[j]);
        }

        for (int j=0; j<A_REGS; j++){
          printf("A%d=%d\n", j, A[j]);
        }

        printf("PC=%d\n", PC);
        printf("SR=%d\n", SR);
      }

      for (int j=0; j<D_REGS; j++ ) {
        if (D_Tests[j].enabled) {
          if (D[j] != D_Tests[j].expected) {
            fprintf(stderr,"D%d Test failed! Expected: %d Actual: %d\n", j, D_Tests[j].expected, D[j]);
            testFailure = true;
          }
        }
      }

      for (int j=0; j<A_REGS; j++ ) {
        if (A_Tests[j].enabled) {
          if (A[j] != A_Tests[j].expected) {
            fprintf(stderr,"A%d Test failed! Expected: %d Actual: %d\n", j, A_Tests[j].expected, A[j]);
            testFailure = true;
          }
        }
      }

      if (PC_Test.enabled) {
        if (PC != PC_Test.expected) {
          fprintf(stderr,"PC Test failed! Expected: %d Actual %d\n", PC_Test.expected, PC);
          testFailure = true;
        }
      }

      if (SR_Test.enabled) {
        if (SR != SR_Test.expected) {
          fprintf(stderr,"SR Test failed! Expected: %d Actual: %d\n", SR_Test.expected, SR);
          testFailure = true;
        }
      }
    }

    delete[] memory;

    if (testFailure) {
      exit(1);
    }
  }
}