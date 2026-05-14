# C Backend Optimization Roadmap

This roadmap tracks the work needed to make the C backend produce compact,
fast C for large Fortran applications such as tblite. The current split-C path
can compile and run the MnO2 tblite case with good energy parity against a
gfortran build, but it still emits code that performs far more work and uses
far more memory than a native Fortran compiler.

## Current Baseline

MnO2, single-threaded, same input:

| Metric | LFortran C backend | gfortran | Gap |
| --- | ---: | ---: | ---: |
| Total energy | 1.3832952100495 Eh | 1.3832952102334 Eh | 1.84e-10 Eh |
| Electronic energy | -14.618552105641 Eh | -14.618552105457 Eh | 1.84e-10 Eh |
| Wall time | 14.39 s | 1.15 s | 12.5x slower |
| Program-reported time | 14.343 s | 0.741 s | 19.4x slower |
| Max RSS | 9.18 GB | 11.1 MB | 825x higher |
| Instructions | 279 B | 15.6 B | 17.9x more |

The parity gap is now small for this case. The remaining problem is generated
code quality: the C backend is materializing too many Fortran array semantics
as heap descriptors, array copies, and generic runtime calls.

## Current Compile Budget Workflow

Use `ci/report_c_emit_budget.py` on a C-emitted build directory or directly on
one or more `*.o.tmp.split` directories before accepting further C-backend
compile-size changes:

```bash
python3 ci/report_c_emit_budget.py /path/to/tblite-build --top 25 \
  --json /path/to/report.json
```

The report records:

- generated `.c` file count, bytes, and lines
- generated header count, bytes, and lines
- largest generated C translation units and headers
- helper/scaffolding pattern counts
- longest `.ninja_log` compile edges when Ninja timing data is present
- whether per-file RSS data is available

Current measured tblite C-emitted artifact:

| Metric | Value |
| --- | ---: |
| split directories | 398 |
| generated `.c` files | 1486 |
| generated `.c` size | 91.19 MiB |
| generated `.c` lines | 984067 |
| generated headers | 398 |
| generated header size | 2.74 MiB |
| largest C TU | `mstore/amino20x4` at 2.59 MiB |
| longest Ninja edge | `tblite/mesh/lebedev.f90.o` at 101.576 s |

Observed top contributors:

- static molecule/reference-data builders emit many local `static const`
  numeric arrays inside generated procedures
- descriptor and temporary scaffolding remains high, with 265740
  `__libasr_created__` occurrences and 14152 array-view occurrences in the
  measured C files
- the existing compact constant-copy helper path did not appear in this older
  tblite artifact, so the next validation must use freshly generated C before
  judging large-constant coverage

## Active Action List

1. Done: add generated-C budget reporting.
2. Done: hoist large immutable local and module-scope parameter-array data into
   split constants-data translation units when the data is fixed-size and has a
   typed C initializer. This keeps the procedure/shared TUs from carrying the
   initializer payload and adds no runtime copy or floating-point reordering.
3. Done: emit large immutable numeric parameter-array payloads as binary data
   files with typed `extern const` declarations and assembler `.incbin`
   definitions. This removes the C frontend's decimal initializer parse work
   for those payloads while preserving the typed read-only object ABI.
4. In progress: deduplicate emitted helpers/scaffolding by ownership and
   structural signature. Current checkpoints prune unreachable split helper
   bodies and make type-bound parent metadata owner-emitted with downstream
   force-link references. Remaining helper families still need structural
   analysis before backend changes.
5. Later: broaden no-copy descriptor/view lowering.
6. Always: avoid math/codegen transformations that reorder floating-point
   operations.

Current blocker: route-level compile cost is still dominated by generated C
volume and descriptor/module-body scaffolding, not by constant initializer
text. The binary-data path removes about 3.4 MB of generated `.c` from the
fresh tblite build, but the build is still dominated by large non-constant TUs
such as `tblite/mesh/lebedev.f90.o`, `mstore/amino20x4`, `multicharge/eeqbc`,
`tblite/coulomb/multipole`, and `tblite/xtb/calculator`. The next high-value
work should reduce real emitted scaffolding in those module bodies, especially
descriptor/view setup, generated temporaries, helper fan-out, and repeated
metadata paths. Do not chase build-policy-only changes unless they remove
emitted work or artifact bloat.

