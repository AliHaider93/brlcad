The Hacker's Guide to BRL-CAD
=============================

Please read this document if you are contributing to BRL-CAD.

BRL-CAD is a relatively large package with a long history and
heritage.  Many people have contributed over the years, and there
continues to be room for involvement and improvement in just about
every aspect of the package.  As such, contributions to BRL-CAD are
very welcome and appreciated.

For the sake of code consistency, project coherency, and the long-term
evolution of BRL-CAD, there are guidelines for getting involved.
Contributors are strongly encouraged to follow these guidelines and to
likewise ensure that other contributors similarly follow the
guidelines.  There are simply too many religious wars over what
usually come down to personal preferences and familiarity that are
distractions from making productive progress.

These guidelines apply to all developers, documentation writers,
testers, porters, graphic designers, web designers, and anyone else
that is directly contributing to BRL-CAD.  As all contributors are
also directly assisting in the development of BRL-CAD as a package,
this guide often simply refers to contributors as developers.

Although BRL-CAD was originally developed and supported primarily by
the U.S. Army Research Laboratory and its affiliates, it is now a true
Open Source project with contributions coming in to the project from
around the world.  The U.S. Army Research Laboratory continues to
support and contribute to BRL-CAD, but now the project is primarily
driven by a team of core developers and the BRL-CAD open source
community.  Contact the BRL-CAD developers for more information.


TABLE OF CONTENTS
-----------------
  Introduction
  Table of Contents
  Getting Started
  How to Contribute
  Source Code Languages
  Filesystem Organization
  Coding Style & Standards
  Documentation
  Testing
  Patch Submission Guidelines
  Bugs & Unexpected Behavior
  Commit Access
  Contributor Responsibilities
  Version Numbers & Compatibility
  Naming a Source Release
  Naming a Binary Release
  Making a Release
  Getting Help


GETTING STARTED
---------------

As there are many ways to get started with BRL-CAD, one of the most
important steps for new contributors to do is get involved in the
discussions and communicate with the BRL-CAD developers.  There are
mailing lists, on-line forums, and an IRC channel dedicated to BRL-CAD
development and project communication.  All contributors are
encouraged to participate in any of the available communication
channels:

* Internet Relay Chat

  The primary and generally preferred mechanism for interactive
  developer discussions is via Internet Relay Chat (IRC).  Several of
  the core developers and core contributors of BRL-CAD hang out in
  #brlcad on the Freenode network.  With most any IRC client, you
  should be able to join #brlcad on irc.freenode.net, port 6667.  See
  http://freenode.net and http://irchelp.org for more information

* E-mail Mailing Lists

  There are several mailing lists available for interaction, e.g. the
  http://sourceforge.net/mail/?group_id=105292 "brlcad-devel" mailing
  list.  More involved contributors may also be interested in joining
  the "brlcad-commits" and "brlcad-tracker" mailing lists.

* On-line Forums

  Discussion forums are available on the project site at
  http://sourceforge.net/forum/?group_id=105292 for both developers
  and users.  Of particular interest to developers is, of course, the
  "Developers" forum where all contributors are encouraged to
  participate.


HOW TO CONTRIBUTE
-----------------

BRL-CAD's open source management structure is best described as a
meritocracy.  Roughly stated, this basically means that the power to
make decisions lies directly with the individuals that have ability or
merit with respect to BRL-CAD.  An individual's ability and merit is
basically a function of their past and present contributions to the
project.  Those who constructively contribute, frequently interact,
and remain highly involved have more say than those who do not.

As BRL-CAD is comprised of a rather large code base with extensive
existing documentation and web resources, there are many many places
where one may begin to get involved with the project.  More than
likely, there is some new goal you already have in mind, be it a new
geometry converter, support for a different image type, a fix to some
bug, an update to existing documentation, a new web page, or something
else entirely.  Regardless of the goal or contribution, it is highly
encouraged that you interact with the existing developers and discuss
your intentions.  This is particularly important if you would like to
see your modification added to BRL-CAD and you do not yet have
contributor access.  When in doubt, working on resolving existing
bugs, improving performance, documentation, and writing tests are
perfect places to begin.

For many, it can be overwhelming at just how much there is.  To a
certain extent, you will need to familiarize yourself with the basic
existing infrastructure before significantly changing or adding a
completely new feature.  There is documentation available in the
source distribution's doc/ directory, throughout the source hierarchy
in manpages, on the website, and potentially in the documentation
tracker at http://sourceforge.net/docman/?group_id=105292 covering a
wide variety of topics.  Consult the existing documentation, sources,
and developers to become more familiar with what already exists.

See the PATCH SUBMISSION GUIDELINES section below for details on
preparing and providing contributions.


REFACTORING
-----------

proportion -> integrity -> clarity

Refactoring is one of the most useful activities a contributor can
make to BRL-CAD.  Code refactoring involves reviewing and rewriting
source code to be more maintainable through reduced complexity and
improved readability, structure, and extensibility.

For each source file in BRL-CAD, the following checklist applies:

* Consistent indentation.  See CODING STYLE & STANDARDS below.
  Indents every 4 characters, tab stops at 8 characters with BSD KNF
  indentation style.  The sh/indent.sh script will format a file
  automatically, but requires a manual review afterwards.

* Consistent whitespace.  See CODING STYLE & STANDARDS below, section
  on stylistic whitespace.

* Headers.  Only including headers that declare functions used by that
  file.  If system headers are required, then common.h should be the
  first header included.

* Comments.  All public functions are documented with doxygen
  comments.  Move public comments to the public header that declares
  the function.  Format block comments to column 70 with only one
  space (not tabs) after the asterisk.  Comments should explain why
  more than what.

* Magic numbers.  Eliminate constant numbers embedded in the code
  wherever feasible, instead preferring dynamic/unbounded allocation.

