# Changelog

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
