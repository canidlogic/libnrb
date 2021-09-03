# NoiR Binary (NRB) specification

- &#x200b;1. [Introduction](#mds1)
- &#x200b;2. [Data types](#mds2)
  - &#x200b;2.1 [32-bit integer](#mds2p1)
  - &#x200b;2.2 [64-bit integer](#mds2p2)
  - &#x200b;2.3 [16-bit integer](#mds2p3)
  - &#x200b;2.4 [Biased byte](#mds2p4)
  - &#x200b;2.5 [Unsigned byte](#mds2p5)
- &#x200b;3. [File header](#mds3)
- &#x200b;4. [Section table](#mds4)
- &#x200b;5. [Note table](#mds5)

## <span id="mds1">1. Introduction</span>

The NoiR Binary (NRB) format is a binary file format that is designed to store the results of interpreting a Noir notation file.  While Noir notation is a text format that is designed to be written manually by the user, NRB is designed to be easy to access from computer programs.

## <span id="mds2">2. Data types</span>

This section documents the data types used in NRB and how they are stored in an NRB byte stream.

### <span id="mds2p1">2.1 32-bit integer</span>

A 32-bit integer is stored with four bytes in big endian order, with the most significant byte first and the least significant byte last.  The most significant bit of the integer must always be zero, so the range is zero up to and including 2,147,483,647.

### <span id="mds2p2">2.2 64-bit integer</span>

A 64-bit integer is stored with eight bytes in big endian order, with the most significant byte first and the least significant byte last.  The most significant bit of the integer must always be zero, so the range is zero up to and including 9,223,372,036,854,775,807.

### <span id="mds2p3">2.3 16-bit integer</span>

A 16-bit integer is stored with two bytes in big endian order, with the most significant byte first and the least significant byte second.  The most significant bit of the integer need not be zero, so the range is zero up to and including 65,535.

### <span id="mds2p4">2.4 Biased byte</span>

A biased byte is an integer stored in a single byte.  The unsigned value of the byte encodes a signed value.  The raw, unsigned range of the byte is \[0, 255\].  The encoded signed value is the unsigned value subtracted by 128.  This results in a signed range of -128 up to and including 127.

### <span id="mds2p5">2.5 Unsigned byte</span>

An unsigned 8-bit integer stored within a single byte.  The valid range is zero up to and including 255.

## <span id="mds3">3. File header</span>

An NRB file begins with the following header:

1. \[32-bit integer\] __Primary signature__
2. \[32-bit integer\] __Secondary signature__
3. \[Unsigned byte\] __Major version__
4. \[Unsigned byte\] __Minor version__
5. \[16-bit integer\] __Section count__
6. \[32-bit integer\] __Note count__
7. \[32-bit integer\] Reserved
8. \[32-bit integer\] Reserved
9. \[32-bit integer\] Reserved
10. \[32-bit integer\] Reserved

The primary signature must be the value 1,928,196,216.  The secondary signature must be the value 778,990,178.  If these two signatures are not present with those values, then the file is not an NRB file.

The major and minor version fields determine the version of the NRB format in use.  This document describes an NRB file of major version one and minor version zero.  If the parser recognizes the major version number but the minor version number is greater than recognized, the parser should warn that the file is of an unsupported version but still try to parse the file.  If the major version number is greater than the parser recognizes, the parser should fail and not attempt to parse the file.