* Public symbols.  Public API symbols should be prefixed with the
  library that they belong to and declared in a public header.  Public
  symbols should consistently (only) use underscores, not CamelCase.

* Private symbols.  Private functions should be declared HIDDEN.

* Dead code.  Code that is commented out should be removed unless it
  serves a specific documentation purpose.

* Duplicate code.  Combine common functionality into a private
  function or new public API routine.  Once and only once.

* Verbose compilation warnings.  Quell them all.

* Globals.  Eliminate globals by pulling them into an appropriate
  scope and passing as parameters or embedding them in structures as
  data.

In addition, don't be afraid to rewrite code or throw away code that
"smells bad".  No code is sacred.  Perfection is achieved not when
there is nothing more to add but, rather, when there is nothing more
to take away.


SYSTEM ARCHITECTURE
-------------------

At a glance, BRL-CAD consists of about a dozen libraries and over 400
executable binaries.  The package has been designed from the ground up
adopting a UNIX methodology, providing many tools that may often be
used in harmony in order to complete a task at hand.  These tools
include geometry and image converters, image and signal processing
tools, various raytrace applications, geometry manipulators, and more.

One of the firm design intents of the architecture is to be as
cross-platform and portable as is realistically and reasonably
possible.  As such, BRL-CAD maintains support for many legacy systems
and devices provided that maintaining such support is not a
significant burden on developer resources.  Whether it is a burden or
not is of course a potentially subjective matter.  As a general
guideline, there needs to be a strong compelling motivation to
actually remove any functionality.  Code posterity, readability, and
complexity are generally not sufficient reasons.  This applies to
sections of code that are no longer being used, might not compile, or
might even have major issues (bugs).  This applies to bundled 3rd
party libraries, compilation environments, compiler support, and
language constructs.

In correlation with a long-standing heritage of support is a design
intent to maintain verifiable repeatable results throughout the
package, in particular in the raytrace library.  BRL-CAD includes
scripts that will compare a given compilation against the performance
of one of the very first systems to support BRL-CAD: a VAX 11/780
running BSD.  As the BRL-CAD Benchmark is a metric of the raytrace
application itself, the performance results are a very useful metric
for weighing the relative computational strength of a given platform.
The mathematically intensive computations exercise the processing
unit, system memory, various levels of data and instruction cache, the
operating system, and compiler optimization capabilities.

To support what has evolved to be a relatively large software package,
there are a variety of support libraries and interfaces that have
aided to encapsulate and simplify application programming.  At the
heart of BRL-CAD is a Constructive Solid Geometry (CSG) raytrace
library.  BRL-CAD has its own database format for storing geometry to
disk which includes both binary and text file format representations.
The raytrace library utilizes a suite of other libraries that provide
other basic application functionality.

LIBRARIES
---------

librt: The BRL-CAD Ray-Trace library is a performance and
accuracy-oriented ray intersection, geometric analysis, and geometry
representation library that supports a wide variety of geometric
forms.  Geometry can be grouped into combinations and regions using
CSG boolean operations.
	Depends on: libbn libbu libtcl libregex libm (openNURBS)

libbu: The BRL-CAD Utility library contains a wide variety of routines
for memory allocation, threading, string handling, argument support,
linked lists, and more.
	Depends on: libtcl (threading) (malloc)

libbn: The BRL-CAD Numerics library provides many floating-point math
manipulation routines for vector and matrix math, a polynomial
equation solver, noise functions, random number generators, complex
number support, and more as well.
	Depends on: libbu libtcl libm

libcursor: The cursor library is a lightweight cursor manipulation
library similar to curses but with less overhead.
	Depends on: termlib

libdm: The display manager library contains the logic for generalizing
a drawable context.  This includes the ability to output
drawing/plotting instructions to a variety of devices such as X11,
Postscript, OpenGL, plot files, text files, and more.  There is
generally structured order to data going to a display manager (like
the wireframe in the geometry editor).
	Depends on: librt libbn libbu libtcl libpng (X11)

libfb: The framebuffer library is an interface for managing a graphics
context that consists of pixel data.  This library supports multiple
devices directly, providing a generic device-independent method of
using a frame buffer or files containing frame buffer images.
	Depends on: libbu libpkg libtcl (X11) (OpenGL)

libfft: The Fast-Fourier Transform library is a signal processing
library for performing FFTs or inverse FFTs efficiently.
	Depends on: libm

libmultispectral: The multispectral optics library is a separate
compilation mode of the optics library where support for additional
bands of energy are added.  By providing a defined spectrum of energy,
the raytrace library will produce different signal analyses for a
given set of geometry.
	Depends on: librt libbn libbu libtcl

liboptical: The optical library is the basis for BRL-CAD's shaders.
This includes shaders such as the Phong and Cook-Torrence lighting
models, as well as various visual effects such as texturing, marble,
wood graining, air, gravel, grass, clouds, fire, and more.
	Depends on: librt libbn libbu libtcl

liborle: The "old" run-length encoding library that will decode and
encode University of Utah Raster Toolkit format Run-Length Encoded
data.
	Depends on nothing

libpkg: The "Package" library is a network communications library that
supports multiplexing and demultiplexing synchronous and asynchronous
messages across stream connections.  The library supports a
client-server communication model.
	Depends on: libbu

librtserver: The ray-trace server library is a thin interface over
librt that simplifies the process of applying individual ray
intersection tests against a given set of geometry.  The library
supports communication with a BRL-CAD geometry repository and
multithreaded ray dispatch.
	Depends on: librt libbn libbu libtcl
	Includes dependent libraries as part of this library

libsysv: The System 5 compatibility library exists to support legacy
systems that lack functionality required by BRL-CAD.  This library is
deprecated and should generally not be used for new development.
	Depends on nothing

