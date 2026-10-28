# Changelog

## V2.6.0

### dymon_srv: readable errors when started via a Windows shortcut

When `dymon_srv` was started with a double-click or a desktop shortcut and
failed (e.g. no USB DYMO found), its console window closed immediately and
the error could not be read. On Windows, it now waits for a key press after
an error if it owns its console window. Started from an existing console,
it exits right away as before.

Invalid command line arguments (e.g. a typo like `--prot 8090`) are no
longer silently ignored: `dymon_srv` prints the error and the usage and
exits with -1.

A second instance on the same port is now refused with
`Can't listen to port … (already in use?)` (exit code -3). Previously,
both instances could bind the port silently. `Start listening to port …`
is now printed only after the port was bound successfully.

### Fix: unwanted left margin on LabelWriter 450 (#17)

The DYMO driver changes the dot tab (`ESC B`, a left offset) per raster line,
and the printer keeps the last value until it is reset or power-cycled. A
dymon job printed after a DYMO driver job therefore inherited that offset
and printed with a left margin. dymon now sends `ESC Q 0 0` and `ESC B 0` at
the start of every job on LW450-class printers, as the DYMO driver does.

### Web UI: 57x32 mm label format

The labelwriter web UI (also served by `dymon_srv` at `/`) offers the
57x32 mm label format.

### Addon: testpattern

New addon to generate PBM test labels.

## V2.5.1

### Fix: wait for LabelWriter 550 to become ready (#16)

After power-on or waking from standby, a LabelWriter 550 is briefly not ready
(status byte 0 = 4 or 5). dymon sent the print data anyway; the printer
discarded it but still fed a blank label, and the job reported success.
`start()` now polls the status every 250 ms until the printer is ready and
fails with error -5 after about 5 s, closing the connection without feeding
a label.

`dymon_pbm` now also reflects print errors in its exit code instead of only
the result of connecting.

### Fix: skewed output for PBM widths not a multiple of 8 (#15)

Bitmaps whose width is not a multiple of 8 (e.g. 295 px for 25 mm labels)
printed as a skewed, striped label. The width sent to the printer is now
rounded up to the padded row length (LW 550/Wireless and LW 450), and PBM
padding bits are cleared so they cannot print as a black column. Bitmaps no
longer need to be padded to a multiple of 8 by the caller.

### Fix: dymon_pbm crashing with SIGPIPE after a successful print (#14)

When the printer closed the network connection right after a job (seen on
the LabelWriter 550 Turbo), sending the final form-feed killed `dymon_pbm`
with SIGPIPE (exit code 141) although the label had printed. Sends now fail
with EPIPE instead of raising a signal (Linux and macOS).
