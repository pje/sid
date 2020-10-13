# SID

Arduino/AVR C code for controlling the MOS6581 SID sound chip over MIDI

### Status

Works on My Computer™

#### Resources

- [The original MOS specsheet for the 6581](http://archive.6502.org/datasheets/mos_6581_sid.pdf). Wonderful example of technical writing, but wrong in some specifics :(
- The original specsheet, [converted to text](http://www.sidmusic.org/sid/sidtech2.html)
- [Sparkfun's "Introduction To MIDI Hardware & Electronic Implementation"](https://learn.sparkfun.com/tutorials/midi-tutorial/hardware--electronic-implementation)

```
ADR envelope 8-bit values, mapped to mS equivalents

8-bit   A             DR
0       2 mS          6 mS
1       8 mS          24 mS
2       16 mS         48 mS
3       24 mS         72 mS
4       38 mS         114 mS
5       56 mS         168 mS
6       68 mS         204 mS
7       80 mS         240 mS
8       100 mS        300 mS
9       250 mS        750 mS
10      500 mS        1.5 S
11      800 mS        2.4 S
12      1 S           3 S
13      3 S           9 S
14      5 S           15 S
15      8 S           24 S
```
