# Swish-Shell
My implementation of a command line shell interface named Swish

Command Line Shell all comes together to create the executable swish file.
Running swish will allow the user to access many different functionalities.

These include:
	- execution of runnable commands
	- piping ablility using |
	- IO Redirection using <, >, >>
	- History storage and recall
	- Bang using ! and a command number or prefix
	- Bang using !! to call the last command run
	- History Navigation using arrow keys
	- Autocompletion of command (but be careful it can fail)


The included file:
	- shell.c - the main shell
	- history.c - stores history data
	- ui.c - controls the user interface

All of these combine to give the user a dynamic shell :)
