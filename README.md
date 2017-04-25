# srs-dolphin

SRS-Dolphin is a multiple process solution for [SRS](https://github.com/ossrs/srs).

It's a TCP proxy over [ST](https://github.com/ossrs/state-threads), which is deprecated.
Recommend to use:

1. [GO-Oryx](https://github.com/ossrs/go-oryx), support multiple processes, SRS++.
1. [SO_REUSEPORT](https://github.com/ossrs/srs/issues/775), directly start multiple SRS to listen at the same port.

For users who want to get srs-dophin code, please checkout to branch [feature/deprecated](https://github.com/ossrs/srs-dolphin/tree/feature/deprecated).

Winlin