## Experiment Log

### 2026-05-14: Binary Data For Large Immutable Constants

Hypothesis: after hoisting large immutable parameter arrays into split
constants-data TUs, the remaining parse/typecheck work comes from decimal C
initializer text. Emitting the payload as binary/object data with typed
`extern const` declarations should reduce C source volume without changing
arithmetic order, runtime initialization, or descriptor semantics.

Exact change: split-C now records raw ASR array-constant bytes for integer,
unsigned integer, real, and complex parameter arrays. Supported large constant
arrays are emitted as `__lfortran_const_data_blob_*.bin` files plus a small
constants-data C file that declares the typed `extern const T name[N]` object
and defines it with GNU/Clang-style assembler `.incbin`. Logical arrays keep
the typed C initializer path because C `bool` does not have the same storage
contract as LFortran logical kinds. Unsupported cases fall back to the existing
typed C initializer path.

Expected win: less generated `.c` text, less Clang frontend work for large
lookup tables, smaller object payloads for constants-heavy files, and no
runtime copy. This is a compile/artifact optimization; it should not change
numeric behavior.

Correctness risk: low for current gcc/clang split-C targets because the typed
C declaration still describes the object used by generated code and the byte
payload comes from the same ASR constant storage order used by the old C
initializer. The portability caveat is explicit: this path currently requires
GNU-style inline assembly and uses absolute `.incbin` paths in generated split
packages, so MSVC and package relocation need a later object-file or CMake
embedding path.

Test evidence:

- Red check: after changing the focused regression to require `.incbin`, the
  old backend emitted no `.incbin` in the generated split C.
- `cmake --build build -j`
- `PATH=build/src/bin:$PATH python3 ./run_tests.py -t c_backend_large_parameter_array_data_unit_01`
- `PATH=build/src/bin:$PATH python3 ./run_tests.py -t c_backend_large_logical_parameter_array_data_unit_01`
- `PATH=build/src/bin:$PATH python3 ./run_tests.py -t c_backend_split_helper_reachability_01`
- `PATH=build/src/bin:$PATH python3 ./run_tests.py -t c_backend_tbp_parent_registration_use_01`
- `python3 -m py_compile run_tests.py tests/c_split_budget.py ci/report_c_emit_budget.py`
- `python3 ci/report_c_emit_budget.py --self-test`
- `lfortran --backend=c --show-c-split <pkg> tests/c_backend_large_parameter_array_data_unit_01.f90`,
  followed by CMake configure, build, and execution of the generated package
- Fresh no-tests tblite LFortran-C build with the same flags as the previous
  helper-graph baseline
- Focused ammonia CLI parity against the fresh gfortran build

Focused regression result: the old constants-data TU was 13905 bytes of typed
initializer text. The new generated package emits three 1600-byte binary
payloads and a 4365-byte constants-data C wrapper. The split package builds
and runs.

Fresh tblite route metrics:

| Metric | Previous LFortran C | Binary-data LFortran C | gfortran |
| --- | ---: | ---: | ---: |
| Build wall | 141.616 s | 145.558 s | 14.213 s |
| Build max RSS | 2225.54 MB | 2222.72 MB | 248.92 MB |
| Generated `.c` | 1499 files, 95,878,233 bytes | 1499 files, 92,475,440 bytes | n/a |
| Binary constant blobs | 0 generated const blobs | 244 files, 2,741,296 bytes | n/a |
| Generated headers | 398 files, 2,876,948 bytes | 398 files, 2,876,948 bytes | n/a |
| Ninja `.o` edge sum | 749.114 s | 775.319 s | n/a |
| Object bytes | 52,798,939 | 51,999,075 | 9,962,296 |
| `libtblite.a` | 8,608,248 bytes | 8,608,656 bytes | 3,075,144 bytes |
| `app/tblite` | 8,879,256 bytes | 8,879,592 bytes | 5,763,992 bytes |
| `__TEXT` segment | 8,011,776 bytes | 5,505,024 bytes | n/a |
| `__text` | 4,755,304 bytes | 4,755,304 bytes | 1,893,604 bytes |

