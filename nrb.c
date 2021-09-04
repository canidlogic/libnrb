/*
 * nrb.c
 * =====
 * 
 * Implementation of nrb.h
 * 
 * See the header for further information.
 */

#include "nrb.h"
#include <stdlib.h>
#include <string.h>

/*
 * Constants
 * =========
 */

/*
 * Boundaries of the integer types used in the format.
 */
#define NRB_MAXUINT64 (UINT64_C(9223372036854775807))
#define NRB_MAXUINT32 (UINT32_C(2147483647))
#define NRB_MAXUINT16 (UINT16_C(65535))

#define NRB_MINBIAS8 (-128)
#define NRB_MAXBIAS8 (127)

/*
 * Biases used for the signed byte.
 */
#define NRB_BIAS8 (128)

/*
 * The signature values.
 */
#define NRB_SIGPRI (INT32_C(1928196216))
#define NRB_SIGSEC (INT32_C(778990178))

/*
 * Initial allocation capacities, in elements, of the section and note
 * tables.
 */
#define NRB_SECTALLOC_INIT (16)
#define NRB_NOTEALLOC_INIT (256)

/*
 * Type definitions
 * ================
 */

/*
 * NRB_DATA structure.
 * 
 * Prototype given in header.
 */
struct NRB_DATA_TAG {
  
  /*
   * The number of sections.
   * 
   * Must be in range [1, NRB_MAXSECT].
   */
  int32_t sect_count;
  
  /*
   * The capacity of the section table.
   * 
   * Must be in range [1, NRB_MAXSECT].
   */
  int32_t sect_cap;
  
  /*
   * The number of notes.
   * 
   * Must be in range [1, NRB_MAXNOTE].
   */
  int32_t note_count;
  
  /*
   * The capacity of the note table.
   * 
   * Must be in range [1, NRB_MAXNOTE].
   */
  int32_t note_cap;
  
  /*
   * Pointer to the section table.
   * 
   * This is a dynamically allocated array with a number of elements
   * given by sect_cap and the total elements used given by sect_count.
   * 
   * Each element is the offset in microseconds from the start of the
   * composition.
   * 
   * The first element must be zero, and all subsequent elements must
   * have an offset that is greater than or equal to the previous
   * element's offset.
   */
  int64_t *pSect;
  
  /*
   * Pointer to the note table.
   * 
   * This is a dynamically allocated array with a number of elements
   * given by note_cap and the total elements used given by note_count.
   * 
   * Start times of notes must not be less than the time offset of the
   * section that the note belongs to.
   */
  NRB_NOTE *pNote;
};

/*
 * Local functions
 * ===============
 */

/* Prototypes */
static void nrb_writeByte(FILE *pOut, int c);
static void nrb_writeUint64(FILE *pOut, uint64_t v);
static void nrb_writeUint32(FILE *pOut, uint32_t v);
static void nrb_writeUint16(FILE *pOut, uint16_t v);
static void nrb_writeBias8(FILE *pOut, int v);

static int nrb_readByte(FILE *pf);
static int nrb_read64(FILE *pf, uint64_t *pv);
static int nrb_read32(FILE *pf, uint32_t *pv);
static int nrb_read16(FILE *pf, uint16_t *pv);

static int64_t nrb_readUint64(FILE *pf);
static int32_t nrb_readUint32(FILE *pf);
static int32_t nrb_readUint16(FILE *pf);
static int nrb_readBias8(FILE *pf, int *pv);

static int nrb_cmp(const void *pva, const void *pvb);

/*
 * Write a byte to the output file.
 * 
 * pOut is the file to write to.  It must be open for writing or
 * undefined behavior occurs.
 * 
 * c is the unsigned byte value to write, in range [0, 255].
 * 
 * Parameters:
 * 
 *   pOut - the output file
 * 
 *   c - the unsigned byte value to write
 */
static void nrb_writeByte(FILE *pOut, int c) {
  
  /* Check parameters */
  if ((pOut == NULL) || (c < 0) || (c > 255)) {
    abort();
  }
  
  /* Write the byte */
  if (putc(c, pOut) != c) {
    abort();  /* I/O error */
  }
}

/*
 * Write an unsigned 32-bit integer value to the given file in big
 * endian order.
 * 
 * This is a wrapper around nrb_writeByte() which writes the integer
 * value as eight bytes with most significant byte first and least
 * significant byte last.
 * 
 * Parameters:
 * 
 *   pOut - output file to write to
 * 
 *   v - the value to write
 */
