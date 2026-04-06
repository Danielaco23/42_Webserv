*This project has been created as part of the 42 curriculum by dgomez-l, pmendez- and owmarqui.*

# Description

This project is composed by four types of files:

- **".cpp" files**.
	From now on referenced as **source files**.

- **".hpp" files**.
	From now on referenced as **header files**.

- **".o" files**.
	These files are only present during and after compilation.
	From now on referenced as **object files**.

- **"webserv" file**.
	This is the main and only executable file, present only after successful compilation.
	From now on referenced as **executable file** or **exec file**.

# Instructions

This section runs through what's needed to know before using/testing this project.

## Compilation

In order to compile this project, a Makefile is provided.

The available Makefile methods are as follows:

- Make all
	Target by default, will compile the entire project.
	This rule will also run when only executing *Make*.

- Make [.o file]
	Will compile the specified object file in the proper "objects" folder.

- Make clean
	Will clean all compilation files except the executable by deleting the "objects" folder.

- Make fclean
	Will return the project to a pre-compilation state, deleting the "objects" folder alongside the executable file.

- Make re
	Will execute *Make clean* and, immediately after, *Make all*, for a clean **re**-compilation of all files, while also re-instating all header files for all source files.

- Make leaks
	This is not a required target, and its intended use is for debugging and testing purposes.
	Will execute *Make all*, clear the console screen for best debugging experience, and run the command ***"valgrind --leak-check=full --show_leak_kinds=all ./webserv"***.
	This command launches the *valgrind* debugger, attaching it to the program born from the exec file, and, essentially, testing for leaks during the execution.
	In doing so, generates a convenient leaks report at the end of the program's execution.

This project's Makefile automates the compilation process by first compiling all source files into object files, and then compiles them into the final and ready executable file.

The reason behind this two-step compilation is to prevent relinking, since, due to how the Makefile is structured, it will check the date of the source files with their corresponding object files, and will only recompile those files that have suffered any changes since last compilation, so long as all object files are present.

Makefile can't compare with deleted files, and will always mark them as outdated if not present.

# Resources

- [GNU make (makefile) documentation](https://www.gnu.org/software/make/manual/)
	- [Relinking and how to avoid it](https://people.cs.pitt.edu/~znati/Courses/CogNet/related/makeintro.html)

- https://www.w3schools.com/cpp/cpp_templates.asp

- So far, as far as dgomez-l's apportations, AI was **not** used in the development of this project.

# Documentation

- Practical guide for HTTP + CGI + uploads:
	- [docs/GUIA_HTTP_CGI_UPLOADS_PMENDEZ.md](docs/GUIA_HTTP_CGI_UPLOADS_PMENDEZ.md)
	- [docs/GUIA_CGI_RUTAS_PMENDEZ.md](docs/GUIA_CGI_RUTAS_PMENDEZ.md)
