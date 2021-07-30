<p align=center>
  <image src=sid.jpg width=333 height=333 align=left>
  </image>
</p>

# SID
Firmware for using the [MOS 6581 SID](https://en.wikipedia.org/wiki/MOS_Technology_6581) as a MIDI synthesizer.

Also:

  1. [A fritzing sketch](data/SID_shield.fzz) that somewhat describes the wiring on the hand-soldered shield I use
  2. [A Max4Live object](data/SID.amxd) to control the SID through Ableton Live:

<p align=center>
  <image src=data/sid_m4l_device.png height=100 align=center> </image>
</p>

### Status

Works on my machine™ (Macbook, Arduino Micro, 6581 + shield).

### Build

```bash
make deps      # install dependencies
make test      # run the unit tests
make upload    # compile and upload to the arduino
```

#### Resources

- The SID specsheet, [converted to text](http://www.sidmusic.org/sid/sidtech2.html). Good example of technical writing, but ~~fuzzy~~ wrong on some details
- [The Insidious manual](https://impactsoundworks.com/docs/inSIDious%20Manual.pdf), which documents a lot of subtle SID characteristics
- [Sparkfun's "Introduction To MIDI Hardware & Electronic Implementation"](https://learn.sparkfun.com/tutorials/midi-tutorial/hardware--electronic-implementation)