static void nrb_writeUint64(FILE *pOut, uint64_t v) {
  
  nrb_writeByte(pOut, (int) (v >> 56));
  nrb_writeByte(pOut, (int) ((v >> 48) & 0xff));
  nrb_writeByte(pOut, (int) ((v >> 40) & 0xff));
  nrb_writeByte(pOut, (int) ((v >> 32) & 0xff));
  nrb_writeByte(pOut, (int) ((v >> 24) & 0xff));
  nrb_writeByte(pOut, (int) ((v >> 16) & 0xff));
  nrb_writeByte(pOut, (int) ((v >> 8) & 0xff));
  nrb_writeByte(pOut, (int) (v & 0xff));
}

/*
 * Write an unsigned 32-bit integer value to the given file in big
 * endian order.
 * 
 * This is a wrapper around nrb_writeByte() which writes the integer
 * value as four bytes with most significant byte first and least
 * significant byte last.
 * 
 * Parameters:
 * 
 *   pOut - output file to write to
 * 
 *   v - the value to write
 */
static void nrb_writeUint32(FILE *pOut, uint32_t v) {
  
  nrb_writeByte(pOut, (int) (v >> 24));
  nrb_writeByte(pOut, (int) ((v >> 16) & 0xff));
  nrb_writeByte(pOut, (int) ((v >> 8) & 0xff));
  nrb_writeByte(pOut, (int) (v & 0xff));
}

/*
 * Write an unsigned 16-bit integer value to the given file in big
 * endian order.
 * 
 * This is a wrapper around nrb_writeByte() which writes the integer
 * value as two bytes with most significant byte first and least
 * significant byte second.
 * 
 * Parameters:
 * 
 *   pOut - output file to write to
 * 
 *   v - the value to write
 */
static void nrb_writeUint16(FILE *pOut, uint16_t v) {
  
  nrb_writeByte(pOut, (int) (v >> 8));
  nrb_writeByte(pOut, (int) (v & 0xff));
}

/*
 * Write a biased 8-bit signed integer value to the given file.
 * 
 * This is a wrapper around nrb_writeByte() which writes an unsigned
 * value that is the given signed value plus NRB_BIAS8.  This biased
 * value must be in unsigned 8-bit range or a fault occurs.
 * 
 * Parameters:
 * 
 *   pOut - output file to write to
 * 
 *   v - the value to write
 */
static void nrb_writeBias8(FILE *pOut, int v) {
  
  /* Check range */
  if ((v < NRB_MINBIAS8) || (v > NRB_MAXBIAS8)) {
    abort();
  }
  
  /* Compute raw/unsigned value */
  v += NRB_BIAS8;
  
  /* Pass through */
  nrb_writeByte(pOut, v);
}

/*
 * Read an unsigned byte value from the given file.
 * 
 * pf is the file to read from.  It must be open for reading or
 * undefined behavior occurs.
 * 
 * The return value is the byte read, in range 0-255.  -1 is returned if
 * there was an I/O error or the End Of File (EOF) was encountered.
 * 
 * Parameters:
 * 
 *   pf - the file to read from
 * 
 * Return:
 * 
 *   the unsigned byte value read, or -1 if error (or EOF)
 */
static int nrb_readByte(FILE *pf) {
  
  int c = 0;
  
  /* Check parameter */
  if (pf == NULL) {
    abort();
  }
  
  /* Read a character */
  c = getc(pf);
  if (c == EOF) {
    c = -1;
  }
  
  /* Return the byte value (or error) */
  return c;
}

/*
 * Read a raw 64-bit integer value from a file.
 * 
 * pf is the file to read from.  It must be open for reading or
 * undefined behavior occurs.
 * 
 * pv points to the variable to receive the raw 64-bit integer value.
 * The value is undefined if the function fails.
 * 
 * Eight bytes are read in big endian order, with the first byte read as
 * the most significant and the last byte read as the least significant.
 * 
 * Parameters:
 * 
 *   pf - the file to read from
 * 
 *   pv - the variable to receive the raw 64-bit integer value
 * 
 * Return:
 * 
 *   non-zero if successful, zero if error
 */
static int nrb_read64(FILE *pf, uint64_t *pv) {
  
  int i = 0;
  int status = 1;
  int c = 0;
  
  /* Check parameters */
  if ((pf == NULL) || (pv == NULL)) {
    abort();
  }
  
  /* Reset return value */
  *pv = 0;
  
  /* Read eight bytes */
  for(i = 0; i < 8; i++) {
    
    /* Read a byte */
    c = nrb_readByte(pf);
    if (c < 0) {
      status = 0;
    }
    
    /* Shift result and add in new byte */
    if (status) {
      *pv <<= 8;
      *pv |= (uint64_t) c;
    }
    
    /* Leave loop if error */
    if (!status) {
      break;
    }
  }
  
  /* Return status */
  return status;
}

