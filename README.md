ipc-benchmark
=============

Kevin Pulo, MongoDB Labs

Linux IPC benchmarking framework, inspired by [detailyang/ipc_benchmark](https://github.com/detailyang/ipc_benchmark).


Compiling
---------

```
make clean
make
```


Running
-------

- Manual benchmark run:

    ```
    $ ./main
    Usage: ./main <size> <target_runtime_us> <num_mangle> [<method> <method> ...]
    ```

    - `<size>` is the size in bytes of the message/buffer.  Will be rounded up to the nearest multiple of 4 bytes.
    - `<target_runtime_us>` is how long each method should aim to run for.  The method will be run for 10 iterations as a warm-up, and the duration of this used to compute how many iterations should be performed to obtain a runtime of (approximately) `target_runtime_us`.
    - `<num_mangle>` is the number of "mangle" operations to perform as the workload, each of which is a 32-bit integer increment.
    - `<method>` is one of the below IPC Method names.  If omitted, all compiled-in methods will be run.

    Output is to stdout, and also detailed run info is appended to the `./stats` file.


- Automated set of benchmark runs:

    ```
    $ ./tests-quick-1
    ```

    This script will run each of the methods configured in it, for the configured combinations of `size` + `target_runtime_us`.


- Basic results processing:

    ```
    $ ./stats-separate <stats-file> [<stats-file> ...]
    ```

    For each given `<stats-file>`, create a file `<stats-file>.<method>` (for each `<method>` in the original `<stats_file>`).  Each file contains the space-separated columns: `size`, `num_mangle`, `mb_sec`, `msgs_sec`.


- Improved results processing:

    ```
    $ ./stats-combined <stats-file> [<stats-file> ...]
    ```

    For each given `<stats-file>`, create a file `<stats-file>.combined`.  The file contains tables of tab-separated columns, collecting the MB/s and msgs/s results for all methods, grouped first by each `num_mangle` value (and then each `size` value within that), and then grouped by each `size` value (and then each `num_mangle` value within that).  The tab-separated output is also copied to the clipboard (for each pasting into a spreadsheet to create charts), while a space-expanded version is output to stdout.


IPC Methods
-----------

- `null`: No communication, just apply the workload to the buffer.  This simulates an in-process function call (passing by reference).
- `nullcopy`: Like `null`, but copy to/from a second buffer before applying the workload.  This simulates an in-process function call (passing by value).
- `pipe`: Use `read(2)`/`write(2)` on anonymous unix pipes.
- `shm`: Shared memory using SysV IPC shm segments.
- `mmap`: Shared memory using `mmap(2)` on a file in `/dev/shm` (ie. `tmpfs`).
- `mmapanon`: Shared memory using `mmap(2)` with `MMAP_ANONYMOUS`.

- `pipesplice`: An attempt at single-copy page handover using the `vmsplice(2)` syscall (with `SPLICE_F_GIFT`) instead of `write(2)`.  Gives similar performance to `pipe`, so unlikely to be actually working.
- `pipesplice`: Similar to `pipesplice`, but attempting to also use `splice(2)` syscall (with `SPLICE_F_MOVE`) instead of `read(2)`.  Also gives similar performance to `pipe`, so unlikely to be actually working.

- `pipe1`: Like `pipe`, but unidirectional.  Outdated, unmaintained, and likely no longer works properly.
- `fifo`: Use `read(2)`/`write(2)` on named pipes/FIFO special files.  Never properly implemented.
- `fifo1`: Intended to be a unidirectional version of `fifo`, but also never properly implemented.

