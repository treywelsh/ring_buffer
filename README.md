# Ring buffer

It's a basic non thread safe implementation of a ring buffer.
The goal was to provide different level of control on the ring
with unsafe/safe operations.

This implementation is mainly based on macros,
so it may not be easy to debug.

In order to reuse this data structure in an other project,
you will have to redefine the type of the stored data "rbuff_data_t".

NOTE: rbuff_init( ... , 3) will initialize a ring buffer
with a capacity of 2^3 - 1 = 7 elements.
It's due to a choice of implementation to detect empty/full ring buffer.