libtclcad: The Tcl-CAD library is a thin interface that assists in the
binding of an image to a Tk graphics context.
	Depends on: libbn libbu libfb libtcl libtk

libtermio: The terminal I/O library is a TTY control library for
managing a terminal interface.
	Depends on nothing

libwdb: The "write database" library provides a simple interface for
the generation of BRL-CAD geometry files supporting a majority of the
various primitives available as well as combination/region support.
The library is a write-only interface (librt is necessary to read &
write) and is useful for procedural geometry.
	Depends on: librt libbn libbu

libbrlcad: This "conglomerate" library provides the core geometry
engine facilities in BRL-CAD by combining the numerics, ray-tracing,
and geometry database processing libraries into one library.
	Depends on: libwdb librt libbn libbu
	Includes dependent libraries as part of this library


FILESYSTEM ORGANIZATION
-----------------------

Included below is a sample (not comprehensive) of how some of the
sources are organized in the source distribution.  For the directories
under src/, see the src/README file for more details on subdirectories
not covered here.

  bench/
	The BRL-CAD Benchmark Suite
  db/
	Example geometry databases
  doc/
	Documentation
  include/
	Public headers
  m4/
	Autoconf compilation resource files
  misc/
	Anything not categorized or is sufficiently en masse
  pix/
	Sample raytrace images, includes benchmark reference images
  regress/
	Scripts and resources for regression testing
  sh/
	Utility scripts, used primarily by the build system
  src/
	Sources, see src/README for more details
  src/adrt
	Advanced Distributed Ray Tracer
  src/conv/
	Various geometry converters
  src/conv/iges/
	IGES converter
  src/fb/
	Tools for displaying data to or reading from framebuffers
  src/fbserv/
	Framebuffer server
  src/java/
	Java geometry server interface to librt
  src/libbn/
	BRL-CAD numerics library
  src/libbu
	BRL-CAD utility library
  src/libfb/
	BRL-CAD Framebuffer library
  src/libfft/
	Fast Fourier transform library
  src/libged/
	Geometry editing library
  src/libpkg/
	Network "package" library
  src/librt/
	BRL-CAD Ray-trace library
  src/libwdb/
	Write database library
  src/mged/
	Multi-device geometry editor
  src/other/
	External frameworks (Tcl/Tk, libpng, zlib, etc)
  src/proc-db/
	Procedural geometry tools, create models programmatically
  src/remrt/
	Distributed raytrace support
  src/rt/
	Raytracers, various
  src/util/
	Various image processing utilities


SOURCE CODE LANGUAGES
---------------------

The vast majority of BRL-CAD is written in ANSI C with the intent to
be strictly conformant to the C standard.  A majority of the MGED
geometry editor is written in a combination of C, Tcl/Tk, and Incr
Tcl/Tk.  The BRL-CAD Benchmark, build system, and utility scripts are
written in what should be POSIX-compliant Bourne Shell Script.  An
initial implementation of a BRL-CAD Geometry Server is written in PHP.

With release 7.0, BRL-CAD has moved forward and worked toward making
all of BRL-CAD C code conform strictly with the ANSI/ISO standard for
C language compilation (ISO/IEC 9899:1990, i.e. c89).  Support for
older compilers and older K&R-based system facilities is being
migrated to build system declarations, preprocessor defines, or being
removed outright.  It's okay to make modifications that assume
compiler conformance with the ANSI C standard (c89).

There is currently no C++ interface to the core BRL-CAD libraries.
There are a few tools and enhancements to libraries that are
implemented in C++ (the new BREP object type in librt, for example),
but there are presently no C++ bindings available beyond experimental
developments.  While C++ as an implementation language of new tools
and new library interfaces is not prohibited, the mixing of C++
semantics and C++ code (including simple // style comments) in the
existing C files is not allowed.  As new interfaces are developed, new
contributors become involved, and C++ code integration becomes more
prevalent, the contributor guidelines will become reinforced with more
details.


CODING STYLE & STANDARDS
------------------------

For anyone who plans on contributing code, the following conventions
should be followed.  Contributions that do not conform are likely to
be ridiculed and rejected until they do.  ;-)

Violations of these rules in the existing code are not excuses to
follow suit.  If code is seen that doesn't conform, it may and should
be fixed.

Code Organization:

Code that is potentially useful to another application, or that may be
abstracted in such a way that it is useful to other applications,
should be put in a library and not in application directories.

C files use the .c extension.  Header files use the .h extension.  C++
files use the .cpp extension.  PHP files use the .php extension.
Tcl/Tk files use the .tcl/.tk extensions.  POSIX Bourne-style shell
scripts use the .sh extension.  Perl files use the .pl (program) or
.pm (module) extensions.


Source files go into the src/ directory on the top level, into a
corresponding directory for the category of binary or library that it
belongs to.  Documentation files go into the doc/ directory on the top
level, with the exception of manual pages that should be colocated
with any corresponding source files.

Header files private to a library go into that library's directory.
Public header files go into the include/ directory on the top level.
Public header files should not include any headers that are private.
Headers should include any other headers that they require for correct
parsing (this is an on-going clean-up effort).  Public header files
should not include the common header.

Headers should be included in a particular order.  That order is
generally as follows:

  - any single "interface" header [optional]
  - the common header (unless the interface header includes it)
  - system headers
  - public headers
  - private headers

Applications may optionally provide an interface header that defines
common structures applicable to most or all files being compiled for
that application.  That interface header will generally be the first
file to be included, as it usually includes the common header and
system headers.  The common header should always be included before
any system header.  Standard C system headers should be included
before library system headers.  Headers should be written to be
self-contained, not requiring other headers to be necessarily included
before they may be used.  If another header is necessary for a header
to function correctly, it should include it.