Fresh binary-data budget:

| Metric | Value |
| --- | ---: |
| split directories | 398 |
| generated `.c` size | 88.19 MiB |
| generated `.c` lines | 992,518 |
| binary constant blobs | 244 files, 2.61 MiB |
| `.incbin` references | 488 |
| longest Ninja edge | `tblite/mesh/lebedev.f90.o` at 100.290 s |
| largest C TU | `mstore/amino20x4` at 2.72 MB |

Focused ammonia CLI parity:

| Metric | LFortran C | gfortran |
| --- | ---: | ---: |
| JSON max numeric diff | 1.42e-14 at `energy` | reference |
| JSON nonnumeric mismatches | 0 | reference |
| Warm CLI wall median | 93.26 ms | 47.63 ms |
| Warm CLI RSS median | 11.08 MB | 13.32 MB |

Result: keep, with revised expectations. The patch does exactly remove real C
initializer text and slightly reduces object bytes, without changing runtime
numeric behavior in the focused tblite smoke. It does not reduce full tblite
build wall time in this run because the remaining dominant work is generated
module-body C and descriptor/helper scaffolding, not constant literal parsing.
The next compile-volume win should target the largest non-constant TUs and
helper/view emission; the next portability revision should replace absolute
`.incbin` paths with a relocatable object-data emission path.

### 2026-05-13: Large Immutable Parameter Data Hoist

Hypothesis: large immutable parameter arrays should live in a split
constants-data TU instead of the generated procedure/shared TU that references
them. This reduces the largest active translation units and cannot change
numeric order because the generated code still reads the same typed const data.

Exact change: for split-C only, local fixed-size `parameter` arrays and
module-scope fixed-size `parameter` arrays with large typed initializers are
registered as `extern const __lfortran_const_data_*[...]` declarations and
defined once in `<unit>_constants_data.c`. The local/module descriptor wrapper
points directly at that data; no runtime allocation or copy is introduced.

Expected win: smaller generated procedure/shared TUs, less repeated C frontend
work in files that also contain executable code, and cleaner budget visibility
through the new `typed_const_data_units` count.

Correctness risk: low for numeric parity, because there is no arithmetic
rewrite and no data copy. The remaining risk is C linkage/type spelling for
extern typed arrays, covered by split-C compile/run tests.

Test evidence:

- `cmake --build build -j`
- `python3 -m py_compile ci/report_c_emit_budget.py`
- `python3 ci/report_c_emit_budget.py --self-test`
- `PATH=build/src/bin:$PATH python3 run_tests.py -t c_backend_large_parameter_array_data_unit_01 -s`
- `PATH=build/src/bin:$PATH python3 run_tests.py -t c_backend_header_only_module_01 -s`
- `PATH=build/src/bin:$PATH python3 run_tests.py -t c_backend_specialized_pass_array_no_copy_01 -s`

Focused generated-C budget for the new regression:

| Metric | Value |
| --- | ---: |
| split directories | 1 |
| generated `.c` files | 4 |
| generated `.c` size | 17355 bytes |
| generated headers | 1 |
| largest C TU | `main_constants_data.c` at 13905 bytes |
| procedure TU size | 1637 bytes |
| module data TU size | 826 bytes |
| typed const data occurrences | 12 |
| malloc/realloc/memcpy calls | 0 |

Result: keep. The focused regression proves both local and module-scope large
parameter arrays are no longer emitted as inline `static const double` data in
the referencing TUs, and the C-emitted run passes. This is a compile-structure
win, not a full literal-parse elimination; the latter is tracked as a separate
binary/object-data design problem.

### 2026-05-13: Loaded-Module Type-Parent Registration Ownership