/*
 * Read a raw 32-bit integer value from a file.
 * 
 * pf is the file to read from.  It must be open for reading or
 * undefined behavior occurs.
 * 
 * pv points to the variable to receive the raw 32-bit integer value.
 * The value is undefined if the function fails.
 * 
 * Four bytes are read in big endian order, with the first byte read as
 * the most significant and the last byte read as the least significant.
 * 
 * Parameters:
 * 
 *   pf - the file to read from
 * 
 *   pv - the variable to receive the raw 32-bit integer value
 * 
 * Return:
 * 
 *   non-zero if successful, zero if error
 */
static int nrb_read32(FILE *pf, uint32_t *pv) {
  
  int i = 0;
  int status = 1;
  int c = 0;
  
  /* Check parameters */
  if ((pf == NULL) || (pv == NULL)) {
    abort();
  }
  
  /* Reset return value */
  *pv = 0;
  
  /* Read four bytes */
  for(i = 0; i < 4; i++) {
    
    /* Read a byte */
    c = nrb_readByte(pf);
    if (c < 0) {
      status = 0;
    }
    
    /* Shift result and add in new byte */
    if (status) {
      *pv <<= 8;
      *pv |= (uint32_t) c;
    }
    
    /* Leave loop if error */
    if (!status) {
      break;
    }
  }
  
  /* Return status */
  return status;
}

/*
 * Read a raw 16-bit integer value from a file.
 * 
 * pf is the file to read from.  It must be open for reading or
 * undefined behavior occurs.
 * 
 * pv points to the variable to receive the raw 16-bit integer value.
 * The value is undefined if the function fails.
 * 
 * Two bytes are read in big endian order, with the first byte read as
 * the most significant and the second byte read as the least
 * significant.
 * 
 * Parameters:
 * 
 *   pf - the file to read from
 * 
 *   pv - the variable to receive the raw 16-bit integer value
 * 
 * Return:
 * 
 *   non-zero if successful, zero if error
 */
static int nrb_read16(FILE *pf, uint16_t *pv) {
  
  int i = 0;
  int status = 1;
  int c = 0;
  
  /* Check parameters */
  if ((pf == NULL) || (pv == NULL)) {
    abort();
  }
  
  /* Reset return value */
  *pv = 0;
  
  /* Read two bytes */
  for(i = 0; i < 2; i++) {
    
    /* Read a byte */
    c = nrb_readByte(pf);
    if (c < 0) {
      status = 0;
    }
    
    /* Shift result and add in new byte */
    if (status) {
      *pv <<= 8;
      *pv |= (uint16_t) c;
    }
    
    /* Leave loop if error */
    if (!status) {
      break;
    }
  }
  
  /* Return status */
  return status;
}

/*
 * Read an unsigned 64-bit integer value from a file.
 * 
 * pf is the file to read from.  It must be open for reading or
 * undefined behavior occurs.
 * 
 * A raw 64-bit value is read in big endian order from the file.  The
 * numeric value must be in range [0, NRB_MAXUINT64].  The raw numeric
 * value is then returned as-is.
 * 
 * Parameters:
 * 
 *   pf - the file to read
 * 
 * Return:
 * 
 *   the integer that was read, or -1 if there was an error
 */
static int64_t nrb_readUint64(FILE *pf) {
  
  int status = 1;
  uint64_t rv = 0;
  int64_t retval = 0;
  
  /* Check parameter */
  if (pf == NULL) {
    abort();
  }
  
  /* Read a raw 64-bit value */
  if (!nrb_read64(pf, &rv)) {
    status = 0;
  }
  
  /* Range-check value */
  if (status) {
    if (rv > NRB_MAXUINT64) {
      status = 0;
    }
  }
  
  /* Determine return value */
  if (status) {
    retval = (int64_t) rv;
  } else {
    retval = -1;
  }
  
  /* Return value */
  return retval;
}

/*
 * Read an unsigned 32-bit integer value from a file.
 * 
 * pf is the file to read from.  It must be open for reading or
 * undefined behavior occurs.
 * 
 * A raw 32-bit value is read in big endian order from the file.  The
 * numeric value must be in range [0, NRB_MAXUINT32].  The raw numeric
 * value is then returned as-is.
 * 
 * Parameters:
 * 
 *   pf - the file to read
 * 
 * Return:
 * 
 *   the integer that was read, or -1 if there was an error
 */