Build System:

The CMake build system (more specifically, compilation test macros
defined in misc/CMake/BRLCAD_CheckFunctions.cmake) should be used 
extensively to test for availability of system services such as 
standard header files, available libraries, and data types.  No 
assumptions should be made regarding the availability of any 
particular header, function, datatype, or other resource.  After
running cmake, there will be an autogenerated include/brlcad_config.h 
file that contains many preprocessor directives and type declarations 
that may be used where needed.

Generic platform checks (e.g. #ifdef unix, #ifdef _WIN32) are highly
discouraged and should generally not be used.  Replace system checks
with tests for the actual facility being utilized instead.

The Windows platform utilizes its own manually-generated configure
results header (include/config_win.h) that has to be manually updated
if new tests are added to the CMake build logic.

Only the BRL-CAD sources should include and utilize the common.h
header.  They should not include brlcad_config.h or config_win.h
directly.  If used, the common.h header should be listed before any
system headers.

Language Compliance:

Features of C that conform to the ISO/IEC 9899-1990 C standard (C90)
are generally the baseline for strict language conformance.  As
BRL-CAD used to be K&R syntax conformant, there remains on-going
effort to ensure a full conversion to a standards compliant ISO C
implementation.

Code Conventions:

Globals variables, global structures, and other public data containers
are highly discouraged - globals are often the easy way around a hard
coding problem but more often complicate refactoring and restructuring
in the future.  Try to find another solution unless there is a very
compelling and good reason for their use.

Functions should always specify a return type, including functions
that return int or void.  ANSI C prototypes should be used to declare
functions, not K&R function prototypes.

There are several functions whose functionality are either wrapped or
implemented in a cross-platform manner by libbu.  This includes
functions related to memory allocation, command option parsing,
logging routines, and more.  The following functions and global
variables should be utilized instead of the standard C facility:

  bu_malloc() instead of malloc()
  bu_fgets() instead of fgets()
  bu_free() instead of free()
  bu_log() instead of printf()
  bu_bomb() instead of abort()
  bu_exit() instead of printf()+exit()
  bu_dirname() instead of dirname()
  bu_getopt() instead of getopt()
  bu_opterr instead of opterr
  bu_optind instead of optind
  bu_optopt instead of optopt
  bu_optarg instead of optarg
  bu_strlcat instead of strcat(), strncat(), and strlcat()
  bu_strlcpy instead of strcpy(), strncpy(), and strlcpy()
  bu_strcmp and BU_STR_EQUAL() instead of strcmp()

Similarly, ANSI C functions are preferred over the BSD and POSIX
interfaces.  The following functions should be used:

  memset instead of bzero
  memcpy instead of bcopy

The code should strive to achieve conformance with the GNU coding
standard with a few exceptions.  One such exception is NOT utilizing
the GNU indentation style, but instead utilizing the BSD KNF
indentation style which is basically the K&R indentation style with 4
character indents.  The following examples should be strictly adhered
to, if only for the sake of being consistent.

1) Indentation whitespace

Indents are 4 characters, tabs are 8 characters.  There should be an
emacs and vi local variables block setting at the end of each file to
adopt, enforce, and otherwise remind this convention.  The following
lines should be in all C source and header files at the end of the
file:

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */

In emacs, the the 'indent-region' command (bound to C-M-\ by default)
does a good job of making the needed changes to conform to this
convention.  Vi can be configured to respect the ex: modeline by
adding 'set modeline=1' to your .vimrc configuration file.  Microsoft
Visual Studio should have tabs size set to 8 and indent size set to 4
with tabs kept under Tools -> Options -> Text Editor -> C/C++ -> Tabs.
The exTabSettings project will also make MSVC conform by reading our
file footers.

A similar block can be used on source and script files in other
languages (such as Tcl, Shell, Perl, etc).  See the local variable
footer script in sh/footer.sh to automatically set/update files.

2) Stylistic whitespace

Space around operators and between multiple statements.
  a = (b + c) * d;                  /* ok */
  b = 2; c = 3;                     /* ok */

No space immediately inside parentheses.
  while (1) { ...                   /* ok */
  for (i = 0; i < max; i++) { ...   /* ok */
  while ( max ) { ...               /* discouraged */

Commas and semicolons are followed by whitespace.
  int main(int argc, char *argv[]); /* ok */
  for (i = 0; i < max; i++) { ...   /* ok */

No space on arrow operators.
  structure->member = 5;            /* ok */
  structure -> member = 5;          /* bad */

Native language statements (if, while, for, switch, and return)
have a separating space, functions do not.
  int my_function(int i);           /* ok, no space */
  while (argc--) ...                /* ok, has space */
  if( var == val )                  /* discouraged */
  switch(foo) ...                   /* discouraged */

Comments should have an interior space and be without tabs.
  /** good single-line doxygen */
  /* good */
  /*bad*/
  /*	discouraged */
  /*  discouraged  */
  /**
   * good:
   * multiple-line doxygen comment
   */

3) Braces

BRL-CAD uses the "The One True Brace Style" from BSD KNF and K&R.
Opening braces should be on the same line as their statement, closing
braces should line up with that same statement.  Functions, however,
are treated special and place the opening brace on a separate line.
See http://en.wikipedia.org/wiki/Indent_style for details.

  int some_function(char *j)
  {
      for (i = 0; i < 100; i++) {
          if (i % 10 == 0) {
              j += 1;
          } else {
              j -= 1;
          }
      }
  }

4) Names

Prefer mixed-case names over underscore-separated ones. (this one is
less important, as there are vast quantities of both in the code)
Variable names should start with a lowercase letter.
  double localVariable; /* ok */
  double LocalVariable; /* bad (looks like class or constructor) */
  double _localVar;     /* bad (looks like member variable)      */

Variables are not to be "decorated" to show their type (i.e. do not
use Hungarian notation or variations thereof) with a slight exception
for pointers on occasion.  The name should use a concise, meaningful
name that is not cryptic (typing a descriptive name is preferred over
someone else hunting down what was meant).
  char *name;    /* ok  */
  char *pName;   /* discouraged for new code, but okay */
  char *fooPtr;  /* bad */
  char *lpszFoo; /* bad */

Constants should be all upper-case with word boundaries optionally
separated by underscores.
  static const int MAX_READ = 2;  /* ok  */
  static const int arraySize = 8; /* bad */

5) Debugging

Compilation preprocessor defines should never change the size of
structures.
  struct Foo {
  #ifdef DEBUG_CODE  // bad
    int _magic;
  #endif
  };

6) Comments