Hypothesis: type-bound parent-registration constructors should be emitted by
the object that owns the defining module. Consumer objects that load the module
from `.mod` need the type declarations, but re-emitting the constructor repeats
compile-time scaffolding and startup work.

Exact change: split-C aggregate declaration collection now suppresses
type-bound parent-registration emission for `m_loaded_from_mod` modules, using
the same ownership rule already applied to struct cleanup/runtime-info
registration. Current-module child types still emit their own parent
registration.

Expected win: fewer generated constructor bodies and force-link declarations in
consumer split objects. This is a no-arithmetic change and does not touch
floating-point execution.

Correctness risk: low when the module object is linked, because the defining
module still emits the registration. The focused test compiles the module and a
separate consumer to catch accidental consumer-side re-emission.

Test evidence:

- Red check before the backend change: the consumer split `main_shared.c`
  contained `_lfortran_register_c_type_parent(...)`.
- `cmake --build build -j`
- `PATH=build/src/bin:$PATH python3 run_tests.py -t c_backend_tbp_parent_registration_use_01 -s`
- `PATH=build/src/bin:$PATH lfortran --backend=c --separate-compilation tests/c_backend_tbp_parent_registration_mod_01.f90 tests/c_backend_tbp_parent_registration_use_01.f90`
- Existing split-C checks rerun:
  `c_backend_large_parameter_array_data_unit_01`,
  `c_backend_header_only_module_01`, and
  `c_backend_specialized_pass_array_no_copy_01`

Focused generated-C budget after the change:

| Metric | Value |
| --- | ---: |
| split directories | 2 |
| generated `.c` files | 4 |
| generated `.c` size | 4204 bytes |
| generated headers | 2 |
| tbp parent registrations | 1 |
| consumer-side parent registrations | 0 |

Result: keep. This removes a repeated ownership-side effect without changing
numeric code. The broader helper problem remains: old tblite generated-C
artifacts still show 1394 `tbp_parent_registration` pattern hits, 2594
`struct_deepcopy` hits, 14152 array-view hits, and heavy descriptor/temporary
scaffolding. The next dedupe changes must be based on measured duplicate helper
families, not name-only matches.

Correction after route testing: suppressing the loaded-module registration body
alone is not sufficient for static archives. A downstream object must still
reference owner metadata, otherwise the object that owns the real constructor
can be left out of `libtblite.a` linking and runtime dispatch can fail before
the first calculation. The failing tblite smoke was:
`Deferred type-bound dispatch failed for info`.

### 2026-05-14: Split Helper Reachability And TBP Owner Anchors

Hypothesis: the split-C backend should treat helper emission as a compiler pass.
Declarations are needed for ABI/type visibility, but helper bodies should be
emitted only when reachable from actual generated call sites or required
constructors. Loaded-module metadata should have one owner; downstream users
should reference the owner rather than re-emitting registration bodies.

Exact change:

- Build a per-split-unit helper-use graph over generated helper function
  definitions.
- Root the graph with emitted unit bodies, owned split definitions, global data,
  compact constant-data hooks, and required runtime constructor bodies.
- Drop unreachable helper bodies and matching helper prototypes.
- Add a focused split-C budget gate for generated C bytes, helper count, object
  bytes, executable bytes, and `__text` bytes.
- For type-bound parent metadata, owner modules now emit the real
  `_lfortran_register_c_type_parent(...)` constructor plus a stable
  `__lfortran_force_link_type_parent_*` anchor. Loaded-module consumers emit
  only extern calls to that owner anchor and TBP method anchors.

Expected win: less generated helper scaffolding in focused split-C cases, fewer
duplicate type-parent registration constructors in real module consumers, and
no numerical change because no arithmetic or data layout order is rewritten.

Correctness risk: medium for static archives. Owner-only metadata is correct
only if consumers keep a force-link reference that pulls the owner object into
the final executable. This is now covered by a consumer regression that requires
the force-link anchor and forbids consumer-side registration.

Test evidence:

- Red check: before the anchor fix, the downstream consumer split C contained
  no `__lfortran_force_link_type_parent_` reference.
