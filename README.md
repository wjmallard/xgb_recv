# xgb_recv

A high-speed 10 Gigabit Ethernet (10GbE) UDP packet receiver, written in C for the [Center for Astronomy Signal Processing and Electronics Research](https://casper.berkeley.edu/) (CASPER) at UC Berkeley.

## Motivation

Many CASPER instruments produce bursts of data at 10Gbps. At the time (2008), the SATA II bus theoretically maxed out at 3Gbps, and in practice even the fastest 10k RPM enterprise drives could only sustain up to ~1Gbps — an order of magnitude slower than the 10GbE line rate.

This ring buffer absorbs network bursts while disk I/O catches up. As long as the average network data rate stays within disk write bandwidth, no data is lost.

## Design

`xgb_recv` uses POSIX mutex/condvar synchronization to move packets from the NIC to disk without drops. The work is divided between two threads:

- **net thread**: reads packets from the NIC into ring buffer slots
- **hdd thread**: drains ring buffer slots to disk

The ring buffer is implemented as a circular linked list. Each slot carries its own mutex and condition variable, so the producer and consumer only block when the ring is full or empty, and there is no polling. Network payloads are stored in a single contiguous block of memory, and each slot points to its payload.

```
               ┌────────────────────────────────────────┐
               │                                        │
               ▼                                        │
            ┌────┐   ┌────┐   ┌────┐   ┌────┐   ┌────┐  │
   slots:   │ s0 │──>│ s1 │──>│ s2 │──>│ s3 │──>│ s4 │──┘
            └──┬─┘   └──┬─┘   └──┬─┘   └──┬─┘   └──┬─┘
               │        │        │        │        │
               ▼        ▼        ▼        ▼        ▼
           ┌────────┬────────┬────────┬────────┬────────┐
payloads:  │   p0   │   p1   │   p2   │   p3   │   p4   │
           └────────┴────────┴────────┴────────┴────────┘
```

A third **vis thread** draws real-time ring buffer occupancy stats on stderr.

## Files

| Path | Description |
|------|-------------|
| `src/ring_buffer.c` | Ring buffer data structure and mutex/condvar logic |
| `src/xgb_recv.c` | UDP socket listener, net/hdd/vis thread functions |
| `src/pkt_gen.c` | Packet generator for load testing |
| `src/ring_buffer_test.c` | Test harness for the ring buffer |
| `include/` | Header files |
| `doc/` | SVN history and provenance |
| `Makefile` | Build configuration |

## Build

Requires a C11 compiler and pthreads.

```
make          # builds xgb_recv
make all      # also builds pkt_gen and ring_buffer_test
```

Configure `MAX_PAYLOAD_SIZE`, `LISTEN_PORT`, and other settings at the top of `src/xgb_recv.c` before building.

## History

This code was developed by William Mallard (`wjm@berkeley.edu`) in 2008-2009. It originally lived in the CASPER Subversion repository at `casper.berkeley.edu`, which is no longer accessible. See [`doc/SVN_PROVENANCE.txt`](doc/SVN_PROVENANCE.txt) for metadata extracted from a surviving SVN working copy, and [`doc/SVN_COMMIT_LOG.txt`](doc/SVN_COMMIT_LOG.txt) for the full development history.

An independent mirror of the CASPER SVN exists at [liuweiseu/casper_svn](https://github.com/liuweiseu/casper_svn/tree/master/projects/xgb_recv).

## Downstream use

The ring buffer and semaphore synchronization from an early version of this project ([`casper-svn-r2051`](https://github.com/wjmallard/xgb_recv/tree/casper-svn-r2051)) were directly incorporated into [PySPEAD](https://github.com/ska-sa/PySPEAD), the reference implementation of the SPEAD protocol used by the [Square Kilometre Array](https://www.skatelescope.org/) telescope project ([commit b20360a](https://github.com/AaronParsons/PySPEAD/commit/b20360a4928485773a4e8d7bb27d0733e9e5c6e5); see [`doc/diffs/`](doc/diffs/) for a file-by-file comparison).

The network receiver was further adapted into PySPEAD's `buffer_socket` layer, which slotted in SPEAD packet parsing, added a callback interface, and wrapped it as a Python extension module.
