# A Demo using "pork"

The Portland L4 Kernel, or "pork" for short, is a small, not-quite
complete implementation of L4 for the 32 bit x86 architecture.

After studying parts of the design and implementation of the L4
API and the pork microkernel in the lectures, we use the trace of
output messages provided in the attached "handout.pdf" to walk
through the sequence of steps that the machine goes through in
booting and running a simple ping pong demo in pork.  All of the
messages were captured from output sent to the serial port when
the demo was executed.  Key questions to be considering as you
work through the trace include:

- Which parts of the code were responsible for each of the output
  messages that you see?

- What can we tell about the state of the machine at the time each
  of those messages were generated?

- How do the numbers that are included in the output, mostly in
  hexadecimal notation, relate to concepts that might be more
  recognizable to L4 developers (e.g., thread numbers, address
  space details, hardware configuration, etc.)?

For best results, this demonstration should be given by someone
who can walk through the output log line by line and provide some
explanations about what each message means, and how all of the
indvidual steps contribute to getting a working system up and
running.