"//" style comments are not allowed in C source files for portability.
Comment blocks should utilize an asterisk at the beginning of each new
line.  Doxygen comments should start on the second line unless it's a
succint /** single-line */ comment.

/* This is a
 * comment block.
 */

/**
 * This is a doxygen comment.
 */


DOCUMENTATION
-------------

BRL-CAD has extensive documentation in various formats and presently
maintained in various locations.  It is an on-going desire and goal of
the project to have all documentation located along with the source
code in our Subversion (SVN) repository.

In line with that goal and where beneficial, a large portion of the
tutorial documentation is being converted to the Docbook XML format.
Having the tutorial documentation in the Docbook XML format allows for
easier maintenances, better export conversion support, and
representation in a textual format that may be revision controlled and
tracked.

Documenting Source Code:

The source code should always be reasonably documented, this almost
goes without saying for any body of code that is to maintain some
longevity.  Determining just how much documentation is sufficient and
how much is too much is generally resolved over time, but it should
generally be looked at from the perspective of "If I look at this code
in a couple years from now, what would help me remember or understand
it better?"  and add documentation accordingly.

All public library functions and most private or application functions
should be appropriately documented using Doxygen/Javadoc style
comments.  Without getting into the advanced details, this minimally
means that you need to add an additional asterisk to a comment that
precedes your functions:

/**
 * Computes the answer to the meaning of life, the universe, and
 * everything.
 */
int the_answer()
{
    return 42;
}


TESTING
-------

The need for a more formal testing plan has become more of a pressing
issue as the size of BRL-CAD continues to grow.  As such, more
importance on various forms of testing is becoming necessary.

While specific plans for unit and black box testing are not yet
finalized, the Corredor Automation Test Suite will be used to assist
in performing cross-platform build tests, regression tests, and
various integration and operability tests.  The suite's home is at:

  http://sourceforge.net/projects/corredor/

A debug build may be specified via:

   cmake ../brlcad -DCMAKE_BUILD_TYPE=Debug

A profile build may be specified via:

   cmake ../brlcad -DBRLCAD-ENABLE_PROFILING=ON

Verbose compilation warnings may be enabled (by default these are on):

   cmake ../brlcad -DBRLCAD-ENABLE_COMPILER_WARNINGS=ON

To perform a run of the benchmark suite to validate raytrace behavior:

  make benchmark

To perform a run of the test suite:

  make regress

Note that individual programs may be tested after a build without
installing. Typically binaries will be found in the bin/ subdirectory
of the toplevel build directory, e.g.:

  ./bin/mged


PATCH SUBMISSION GUIDELINES
---------------------------

To contribute to BRL-CAD, begin by submitting modifications to the
patches section at http://sf.net/projects/brlcad/.  Patches in the
unified diff format (diff -u) are generally preferred when modifying
existing source or other text files.  Otherwise, contributors are
welcome to submit their changes in full to the patches tracker as
compressed file attachments.

All text patches should be submitted in the unified diff format where
it's feasible to create one either using SVN or using the unmodified
original file.  This is generally the "-u" option to diff, and is also
supported by the SVN diff command:

  svn diff > mychanges.patch

Where possible, patch files should be generated against the latest
sources available to make it easier to review and apply the changes.
If a modification involves the addition or removal of files, those
files can be provided separately with instructions on where they
belong (or what should be removed).  If SVN cannot be used, please
provide the complete release and build number of the files you worked
with.

ANY MODIFICATIONS THAT ARE PROVIDED MUST NOT MODIFY OR CONFLICT WITH
THE EXISTING "COPYING" FILE DISTRIBUTION REQUIREMENTS.  This means
that most modifications must be LGPL-compatible.  Contributors are
asked to only provide patches that may legally be incorporated into
BRL-CAD under the existing distribution provisions described in the
COPYING file.

Patches that are difficult to apply are difficult to accept.


BUGS & UNEXPECTED BEHAVIOR
--------------------------

When a bug or unexpected behavior is encountered that cannot be
quickly fixed, it minimally needs to be documented.  If you are
already a BRL-CAD contributor, this means either making a comment in
the BUGS file or by making a formal report to the SourceForge bug
tracker at:

http://sourceforge.net/tracker/?atid=640802&group_id=105292&func=browse

If the bug is complicated, will require significant discussion or
collaboration, cannot be fixed any time soon, or if you do not have
commit access, the issue should be reported to the SourceForge bug
tracker.  The issues listed in BUGS are not necessarily listed in bug
tracker, and vice-versa.  The BUGS file is merely maintained as a
convenience notepad of long and short term issues for the developers
only.


COMMIT ACCESS
-------------

BRL-CAD has a STABLE branch in SVN that should always compile and run
with expected behavior on all supported platforms.  Contrary to
STABLE, the SVN trunk is generally expected to compile but more
flexibility is allowed for resolution of cross-platform build issues
and on-going development.