- `cmake --build build -j`
- `python3 -m py_compile run_tests.py tests/c_split_budget.py`
- `PATH=build/src/bin:$PATH python3 run_tests.py -t c_backend_tbp_parent_registration_use_01`
- `PATH=build/src/bin:$PATH python3 run_tests.py -t c_backend_split_helper_reachability_01`
- `PATH=build/src/bin:$PATH python3 run_tests.py -t c_backend_large_parameter_array_data_unit_01`
- `cd integration_tests && PATH=../build/src/bin:$PATH python3 run_tests.py -b c -t c_backend_tbp_parent_force_link_dedup_01`
- `cd integration_tests && PATH=../build/src/bin:$PATH python3 run_tests.py -b c -t c_backend_alloc_class_constructor_result_01`
- `cd integration_tests && PATH=../build/src/bin:$PATH python3 run_tests.py -b c -t separate_compilation_53`
- `cd integration_tests && PATH=../build/src/bin:$PATH python3 run_tests.py -b c -t class_15`

Route evidence on fresh no-tests tblite builds:

| Metric | LFortran C emit | gfortran |
| --- | ---: | ---: |
| Build wall | 141.616 s | 14.213 s |
| Build max RSS | 2225.54 MB | 248.92 MB |
| Generated `.c` | 1499 files, 95,878,233 bytes | n/a |
| Generated headers | 398 files, 2,876,948 bytes | n/a |
| Ninja `.o` edge sum | 749.114 s | n/a |
| Object bytes | 52,798,939 | 9,962,296 |
| `libtblite.a` | 8,608,248 bytes | 3,075,144 bytes |
| `app/tblite` | 8,879,256 bytes | 5,763,992 bytes |
| `__text` | 4,755,304 bytes | 1,893,604 bytes |
| `tblite --version` warm avg | 2.812 ms | 5.314 ms |

Focused ammonia CLI parity against the fresh gfortran build:

| Observable | Difference |
| --- | ---: |
| Total energy | 1.42e-14 Eh |
| Orbital energies max abs | 4.88e-15 Eh |
| Gradient max abs | 4.71e-16 Eh/bohr |
| Virial max abs | 4.44e-16 |
| Warm CLI wall median | 93.72 ms vs 48.24 ms |
| Warm CLI RSS median | 10.99 MB vs 13.29 MB |

Result: keep, with revised interpretation. The owner-anchor fix is required for
correct static-archive behavior and the helper graph is a valid split-C
invariant. The route-level compile-size win is still small: compared with the
older measured LFortran-C artifact, `tbp_parent_registration` pattern hits drop
from 1394 to 93, but generated C remains about 96 MB because packed data,
module-body scaffolding, descriptor temporaries, and generic helper calls are
still the dominant work.

## Design Goals

1. Keep Fortran semantics explicit before final C emission.
2. Lower array expressions into views, scalar loops, or temporaries only when
   required by aliasing, contiguity, lifetime, or definition-use constraints.
3. Avoid heap allocation for descriptor-only operations.
4. Emit C that a normal C compiler can optimize without needing to reconstruct
   high-level Fortran semantics from opaque runtime calls.
5. Keep split-C output compilable with ordinary gcc/clang/CMake toolchains.

## Phase 1: Allocation And Descriptor Accounting

Purpose: make the biggest waste visible before changing lowering rules.

- Add optional C-runtime allocation tracing for `_lfortran_malloc`,
  descriptor allocation helpers, and array-expression temporaries.
- Record allocation size, source lowering site, type/rank, and lifetime owner.
- Add a tblite MnO2 benchmark script that reports wall time, max RSS,
  instruction count, and top allocation sites.
- Add focused integration tests that verify no-copy descriptor views preserve
  lower bounds, offsets, strides, and data pointers.

Exit criteria:

- The top allocation sites for MnO2 are known.
- A regression test can distinguish descriptor view construction from data copy
  materialization.

## Phase 2: No-Copy Descriptor View Construction

Purpose: make array argument passing cheap when the actual storage can be
described safely.

