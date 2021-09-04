#ifndef NRB_H_INCLUDED
#define NRB_H_INCLUDED

/*
 * nrb.h
 * =====
 * 
 * NoiR Binary (NRB) file library.
 */

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

/*
 * Constants
 * ---------
 */

/*
 * The maximum section count possible for an NRB file.
 */
#define NRB_MAXSECT (INT32_C(65535))

/*
 * The maximum note count possible for an NRB file.
 */
#define NRB_MAXNOTE (INT32_C(1048576))

/*
 * The minimum and maximum pitch values.
 */
#define NRB_MINPITCH (-39)
#define NRB_MAXPITCH (48)

/*
 * The maximum articulation value.
 */
#define NRB_MAXART (61)

/*
 * The maximum integer ramp value.
 */
#define NRB_MAXRAMP (16384)

/*
 * Version status codes.
 */
#define NRB_VER_OK    (0)   /* NRB version supported */
#define NRB_VER_MINOR (1)   /* NRB minor version unsupported */
#define NRB_VER_MAJOR (2)   /* NRB major version unsupported */
#define NRB_VER_ERROR (3)   /* Can't read NRB version */

/*
 * Type declarations
 * -----------------
 */

/*
 * NRB_DATA structure prototype.
 * 
 * Definition given in the implementation.
 */
struct NRB_DATA_TAG;
typedef struct NRB_DATA_TAG NRB_DATA;

/*
 * Representation of a parsed note.
 */
typedef struct {
  
  /*
   * The starting time offset in microseconds.
   * 
   * This is zero or greater.  It must be greater than or equal to the
   * offset of the section that the note belongs to.
   */
  int64_t start;
  
  /*
   * The release time offset in microseconds.
   * 
   * This must be greater than the start time.  The release time
   * subtracted by the starting time equals the duration of the note in
   * microseconds.
   */
  int64_t release;
  
  /*
   * The pitch of this note.
   * 
   * This is the number of semitones (half steps) away from middle C.  A
   * value of zero is middle C, -1 is B below middle C, 2 is D above
   * middle C, and so forth.
   * 
   * The range is [NRB_MINPITCH, NRB_MAXPITCH].
   */
  int8_t pitch;
  
  /*
   * The articulation of this note.
   * 
   * The most significant bit "P" is a flag indicating whether the note
   * was modified by a sustain pedal.
   * 
   * The second most significant bit "G" is a flag indicating whether
   * the note is a grace note.
   * 
   * The six least significant bits are an articulation index "A" in
   * range [0, NRB_MAXART].
   */
  uint8_t art;
  
  /*
   * The ramp value for the note.
   * 
   * This is in range [0, NRB_MAXRAMP], which encodes a floating-point
   * range [0.0, 1.0].
   */
  uint16_t ramp;
  
  /*
   * The section index of this note.
   * 
   * This must reference a section that has been defined in the section
   * table.
   */
  uint16_t sect;
  
  /*
   * One less than the layer index of this note within the section.
   */
  uint16_t layer_i;
  
} NRB_NOTE;

/*
 * Public functions
 * ----------------
 */

/*
 * Allocate a new, empty data object.
 * 
 * The returned object will have section zero defined at offset zero and
 * an empty note table.
 * 
 * The returned object must eventually be freed with nrb_free().
 * 
 * To define a new data object from an NRB file, use nrb_parse()
 * instead.
 * 
 * Return:
 * 
 *   a new, empty data object
 */
NRB_DATA *nrb_alloc(void);

/*
 * Parse the given file as an NRB file and return an object representing
 * the parsed data.
 * 
 * pf is the file to read.  The file is read from the current position
 * in sequential order.  Any additional data after the NRB file is
 * ignored.  Undefined behavior occurs if pf is not open for reading.
 * If there is an error reading the data file, NULL is returned.
 * 
 * If pVer is provided, then a NRB_VER constant will always be written
 * into it by this function.  OK means the version was supported.  The
 * function might still fail if there was a problem reading the data.
 * MINOR means the major version of the file is supported but not the
 * minor version.  The function might fail or succeed.  The user should
 * be warned even if the function succeeds.  MAJOR means the major
 * version of the file is not supported.  The function always fails in
 * this case.  ERROR means the version couldn't be read, which means the
 * file isn't an NRB file.  The function always fails in this case.
 * 
 * The returned object must eventually be freed with nrb_free().
 * 
 * Parameters:
 * 
 *   pf - the NRB file to parse
 * 
 *   pVer - pointer to a variable to receive a version status, or NULL
 * 
 * Return:
 * 
 *   a parsed representation of the NRB file data, or NULL if there was
 *   an error
 */
NRB_DATA *nrb_parse(FILE *pf, int *pVer);

/*
 * Wrapper around nrb_parse that accepts a file path instead of an open
 * file.
 * 
 * Parameters:
 * 
 *   pPath - the path to the NRB file to parse
 * 
 *   pVer - pointer to a variable to receive a version status, or NULL
 * 
 * Return:
 * 
 *   a parsed representation of the NRB file, or NULL if there was an
 *   error
 */
NRB_DATA *nrb_parse_path(const char *pPath, int *pVer);