static int32_t nrb_readUint32(FILE *pf) {
  
  int status = 1;
  uint32_t rv = 0;
  int32_t retval = 0;
  
  /* Check parameter */
  if (pf == NULL) {
    abort();
  }
  
  /* Read a raw 32-bit value */
  if (!nrb_read32(pf, &rv)) {
    status = 0;
  }
  
  /* Range-check value */
  if (status) {
    if (rv > NRB_MAXUINT32) {
      status = 0;
    }
  }
  
  /* Determine return value */
  if (status) {
    retval = (int32_t) rv;
  } else {
    retval = -1;
  }
  
  /* Return value */
  return retval;
}

/*
 * Read an unsigned 16-bit integer value from a file.
 * 
 * pf is the file to read from.  It must be open for reading or
 * undefined behavior occurs.
 * 
 * A raw 16-bit value is read in big endian order from the file.  The
 * numeric value must be in range [0, NRB_MAXUINT16].  The raw numeric
 * value is then returned as-is.
 * 
 * Parameters:
 * 
 *   pf - the file to read
 * 
 * Return:
 * 
 *   the integer that was read, or -1 if there was an error
 */
static int32_t nrb_readUint16(FILE *pf) {
  
  int status = 1;
  uint16_t rv = 0;
  int32_t retval = 0;
  
  /* Check parameter */
  if (pf == NULL) {
    abort();
  }
  
  /* Read a raw 16-bit value */
  if (!nrb_read16(pf, &rv)) {
    status = 0;
  }
  
  /* Range-check value */
  if (status) {
    if (rv > NRB_MAXUINT16) {
      status = 0;
    }
  }
  
  /* Determine return value */
  if (status) {
    retval = (int32_t) rv;
  } else {
    retval = -1;
  }
  
  /* Return value */
  return retval;
}

/*
 * Read a biased 8-bit integer value from a file.
 * 
 * pf is the file to read from.  It must be open for reading or
 * undefined behavior occurs.
 * 
 * pv points to the variable to receive the numeric value that was read.
 * Its value is undefined if the function fails.
 * 
 * A raw nyt value is read from the file.  The raw byte value is then
 * subtracted by NRB_BIAS8 to yield the final result, which is in range
 * [NRB_MINBIAS8, MRB_MAXBIAS8].
 * 
 * Parameters:
 * 
 *   pf - the file to read
 * 
 *   pv - the variable to receive the numeric value
 * 
 * Return:
 * 
 *   non-zero if successful, zero if error
 */
static int nrb_readBias8(FILE *pf, int *pv) {
  
  int status = 1;
  int rv = 0;
  
  /* Check parameters */
  if ((pf == NULL) || (pv == NULL)) {
    abort();
  }
  
  /* Read a byte value */
  rv = nrb_readByte(pf);
  if (rv < 0) {
    status = 0;
  }
  
  /* Range-check value */
  if (status) {
    if ((rv < 0) || (rv > 255)) {
      abort();
    }
  }
  
  /* Bias value to get final result */
  if (status) {
    rv = rv - NRB_BIAS8;
  }
  
  /* Set result */
  if (status) {
    *pv = rv;
  }
  
  /* Return status */
  return status;
}

/*
 * Compare two note events to each other.
 * 
 * The comparison is less than, equals, or greater than according to the
 * starting time of the note events.
 * 
 * The function is designed to be used with the qsort() function of the
 * standard library.  The pointers are to NRB_NOTE structures.
 * 
 * Parameters:
 * 
 *   pva - void pointer to the first note event
 * 
 *   pvb - void pointer to the second note event
 * 
 * Return:
 * 
 *   less than, equal to, or greater than zero as note event a is less
 *   than, equal to, or greater than note event b
 */
static int nrb_cmp(const void *pva, const void *pvb) {
  
  int result = 0;
  const NRB_NOTE *pa = NULL;
  const NRB_NOTE *pb = NULL;
  
  /* Check parameters */
  if ((pva == NULL) || (pvb == NULL)) {
    abort();
  }
  
  /* Get pointers to note events a and b */
  pa = (const NRB_NOTE *) pva;
  pb = (const NRB_NOTE *) pvb;
  
  /* Compare time offsets */
  if (pa->start < pb->start) {
    /* First t less than second t */
    result = -1;
    
  } else if (pa->start > pb->start) {
    /* First t greater than second t */
    result = 1;
    
  } else if (pa->start == pb->start) {
    /* Both events have same t */
    result = 0;
    
  } else {
    /* Shouldn't happen */
    abort();
  }
  
  /* Return result */
  return result;
}

