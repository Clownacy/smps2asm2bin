SMPS2ASM is nice and all, but its dependency on the AS Macro-Assembler is annoying. This repo contains a tool that's essentially a mini-assembler with SMPS2ASM support built-in.

Right now it's a little rough around the edges, and doesn't handle exotic stuff like songs sharing data (see Sonic 3 & Knuckles), but I hope to fix all that at some point.


USAGE:
        smps2asm2bin [-v driver_version] [-o hex_offset] in_file_path [out_file_path]

OPTIONS:
        -v driver_version
                Specifies the target driver version:
                        1 = Sonic 1 (default)
                        2 = Sonic 2
                        3 = Sonic 3 & Knuckles

        -o hex_offset
                Base offset for the binary file (hexadecimal).


As far as the licence goes, my code is under the zlib licence, but the SMPS2ASM support is derived from the original SMPS2ASM macros by flamewing and Cinossu, which they never clarified the licence for. Use at your own risk I suppose, O' legally-concious Sonic hacker.