Commit access is evaluated on a person-to-person basis at the
discretion of existing contributors, but generally readily granted so
long as there is an existing developer with commit access willing to
mentor the new developer.  If you would like to have commit access,
there is usually no need to ask -- they will ask you.  Getting
involved with the other contributors and making patches will generally
result in automatic consideration for commit access.  If you have to
ask for it, you probably won't get it.

Those with commit access have a responsibility to ensure that other
developers are following the guidelines that have been described in
this developer's guide within reasonable expectations.  A basic rule
of thumb is: don't break anything.

While the testing process will strive to ensure consistent behavior as
changes are made, the responsibility still remains with the individual
contributors to ensure that everything still compiles and runs as it
should.  At any given time, BRL-CAD should compile on all of the
primary platforms and (minimally) pass the BRL-CAD Benchmark
successfully.  If changes are made that are not exercised by the
Benchmark suite, testing should be manually performed in a manner that
exercises those particular changes.


CONTRIBUTOR RESPONSIBILITIES
----------------------------

Contributors of BRL-CAD that are granted commit access are expected to
uphold standards of conduct and adhere to conventions and procedures
outlines in this guide.  All contributors are directly supporting the
on-going development of BRL-CAD by being granted commit access.  As
such, these individuals are also considered "developers" whether they
are modifying source code or not, and have similar obligations and
expectations.  To summarize some of the expected responsibilities:

0) Primum non nocere.  All contributors are expected in good faith to
help, or at least to do no harm.  Be helpful and respectful.

1) Developers are expected to uphold the quality, functionality,
maintainability, and portability of the source code at all times.  In
part, this means that changes should be tested to avoid breaking the
build and short-term fixes are discouraged.  Committing code that is
actively being worked on is encouraged but care should be taken to
minimize impact on others and to respond quickly when an issue arises.

2) Bugs, typos, and compilation errors are to be expected as part of
the process of active software development and documentation, but it
is ultimately unacceptable to allow them to persist.  If it is
discovered that a recent modification introduces a new problem, such
as causing a compilation portability failure, then it is the
responsibility of the contributor that introduced the change to assist
in resolving the issue promptly.  It is the responsibility of all
developers to address issues as they are encountered regardless of who
introduces the problem.

3) Contributors making commits to the repository are required to
uphold the legal conventions and requirements summarized in the
COPYING file.  This includes the implicit assignment of copyright to
the U.S. Government on all contributed code unless otherwise
explicitly arranged as well as the usage of appropriate license
headers in all files where expected.  Contributors are also expected
to denote appropriate credits into the AUTHORS file when applying
contributed code and patches.

4) It is expected that developers will adhere to the coding style
conventions and filesystem organization outlined in this developer's
guide.  This includes the utilization of the specified coding style as
well as the prescribed K&R indentation convention.

5) Contributors are expected to communicate with other contributors
and to avoid code or file territorialism.  All contributors are
expected and encouraged to fix problems as they are found regardless
of where in the package they are located.  Care should of course be
taken to ensure that fixing in a bug in a section of code does not
introduce other issues, especially for unfamiliar code.  All
contributors are expect to communicate their changes publicly by
keeping documentation up-to-date, including making note of
user-visible changes in the NEWS file following the inscribed format
convention.


VERSION NUMBERS & COMPATIBILITY
-------------------------------

The BRL-CAD version number is in the include/conf directory in the
MAJOR, MINOR, and PATCH files.  The README, ChangeLog, NEWS files as
well as a variety of documents in the doc/ directory may also make
references to version numbers.  See the MAKING A RELEASE steps listed
below for a more concise list of what needs to be updated.

Starting with release 7.0.0, BRL-CAD has adopted a three-digit version
number convention for identifying and tracking future releases.  This
number follows a common convention where the three numbers represent:

	{MAJOR_VERSION}.{MINOR_VERSION}.{PATCH_VERSION}

All "development" builds use an odd number for the minor version.  All
"release" builds use an even number for the minor version.

The MAJOR_VERSION should only increment when it is deemed that a
significant amount of major changes have accumulated, new features
have been added, or enough significant backwards incompatibilities
were added to make a release warrant a major version increment.  In
general, releases of BRL-CAD that differ in the MAJOR_VERSION are not
considered compatible with each other.

The MINOR_VERSION is updated more frequently and serves the dual role
as previously mentioned of easily identifying a build as a public
release.  A minor version update is generally issued after significant
development activity (generally several months of activity) has been
tested and deemed sufficiently stable.

The PATCH_VERSION may and should be updated as frequently as is
necessary.  Every public maintenance release should increment the
patch version.  Every development version modification that is
backwards incompatible in some manner should increment the patch
version number.


NAMING A SOURCE RELEASE
-----------------------

In order to achieve some consistency when preparing a source release,
the following format should be used as the filename convention:

  brlcad-{VERSION}.{EXTENSION}

VERSION is the usual version triplet described above under the section
entitled VERSION NUMBERS & COMPATIBILITY.  Example: 7.12.4

EXTENSION is the filename extension.  Compressed tar files should use
the expanded form (i.e. not tgz, etc) unless otherwise dictated as
necessary convention by a source package management system.  Examples:

  tar.gz
  tar.bz2
  dmg
  exe
  zip


NAMING A BINARY RELEASE
-----------------------

In order to achieve some consistency when preparing a binary release,
the following format should be used as the filename convention:

  BRL-CAD_{VERSION}[_{OS}][_{VENDOR}][_{CPU}][_{NOTE}].{EXTENSION}

Notably, the filename should use 'BRL-CAD' instead of 'brlcad' for the
name, it should delimit sections of the filename with underscores
instead of spaces or dashes, and should always include the version
number and an extension for the file type.  The optional
_{OS}_{VENDOR}_{CPU} portion is a reverse (and simplified version) of
the GNU autotools config.guess canonical host triplet identifier.

