# xgb_recv

A high-speed 10 Gigabit Ethernet (10GbE) UDP packet receiver, written in C for the
[Center for Astronomy Signal Processing and Electronics Research](https://casper.berkeley.edu/)
(CASPER) at UC Berkeley.

## Overview

Many CASPER instruments produce bursts of data at 10Gbps. In 2008, the SATA II
bus theoretically maxed out at 3Gbps, and in practice even 10k RPM enterprise
drives could only sustain up to ~1Gbps -- an order of magnitude slower than the
10GbE network burst rate.

The ring buffer absorbs these bursts while disk I/O catches up. As long
as the average network data rate stays within disk write bandwidth, no data
is lost.

`xgb_recv` uses POSIX semaphore synchronization to move packets from the NIC
to disk without drops. The work is divided between two threads:

- **net thread**: reads packets from the NIC into ring buffer slots
- **hdd thread**: drains ring buffer slots to disk

Each ring buffer slot carries a pair of semaphores (`write_mutex` and
`read_mutex`) that enforce producer/consumer ordering without polling or
busy-waiting. The ring is implemented as a circular linked list of `RING_ITEM`
structs allocated in a single contiguous block.

## Files

| File | Description |
|------|-------------|
| `ring_buffer.c` / `.h` | Ring buffer data structure and semaphore logic |
| `xgb_recv.c` / `.h` | UDP socket listener, net/hdd thread functions |
| `ring_buffer_test.c` | Test harness for the ring buffer |
| `debug_macros.h` | Conditional debug printing macros |
| `Makefile` | Build configuration |

## Build

Requires GCC and pthreads (POSIX).

```
make
```

## History

This code was developed by William Mallard (`wjm@berkeley.edu`) in 2008-2009.
It originally lived in the CASPER Subversion repository at `casper.berkeley.edu`,
which is no longer accessible.
See [`SVN_PROVENANCE.txt`](SVN_PROVENANCE.txt) for metadata extracted from a
surviving SVN working copy, and [`SVN_COMMIT_LOG.txt`](SVN_COMMIT_LOG.txt) for
the full development history.

An independent mirror of the CASPER SVN exists at
[liuweiseu/casper_svn](https://github.com/liuweiseu/casper_svn/tree/master/projects/xgb_recv).

## Downstream use

The ring buffer and semaphore synchronization from this project were directly
incorporated into
[PySPEAD](https://github.com/ska-sa/PySPEAD), the reference implementation of
the SPEAD protocol used by the
[Square Kilometre Array](https://www.skatelescope.org/) telescope project
([commit b20360a](https://github.com/AaronParsons/PySPEAD/commit/b20360a4928485773a4e8d7bb27d0733e9e5c6e5); see [`diffs/`](diffs/) for a file-by-file comparison).

The network receiver was further adapted into PySPEAD's `buffer_socket` layer,
which slotted in SPEAD packet parsing, added a callback interface, and wrapped
it as a Python extension module.