The section count is the total number of sections in the section table (see &sect;4 [Section table](#mds4)).  The range of the section count is one up to and including 65,535.

The note count is the total number of notes in the note table (see &sect;5 [Note table](#mds5)).  The range of the note count is zero up to and including 1,048,576.

The reserved fields in the header are reserved for possible use in future versions of the format.  Parsers of the current version should ignore their contents when reading and always write zero values for the fields when writing.

## <span id="mds4">4. Section table</span>

The section table stores the starting offset of each section in the composition.  The total number of sections is given in the file header (see &sect;3 [File header](#mds3)).

The section table has one 64-bit integer for each section.  Each integer indicates how many microseconds have elapsed from the start of the composition up to the start of the section.  The first section must always have a value of zero, meaning it starts at the beginning of the composition.

If there is more than one section, each section after the first must have a starting offset that is greater than or equal to the starting offset of the previous section.

The section index of a particular section is its zero-based index in the section table.  Therefore, the first section has section index zero, the second section has section index one, and so forth.  Each note in the note table (see &sect;5 [Note table](#mds5)) has a section index identifying which section it belongs to.  All notes in a section must have starting offsets that are greater than or equal to the starting offset of the section given in the section table.

Note that earlier sections may overlap with later sections.  That is, an event in section 2 might take place after an event in section 3, for example.

## <span id="mds5">5. Note table</span>

The note table stores all the notes of the composition.  The total number of notes is given in the file header (see &sect;3 [File header](#mds3)).  The notes may be in any order; there are no sorting requirements.  Each note has the following structure:

1. \[64-bit integer\] __Start time__
2. \[64-bit integer\] __Release time__
3. \[Biased byte\] __Pitch__
4. \[Unsigned byte\] __Articulation__
5. \[16-bit integer\] __Ramp__
6. \[16-bit integer\] __Section index__
7. \[16-bit integer\] __Layer index__

The _start time_ is the number of microseconds since the start of the composition when the note begins to be played.  The _release time_ is the number of microseconds since the start of the composition when the note finishes, such that the release time subtracted by the start time equals the duration in microseconds of the note.  The release time must be greater than the start time, and the start time must be greater than or equal to the starting offset of the section this note belongs to (see below).

The _pitch_ is the number of semitones (half steps) away from middle C.  A value of zero means middle C, a value of -1 means B below middle C, a value of 2 means D above middle C, and so forth.  The valid range is \[-39, 48\], which covers all pitches on the 88-key piano keyboard.

The _articulation_ is a bit field with the following structure:

    MSB   7   6   5 .. 0   LSB
        +---+---+--------+
        | P | G |   A    |
        +---+---+--------+

The most significant bit `P` in the articulation byte is set if the note's duration has been modified by a sustain pedal, clear otherwise.  The pedal-modified duration of the note has already been computed by the Noir compiler.  This bit is just provided in case knowing whether a note is sustained by pedal has an effect on how the note is rendered.

The bit `G` in the articulation byte is set if the note is a grace note, clear otherwise.  The start and release times have already been modified appropriately for a grace note by the Noir compiler.  This bit is just provided in case knowing whether a note is a grace note has an effect on how the note is rendered.

The six least significant bits `A` of the articulation byte select an articulation index to apply to the note.  The valid range is \[0, 61\] &mdash; articulation indices 62 and 63 are reserved and should not be used.  NRB does not specify the meaning of any particular articulation index.

The _ramp_ encodes a floating-point value in range [0.0, 1.0].  A ramp value of zero means 0.0, a ramp value of 16,384 means 1.0, and all integer values `i` between those two extremes correspond to floating-point values `x` by the following formula:

    x = i / 16384.0

The ramp is intended for use in conjunction with the articulation for rendering effects such as crescendi and diminuendi, though the specific meaning of the ramp is not specified by NRB.  A value of zero is the default value used when no ramp is in effect (and also at the start of a ramp).  Ramps may proceed in both directions, from zero to one, and from one to zero.

The section index must be in range \[0, _max_\], where _max_ is one less than the section count given in the header.  The section index selects a particular section entry in the section table.  The start time of the note must be greater than or equal to the starting offset of the section.

The layer index selects a layer within the particular section specified for this note.  The layer index is one less than the layer number, so a value of zero selects layer one within this section, one selects layer two within this section, and so forth.

Parsers should ignore any data in the file that is present after the end of the note table, to allow for forward compatibility with future versions of the format.