VERSION is the usual version triplet described above under the section
entitled VERSION NUMBERS & COMPATIBILITY.  Example: 7.12.4

OS is an optional identifier for the target operating system for
binary distributions.  It should only be used if the file extension is
not already platform specific (e.g., .dmg already indicates Mac OS X,
.exe indicates Windows, etc).  The OS name can be the same as the GNU
autotools OS identifier without the version number.  If there are
other platform considerations like the version of libc or the version
of the OS that need to be called out, they can be included.  Examples:

  freebsd
  linux
  linux_glibc3
  solaris

VENDOR is an optional operating system qualifier that isn't necessary
for most platforms.  It's generally only useful for vendors that use
customized versions of common operating systems.  An example would be
something like SGI using a fairly customized variant of Linux on their
Altix line.  In that case, it's useful to include the vendor: sgi

CPU is the hardware architecture identifier.  It should be the first
identifier in the config.guess triplet or the lowercase hardware
identifier returned by uname -a (if any).  Examples:

  x86
  x86_64
  ia64
  sparc
  ppc
  power5
  mips
  mips_64

NOTE can be just about anything but should only be used when
absolutely necessary.  As an example, a note might be used to indicate
that a binary distribution is a partial or custom release.  Examples:

  dll
  benchmark

EXTENSION is the filename extension.  Compressed tar files should use
the expanded form (i.e. not tgz, etc) unless otherwise dictated as
necessary convention by a binary package management system.  Examples:

  tar.gz
  tar.bz2
  dmg
  exe
  zip


MAKING A RELEASE
----------------

BRL-CAD uses a monthly iteration development cycle.  If there have
been significant changes since the previous iteration, the
contributors are expected to "settle down" around the last couple days
of the month so that the source code may be verified, tested, and
released.  Major changes to the source code and documentation should
not occur during these last couple days of the iteration, only minor
bug and build fixes.  A release should then be made within the first
few days of the next month's iteration.  Repeat, rinse, recycle.

In addition to the monthly iteration release, additional releases may
be made on an as-needed basis.  This is particularly relevant for
emergency bug-fix releases and in response to security issues.  The
December/January timeframe is considered a single iteration with only
one iteration release necessarily being made in that timeframe.

When making a release, certain public repositories, mailing lists, and
news outlets require updating and notification.  Especially when the
major or minor version of BRL-CAD is going to be updated, it is very
important to follow and ensure that the following steps have been
successfully performed:

0) All code is committed to SVN.  Notify developer mailing list.

1) Verify that the NEWS file includes a comment for all relevant and
significant items added since the previous release.

2) All existing manual pages and the TODO file are updated with the
latest changes.

3) Update or verify the version numbers minimally in the include/conf
version files, NEWS, and README.  See the above VERSION NUMBERS &
COMPATIBILITY section.  Commit the changes to the source repository.

4) Update the ChangeLog with entries since the last release; commit
the updated ChangeLog.  Use the YYYY-MM-DD of the previous release.
Get the date from the NEWS file.

    svn2cl --break-before-msg --include-rev --stdout -r HEAD:{YYYY-MM-DD} > ChangeLog

5) Make sure default builds complete successfully on all platforms.

    cmake ../brlcad && make distcheck 

6) Sync trunk with the STABLE branch.  Make sure all changes since the
previous sync are included and that STABLE matches trunk.

    # review the log and obtain the last trunk merge revision number from comments
    svn log --stop-on-copy https://brlcad.svn.sourceforge.net/svnroot/brlcad/brlcad/branches/STABLE . | more
    export PREV=[[last_trunk_rev]]
	*** OR ***
    export PREV=`svn log --xml --stop-on-copy https://brlcad.svn.sourceforge.net/svnroot/brlcad/brlcad/branches/STABLE | grep 'revision=' | head -n 1 | sed 's/.*revision="\([0-9][0-9]*\)".*/\1/g'`
    echo "PREV=$PREV"

    # NOTE: if using a copy of the subversion repository checked out
    # using the abbreviation "sf" for sourceforge, the svn merge
    # command below also needs to use "sf" - subversion will not
    # automatically make the substitution.  The error corresponding
    # to this is:
    #   svn: 'https://brlcad.svn.sf.net/...' isn't in the same repository 
    #     as 'https://brlcad.svn.sourceforge.net/...'

    # merge that range of changes into STABLE and make sure it works
    svn co https://brlcad.svn.sourceforge.net/svnroot/brlcad/brlcad/branches/STABLE brlcad.STABLE && cd brlcad.STABLE
    svn merge https://brlcad.svn.sourceforge.net/svnroot/brlcad/brlcad/trunk@$PREV https://brlcad.svn.sourceforge.net/svnroot/brlcad/brlcad/trunk@HEAD .
    sh autogen.sh && ./configure --enable-all && make distcheck
    export CURR=`svn log --xml https://brlcad.svn.sourceforge.net/svnroot/brlcad/brlcad/trunk | grep 'revision=' | head -n 1 | sed 's/.*="\([0-9][0-9]*\)".*/\1/g'`
    echo "CURR=$CURR"
    svn commit -m "merging trunk to STABLE from r$PREV to HEAD r$CURR"

7) Tag the release using "rel-MAJOR-MINOR-PATCH" format:

    svn cp https://brlcad.svn.sf.net/svnroot/brlcad/brlcad/branches/STABLE https://brlcad.svn.sf.net/svnroot/brlcad/brlcad/tags/rel-MAJOR-MINOR-PATCH

8) Obtain a tagged version of the sources from the repository
(required to ensure the tagging and source distribution is correct):

    svn checkout https://brlcad.svn.sourceforge.net/svnroot/brlcad/brlcad/tags/rel-MAJOR-MINOR-PATCH brlcad-MAJOR.MINOR.PATCH

