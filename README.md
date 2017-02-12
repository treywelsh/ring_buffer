# Ring buffer

It's a basic implementation of a ring buffer.
The goal was to provide different level of control on the ring
with unsafe/safe operations regarding if the thread is empty/full.

If RINGB_SPSC_SAFE is defined at compile time it's thread safe regarding atomics from the C11 standard.

In order to reuse this data structure in an other project,
you will have to redefine the type of the stored data "rbuff_data_t".

NOTE: ringb_init( ... , 3) will initialize a ring buffer
with a capacity of 2^3 - 1 = 7 elements.
It's due to a choice of implementation to detect empty/full ring buffer.