/*
 * Release the given parsed data object.
 * 
 * If NULL is passed, the call is ignored.
 * 
 * Parameters:
 * 
 *   pd - the parsed data object to free, or NULL
 */
void nrb_free(NRB_DATA *pd);

/*
 * Return the number of sections in the parsed data object.
 * 
 * The range is [1, NRB_MAXSECT].
 * 
 * Parameters:
 * 
 *   pd - the parsed data object
 * 
 * Return:
 * 
 *   the number of sections
 */
int32_t nrb_sections(NRB_DATA *pd);

/*
 * Return the number of notes in the parsed data object.
 * 
 * The range is [0, NRB_MAXNOTE].  Parsed files will never return a
 * count of zero, but blank constructed NRB_DATA objects might.
 * 
 * Parameters:
 * 
 *   pd - the parsed data object
 * 
 * Return:
 * 
 *   the number of notes
 */
int32_t nrb_notes(NRB_DATA *pd);

/*
 * Return the starting offset in microseconds of the given section
 * index.
 * 
 * sect_i is the section index within the parsed data object.  It must
 * be at least zero and less than nrb_sections().
 * 
 * Each section after the first section must have a starting offset that
 * is greater than or equal to the previous section.  The first section
 * must have a starting offset of zero.
 * 
 * Parameters:
 * 
 *   pd - the parsed data object
 * 
 *   sect_i - the section index
 * 
 * Return:
 * 
 *   the starting microsecond offset of this section
 */
int64_t nrb_offset(NRB_DATA *pd, int32_t sect_i);

/*
 * Return a specific note within the parsed data object.
 * 
 * note_i is the note index within the parsed data object.  It must be
 * at least zero and less than nrb_notes().
 * 
 * pn points to the structure to fill with information about the note.
 * 
 * The notes are in any order; there are no sorting guarantees.
 * 
 * Parameters:
 * 
 *   pd - the parsed data object
 * 
 *   note_i - the note index
 * 
 *   pn - the note structure to fill
 */
void nrb_get(NRB_DATA *pd, int32_t note_i, NRB_NOTE *pn);

/*
 * Set a specific note within the parsed data object.
 * 
 * note_i is the note index within the parsed data object.  It must be
 * at least zero and less than nrb_notes().
 * 
 * pn points to the new note data to replace the existing note data.
 * See the structure definition for requirements.  A fault occurs if the
 * note description is not valid.
 * 
 * The data is copied in from the given structure.
 * 
 * This function should be used to change an existing note.  To add an
 * entirely new note, use nrb_append().
 * 
 * Parameters:
 * 
 *   pd - the data object to modify
 * 
 *   note_i - the note index
 * 
 *   pn - the new note data to set
 */
void nrb_set(NRB_DATA *pd, int32_t note_i, const NRB_NOTE *pn);

/*
 * Define a new section beginning at the given offset in microseconds.
 * 
 * pd is the data object to add a section to.
 * 
 * offset is the offset in microseconds from the beginning of the piece.
 * An offset of zero is the beginning of the piece.  offset must be zero
 * or greater.
 * 
 * The first section (section index zero) is always already defined with
 * an offset zero.  This call is only made for sections after section
 * zero.
 * 
 * Subsequent sections must have an offset that is greater than or equal
 * to the offset of the previous section or a fault occurs.
 * 
 * If the number of sections exceeds NRB_MAXSECT, then the function will
 * fail.
 * 
 * Parameters:
 * 
 *   pd - the data object to modify
 * 
 *   offset - the offset of the new section
 * 
 * Return:
 * 
 *   non-zero if successful, zero if too many sections
 */
int nrb_sect(NRB_DATA *pd, int64_t offset);

/*
 * Append a new note event to the given data object.
 * 
 * Note events may be defined in any order.  However, notes that have a
 * section index that is greater than zero may only be defined after
 * that section has been defined.
 * 
 * See the NRB_NOTE structure documentation for the details of how to
 * fill the structure out.  The given structure is copied into the data
 * object.
 * 
 * A fault occurs if any of the structure fields are invalid.  The
 * function fails if too many notes have been added.
 * 
 * Parameters:
 * 
 *   pd - the data object to modify
 * 
 *   pn - the note to append
 * 
 * Return:
 * 
 *   non-zero if successful, zero if too many notes
 */
int nrb_append(NRB_DATA *pd, const NRB_NOTE *pn);

/*
 * Sort all the note events in the given data object.
 * 
 * The notes are put in ascending order of starting time offsets.
 * 
 * Parameters:
 * 
 *   pd - the data object to sort
 */
void nrb_sort(NRB_DATA *pd);

/*
 * Output the contents of the given data object to the given file in the
 * NoiR Binary (NRB) file format.
 * 
 * pd is the data object to serialize.  It must have at least one note
 * or the function will fail.
 * 
 * pf is the file to write the output to.  It must be open for writing
 * or undefined behavior occurs.  Writing is fully sequential.
 * 
 * Parameters:
 * 
 *   pd - the data object to serialize
 * 
 *   pf - the file to write the NRB output to
 * 
 * Return:
 * 
 *   non-zero if successful, zero if no notes have been defined
 */
int nrb_serialize(NRB_DATA *pd, FILE *pf);

#endif
