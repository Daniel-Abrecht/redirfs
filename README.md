# redirfs

This program was mainly written because some programs log to logfiles
in their own logdirectory without using the system logger. This program
was written to make it possible to forward the logs to another file,
like /dev/null for example, using a simulated symlink, or to execute
a command whenever a file is opened and forward the logs to the standard
input of the command. In the latter case, the environment variable `FILE`
will be set to the name of the opened file.

## Usage

Redirecting output to a file (in this case /dev/null):
```
redirfs -o target=/dev/null mountpoint
```

Redirecting output to a program (in this case logger, which sends them to the system logger):
```
redirfs -o target='|logger' mountpoint
```

You can also set any fuse option you want. For example, you may want to set
`-o default_permissions` to enable the permission checks of the OS,
or `-o allow_other` to allow other users than the user you run it at to
use this file systrem.