- Introduce a C-backend helper that builds stack-local descriptor views.
- Normalize assumed-shape dummy lower bounds to Fortran dummy semantics.
- Preserve the actual storage base pointer and adjust offset so dummy index 1
  maps to the first element of the actual argument.
- Preserve strides for contiguous actuals and known safe strided actuals.
- Keep copy-in/copy-out only for cases that require it, such as non-contiguous
  actuals passed to callees that require contiguous storage.
- Stop casting allocatable-wrapper descriptors to raw descriptors. Emit the
  correct descriptor type and pass its address.

Exit criteria:

- Contiguous array sections passed to assumed-shape dummies do not allocate or
  copy.
- Existing lower-bound and rank-remap tests still pass.

## Phase 3: Array-Expression Lowering

Purpose: avoid materializing whole-array temporaries when scalar loops or fused
loops preserve semantics.

- Add an ASR-level or pre-C-lowering representation for array expression plans:
  view, scalarized loop, fused loop, or materialized temporary.
- Scalarize elemental operations with known rank/extent into direct loops.
- Fuse producer-consumer chains when the temporary has a single use and no
  semantic need for materialization.
- Preserve required temporaries for aliasing-sensitive assignments such as
  overlapping array sections.
- Lower reductions and intrinsics with specialized loops when rank, type, and
  contiguity are known.

Exit criteria:

- Hot array expressions in tblite dispersion and Hamiltonian paths no longer
  allocate heap temporaries in the common contiguous case.
- Instruction count drops measurably on MnO2.

## Phase 4: Scalar Replacement And Loop Specialization

Purpose: convert small fixed-shape arrays and descriptors into scalar or
stack-local state before final C emission.

- Replace small fixed-size array temporaries with scalar variables or fixed C
  arrays.
- Specialize descriptor helpers for known rank, type, and contiguous layout.
- Hoist descriptor field reads, bounds, and stride calculations out of inner
  loops.
- Avoid generic runtime dispatch when compile-time rank and type are known.
- Emit restrict-like local aliases only when alias analysis proves it is safe.

Exit criteria:

- Generated C hot loops contain simple pointer arithmetic and scalar operations.
- MnO2 instruction count approaches the gfortran order of magnitude.

## Phase 5: Temporary Lifetime Sinking

Purpose: reduce peak RSS and keep generated code maintainable.

- Assign every generated temporary a statement, block, or procedure lifetime.
- Free heap temporaries at the narrowest valid scope.
- Prefer stack descriptors for metadata-only views.
- Use scoped cleanup lists in C emission so early exits and branches do not
  leak generated temporaries.
- Add tests for temporaries created inside loops, conditionals, and intrinsic
  calls.

Exit criteria:

- MnO2 max RSS no longer grows into gigabytes.
- Allocation high-water is proportional to actual live numerical data, not the
  number of executed array expressions.

## Phase 6: Constant Placement

Purpose: keep compile time and runtime initialization costs low.

- Emit large constants into compact data translation units.
- Use static const data in `.rodata` for immutable arrays.
- Copy compact constant data into mutable descriptors only when the program
  semantically assigns to mutable storage.
- Avoid embedding huge initializer lists in hot procedure translation units.
- Share identical immutable constants when safe.

Exit criteria:

- Split-C compile time is not dominated by huge procedure translation units.
- Large lookup tables do not allocate or initialize repeatedly at runtime.

## Phase 7: Benchmark Gates

Purpose: prevent correctness and performance regressions.

- Keep the MnO2 C-backend benchmark as a tracked benchmark, not a normal fast
  test.
- Compare against the gfortran executable with single-threaded BLAS/runtime
  settings.
- Track energy parity, wall time, RSS, instruction count, and emitted C size.
- Add smaller integration tests for each lowering rule so failures are easy to
  isolate without rebuilding tblite.

Target direction:

- Energy parity stays within the current MnO2 tolerance envelope.
- RSS should fall from gigabytes to the same order as gfortran for small inputs.
- Instruction count should first drop by removing heap temporary churn, then by
  scalarizing and specializing hot loops.
