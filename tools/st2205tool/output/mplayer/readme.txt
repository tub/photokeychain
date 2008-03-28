This is a patch against MPlayer v1.0rc2 to add support for a hacked ST2205
device as a video output device.

Ultra-short howto:
- Get & unpack MPlayer (www.mplayerhq.hu)
- Apply patch
- ./configure && ./make
- if you want, sudo make install
- mplayer -vo st2205u:/dev/sdX -x 128 -y 128 -zoom movie.avi
  (with /dev/sdX being the device your photo frame is connected to)