/*
 * Public function implementations
 * ===============================
 * 
 * See the header for specifications.
 */

/*
 * nrb_alloc function.
 */
NRB_DATA *nrb_alloc(void) {
  
  NRB_DATA *pd = NULL;
  
  /* Allocate data structure */
  pd = (NRB_DATA *) malloc(sizeof(NRB_DATA));
  if (pd == NULL) {
    abort();
  }
  memset(pd, 0, sizeof(NRB_DATA));
  
  /* Allocate the section and note tables with initial capacities */
  pd->pSect = (int64_t *) calloc(NRB_SECTALLOC_INIT, sizeof(int64_t));
  if (pd->pSect == NULL) {
    abort();
  }
  
  pd->pNote = (NRB_NOTE *) calloc(NRB_NOTEALLOC_INIT, sizeof(NRB_NOTE));
  if (pd->pNote == NULL) {
    abort();
  }
  
  /* Set the initial state */
  pd->sect_count = 1;
  pd->sect_cap = NRB_SECTALLOC_INIT;
  pd->note_count = 0;
  pd->note_cap = NRB_NOTEALLOC_INIT;
  (pd->pSect)[0] = 0;
  
  /* Return the new object */
  return pd;
}

/*
 * nrb_parse function.
 */
NRB_DATA *nrb_parse(FILE *pf, int *pVer) {
  
  int status = 1;
  int dummy = 0;
  int32_t i = 0;
  int32_t u = 0;
  int v = 0;
  int ver_major = 0;
  int ver_minor = 0;
  NRB_DATA *pd = NULL;
  NRB_NOTE *pn = NULL;
  
  /* Check parameter */
  if (pf == NULL) {
    abort();
  }
  
  /* If pVer not provided, redirect to dummy */
  if (pVer == NULL) {
    pVer = &dummy;
  }
  
  /* Read the primary and secondary signatures */
  if (nrb_readUint32(pf) != NRB_SIGPRI) {
    status = 0;
    *pVer = NRB_VER_ERROR;
  }
  if (status) {
    if (nrb_readUint32(pf) != NRB_SIGSEC) {
      status = 0;
      *pVer = NRB_VER_ERROR;
    }
  }
  
  /* Read the major and minor version numbers */
  if (status) {
    ver_major = nrb_readByte(pf);
    if (ver_major < 0) {
      status = 0;
      *pVer = NRB_VER_ERROR;
    }
  }
  if (status) {
    ver_minor = nrb_readByte(pf);
    if (ver_minor < 0) {
      status = 0;
      *pVer = NRB_VER_ERROR;
    }
  }
  
  /* If major version number is not one, then fail */
  if (status && (ver_major != 1)) {
    status = 0;
    *pVer = NRB_VER_MAJOR;
  }
  
  /* If minor version zero, then set OK status for version; else, set
   * MINOR status for version but still continue */
  if (status) {
    if (ver_minor == 0) {
      *pVer = NRB_VER_OK;
    } else {
      *pVer = NRB_VER_MINOR;
    }
  }
  
  /* Allocate a parser structure */
  if (status) {
    pd = (NRB_DATA *) malloc(sizeof(NRB_DATA));
    if (pd == NULL) {
      abort();
    }
    memset(pd, 0, sizeof(NRB_DATA));
    pd->pSect = NULL;
    pd->pNote = NULL;
  }
  
  /* Read the section count and note count */
  if (status) {
    pd->sect_count = nrb_readUint16(pf);
    if ((pd->sect_count < 1) || (pd->sect_count > NRB_MAXSECT)) {
      status = 0;
    }
  }
  if (status) {
    pd->note_count = nrb_readUint32(pf);
    if ((pd->note_count < 1) || (pd->note_count > NRB_MAXNOTE)) {
      status = 0;
    }
  }
  
  /* Read but ignore four reserved fields */
  if (status) {
    if (nrb_readUint32(pf) < 0) {
      status = 0;
    }
  }
  if (status) {
    if (nrb_readUint32(pf) < 0) {
      status = 0;
    }
  }
  if (status) {
    if (nrb_readUint32(pf) < 0) {
      status = 0;
    }
  }
  if (status) {
    if (nrb_readUint32(pf) < 0) {
      status = 0;
    }
  }
  
  /* Set the capacities equal to the counts */
  if (status) {
    pd->sect_cap = pd->sect_count;
    pd->note_cap = pd->note_count;
  }
  
  /* Allocate the section and note arrays */
  if (status) {
    pd->pSect = (int64_t *) calloc(
                              (size_t) pd->sect_count,
                              sizeof(int64_t));
    if (pd->pSect == NULL) {
      abort();
    }
    
    pd->pNote = (NRB_NOTE *) calloc(
                              (size_t) pd->note_count,
                              sizeof(NRB_NOTE));
    if (pd->pNote == NULL) {
      abort();
    }
  }
  
  /* Read the section table */
  if (status) {
    for(i = 0; i < pd->sect_count; i++) {
      
      /* Read the section value */
      (pd->pSect)[i] = nrb_readUint64(pf);
      if ((pd->pSect)[i] < 0) {
        status = 0;
      }
      
      /* If this is first section, it must have value of zero; else, it
       * must have a value greater than or equal to previous section */
      if (status && (i < 1)) {
        if ((pd->pSect)[i] != 0) {
          status = 0;
        }
      } else if (status) {
        if ((pd->pSect)[i] < (pd->pSect)[i - 1]) {
          status = 0;
        }
      }
      
      /* Leave loop if error */
      if (!status) {
        break;
      }
    }
  }
  
  /* Read the note table */
  if (status) {
    for(i = 0; i < pd->note_count; i++) {
      
      /* Get pointer to current note */
      pn = &((pd->pNote)[i]);
      
      /* Read the fields of the current note */
      pn->start = nrb_readUint64(pf);
      if (pn->start < 0) {
        status = 0;
      }
      
      if (status) {
        pn->release = nrb_readUint64(pf);
        if (pn->release <= pn->start) {
          status = 0;
        }
      }
      
      if (status) {
        if (nrb_readBias8(pf, &v)) {
          if ((v >= NRB_MINPITCH) && (v <= NRB_MAXPITCH)) {
            pn->pitch = (int8_t) v;
          } else {
            status = 0;
          }
        } else {
          status = 0;
        }
      }
      
      if (status) {
        v = nrb_readByte(pf);
        if (v >= 0) {
          if ((v & 0x3f) <= NRB_MAXART) { 
            pn->art = (uint8_t) v;
          } else {
            status = 0;
          }
        } else {
          status = 0;
        }
      }
      
      if (status) {
        u = nrb_readUint16(pf);
        if ((u >= 0) && (u <= NRB_MAXRAMP)) {
          pn->ramp = (uint16_t) u;
        } else {
          status = 0;
        }
      }
      
      if (status) {
        v = nrb_readUint16(pf);
        if ((v >= 0) && (v < pd->sect_count)) {
          pn->sect = (uint16_t) v;
        } else {
          status = 0;
        }
      }
      
      if (status) {
        v = nrb_readUint16(pf);
        if (v >= 0) {
          pn->layer_i = (uint16_t) v;
        } else {
          status = 0;
        }
      }
      
      /* Make sure start time offset is greater than or equal to start
       * of note's section */
      if (status) {
        if (pn->start < (pd->pSect)[pn->sect]) {
          status = 0;
        }
      }
      
      /* Leave loop if error */
      if (!status) {
        break;
      }
    }
  }
  
  /* If failure, make sure structure free */
  if ((!status) && (pd != NULL)) {
    if (pd->pSect != NULL) {
      free(pd->pSect);
    }
    if (pd->pNote != NULL) {
      free(pd->pNote);
    }
    free(pd);
    pd = NULL;
  }
  
  /* Return pointer to new structure, or NULL */
  return pd;
}

