# pipebuf
Measure size of POSIX pipe buffers

## Summary

This small package provides two utilities:

 * _pipebuf-constants_: retrieve the maximum number of bytes
   that are guaranteed to be atomic when writing to a pipe, i.e.
   `PIPE_BUF` and `_PC_PIPE_BUF`, and print them.
 * _measure-pipebuf_: measure the actual capacity of a pipe
   buffer which usually exceeds the value returned by _pipebuf-constants_.

## Pipe buffers

The pipe system call was introduced in very early releases of UNIX.
In the 6th Edition of UNIX a pipe buffer was represented by an inode
on the root device with a fixed size of 4096 bytes. (See sheet 77 in
John Lions' source code listing.)

The POSIX standard does not provide any function to retrieve the
actual size of a pipe buffer. Instead, the standard focuses on
the maximum number of bytes that are guaranteed to be written
atomically into a pipe buffer. This has a minimal value of 512.
The actual limit can be retrieved using the `PIPE_BUF` macro from
`<limits.h>`, if defined, or using _fpathconf_ on an actual
file descriptor that is associated with a pipe. The _pipebuf-constants_
utility prints these constants. The same limit is also reported
by `ulimit -p` in 512 kb blocks.

However, the actual size of a pipe buffer possibly exceeds that
of `PIPE_BUF` or `_PC_PIPE_BUF`. The second utility, _measure-pipebuf_,
executes series of tests measuring

 * which amount of bytes can be written into a pipe without being
   blocked and
 * be successfully read subsequently.

As kernel implementations tend to use not just one buffer for
a pipe but a series of buffers, the total capacity of a pipe buffer
is not necessarily a power of two. _measure-pipebuf_ checks first
powers of two and continues with intermediate size checks until the
exact maximum is found.

## Some results

| Operating system | `PIPE_BUF` | pipe buffer size
| ---------------- | ---------- | ----------------
| Darwin 13.4.0    |        512 |            65536
| Linux 3.16.0     |       4096 |            65536
| Linux 4.4.59     |       4096 |            65536
| Solaris 10       |       5120 |            20480
| Solaris 11.3     |       5120 |            25599

## Related discussions

The same question arised at stackexchange multiple times, some
answers even coming with scripts that test various pipe buffer
sizes (usually powers of 2):

 * In [this discussion](https://unix.stackexchange.com/questions/11946/how-big-is-the-pipe-buffer)
   Chris Johnson not just presents a script but also explains that

   > Mac OS X [..] uses a capacity of 16384 bytes by default,
   > but can switch to 65336 [sic] byte capacities if large writes
   > are made to the pipe, or will switch to a capacity of a single
   > system page if too much kernel memory is already being used by
   > pipe buffers [..].

 * Linux allows to increase the pipe buffer size by using
   the `F_SETPIPE_SZ` option for _fcntl_ as explained by
   Jeremy Fishman in [this posting](https://stackoverflow.com/questions/5218741/set-pipe-buffer-size). According to him this was introduced
   in Linux 2.6.35.

The important point of these discussions is that the
pipe buffer size is not necessarily fixed. There are systems
that allow the size to be dynamically changed and other systems
may possible downsize pipe buffers if memory resources become scarce.
The [POSIX standard in regard to write](http://pubs.opengroup.org/onlinepubs/9699919799/functions/write.html)
notes in its rationale:

> The concept of a `{PIPE_MAX}` limit (indicating the maximum number
> of bytes that can be written to a pipe in a single operation) was
> considered, but rejected, because this concept would unnecessarily
> limit application writing.
