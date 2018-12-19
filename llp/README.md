# Source materials for the Languages and Low-Level Programming Labs

This folder contains the source materials for the seven lab
sessions that were developed for and used in the Languages and
Low-Level Programming (LLP) course at Portland State University in
Spring 2015, Spring 2016, and Fall 2017.  The materials provided
here have been lightly edited to remove details that would only
be useful to PSU students (such as links to the online course
management system, and specifics of the lab configuration at PSU),
but otherwise reflect the final versions of the labs that were
used by the students.

The lab materials are in folders that begin with numbers indicating
the order in which students are expected to work through labs, with
a schedule of approximately one lab per week.  After seven weeks,
students were expected to use their lab and homework time for
finishing any remaining parts of these labs and for working on
individual projects.

The labs provided here are as follows:

* `1-vram`: A Video RAM Simulation

* `2-baremetal`: Basic Examples and Exercises for Bare Metal Programming

* `3-switching`: Context Switching and Timer Interrupts

* `4-paging`: Paging, Part 1: Managing the MMU

* `5-processes`: Paging, Part 2: Processes

* `6-pork`: A Demo Using "pork" (the Portland OR Kernel)

* `7-caps`: Capabilities

These folders are intended to be hosted on the web and each one
includes appropriate HTML and image files.  The `buildall` script
(executed using `source buildall`) can be used to generate `.tar.gz`
files containing the code resources for all of the labs; the companion
`cleanall` script (executed using `source cleanall`) can be used to
remove those files.  (The `.tar.gz` files referenced here are just
packaged up versions of the files in the root directory of this
repository.)
