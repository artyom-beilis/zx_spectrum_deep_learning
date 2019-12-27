# Deep Learing For ZX Spectrum

Implementation of a simple MNIST training for sinclair:

To build and run you need:

1. z88dk: https://github.com/z88dk/z88dk.git
2. fuse - zx spectrum emulator: http://fuse-emulator.sourceforge.net/ 
3. bas2tap - text to zx spectrum basic tape: https://github.com/andybalaam/bas2tap.git

It builds 4 tapes to run:

- train\_C\_float.tap - tape to run C training
- train\_C\_fixed.tap - tape to run fixed point C training
- train\_basic.tap - tape for BASIC traing
- train\_basic\_tobos.tap - tape to be used with ToBoSFP basic compiler