/*
 * nrb_parse_path function.
 */
NRB_DATA *nrb_parse_path(const char *pPath, int *pVer) {
  
  FILE *fp = NULL;
  NRB_DATA *pResult = NULL;
  
  /* Check parameter */
  if (pPath == NULL) {
    abort();
  }
  
  /* Open the file for reading */
  fp = fopen(pPath, "rb");
  if (fp != NULL) {
    /* Sucessfully opened file, so pass through and close file */
    pResult = nrb_parse(fp, pVer);
    fclose(fp);
    fp = NULL;
  
  } else {
    /* Didn't open file successfully, so fail and set ERROR if version
     * status provided */
    pResult = NULL;
    if (pVer != NULL) {
      *pVer = NRB_VER_ERROR;
    }
  }
  
  /* Return result */
  return pResult;
}

/*
 * nrb_free function.
 */
void nrb_free(NRB_DATA *pd) {
  if (pd != NULL) {
    free(pd->pSect);
    free(pd->pNote);
    free(pd);
  }
}

/*
 * nrb_sections function.
 */
int32_t nrb_sections(NRB_DATA *pd) {
  if (pd == NULL) {
    abort();
  }
  return pd->sect_count;
}

/*
 * nrb_notes function.
 */
int32_t nrb_notes(NRB_DATA *pd) {
  if (pd == NULL) {
    abort();
  }
  return pd->note_count;
}

