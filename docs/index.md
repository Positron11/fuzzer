<script type="text/javascript" id="MathJax-script" async src="https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-mml-chtml.js"></script>

<script>window.MathJax = { tex: { inlineMath: [['$', '$'], ['\\(', '\\)']] } };</script>

# Starting Out

The current assignment involves the creation of a simple depth-limited grammar fuzzer that can be called reasonably performant. This task might be divided into two equally important sub-tasks:

1. Developing a performant grammar data structure.
2. Developing a performant fuzzer function, preferably grammar structure-agnostic.

I have two things to go off at present:

1. Although it's probably much easier to write a recursive fuzzer at this stage, I want to start out iterative - common wisdom and for performance reasons down the line.
2. I'll want to implement a depth-limiting mechanism, ie. `min_depth` and `max_depth` parameters. Again, easier to implement in a recursive variant, but I believe I've an idea.

# Initial Implementation `[V0.0]`

I've created the base for what what will become the fuzzer in coming iterations <sup>[[gist](https://gist.github.com/Positron11/86c9c0c98d16ad1f35019a4bd3582ef5/4e13563e1152122d79fb98dabe5e8f8cb611d8f3)]</sup>. At this point, the code is fully "functional", but that's generous. Rather heavy use of pointers doesn't seem quite right, given how convoluted some function definitions, return types in particular, have become (what even is `char *(*getopt(struct g_entry grammar[], char key[]))[]`). Performance is abysmal - the program outright crashes the IDE at a minimum recursion depth of 20.

## The Grammar

```c
struct g_entry grammar[GRAMSIZE] = 
{ 
	(struct g_entry { .key=STARTKEY, .rcrsv=0, .optcnt=1, .options={ 
		{ "~phn" } 
	} },
	(struct g_entry) { .key="~phn", .rcrsv=0, .optcnt=2, .options={ 
		{ "~arc", " ", "~@num", "-", "~@num" }, 
		{ "~@num", "-", "~@num" } 
	} },
	(struct g_entry) { .key="~arc", .rcrsv=0, .optcnt=1, .options={ 
		{ "(+", "~dgt", "~dgt", ")" } 
	} },
	(struct g_entry) { .key="~@num", .rcrsv=1, .optcnt=2, .options={ 
		{ "~dgt" }, { "~@num", "~dgt" } 
	} },
	(struct g_entry) { .key="~dgt", .rcrsv=0, .optcnt=10, .options={ 
		{ "0" }, { "1" }, { "2" }, { "3" }, { "4" }, 
		{ "5" }, { "6" }, { "7" }, { "8" }, { "9" } 
	} }
};
```

The grammar is defined as seen above, and consists of a few key components:

- `grammar`: a collection of `g_entry` structures

- `g_entry`: a structure defining a grammar entry (a.k.a definition)
  - `.key`: a unique string identifier 
  - `.rcsv` and `.optcnt`: hold meta information about the entry's recursive nature and the number of unique options (a.k.a rules) the entry contains
  - `.options`: an array which contains options, ie. arrays of tokens represented by strings. 

_**Retrospect:** breaking a string into substrings consisting of single characters is an... interesting approach - It's striking how close to yet how so far I was from the final grammar structure)._ 

## The Fuzzer

The fuzzer algorithm, or at least how it plays out in my head, is now described. The way it's actually implemented in the code at the moment is too convoluted to go into here - the reader is free to expose themselves to the source at their own leisure.

### Expansion Algorithm

1. The fuzzer declares an expansion stack (initialized to the `start` token), a buffer (which just contains everything in the expansion stack except the first token), and an output store. 
2. The fuzzer evaluates the first token in the stack.
3. i. If the token is terminal, it is written to the output store and the stack is overwritten with the buffer.
   ii. Otherwise if the token is nonterminal, a random option with the appropriate cost is found and overwritten onto the stack, which is then appended by the buffer
4. This repeats until the expansion stack is empty.

### Depth Limiting Mechanism

Depth limiting is done by the use of a simple `depthlock` token - when a recursive token is encountered, a `depthlock` token is appended to the recursive token's expansion before the buffer is fnally tacked on (step _3i_ above). A depth counter is then incremented each iteration and checked against limits until the previously inserted `depthlock` is again encountered, at which point all recursion limit variables are reset.

### Expansion Method Analysis

I've rather diverged from my reference [^ifuzrf] in the expansion/parsing method. The reference presents a simple fuzzer and its shortcomings:

