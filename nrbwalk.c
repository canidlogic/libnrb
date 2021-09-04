/*
 * nrbwalk.c
 * =========
 * 
 * Walk through a NoiR Binary (NRB) file, verify it, and optionally
 * print a textual description of its data.
 * 
 * Syntax
 * ------
 * 
 *   nrbwalk
 *   nrbwalk -check
 * 
 * Both invocations read an NRB file from standard input and verify it.
 * 
 * The "-check" invocation does nothing beyond verifying the NRB file.
 * 
 * The parameter-less invocation also prints out a textual description
 * of the data within the NRB file to standard output.
 * 
 * File format
 * -----------
 * 
 * See nrb_spec.md for the format of the input NRB file.
 * 
 * Compilation
 * -----------
 * 
 * Compile with the libnrb.
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nrb.h"

/*
 * Local functions
 * ===============
 */

/* Prototypes */
static void report(NRB_DATA *pd, FILE *po);

/*
 * Print a textual representation of the given parsed data object to the
 * given file.
 * 
 * Parameters:
 * 
 *   pd - the parsed data object
 * 
 *   po - the output file
 */
static void report(NRB_DATA *pd, FILE *po) {
  
  int32_t x = 0;
  int32_t scount = 0;
  int32_t ncount = 0;
  NRB_NOTE n;
  
  /* Initialize structure */
  memset(&n, 0, sizeof(NRB_NOTE));
  
  /* Check parameter */
  if ((pd == NULL) || (po == NULL)) {
    abort();
  }
  
  /* Get section count and note count */
  scount = nrb_sections(pd);
  ncount = nrb_notes(pd);
  
  /* Print the section and note counts */
  fprintf(po, "SECTIONS: %ld\n", (long) scount);
  fprintf(po, "NOTES   : %ld\n", (long) ncount);
  fprintf(po, "\n");
  
  /* Print each section location */
  for(x = 0; x < scount; x++) {
    fprintf(po, "SECTION %ld AT %lld\n",
              (long) x,
              (long long) nrb_offset(pd, x));
  }
  fprintf(po, "\n");
  
  /* Print each note */
  for(x = 0; x < ncount; x++) {
    
    /* Get the note */
    nrb_get(pd, x, &n);
    
    /* Print the information */
    fprintf(po,
      "NOTE T=%lld DUR=%lld Pi=%d Pd=%d Gr=%d A=%d R=%ld S=%ld L=%ld\n",
            (long long) n.start,
            (long long) (n.release - n.start),
            (int) n.pitch,
            (int) ((n.art & 0x80) >> 7),
            (int) ((n.art & 0x40) >> 6),
            (int) n.art,
            (long) n.ramp,
            (long) n.sect,
            ((long) n.layer_i) + 1);
  }
}

/*
 * Program entrypoint
 * ==================
 */

int main(int argc, char *argv[]) {
  
  int status = 1;
  const char *pModule = NULL;
  int silent = 0;
  int ver = 0;
  NRB_DATA *pd = NULL;
  
  /* Get module name */
  if (argc > 0) {
    if (argv != NULL) {
      if (argv[0] != NULL) {
        pModule = argv[0];
      }
    }
  }
  if (pModule == NULL) {
    pModule = "nrbwalk";
  }
  
  /* There may be at most one argument beyond the module name */
  if (argc > 2) {
    fprintf(stderr, "%s: Too many arguments!\n", pModule);
    status = 0;
  }
  
  /* If there is one argument, it must be "-check", and set the silent
   * flag in that case */
  if (status && (argc == 2)) {
    if (argv == NULL) {
      abort();
    }
    if (argv[0] == NULL) {
      abort();
    }
    if (strcmp(argv[1], "-check") != 0) {
      fprintf(stderr, "%s: Unrecognized argument!\n", pModule);
      status = 0;
    }
    if (status) {
      silent = 1;
    }
  }
  
  /* Parse standard input as an NRB file */
  if (status) {
    pd = nrb_parse(stdin, &ver);
    if (ver != NRB_VER_OK) {
      if (ver == NRB_VER_MINOR) {
        fprintf(stderr, "%s: WARNING: Unsupported minor NRB version!\n",
                  pModule);
        
      } else if (ver == NRB_VER_MAJOR) {
        fprintf(stderr, "%s: ERROR: Unsupported major NRB version!\n",
                  pModule);
        
      } else {
        fprintf(stderr, "%s: Couldn't read valid NRB version!\n",
                  pModule);
      }
    }
    
    if (pd == NULL) {
      fprintf(stderr, "%s: A valid NRB file could not be read!\n",
                pModule);
      status = 0;
    }
  }
  
  /* If not silent, then report the contents */
  if (status && (!silent)) {
    report(pd, stdout);
  }
  
  /* Free parsed data object if allocated */
  nrb_free(pd);
  pd = NULL;
  
  /* Invert status and return */
  if (status) {
    status = 0;
  } else {
    status = 1;
  }
  return status;
}
