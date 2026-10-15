# Pipeline Binary Caching (`VK_KHR_pipeline_binary`)

## Overview

Pipeline compilation is a major source of stutter and startup latency, and a GLES application
triggers it at draw time, whenever it first uses a new combination of state.

`VK_KHR_pipeline_binary` lets ANGLE take the compiled binaries out of a pipeline the driver has just
created, and hand them back on a later run to create the same pipeline without compiling it. ANGLE
stores the binaries itself, in the platform blob cache:

- The driver gives every pipeline a **pipeline key**: its key for what it compiled into the
  pipeline.
- The binaries a pipeline was built from are stored in **one blob cache entry per pipeline key**.
- A later run creates each pipeline whose key has an entry from the stored binaries instead of
  compiling it.

When pipeline binaries are in use, they replace `VkPipelineCache` entirely. They are off by default
(`preferPipelineBinaries`). Tests turn them on with `allowPipelineBinariesForTesting`, which takes
effect where the extension is supported.

## Where It Fits

ANGLE caches at three levels. The front end's program cache stores translated shaders and program
metadata in the blob cache. `GraphicsPipelineCache` and `ComputePipelineCache` map ANGLE's pipeline
descs to the pipelines already created. On a miss there, the pipeline is created through
`PipelineCacheAccess::createGraphicsPipeline()` or `createComputePipeline()`, which hand it to
`PipelineBinariesCache` when pipeline binaries are in use, and otherwise create it against a
`VkPipelineCache`.

Every `PipelineCacheAccess` is given the renderer's `PipelineBinariesCache`, so every graphics and
compute pipeline ANGLE creates goes through it, graphics pipeline libraries included: draws, for
programs and program pipeline objects alike, dispatches, the link-time warm up
(`warmUpPipelineCacheAtLink`), and the pipelines of `UtilsVk` and OpenCL kernels. A program loaded
from a program binary is not warmed up, so it reads its pipelines' entries at its first draw or
dispatch.

With pipeline binaries, no `VkPipelineCache` data is stored or loaded anywhere:
`mergeProgramPipelineCachesToGlobalCache` and `syncGlobalPipelineCacheToBlobCache` are off,
`Renderer::initPipelineCache()` does not look for one in the blob cache, and program binaries carry
no pipeline cache data.

### Graphics pipeline libraries

With `supportsGraphicsPipelineLibrary`, each program gets a library holding its compiled shaders,
made at link time. A draw's pipeline is linked from that library, with the draw's vertex input and
fragment output state. The library is stored like any other pipeline, under its own key. Whether the
linked pipeline is too depends on `cachePipelineBinariesForLinkedPipelines`, because drivers differ:

- Some give no binaries for linked pipelines.
- Some give a linked pipeline the key of the library it links, so its lookup reads the library's
  entry a second time.
- Some give every linked pipeline the same placeholder key, of all zeros.
- Some do much of their compiling when linking, and give linked pipelines binaries of their own,
  without which linking is far slower.

Only for the last are linked pipelines looked up and stored. Elsewhere, `PipelineBinariesCache`
creates them from their libraries directly, and does not count them as hits or misses. A pipeline
whose key is all zeros is never stored, whatever the driver.

## Keys

`vkGetPipelineKeyKHR` gives a pipeline a key derived from its create info, identifying what the
driver compiles into it. Pipelines that differ only in state the driver does not compile in get the
same key, and so share one entry. How much state that covers varies by vendor.

`PipelineBinariesCache::init()` queries the driver's **global key** when the renderer is
initialized. It changes whenever previously captured binaries stop being valid, for example after a
driver update.

An entry is stored under:

```
 blob key = BlobCacheHasher( "angle_pipeline_binaries"
                             || kFormatVersion
                             || global key size || global key       vkGetPipelineKeyKHR(nullptr)
                             || pipeline key size || pipeline key   vkGetPipelineKeyKHR(createInfo))
```

`angle::BlobCacheHasher` is what ANGLE's other blob cache keys use: SHA-1 where
`ANGLE_USE_CRYPTO_HASHER` is defined, and XXH3-128 otherwise. Its digest is the blob cache key's
length. Everything before the pipeline key is the same for every entry, so it is hashed once, when
the global key is set, and each blob key hashes that digest and the pipeline key.

What changes for every entry at once, the format and the driver, is part of the blob key. After such
a change, old entries are never looked up, rather than read and rejected one at a time, and they age
out of the blob cache like any other unused entry. The ANGLE version is not part of it: the binaries
are the driver's, and an ANGLE build that creates a pipeline differently gets another pipeline key
from the driver anyway.

## Entry Format

An entry is one pipeline's binaries, compressed:

```
 +--------------------------------------------------------------------------+
 | gzip or zstd, with the codec's checksum:                                 |
 |   kFormatVersion                                                         |
 |   globalKeySize | globalKey [32B]                                        |
 |   pipelineKeySize | pipelineKey [32B]                                    |
 |   binaryCount N                                                          |
 |   N x { keySize | key [32B] | dataSize D | data [D] }                    |
 +--------------------------------------------------------------------------+
```