1. Searches the string for tokens every iteration (inefficient as production string grows)
2. No control over recursion depth

and then goes on to present an algorithm that explicitly constructs a derivation tree in the fuzzing process, and then converts the tree to an output string.

However, I believe my approach combats the shortcomings identified while also being able to work directly on the production string (expansion stack). Although it seems linear at first glance, a visual abstraction will show that my algorithm is analogous to a depth-first pre-order tree traversal implementation.

A _very_ cursory time complexity analysis of the expansion algorithm itself puts the big-O time complexity at:

$$\lt \text{O}\left(\frac{k^{n + 1} - 1}{n - 1}\right)$$

Where $k$ is the largest expansion (most number of nonterminals, assumed recursive) and $n$ is the `max_depth`. This is almost always likely an overestimation.

## Performance (Retrospective)

Maximum performant depth: $\approx 10^3$. [^prfmdpt]

I was yet to begin profiling anything at this point, but graphing task-clock against recursion depth produced the following:

![V0.0 fuzzer: task clock (msec) vs. recursion depth](graphs/v0.0-tc_d.png)

## To Do Items

A to-do item identified, albeit probably a "late future" one, is implementing support for grammar options - recursion limits for individual rules and expansion probabilities being among the more significant.

# Slight Improvements `[V0.1]`

Being well aware of the complete inadequacy of the previous fuzzer, I've decided to make a few changes <sup>[[gist](https://gist.github.com/Positron11/86c9c0c98d16ad1f35019a4bd3582ef5/a42cefcbf8f52b06bbd35cb3e8937f3d1e519ada)]</sup>. 

_**Note:** somewhere among these changes is something that causes the printed output of the program to go completely haywire - sometimes._ 

## ... To the Grammar

The way tokens are stored (single tokens aggregated into a string array) seemed exceedingly wasteful, as well as difficult to read and maintain. The major change made here is that expansions are now represented as full strings:

```c
&(Rule) { .key="<phone>", .expansions={
	(char *[]) { "<area> <number>-<number>", "<number>-<number>", NULL }
} },

// [...]

&(Rule) { .key="<number>", .expansions={
	(char *[]) { "<digit>", NULL }, 
	(char *[]) { "<digit><number>", NULL }
} },
```

The tokens are now represented in the BNF format (`<nonterminal>`). `g_entry` has been renamed to `Rule` and simplified. `.options` has been renamed to `.expansions` which is now comprised of two sub-arrays of string expansions where the first sub-array contains only cheap (non-recursive expansions) and the second contains only costly (recursive) expansions. I could get therefore get rid of `.rcrsv` (can just check if `grammar.expansions[1]` exists) and `.optcount` (no longer applicable)

Expansions being strings does of course introduce some computational overhead in the form of the string parsing that now needs to be done to extract tokens.

_**Retrospect:** renamed pretty much everything, but everything's still incorrectly named, nice going_

## ... To the Fuzzer

The fuzzer is a lot cleaner as a result of built-in string functions, and also now first sets the desired expansion cost before retrieving a random expansion, instead of poking around until it happens to find a recursive expansion, as it did previously.

## Performance (Retrospective)

Maximum performant depth: $\approx 10^4$.

Again, being yet to begin profiling anything at this point, graphing task-clock against recursion depth, with `V0.0` included for reference produced the following:

![V0.1, V0.0 fuzzer: task clock (msec) vs. recursion depth](graphs/v0.1_v0.0-tc_d.png)

# A Somewhat Solid Base `[V1.0]`

