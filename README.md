# NMF legacy tools

Provides legacy tools for working with NMF files.

The NMF format is defined by [libnmf](https://www.purl.org/canidtech/r/libnmf).  See that project for further information.

## Included legacy tools

To convert an NMF file with a quantum basis of 96 quanta per quarter note into an NMF file with a 44.1kHz or 48kHz quanta rate, you can use the `nmfrate` and `nmftempo` utility programs.  `nmfrate` uses a fixed tempo for the conversion.  `nmftempo` allows for variable tempi, including gradual tempo changes.  However, you must create a separate tempo map file in order to use `nmftempo`.  See the sample tempo map `Tempo_Map.txt` in the `doc` directory for further information.

To convert an NMF file into a data format that can be used with the Retro synthesizer, you can use the `nmfgraph` and `nmfsimple` utility programs.  `nmfsimple` converts an NMF file into a sequence of note events that can be included in a Retro synthesis script.  `nmfgraph` can convert specially-formatted NMF files into dynamic graphs for use with Retro.

## Modern equivalents

The tools provided in this project were used in Noir/Retro projects designed for the initial Beta versions.  These tools have since been replaced with modern equivalents:

The `nmfrate` and `nmftempo` programs have been replaced with [Halogen](https://www.purl.org/canidtech/r/halogen).

The `nmfgraph` tool can be replaced by creating a Retro layer template with [Infrared](https://www.purl.org/canidtech/r/infrared) that uses cues to determine the positions of events.

The `nmfsimple` tool is replaced by [Infrared](https://www.purl.org/canidtech/r/infrared), which is much more flexible and sophisticated in its abilities to convert NMF into Retro synthesis scripts.

## Releases

### Version 1.0.0

This release contains the legacy tools as they were in Noir beta 0.5.2, applying just a few edits to documentation to account for the programs now being separate from the main Noir project.