The format version and the keys are what the blob key is made of. They are stored too, so that an
entry found under a colliding blob key is rejected.

`CompressEntry()` compresses with zstd (`angle::CompressBlobZstd()`) where the build has it
(`angle_has_zstd`), and otherwise gzip (`angle::CompressBlob()`). `DecompressEntry()` tells them
apart by the magic number each format starts with, as defined by RFC 1952 and RFC 8878, so either
build reads what the other wrote, except that a build without zstd misses on zstd entries.
`SerializeBinarySet()` and `DeserializeBinarySet()` write and read what is compressed.

There is no checksum of ANGLE's own: the codec's, which decompression verifies, covers the entry.
Android's blob cache also checks a CRC of each entry it reads.

Every integer is a uint32 in host byte order, which is safe because an entry is never read on
another device: the global key is part of every blob key. Keys are written as the whole
`VK_MAX_PIPELINE_BINARY_KEY_SIZE_KHR` array, with the size saying how much of it is used. How many
binaries a pipeline has varies by vendor.

Before anything in an entry is used, reading checks:

- that it is gzip or zstd;
- that it decompresses to at most `PipelineBinariesCache::kMaxBlobSize` (8 MiB, the largest value
  Android's blob cache stores), checked against the size the codec records before allocating;
- the codec's checksum;
- key sizes of at most 32 bytes, 1 to 64 binaries, every data size non-zero and within the entry,
  and nothing left over;
- that the stored format version, global key and pipeline key are the ones looked up.

An entry that fails any check is a miss. A pipeline whose entry would be larger than `kMaxBlobSize`
is not stored.

## Creating a Pipeline

`createPipelineWithBinaries()` is all of pipeline creation through the extension, for graphics and
compute alike:

```
 PipelineBinariesCache::createPipelineWithBinaries(createInfo)
 -------------------------------------------------------------------------------------------
  1. vkGetPipelineKeyKHR(createInfo) -> pipeline key
       failed: return the error
      |
      v
  2. loadBinaries(pipeline key): getBlob(blob key), DecompressEntry(), DeserializeBinarySet()
       found: vkCreatePipelineBinariesKHR(keys, data) -> VkPipelineBinaryKHR handles
              create the pipeline with VkPipelineBinaryInfoKHR and no shader modules,
              destroy the handles
       success: a hit; done
       failure: WARN, and fall through
      |
      v
  3. a miss
       create with VK_PIPELINE_CREATE_2_CAPTURE_DATA_BIT_KHR
       CaptureBinaries(): vkCreatePipelineBinariesKHR(pipeline) -> handles
                          vkGetPipelineBinaryDataKHR(each handle) -> the binary's key and bytes
       storeBinaries(): SerializeBinarySet(), CompressEntry(), putBlob(blob key)
       vkReleaseCapturedPipelineDataKHR(pipeline)
```

The driver ignores the shader modules of a pipeline created from binaries, so they are left out of
the stages. A driver that compiled them instead would fail, and the pipeline would be compiled and
counted as a miss. The modules are still needed for the pipeline key.

The capture bit exists only in `VkPipelineCreateFlags2CreateInfoKHR`, and the spec does not allow a
second one in a `pNext` chain. With pipeline binaries, the callers chain one to every pipeline's
create info, moving its flags into it. The cache finds it there and sets the bit when it compiles
the pipeline. The callers don't use the create info again, so the bit is left set.

The binaries are stored on the creating thread: compressing one pipeline's binaries costs far less
than compiling them, and the platform's blob cache writes to disk on a thread of its own.

`getCacheHits()` counts the pipelines created from stored binaries, and `getCacheMisses()` the
others whose key query succeeded. Tests use them to check warm behavior without relying on timing.

## Errors and Warnings

- A failed key query, or a failed pipeline creation, is returned like any other.
- A damaged entry, a driver refusing stored binaries, a pipeline key larger than the spec allows,
  and a failed capture are logged as warnings. They concern one pipeline, which is still created.
  A damaged entry read back from storage is expected input rather than an ANGLE bug.
- A change that affects every entry produces no warnings, as it changes the blob keys.
- A store the platform refuses, or later evicts, is not reported, and shows up only as a miss on the
  next run, which stores the pipeline again.

## Why One Entry per Pipeline Key

One entry per pipeline key is the simplest layout: a lookup reads exactly the binaries it needs, a
store writes exactly the binaries it captured, and nothing has to be kept in memory, rewritten, or
coordinated between threads.

The cost is entries. Where a program has several pipeline keys, which varies by vendor, it has an
entry for each, and their similar binaries no longer share a compressed stream. A platform that
reads every entry when it opens the cache then takes longer to start. Grouping entries per program,
or compressing with a dictionary, can recover that later if it matters.

## Platform Blob Cache

ANGLE's Vulkan backend does no file access of its own. Binaries are stored through the display's
blob cache callbacks, which the application sets with `EGL_ANDROID_blob_cache`.
`DisplayVk::getBlob()` and `putBlob()` pass no context, so per-context `GL_ANGLE_blob_cache`
callbacks are not used. Without display callbacks, nothing is stored, and captures are overhead.

Android's multifile blob cache stores each entry as a file. It limits the number of entries and
their total size, evicting the least recently used when either is crossed, and reads every entry
when it is first opened. One entry per pipeline key multiplies an application's entries, so both
matter.

## Configuration

- **Device requirements:** `supportsPipelineBinary` requires the `VK_KHR_pipeline_binary`
  extension, its `pipelineBinaries` feature, and `VK_KHR_maintenance5`, which provides
  `VkPipelineCreateFlags2CreateInfo` for the capture bit. ANGLE enables `supportsMaintenance5` only
  where dynamic rendering is supported, whether or not it renders with it
  (`preferDynamicRendering`).
- **Feature flags:** `preferPipelineBinaries` decides whether ANGLE uses pipeline binaries instead
  of `VkPipelineCache`. It is on only when `allowPipelineBinariesForTesting` is set, which it is not
  by default, and `supportsPipelineBinary` is on.
  `cachePipelineBinariesForLinkedPipelines` decides whether pipelines linked from graphics pipeline
  libraries are stored too. It is on for Qualcomm.

## Testing Notes

- [`VulkanPipelineBinaryTest.cpp`](../../../../tests/gl_tests/VulkanPipelineBinaryTest.cpp), in
  `angle_white_box_tests`, backs the blob cache with `TestBlobCache` (`util/test_blob_cache.h`) in a
  temporary directory, through `ANGLETestBase::setNeedsBlobCache()`.
- `VulkanPipelineBinaryTest` runs with `allowPipelineBinariesForTesting` on, with graphics pipeline
  libraries at the device's default and, in one configuration, off. It checks warm draws, restarts
  and links, and the global key, through `getCacheHits()` and `getCacheMisses()`.
  `VulkanPipelineBinaryTestES31` checks warm dispatches.
- A restart tears down the display and renderer (`forceNewDisplay()`) and brings up new ones, as an
  application restart would. Some drivers report a different global key for a later device in the
  same process, and nothing stored under the old key is then read, so those tests skip.
  Applications are unaffected, since an application restart is a new process.
- `PipelineBinariesCacheTest`, in
  [`vk_pipeline_binary_cache_unittest.cpp`](../vk_pipeline_binary_cache_unittest.cpp) in
  `angle_unittests`, checks the entry format, malformed entries, entries for other keys, and blob
  keys, with made up keys and binaries. It needs no device.
- To turn pipeline binaries on: `ANGLE_FEATURE_OVERRIDES_ENABLED=allowPipelineBinariesForTesting`.

### Running the Benchmarks

The benchmark is `VulkanPipelineBinaryBenchmark` in `angle_perftests`. Each story runs, in one
process, a cold phase that fills the cache, a warm phase that reads it back, and a partial phase
that reads back a small working set. Each phase gets a fresh Display, and `TestBlobCache` reopens
its directory in between, which is what a process restart looks like to ANGLE. It reports time to
first and all draws, resident memory, and the blob cache's size on disk. Stories are named
`<N>_<workload>_<cache type>`, and cold and warm draw all N programs. Partial draws M of them, and
records its results for `<N>_<workload>_<M>_partial_<cache type>`.

There are two workloads, both of N programs with one pipeline each, compiled and linked from source
in every phase, so ANGLE's shader and program blobs are part of what the cache gives back:

- `draw_immediately`: each program is linked and drawn in turn, so each pipeline is created as
  it's drawn.
- `deferred_draw`: every program is linked before any is drawn, as applications do while loading.

Each comes as `..._pipeline_binary` and, for comparison, `..._pipeline_cache`. Stories run with
the device's defaults, graphics pipeline libraries included. Frames are presented every 16.7 ms
while loading and for two seconds after, so ANGLE syncs its pipeline cache to the blob cache on
its own schedule, as it would in an application.

This runs the same way on every platform, and on a bot:

```sh
autoninja -C out/Release angle_perftests
out/Release/angle_perftests --gtest_filter='*VulkanPipelineBinaryBenchmark*_100_deferred_draw_*'
# The pipeline cache stories, to compare with
out/Release/angle_perftests --gtest_also_run_disabled_tests \
    --gtest_filter='*VulkanPipelineBinaryPipelineCacheBenchmark*_100_deferred_draw_*'
```

On Linux and Android, the blob cache is dropped from the page cache before each phase, so entries
are read from storage. On Linux it's kept under `~/.cache`, as `/tmp` is often in memory. Stories of
10000 and 20000 pipelines and the pipeline cache stories are disabled; add
`--gtest_also_run_disabled_tests` to run them. A later
phase that stores entries fails, as it didn't find what the cold phase stored. A driver can cause
that by reporting a different global pipeline key for a later device in the same process (see
[Testing Notes](#testing-notes)).