This iteration is mostly improvements to the code and minor optimizations, in addition to one major change to the grammar structure <sup>[[gist](https://gist.github.com/Positron11/86c9c0c98d16ad1f35019a4bd3582ef5/9a55a09f2cf21b3b564cec1bf93c039772fd61a5)]</sup>. 

The primary improvement is with regard to the grammar lookup Given I've got keys stored as strings, manually scanning the array doesn't seem the most efficient way of doing this. I've attempted to fix this using a hashtable for the grammar structure, which I did with [uthash](https://troydhanson.github.io/uthash/):

```c
Rule start = { .key=START_TOKEN, .expansion_count={1,0}, .expansions={ 
	(char const*[]) {"<phone>"},
} };
Rule phone = { .key="<phone>", .expansion_count={2,0}, .expansions={ 
	(char const*[]) {"<area><number>-<number>", "<number>-<number>"},
} };	

// [...]
	
HASH_ADD_INT(grammar, key, &start);
HASH_ADD_INT(grammar, key, &phone);

// [...]
```

Data is now statically allocated where possible so as to keep it mostly on the stack, variables are `const` qualified and `unsigned` where possible to assist the compiler in optimization, an `enum` replaces the depth lock macros, and token search uses the built-in `strcspn` instead of the slower custom `get_token` function:

```c
// before
int get_token(char (*stack)[], char (*token)[], size_t token_size) {
	int index = 0;
	for (size_t i = 0; (*stack)[i] != '>'; i++) index++;
	slice(*stack, *token, token_size, 0, ++index);
	return index;
}

// now
SLICE(token, stack, 0, strcspn(stack, ">") + 1);
```

I've also cut down the number of secondary functions from 8 to 2 and one function-like macro, inlining one-off functionality where possible (`is_nonterminal`, `get_token`, `is_recursive` etc.), and my string operations are a great deal more reliable. The pointer-walking of arrays and relying on null pointers to demarcate string and array iterations are also entirely gone.

## Performance (Retrospective)

Maximum performant depth: $\approx 10^5$.

For the last time, being yet to begin profiling anything at this point, graphing task-clock against recursion depth, with `V0.1` included for reference produced the following:

![V1.0, V0.1 fuzzer: task clock (msec) vs. recursion depth](graphs/v1.0_v0.1-tc_d.png)

# String Operations Optimization `[V1.1~s]`

Cursory profiling [^profmtd] to the extent I'm able to comprehend the results of which showed that the majority of execution overhead came from string functions (`strcat` in particular) <sup>[[logfile](logfiles/v1.0.data)]</sup>, which suggests either a change to the grammar or the concatenation function.

A brief inquiry into how `strcat` works brought up _Schlemiel the Painter's Algorithm_, and I decided to implement a custom concatenation function using pointers to keep state, that would pass only once over the length of the source array <sup>[commit: [c390831](https://github.com/Positron11/fuzzer/commit/c390831b9e10fb5ee50d4d2bf2fe5e6c81f500f0)]</sup> :

```c
char* append(char* dest, char const* src) {
     while (*dest) dest++;
     while (*dest++ = *src++);
     return --dest;
}
```

The resulting improvements were again significant:

![V1.1~s, V1.0 fuzzer: task clock (msec) vs. recursion depth](graphs/v1.1~s_v1.0-tc_d.png)

by my own criteria, I _could_ state a maximum performant depth of $\approx 10^6$, but it doesn't feel fast enough at that depth to justify it.

# A Token Grammar (Pt. 1)

It's been suggested by my supervising professor that I consider a token-based grammar.

## The New Grammar

```c
enum special {
	start = START_TOKEN, 
	phone, 
	area, 
	number, 
	digit,
};

Grammar grammar = { .def_count=5, .definitions=(Definition []) {
	(Definition) { .name="start", .rule_count={1, 0}, .rules={
		(Rule []) {
			(Rule) { .token_count=1, .tokens=(int const[]) {phone} }
		}
	} },
	(Definition) { .name="phone", .rule_count={2, 0}, .rules={
		(Rule []) {
			(Rule) { .token_count=3, .tokens=(int const[]){number, '-', number} },
			(Rule) { .token_count=4, .tokens=(int const[]){area, number, '-', number} }
		}
	} },
	(Definition) { .name="area", .rule_count={1, 0}, .rules={
		(Rule []) {
			(Rule) { .token_count=5, .tokens=(int const[]){'(', '+', digit, digit, ')'} }
		}
	} },
	(Definition) { .name="number", .rule_count={1, 1}, .rules={
		(Rule []) {
			(Rule) { .token_count=1, .tokens=(int const[]){digit} },
		},
		(Rule []) {
			(Rule) { .token_count=2, .tokens=(int const[]){number, digit} }
		}
	} },
	(Definition) { .name="digit", .rule_count={10, 0}, .rules={
		(Rule []) {
			(Rule) { .token_count=1, .tokens=(int const[]){'0'} },
			(Rule) { .token_count=1, .tokens=(int const[]){'1'} },
			(Rule) { .token_count=1, .tokens=(int const[]){'2'} },
			(Rule) { .token_count=1, .tokens=(int const[]){'3'} },
			(Rule) { .token_count=1, .tokens=(int const[]){'4'} },
			(Rule) { .token_count=1, .tokens=(int const[]){'5'} },
			(Rule) { .token_count=1, .tokens=(int const[]){'6'} },
			(Rule) { .token_count=1, .tokens=(int const[]){'7'} },
			(Rule) { .token_count=1, .tokens=(int const[]){'8'} },
			(Rule) { .token_count=1, .tokens=(int const[]){'9'} },
		}
	} }
} };
```

The token grammar is constructed as shown above. You will notice that the names of the component structures are finally correct. The grammar consists of the following:

- `grammar`: a `Grammar` structure instance
  - `.def_count`: keeps count of the number of definitions contained within the grammar
  - `.definitions`: array of `Definition` structures

- `Definition`: structure that defines a set of rules for a given non-terminal token
  - `.name`: rather pointless, actually
  - `.rule_count`: `size_t[2]` array where:
    - `.rule_count[0]`: number of non-recursive (cheap) rules
    - `.rule_count[1]`: number of recursive (costly) rules
  - `.rules`: array of `Rule` structures

- `Rule`: the actual expansion of a given non-terminal token
  - `.token_count`: count of how many individual tokens the rule contains 
- `.tokens`: `int[]` array of tokens, where non-terminal tokens are represented by a negative number corresponding to their index in `grammar.definitions`, and terminal tokens by their ASCII code (represented as single characters in the code itself).

## Surprising Results `V1.1~t`

Inserting the new token grammar into the existing fuzzer algorithm has surprising results. The initial expectation is naturally that the token fuzzer would beat the string fuzzer (`V1.1~s`), given the lack of expensive string operations which were required to extract the initial segment in the string grammar fuzzer. Instead, the maximum performant depth has dropped to $10^4$.

## Slight Improvements `V1.2~t`

Initial profiling shows that the append function carried over from `V1.1~s` was the major offender in terms of overhead. Changing the manual byte-copying mechanism to `memcpy` leads to a notable performance improvement, and is only 6 times slower at a depth of $10^4$, as compared to 100 times previously <sup>[commit: [60bb03d](https://github.com/Positron11/fuzzer/commit/60bb03dc47388e0d298270b2b5b7184e37ecf26b)]</sup>. I've ported this change over to the string fuzzer <sup>[commit: [105cde1](https://github.com/Positron11/fuzzer/commit/105cde102a7891ac6450624e2d8a21d3be483480)]</sup>, improving performance slightly (`V1.2~s`).

Subsequent profiling shows that none of the functions I've defined have any significant overhead anymore. However, the results are even more extreme than the previous round of profiling in that a new symbol `__memmove_avx_unaligned_erms` now accounts for $\approx 97\%$ of overhead <sup>[[logfile](logfiles/v1.2~t.data)]</sup>.

# Recursive Interlude

I've been advised by my supervising professor to develop recursive versions of both the string fuzzer <sup>[commit: [704560a](https://github.com/Positron11/fuzzer/commit/704560a83056b06e67cb825953a64827ca7345a0)]</sup> and the token fuzzer <sup>[commit: [cb6f133](https://github.com/Positron11/fuzzer/commit/cb6f133707380b349881950ec1608d38b6fe986d)]</sup>.

## Recursive Insights

To briefly summarize the results herein, each variant ordered by performance looked like so:

$$\text{recursive token} \gt \text{iterative string} \gg \text{recursive string} \gg \text{iterative token}$$

This, to me, demonstrates an important point - the iterative string fuzzer being faster than the recursive version meant the token fuzzer is absolutely under-performing, and due to no fault of the the grammar itself (in how it's constructed, at least).

Less importantly, I note that default stack sizes are being overrun by the program at $\text{depth} \gt 10^4$, so all considered the recursive string grammar might well be abandoned.

# A Token Grammar (Pt. 2)

I've take efforts to increase the code parity between `v1.2~s` and `v1.2~t` as much as possible, which are now at almost complete parity. I'll be maintaining this parity to whatever degree possible going forward. This being done, the confusion can be narrowed down to the fact that the string fuzzer has about twice as many calls to `memcpy` and yet runs about 100x faster.

Closer inspection of the trends in the benchmarking results of the fuzzers ($\text{recursion depth} \in [0,10^5]$) reveals a few important points in the correlation between cache misses and performance.

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

[^ifuzrf]: https://www.fuzzingbook.org/html/GrammarFuzzer.html

[^profmtd]: All profiling unless otherwise specified was carried out using `perf stat` and `perf record`.

[^prfmdpt]: Subjective value, basically the order of $10$ at which $\text{execution time} \lt 1\text{s}$