9) Perform another "make distcheck" on the exported/updated sources on
a system with the latest GNU Autotools installed to generate the
distributable compressed source tarballs.

10) Verify that the source distribution tarballs may be expanded; and
that they build, run, and that the file permissions are correct.
Minimally check:

    rt        # should report usage with correct library versions
    benchmark run TIMEFRAME=1            # completes without ERROR
    mged -c test.g "make sph sph ; rt"   # pops up a sphere
    rtwizard  # displays gui
    mged      # displays gui, run: opendb test2.g ; make sph sph ; rt

11) Use one of the source distribution tarballs to generate
platform-specific builds.  Use the NAMING A SOURCE RELEASE and the
NAMING A BINARY RELEASE sections above for naming source and binary
distribution files respectively.  Test the build: make regress && make
bench

12) Post the source and platform-specific binary distributions to
SourceForge and then select them on the sf.net File Release System:

SFUSERNAME=`ls ~/.subversion/auth/svn.simple/* | xargs -n 1 grep -A4 sourceforge | tail -1`
MAJOR=`cat include/conf/MAJOR`
MINOR=`cat include/conf/MINOR`
PATCH=`cat include/conf/PATCH`
echo "SFUSERNAME=$SFUSERNAME MAJOR=$MAJOR MINOR=$MINOR PATCH=$PATCH"
ssh -v $SFUSERNAME,brlcad@shell.sourceforge.net create
ssh -v $SFUSERNAME,brlcad@shell.sourceforge.net mkdir "/home/frs/project/b/br/brlcad/BRL-CAD\ Source/$MAJOR.$MINOR.$PATCH"
scp brlcad-$MAJOR.$MINOR.$PATCH* "$SFUSERNAME,brlcad@shell.sourceforge.net:/home/frs/project/b/br/brlcad/BRL-CAD\ Source/$MAJOR.$MINOR.$PATCH/."
ssh -v $SFUSERNAME,brlcad@shell.sourceforge.net shutdown

This can all be done through the sf.net project web interface too.

13) Increment and commit the next BRL-CAD release numbers to SVN;
update the include/conf/(MAJOR|MINOR|PATCH) version files immediately
to an odd-numbered minor version or a new patch developer version
(e.g. 7.11.0 or 7.4.1).  Update README and NEWS version to next
_expected_ release number (e.g. 7.12.0 or 7.4.2).

14) Announce the new release.

The NEWS file should generally be used as a basis for making release
announcements though the announcements almost always require
modification and customization tailored to the particular forum and
audience.  Always notify the following when a release is made:

      14.1) BRL-CAD Website
         http://brlcad.org/d/node/add/story

      14.2) BRL-CAD NEWS Mailing List
         brlcad-news@lists.sourceforge.net

      14.3) BRL-CAD Sourceforge NEWS
         https://sourceforge.net/news/submit.php?group_id=105292

      14.4) Freshmeat  (anyone can submit update)
         http://freshmeat.net/projects/BRL-CAD/
	 short summary without news details (sentence format)

If appropriate, notify and/or update the following information outlets
with the details of the new release:

Source or binary release:

      T2 package maintainer
      http://t2-project.org/packages/brlcad.html

      OpenSUSE package maintainer
      https://build.opensuse.org/package/users?package=brlcad&project=Education

      FreeBSD ports maintainer
      http://www.freebsd.org/cgi/cvsweb.cgi/ports/cad/brlcad/

      Gentoo portage maintainer
      http://packages.gentoo.org/package/sci-misc/brlcad

      Ubuntu/Debian .deb maintaner
      Jordi Sayol <g.sayol@yahoo.es>

      Debian apt package maintainer
      http://git.debian.org/?p=debian-science/packages/brlcad.git

      Slackware maintainer
      http://slackbuilds.org/result/?search=brlcad

Linux binary release:

      CAD on Linux mailing list
      cad-linux@freelists.org
	with news details

      Linux Softpedia
      http://linux.softpedia.com/get/Multimedia/Graphics/BRL-CAD-105.shtml

Mac OS X binary release:

      Versiontracker  (only 'brlcad' can update)
      http://www.versiontracker.com/dyn/moreinfo/macosx/26289
      http://www.versiontracker.com/dyn/moreinfo/win/64903
	short without news details (list format)

      Mac Softpedia (anyone can update)
      http://mac.softpedia.com/user/pad.shtml
      maceditor@softpedia.com
	(either provide doc/pad_file.xml or e-mail)
      http://mac.softpedia.com/get/Multimedia/BRLCAD.shtml

      Fink package maintainer
      jack@krass.com

Multiple platform major binary release:

      upFront.ezine
      Ralph Grabowski
      ralphg@upfrontezine.com
	with news details

      CADinfo.NET
      copyboy@cadinfo.net
	with news details

      TenLinks Daily
      http://www.tenlinks.com/NEWS/tl_daily/submit_news.htm
      news@tenlinks.com
	with news details

      Slashdot
      http://slashdot.org
	short without news details

      Government Computer News
      Joab Jackson
      jjackson@gcn.com
	short without news details

      DevMaster.net (if engine-related)
      http://www.devmaster.net/news/submit.php

      CADCAM Insider (plain text)
      http://cadcam-insider.com/index.php/News-submission-guidelines.html
      copyboy@cadcam-insider.com

16) Sit back and enjoy a beverage for a job well done.

Note:  If it should be necessary to build source or binary packages
in isolation, instead of within the distcheck framework, the following
two make targets are defined:

make package_source - Builds source archives
make package        - Builds binary packages

GETTING HELP
------------

See the GETTING STARTED section above.  Basically, communicate with
the existing developers.  There is an IRC channel, e-mail mailing
lists, and on-line forums dedicated to developer discussions.