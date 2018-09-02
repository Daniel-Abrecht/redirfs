# redirfs

This program was mainly written because some programs log to logfiles
in their own log directory without using the system logger. This program
was written to make it possible to forward the logs to another file,
like /dev/null for example, using a simulated symlink, or to execute
a command whenever a file is opened and forward the logs to the standard
input of the command. In the latter case, the environment variable `FILE`
will be set to the name of the opened file.

## Usage

Redirecting output to a file (in this case /dev/null):
```
redirfs /dev/null mountpoint
```

Redirecting output to a program (in this case logger, which sends them to the system logger):
```
redirfs '|logger' mountpoint
```

You can also set any fuse option you want. For example, you may want to set
`-o default_permissions` to enable the permission checks of the OS,
or `-o allow_other` to allow other users than the user you run it at to
use this file systrem.

It is possible to mount redirfs using fstab. For this, you need to copy `redirfs` to `/sbin/mount.redirfs`.
Make sure to escape spaces in paths with \040. If you want to send everything written to `/var/log/something`
to the `logger` program, the fstab entry could look as follows:
```
|logger  /var/log/something/  redirfs  allow_other  0 0
```

If only users in group daemon should be able to write to files in the mounted directory, you can use the gid and default_permissions options:
```
|logger  /var/log/something/  redirfs  gid=users,allow_other,default_permissions  0 0
```

And the same example with some more command line arguments and escaping for logging a few additional information:
```
|logger\040--rfc5424\040--sd-id\040redirfs@1\040--sd-param\040dir=\"something\"\040--sd-param\040file="\"$FILE\""
  /mnt/test1  redirfs  allow_other  0 0
```

This is equivalent to the command line:
```
sudo mount.redirfs '|logger --rfc5424 --sd-id redirfs@1 --sd-param dir=\"something\" --sd-param file="\"$FILE\""' -o allow_other /var/log/something/
```

You can use any command which works with sh, so even using pipes and so on will work.

== Limitations ==

At the moment, it isn't possible to create subdirectories in a redirfs file system, its not yet possible to mount a single file.
