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
