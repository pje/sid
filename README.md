<p align=center>
  <image src=sid.jpg width=333 height=333 align=left>
  </image>
</p>

# SID
Firmware for using the [MOS 6581 SID](https://en.wikipedia.org/wiki/MOS_Technology_6581)(the commodore 64's sound chip) as a MIDI synthesizer.

Also:

  - [A Max4Live object](data/SID.amxd) to control the SID through Ableton Live
  - [A fritzing sketch](data/SID_shield.fzz) that (mostly correctly?) captures the wiring on the hand-soldered shield prototype I use

### Status

Works on my Macbook, Arduino Micro™, and shield.

### Build

```bash
make deps      # install dependencies
make test      # run some unit tests
make upload    # compile and upload to the arduino
```

#### Resources

- The original specsheet, [converted to text](http://www.sidmusic.org/sid/sidtech2.html). Good example of technical writing, but ~~fuzzy~~ wrong in some specifics
- [The Insidious manual](https://impactsoundworks.com/docs/inSIDious%20Manual.pdf), which incidentally describes a lot of subtle stuff about the SID
- [Sparkfun's "Introduction To MIDI Hardware & Electronic Implementation"](https://learn.sparkfun.com/tutorials/midi-tutorial/hardware--electronic-implementation)
