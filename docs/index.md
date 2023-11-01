# Background

The current assignment involves the creation of a (simple, so far) depth-limited grammar fuzzer that can be called reasonably performant. This task might be divided into two equally important sub-tasks:

1. Developing a performant grammar data structure
2. Developing a performant fuzzer function, preferably grammar structure-agnostic

# Entry Point

Choosing an entry point for documenting the challenge of interest, I'll begin at the point at which the fuzzer [repository](https://github.com/Positron11/fuzzer) first diverged into the [string fuzzer](https://github.com/Positron11/fuzzer/blob/master/fuzzer.c) and the [token fuzzer](https://github.com/Positron11/fuzzer/blob/alernate-grammar/fuzzer.c).

## The Grammar

My first attempt at developing a grammar structure naively imitated BNF grammar, ie. non-terminal tokens were represented as strings: `<token>`, and rules were therefore represented by C strings. The most apparent way to optimize rule lookups was by turning the grammar into a hashmap.

## The Fuzzer

The fuzzer itself (iterative), after I'd overcome the initial hurdle of not knowing C, ended up being performant up to a depth of $10^5$. I'd also begun profiling [^1] at this point, and the primary source of overhead at this point was `strcat`. Discovering Schlemiel the Painter's Algorithm in reference to the same led to the development of a custom concatenation function (individually copying bytes via walking pointers) which boosted performance by $\approx 3000\%$ up to a maximum performant depth of $10^6$. <sup>commit [c390831](https://github.com/Positron11/fuzzer/commit/c390831b9e10fb5ee50d4d2bf2fe5e6c81f500f0)</sup>

## Alternative Grammar

At this point, I was advised to compare my grammar structure to a token-based grammar, where non-terminals would be represented by negative integers directly corresponding with the index of their respective definitions in the grammar structure (not a hashmap this time).

# Surprising Results

Inserting the new token grammar into the existing fuzzer algorithm had surprising results. The initial expectation was naturally that the token fuzzer would beat the string fuzzer, given the lack of string operations which were required to extract the initial segment in the string grammar fuzzer. Instead, the maximum performant depth dropped to $10^4$.

## Slight Improvements

Initial profiling showed that the append function carried over from the previous was the major offender in terms of overhead. Changing the manual byte-copying mechanism to `memcpy` led to a notable performance improvement, and was only 6 times slower at a depth of $10^4$, as compared to 100 times previously <sup>commit [60bb03d](https://github.com/Positron11/fuzzer/commit/60bb03dc47388e0d298270b2b5b7184e37ecf26b)</sup>. This change was subsequently ported over to the string fuzzer, improving performance slightly.

Subsequent profiling showed that all functions defined by me no longer had any significant overhead. However, the results were even more extreme than the previous round of profiling in that a new symbol `__memmove_avx_unaligned_erms` now accounted for $\approx 97\%$ of overhead.

## Alternate Fuzzer

At this point I was advised to develop recursive versions of both the token fuzzer and the string fuzzer.

# Recursive Insights

To briefly summarize the results herein, each variant ordered by performance looked like so:

$$\text{recursive token} \gt \text{iterative string} \gg \text{recursive string} \gg \text{iterative token}$$

This demonstrated an important point - the iterative version of the string fuzzer being faster than the recursive version meant the token fuzzer was absolutely under-performing, and due to no fault of the the grammar itself (in how it was constructed, at least).

I did also note that default stack sizes were being overrun by the program at depths $\gt 10^4$, so al considered the recursive string grammar might well be abandoned at this point.

# Returning to Iterative

By this point, I'd begun to take efforts to increase the code parity between both iterative variants as much as possible. This being done, the confusion could be narrowed down to the fact that the string fuzzer had about twice as many calls to `memcpy` and yet ran about 100 times faster.

Closer inspection of the trends in the benchmarking results of the iterative fuzzers ($\text{recursion depth} \in [0,10^5]$) revealed a few important points in the correlation between the `L1D` cache and performance.

## Graph Interpretation

The first observation was that while the string fuzzer maintained essentially a flat line for cache misses vs. recursion depth throughout, the token fuzzer was flat up to a depth of $4 \times 10^3$ and then suddenly shot up in a steep, rough linear increase.

Plotting performance for both (execution time vs. recursion depth) showed a very gradual linear increase for the string fuzzer and a polynomial increase for the token fuzzer. An interesting observation here was that the token fuzzer started out as being more performant than the string fuzzer but quickly crossed over, the crossover point and the shape of both graphs bearing an not-entirely vague correspondence to the cache misses graph.

## Uneventful Improvements

I attempted allocating the stack and buffer on the heap, changing the token symbol type in the token fuzzer from `int` to `signed char`, both of which had minimal to no effect.

Strangely, putting the `APPEND` and `OVERWRITE` macro functions (the source of calls to `memcpy`) into regular functions did not make them show up in perf record, which still maintained an $\approx 99\%$ overhead for `__memmove_avx_unaligned_erms`.

Annotating `__memmove_avx_unaligned_erms` showed that $\approx 90\%$ of the samples were recorded on `rep movsb`, where I would have expected larger byte blocks (ie. `vmovdqu` etc.).

# Finding a Solution

Having still only a vague idea of what might be causing cache misses (or, to be honest, what cache misses even are), I attempted the next thing I could think of to reduce the amount of data being moved around in each cycle of the fuzzer - a prepend mechanism to replace copying to/from a buffer <sup>commit [5ef8dd7](https://github.com/Positron11/fuzzer/commit/5ef8dd771223df9ae0042c2ffbf964f2c715c777)</sup>. 

This has had the immediate effect of slingshotting the iterative token fuzzer to first place, such that: 

$$\text{iterative token} \gt \text{recursive token} \gt \text{iterative string} \gg \text{recursive string}$$

[^1]: All profiling unless otherwise specified was carried out using `perf stat` and `perf record`.