/*
 * nrb_offset function.
 */
int64_t nrb_offset(NRB_DATA *pd, int32_t sect_i) {
  if (pd == NULL) {
    abort();
  }
  if ((sect_i < 0) || (sect_i >= pd->sect_count)) {
    abort();
  }
  return (pd->pSect)[sect_i];
}

/*
 * nrb_get function.
 */
void nrb_get(NRB_DATA *pd, int32_t note_i, NRB_NOTE *pn) {
  if ((pd == NULL) || (pn == NULL)) {
    abort();
  }
  if ((note_i < 0) || (note_i >= pd->note_count)) {
    abort();
  }
  memcpy(pn, &((pd->pNote)[note_i]), sizeof(NRB_NOTE));
}

/*
 * nrb_set function.
 */
void nrb_set(NRB_DATA *pd, int32_t note_i, const NRB_NOTE *pn) {
  
  /* Check parameters */
  if ((pd == NULL) || (pn == NULL)) {
    abort();
  }
  if ((note_i < 0) || (note_i >= pd->note_count)) {
    abort();
  }
  
  /* Check fields of note parameter */
  if (pn->start < 0) {
    abort();
  }
  if (pn->release <= pn->start) {
    abort();
  }
  if ((pn->pitch < NRB_MINPITCH) || (pn->pitch > NRB_MAXPITCH)) {
    abort();
  }
  if ((pn->art & 0x3f) > NRB_MAXART) {
    abort();
  }
  if (pn->ramp > NRB_MAXRAMP) {
    abort();
  }
  if (pn->sect >= pd->sect_count) {
    abort();
  }
  if (pn->start < nrb_offset(pd, pn->sect)) {
    abort();
  }
  
  /* Copy new note data in */
  memcpy(&((pd->pNote)[note_i]), pn, sizeof(NRB_NOTE));
}

/*
 * nrb_sect function.
 */
int nrb_sect(NRB_DATA *pd, int64_t offset) {
  
  int status = 1;
  int32_t newcap = 0;
  
  /* Check parameters */
  if ((pd == NULL) || (offset < 0)) {
    abort();
  }
  
  /* Make sure offset is greater than or equal to offset of current last
   * section (the first section is defined automatically right away, so
   * there will always be at least one section) */
  if (offset < nrb_offset(pd, pd->sect_count - 1)) {
    abort();
  }
  
  /* Only proceed if room for another section; otherwise, fail */
  if (pd->sect_count < NRB_MAXSECT) {
    
    /* We have room for another section -- check if capacity of section
     * table needs to be expanded */
    if (pd->sect_count >= pd->sect_cap) {
      
      /* We need to expand capacity -- first, try doubling */
      newcap = pd->sect_cap * 2;
      
      /* If doubled capacity is less than initial allocation, set to
       * initial allocation */
      if (newcap < NRB_SECTALLOC_INIT) {
        newcap = NRB_SECTALLOC_INIT;
      }
      
      /* If doubling exceeds maximum section count, set to maximum
       * section count */
      if (newcap > NRB_MAXSECT) {
        newcap = NRB_MAXSECT;
      }
      
      /* Reallocate with the new capacity */
      pd->pSect = (int64_t *) realloc(
                          pd->pSect,
                          newcap * sizeof(int64_t));
      if (pd->pSect == NULL) {
        abort();
      }
      memset(
          &((pd->pSect)[pd->sect_cap]),
          0,
          (newcap - pd->sect_cap) * sizeof(int64_t));
      
      /* Update capacity variable */
      pd->sect_cap = newcap;
    }
    
    /* Add the new section to the table */
    (pd->pSect)[pd->sect_count] = offset;
    (pd->sect_count)++;
    
  } else {
    /* Section table is full */
    status = 0;
  }
  
  /* Return status */
  return status;
}

/*
 * nrb_append function.
 */
