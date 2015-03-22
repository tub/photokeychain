# Introduction #

The photoframe source tree is divided into several components.  This page briefly describes what each of them does.

  * tools/st2205tool : Software to communicate with the photoframe device over usb.  This is used to upload firmware code to the device and also to write to other portions of the flash memory.
  * tools/sim65 : A fork of the sim65 project, specifically aimed at emulating the st2203u-based photo keychain.
  * tools/crasm : A 6502 assembler, with some minor patches so that it correctly assembles CPX/CPY instructions, and has support for a few extended opcodes.
  * tools/subpixel : A utility to scale images 'subpixel'-accurately, by using a cleartype-like algorithm that takes into account the positioning of red green and blue pixels on the TFT.
  * tools/projects : The main development kit. Each subdirectory here contains a stand-alone application, and all of them share the same makefile system that provides some handy utilities for quick deployment and testing.