int nrb_append(NRB_DATA *pd, const NRB_NOTE *pn) {
  
  int status = 1;
  int32_t newcap = 0;
  
  /* Check parameters */
  if ((pd == NULL) || (pn == NULL)) {
    abort();
  }
  
  /* Check fields of note parameter */
  if (pn->start < 0) {
    abort();
  }
  if (pn->release <= pn->start) {
    abort();
  }
  if ((pn->pitch < NRB_MINPITCH) || (pn->pitch > NRB_MAXPITCH)) {
    abort();
  }
  if ((pn->art & 0x3f) > NRB_MAXART) {
    abort();
  }
  if (pn->ramp > NRB_MAXRAMP) {
    abort();
  }
  if (pn->sect >= pd->sect_count) {
    abort();
  }
  if (pn->start < nrb_offset(pd, pn->sect)) {
    abort();
  }
  
  /* Only proceed if room for another note; otherwise, fail */
  if (pd->note_count < NRB_MAXNOTE) {
    
    /* We have room for another note -- check if capacity of note table
     * needs to be expanded */
    if (pd->note_count >= pd->note_cap) {
      
      /* We need to expand capacity -- first, try doubling */
      newcap = pd->note_cap * 2;
      
      /* If doubling is less than initial capacity, set to initial
       * capacity */
      if (newcap < NRB_NOTEALLOC_INIT) {
        newcap = NRB_NOTEALLOC_INIT;
      }
      
      /* If doubling exceeds maximum note count, set to maximum note
       * count */
      if (newcap > NRB_MAXNOTE) {
        newcap = NRB_MAXNOTE;
      }
      
      /* Reallocate with the new capacity */
      pd->pNote = (NRB_NOTE *) realloc(
                          pd->pNote,
                          newcap * sizeof(NRB_NOTE));
      if (pd->pNote == NULL) {
        abort();
      }
      memset(
          &((pd->pNote)[pd->note_cap]),
          0,
          (newcap - pd->note_cap) * sizeof(NRB_NOTE));
      
      /* Update capacity variable */
      pd->note_cap = newcap;
    }
    
    /* Add the new note to the table */
    memcpy(&((pd->pNote)[pd->note_count]), pn, sizeof(NRB_NOTE));
    (pd->note_count)++;
    
  } else {
    /* Note table is full */
    status = 0;
  }
  
  /* Return status */
  return status;
}

/*
 * nrb_sort function.
 */
void nrb_sort(NRB_DATA *pd) {
  
  /* Check parameter */
  if (pd == NULL) {
    abort();
  }
  
  /* Only proceed if at least two notes */
  if (pd->note_count > 1) {
  
    /* Sort the array of note events */
    qsort(
        (void *) pd->pNote,
        (size_t) pd->note_count,
        sizeof(NRB_NOTE),
        &nrb_cmp);
  }
}

/*
 * nrb_serialize function.
 */
int nrb_serialize(NRB_DATA *pd, FILE *pf) {
  
  int status = 1;
  int32_t i = 0;
  NRB_NOTE *pn = NULL;
  
  /* Check parameters */
  if ((pd == NULL) || (pf == NULL)) {
    abort();
  }
  
  /* Only proceed if at least one note */
  if (pd->note_count > 0) {
    
    /* Write the NRB signatures */
    nrb_writeUint32(pf, (uint32_t) NRB_SIGPRI);
    nrb_writeUint32(pf, (uint32_t) NRB_SIGSEC);
    
    /* Write the NRB version number */
    nrb_writeByte(pf, 1);
    nrb_writeByte(pf, 0);
    
    /* Write the section count and note counts */
    nrb_writeUint16(pf, (uint16_t) pd->sect_count);
    nrb_writeUint32(pf, (uint32_t) pd->note_count);
    
    /* Write the reserved fields */
    nrb_writeUint32(pf, 0);
    nrb_writeUint32(pf, 0);
    nrb_writeUint32(pf, 0);
    nrb_writeUint32(pf, 0);
    
    /* Write the section table */
    for(i = 0; i < pd->sect_count; i++) {
      nrb_writeUint64(pf, (uint64_t) ((pd->pSect)[i]));
    }
    
    /* Write each note */
    for(i = 0; i < pd->note_count; i++) {
      
      /* Get pointer to current note */
      pn = &((pd->pNote)[i]);
      
      /* Serialize the structure */
      nrb_writeUint64(pf, (uint64_t) pn->start);
      nrb_writeUint64(pf, (uint64_t) pn->release);
      nrb_writeBias8(pf, (int) pn->pitch);
      nrb_writeByte(pf, (int) pn->art);
      nrb_writeUint16(pf, pn->ramp);
      nrb_writeUint16(pf, pn->sect);
      nrb_writeUint16(pf, pn->layer_i);
    }
  
  } else {
    /* No notes have been defined */
    status = 0;
  }
  
  /* Return status */
  return status;
}
