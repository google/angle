//
// Copyright 2026 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// VulkanPipelineBinaryPerf:
//   Measures how quickly an application gets its pipelines back on a later run, from ANGLE's
//   monolithic VkPipelineCache stored in the blob cache.
//
//   Each story runs three phases against one cache, each a run of the same application:
//     - cold: the first run.  The cache is empty, so every pipeline is created from scratch and
//       stored.
//     - warm: a later run that draws every program again, all of them found in the cache.
//     - partial: a later run that also starts with the full cache, but draws only a few of its
//       programs, about 5% and at least 10.  An application rarely needs everything it has ever
//       cached: a run loads one level or scene, so it needs only that part.  This phase measures
//       getting those few pipelines back from a cache that holds many more, which warm, drawing
//       everything, can't show.
//   They are phases of one test so that sharding can't separate a later phase from the cold phase
//   that filled its cache.  Each phase gets a new Display, and util/test_blob_cache reopens the
//   cache from disk, as a new process would.
//
//   Stories are named <N>_<workload>_<cache type>, and cold and warm draw all N programs.  Partial
//   draws M of them, so it records its results for <N>_<workload>_<M>_partial_<cache type>, which
//   keeps them from being read against warm's.
//
//   Workloads, both of N programs with one pipeline each, compiled and linked from source in every
//   phase, so ANGLE's shader and program blobs come from the cache too:
//     - draw_immediately: each program is linked and drawn in turn, as some applications do while
//       running.  ANGLE has to create each pipeline as it's drawn.
//     - deferred_draw: every program is linked before any is drawn, as applications do while
//       loading.
//
//   Frames are presented every 16.7 ms while loading and for two seconds after, so ANGLE syncs
//   the monolithic cache to the blob cache as often as it would in an application.  On Linux and
//   Android, the blob cache is dropped from the page cache before each phase, so entries are read
//   from storage as they would be after a reboot or under memory pressure.  On Linux the cache is
//   kept under ~/.cache, as /tmp is often in memory.  Each story salts its shaders with the time
//   it started, so that no driver's own shader cache has seen them, and the cold phase compiles
//   every pipeline from scratch.
//
//   Stories of 10000 and 20000 pipelines are disabled, as filling them takes minutes.  Run them
//   with --gtest_also_run_disabled_tests.
//
//   Metrics, per phase, as .<phase>_<metric>:
//     - time_to_link_all (ms, deferred draw only): linking every program
//     - time_to_first_draw, time_to_all_draws (ms): from the start of drawing until that many
//       draws have finished on the GPU
//     - resident_memory (bytes): process RSS at the end of the phase
//     - disk_cache_size (bytes), disk_cache_files (count): the blob cache on disk after the phase
//     - blob_cache_hits, blob_cache_stores (count)
//

#include "ANGLEPerfTest.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string_view>
#include <thread>
#include <vector>

#include "common/platform.h"

#if defined(ANGLE_PLATFORM_LINUX) || defined(ANGLE_PLATFORM_ANDROID)
#    include <fcntl.h>
#    include <sys/mman.h>
#    include <sys/stat.h>
#    include <unistd.h>
#    define ANGLE_CAN_EVICT_PAGE_CACHE 1
#endif

#include "common/debug.h"
#include "common/system_utils.h"
#include "util/random_utils.h"
#include "util/shader_utils.h"
#include "util/test_blob_cache.h"

using namespace angle;

namespace
{
enum class Workload
{
    DrawImmediately,
    DeferredDraw,
};

enum class CachePhase
{
    Cold,
    Warm,
    Partial,
};

constexpr std::array<CachePhase, 3> kPhases = {CachePhase::Cold, CachePhase::Warm,
                                               CachePhase::Partial};

// Frames are presented every 16.7 ms while loading, as an application presents them, rather than
// after every draw or not at all.  ANGLE syncs the monolithic cache to the blob cache every 60
// frames, so presenting after every draw would sync it far more often than an application does,
// and not presenting would leave it only the idle frames.
constexpr double kFrameSeconds = 1.0 / 60.0;
// An application keeps running after its last new pipeline; two seconds give ANGLE two more syncs.
constexpr int kIdleFrames = 2 * 60;

// Large enough that the blob cache never evicts
constexpr size_t kBlobCacheMaxTotalSize    = 1024 * 1024 * 1024;
constexpr size_t kBlobCacheMaxTotalEntries = 1000000;

const char *PhaseName(CachePhase phase)
{
    switch (phase)
    {
        case CachePhase::Cold:
            return "cold";
        case CachePhase::Warm:
            return "warm";
        case CachePhase::Partial:
            return "partial";
        default:
            UNREACHABLE();
            return "";
    }
}

const char *WorkloadName(Workload workload)
{
    switch (workload)
    {
        case Workload::DrawImmediately:
            return "draw_immediately";
        case Workload::DeferredDraw:
            return "deferred_draw";
        default:
            UNREACHABLE();
            return "";
    }
}

// Whether the workload links every program before drawing any.
bool LinksAllFirst(Workload workload)
{
    switch (workload)
    {
        case Workload::DrawImmediately:
            return false;
        case Workload::DeferredDraw:
            return true;
        default:
            UNREACHABLE();
            return false;
    }
}

// Draws go to an RGBA8 target of kTargetSize square, each covering one cell of a grid, so the
// window shows draws accumulating.  The viewport is dynamic state, so the cell changes no pipeline.
constexpr GLsizei kTargetSize  = 64;
constexpr GLsizei kCellSize    = 8;
constexpr GLsizei kCellsPerRow = kTargetSize / kCellSize;
constexpr size_t kCellCount    = static_cast<size_t>(kCellsPerRow * kCellsPerRow);

// Programs differ in code, not just constants: each takes a different subset of these snippets
// with different loop counts, and different numbers of varyings, samplers and uniforms.  In a
// snippet, LOOPS is replaced by its loop count.
struct ShaderSnippet
{
    const char *name;
    const char *body;
};

constexpr std::array<ShaderSnippet, 5> kShaderSnippets = {{
    {"fractalNoise", R"(
    vec4 acc = vec4(0.0);
    float amp = 0.5;
    for (int i = 0; i < LOOPS; ++i) {
        c = sin(c.yzwx * 1.618 + kTable[(i * 2) % 32]) + cos(c.wxyz * 2.718 + kTable[(i * 2 + 1) % 32]);
        acc += c * amp;
        amp *= 0.5;
    }
    return acc;
)"},
    {"lighting", R"(
    vec3 n = normalize(c.xyz + vec3(0.0, 0.0, 1.0));
    vec4 total = vec4(0.0);
    for (int i = 0; i < LOOPS; ++i) {
        vec3 l = normalize(kTable[i % 32].xyz + vec3(0.001));
        vec3 h = normalize(l + vec3(0.0, 0.0, 1.0));
        float diffuse = max(dot(n, l), 0.0);
        float specular = pow(max(dot(n, h), 0.0), 16.0 + float(i));
        total += abs(kTable[(i + 7) % 32]) * (diffuse + specular);
    }
    return c * 0.5 + total / float(LOOPS);
)"},
    {"cellularNoise", R"(
    vec2 g = floor(c.xy * 5.0);
    vec2 f = fract(c.xy * 5.0);
    float minDist = 8.0;
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            vec2 lattice = vec2(float(x), float(y));
            int offset = abs(int(g.x + lattice.x) + int(g.y + lattice.y) * 7) % 32;
            vec2 diff = lattice + sin(kTable[offset].xy * 6.28) * 0.5 + 0.5 - f;
            minDist = min(minDist, dot(diff, diff));
        }
    }
    return c * 0.5 + vec4(sqrt(minDist));
)"},
    {"matrixPalette", R"(
    for (int i = 0; i < LOOPS; ++i) {
        int b = (i * 4) % 28;
        c = normalize(mat4(kTable[b], kTable[b + 1], kTable[b + 2], kTable[b + 3]) * c + vec4(0.001));
    }
    return c;
)"},
    {"toneMap", R"(
    vec4 x = max(vec4(0.0), mat4(kTable[0], kTable[1], kTable[2], kTable[3]) * c - 0.004);
    x = (x * (6.2 * x + 0.5)) / (x * (6.2 * x + 1.7) + 0.06);
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
)"},
}};

constexpr std::array<uint32_t, 4> kLoopCounts = {2, 4, 8, 16};

// Returns a random number in [0, bound).
uint32_t RandomBelow(RNG *rng, uint32_t bound)
{
    return static_cast<uint32_t>(rng->randomIntBetween(0, static_cast<int>(bound) - 1));
}

// A constant table whose values depend on the program.  The translator drops data a program never
// reads, so main() also adds the program's index to a live value to keep the SPIR-V unique.
void EmitTable(const char *type, const char *name, uint32_t count, float seed, std::ostream *out)
{
    *out << "const " << type << " " << name << "[" << count << "] = " << type << "[" << count
         << "](\n";
    const bool isMatrix = std::string_view(type) == "mat4";
    for (uint32_t i = 0; i < count; ++i)
    {
        const float c0 = std::sin(seed * 1.1f + i * 0.43f);
        const float c1 = std::cos(seed * 1.7f + i * 0.71f);
        const float c2 = std::sin(seed * 2.3f + i * 1.13f);
        const float c3 = std::cos(seed * 2.9f + i * 1.37f);
        *out << "    " << type << "(" << c0 << ", " << c1 << ", " << c2 << ", " << c3;
        if (isMatrix)
        {
            *out << ", " << c1 << ", " << c2 << ", " << c3 << ", " << c0 << ", " << c2 << ", " << c3
                 << ", " << c0 << ", " << c1 << ", " << c3 << ", " << c0 << ", " << c1 << ", "
                 << c2;
        }
        *out << (i + 1 < count ? "),\n" : ")\n");
    }
    *out << ");\n";
}

struct ProgramSource
{
    std::string vertex;
    std::string fragment;
};

// Adds the salt into |value|.  Its halves are mixed with |input|, which varies per vertex or
// fragment, so no compiler knows the result; xor and add don't combine, so neither half folds into
// the other; and converting all 32 bits to float carries every bit of both to the output.  The
// salt stays in the SPIR-V as two exact 32-bit constants, unlike a float, which would lose bits.
void EmitSalt(uint64_t salt, const char *input, const char *value, std::ostream *out)
{
    // Emits, with <hi> and <lo> the salt's upper and lower 32 bits:
    //   highp uint salt = (floatBitsToUint(<input>) ^ <hi>u) + <lo>u;
    //   <value> += float(salt) * 0.000000000001;
    *out << "    highp uint salt = (floatBitsToUint(" << input << ") ^ "
         << static_cast<uint32_t>(salt >> 32) << "u) + " << static_cast<uint32_t>(salt) << "u;\n"
         << "    " << value << " += float(salt) * 0.000000000001;\n";
}

// Generates a program's shaders, the same for a given index and salt.  A new salt gives every
// shader new SPIR-V.
ProgramSource GenerateProgramSource(size_t program, uint64_t salt)
{
    RNG rng(static_cast<unsigned int>(program));
    const float seed                = static_cast<float>(program + 1);
    const uint32_t varyingCount     = 2 + RandomBelow(&rng, 7);  // 2 to 8
    const uint32_t transformCount   = 4 + RandomBelow(&rng, 9);  // 4 to 12
    const uint32_t skinningLoops    = RandomBelow(&rng, 4) * 4;  // 0 to 12
    const uint32_t samplerCount     = RandomBelow(&rng, 4);      // 0 to 3
    const uint32_t extraUniformSize = std::array<uint32_t, 3>{0, 4, 16}[RandomBelow(&rng, 3)];

    ProgramSource source;
    std::ostringstream vs;
    vs << std::fixed << std::setprecision(6);
    vs << "#version 300 es\n"
       << "precision highp float;\n"
       << "layout(location = 0) in vec2 a_position;\n"
       << "layout(location = 1) in vec2 a_texCoord;\n"
       << "uniform vec4 u_param;\n";
    for (uint32_t v = 0; v < varyingCount; ++v)
    {
        vs << "out vec4 v_var" << v << ";\n";
    }
    EmitTable("mat4", "kTransforms", transformCount, seed * 0.31f, &vs);
    vs << "void main()\n"
       << "{\n"
       << "    vec4 r = vec4(a_position, a_texCoord);\n"
       << "    r.x += " << program << ".0 * 0.000001;\n";
    // Salt the vertex shader with a per-vertex input
    EmitSalt(salt, "a_position.x", "r.y", &vs);
    if (skinningLoops > 0)
    {
        vs << "    for (int i = 0; i < " << skinningLoops << "; ++i) {\n"
           << "        float w = sin(float(i) * 0.523 + u_param.x) * 0.5 + 0.5;\n"
           << "        r += kTransforms[i % " << transformCount << "] * r * w;\n"
           << "    }\n";
    }
    vs << "    v_var0 = vec4(a_texCoord, r.xy);\n"
       << "    v_var1 = vec4(r.zw, a_position) + vec4(" << std::sin(seed * 0.17f) << ");\n";
    for (uint32_t v = 2; v < varyingCount; ++v)
    {
        vs << "    v_var" << v << " = kTransforms[" << v % transformCount << "] * vec4(r.xy, "
           << std::cos(seed * 0.23f * v) << ", 1.0);\n";
    }
    vs << "    gl_Position = vec4(a_position, " << std::cos(seed * 0.73f) << " * 0.01, 1.0);\n"
       << "}\n";
    source.vertex = vs.str();

    std::ostringstream fs;
    fs << std::fixed << std::setprecision(6);
    fs << "#version 300 es\n"
       << "precision highp float;\n"
       << "out vec4 fragColor;\n"
       << "uniform vec4 u_param;\n";
    for (uint32_t v = 0; v < varyingCount; ++v)
    {
        fs << "in vec4 v_var" << v << ";\n";
    }
    for (uint32_t t = 0; t < samplerCount; ++t)
    {
        fs << "uniform sampler2D u_tex" << t << ";\n";
    }
    if (extraUniformSize > 0)
    {
        fs << "uniform vec4 u_extra[" << extraUniformSize << "];\n";
    }
    EmitTable("vec4", "kTable", 32, seed, &fs);

    // One to four distinct snippets, by a partial shuffle
    std::array<uint32_t, kShaderSnippets.size()> order = {0, 1, 2, 3, 4};
    const uint32_t snippetCount                        = 1 + RandomBelow(&rng, 4);
    std::vector<std::string> calls;
    for (uint32_t i = 0; i < snippetCount; ++i)
    {
        std::swap(order[i], order[i + RandomBelow(&rng, static_cast<uint32_t>(order.size()) - i)]);
        const ShaderSnippet &snippet = kShaderSnippets[order[i]];
        std::string body(snippet.body);
        const std::string loops = std::to_string(kLoopCounts[RandomBelow(&rng, 4)]);
        for (size_t pos = body.find("LOOPS"); pos != std::string::npos; pos = body.find("LOOPS"))
        {
            body.replace(pos, 5, loops);
        }
        fs << "vec4 " << snippet.name << "(vec4 c)\n"
           << "{" << body << "}\n";

        std::ostringstream call;
        call << std::fixed << std::setprecision(6);
        switch (RandomBelow(&rng, 3))
        {
            case 0:
                call << "    c = " << snippet.name << "(c);\n";
                break;
            case 1:
                call << "    c = " << snippet.name << "(c) + c * 0.5;\n";
                break;
            case 2:
                call << "    c = mix(c, " << snippet.name << "(c.wzyx), "
                     << std::abs(std::cos(seed * 0.47f * (i + 1))) << ");\n";
                break;
            default:
                UNREACHABLE();
                break;
        }
        calls.push_back(call.str());
    }

    // Every varying, sampler and uniform feeds the color, so none is optimized away
    fs << "void main()\n"
       << "{\n"
       << "    vec4 c = v_var0 * " << std::sin(seed * 0.31f) << " + v_var1 + u_param;\n";
    // Salt the fragment shader with a varying
    EmitSalt(salt, "v_var0.x", "c.y", &fs);
    for (uint32_t v = 2; v < varyingCount; ++v)
    {
        fs << "    c += v_var" << v << " * 0.1;\n";
    }
    for (uint32_t t = 0; t < samplerCount; ++t)
    {
        fs << "    c += texture(u_tex" << t << ", c.xy) * 0.1;\n";
    }
    if (extraUniformSize > 0)
    {
        fs << "    for (int i = 0; i < " << extraUniformSize << "; ++i) {\n"
           << "        c += u_extra[i] * 0.01;\n"
           << "    }\n";
    }
    fs << "    c.x += " << program << ".0 * 0.000001;\n";
    for (const std::string &call : calls)
    {
        fs << call;
    }
    fs << "    fragColor = clamp(c, 0.0, 1.0);\n"
       << "}\n";
    source.fragment = fs.str();
    return source;
}

GLuint CreateShader(GLenum type, const std::string &source)
{
    GLuint shader    = glCreateShader(type);
    const char *text = source.c_str();
    glShaderSource(shader, 1, &text, nullptr);
    glCompileShader(shader);
    return shader;
}

// Compiles and links a program without waiting for either, so ANGLE can run them in parallel.
GLuint CompileAndLinkProgram(const ProgramSource &source)
{
    GLuint vs      = CreateShader(GL_VERTEX_SHADER, source.vertex);
    GLuint fs      = CreateShader(GL_FRAGMENT_SHADER, source.fragment);
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDetachShader(program, vs);
    glDetachShader(program, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

struct PipelineBinaryPerfParams final : public RenderTestParams
{
    PipelineBinaryPerfParams(Workload workloadIn, size_t programCountIn)
        : workload(workloadIn), programCount(programCountIn)
    {
        iterationsPerStep = 1;
        majorVersion      = 3;
        minorVersion      = 0;
        windowWidth       = 256;
        windowHeight      = 256;

        eglParameters = egl_platform::VULKAN();

        // ANGLE doesn't sync the monolithic cache by default when it warms pipelines up at link
        // time, and syncing it is what the monolithic cache stories measure
        eglParameters.enable(Feature::SyncGlobalPipelineCacheToBlobCache);
        eglParameters.disable(Feature::PreferPipelineBinaries);
    }

    std::string story() const override
    {
        std::stringstream strstr;
        strstr << RenderTestParams::story() << "_" << programCount << "_" << WorkloadName(workload)
               << "_monolithic_cache";
        return strstr.str();
    }

    // The story partial's results are recorded for, which also names the programs it draws, so
    // they aren't read against the whole set.
    std::string partialStory() const
    {
        std::stringstream strstr;
        strstr << RenderTestParams::story() << "_" << programCount << "_" << WorkloadName(workload)
               << "_" << partialProgramCount() << "_partial_monolithic_cache";
        return strstr.str();
    }

    // The programs the partial phase draws: about 5% of them, at least 10, as loading one level of
    // a game would.
    size_t partialProgramCount() const
    {
        return std::min(programCount, std::max<size_t>(10, programCount / 20));
    }

    // The programs a phase draws.  Partial's are spread over the catalog.
    std::vector<size_t> workingSet(CachePhase phase) const
    {
        size_t count = programCount;
        if (phase == CachePhase::Partial)
        {
            count = partialProgramCount();
        }
        else
        {
            ASSERT(phase == CachePhase::Cold || phase == CachePhase::Warm);
        }
        std::vector<size_t> programs;
        for (size_t i = 0; i < count; ++i)
        {
            programs.push_back(i * programCount / count);
        }
        return programs;
    }

    Workload workload;
    size_t programCount;
};

std::ostream &operator<<(std::ostream &os, const PipelineBinaryPerfParams &params)
{
    os << params.backendAndStory().substr(1);
    return os;
}

struct DiskUsage
{
    size_t bytes = 0;
    size_t files = 0;
};

// Measures the blob cache on disk.
void MeasureDiskUsage(const std::string &dir, DiskUsage *usageOut)
{
    DiskUsage usage;
    std::error_code ec;
    std::filesystem::directory_iterator it(dir, ec);
    for (std::filesystem::directory_iterator end; !ec && it != end; it.increment(ec))
    {
        std::error_code entryEc;
        if (it->is_regular_file(entryEc))
        {
            const uintmax_t size = it->file_size(entryEc);
            ASSERT_FALSE(entryEc) << "Failed to measure " << it->path() << ": "
                                  << entryEc.message();
            usage.bytes += static_cast<size_t>(size);
            ++usage.files;
        }
    }
    ASSERT_FALSE(ec) << "Failed to list " << dir << ": " << ec.message();
    *usageOut = usage;
}

// Drops a file from the kernel's page cache, so that it's next read from storage.  Sets
// |residentOut| to whether any is still resident, as it is in memory backed storage like tmpfs.
void EvictFromPageCache(const std::filesystem::path &path, bool *residentOut)
{
#if defined(ANGLE_CAN_EVICT_PAGE_CACHE)
    const int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
    ASSERT_GE(fd, 0) << "Failed to open " << path << ": " << strerror(errno);

    // Only clean pages can be dropped, so write the file out first
    bool evicted = fdatasync(fd) == 0 && posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED) == 0;

    // posix_fadvise() is only advice, so check with mincore(), whose mapping reads nothing
    struct stat fileStat = {};
    if (evicted && fstat(fd, &fileStat) == 0 && fileStat.st_size > 0)
    {
        const size_t size = static_cast<size_t>(fileStat.st_size);
        void *mapping     = mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
        if (mapping != MAP_FAILED)
        {
            const size_t pageSize = static_cast<size_t>(sysconf(_SC_PAGESIZE));
            std::vector<unsigned char> residency((size + pageSize - 1) / pageSize);
            if (mincore(mapping, size, residency.data()) == 0)
            {
                for (unsigned char page : residency)
                {
                    evicted = evicted && (page & 1) == 0;
                }
            }
            munmap(mapping, size);
        }
    }
    close(fd);
    *residentOut = !evicted;
#else
    *residentOut = false;
#endif
}

// Creates a directory for the benchmark's storage.  Linux often keeps /tmp in memory, where the
// page cache can't be dropped, so there it goes under the user's cache directory instead.
std::string CreateStorageDirectory()
{
#if defined(ANGLE_PLATFORM_LINUX)
    std::string cacheHome = GetEnvironmentVar("XDG_CACHE_HOME");
    if (cacheHome.empty() && !GetEnvironmentVar("HOME").empty())
    {
        cacheHome = GetEnvironmentVar("HOME") + "/.cache";
    }
    if (!cacheHome.empty())
    {
        const std::string dir =
            cacheHome + "/angle_pipeline_binary_perf." + std::to_string(getpid());
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
        std::filesystem::create_directories(dir, ec);
        if (!ec)
        {
            return dir;
        }
        std::cout << "Failed to create " << dir << ": " << ec.message() << std::endl;
    }
#endif
    return CreateTestBlobCacheDirectory();
}

// What a phase measured while it ran, recorded once its Display is gone.
struct PhaseResult
{
    double timeToLinkAllMs   = 0;
    double timeToFirstDrawMs = 0;
    double timeToAllDrawsMs  = 0;
    size_t residentMemory    = 0;
};

class VulkanPipelineBinaryBenchmark : public ANGLERenderTest,
                                      public ::testing::WithParamInterface<PipelineBinaryPerfParams>
{
  public:
    VulkanPipelineBinaryBenchmark();
    ~VulkanPipelineBinaryBenchmark() override;

    void initializeBenchmark() override;
    void drawBenchmark() override;

  private:
    void registerBlobCache();
    void evictBlobCache();
    void setUpDrawResources();
    void showDrawsInWindow();
    void present();
    void presentIfFrameElapsed();
    void runPhase(CachePhase phase);
    void recordPhase(CachePhase phase);
    perf_test::PerfResultReporter *getPhaseReporter(CachePhase phase) const;
    const std::string &getPhaseStory(CachePhase phase) const;
    void recordPhaseTime(CachePhase phase, const char *metric, double milliseconds);
    void recordPhaseInteger(CachePhase phase,
                            const char *metric,
                            size_t value,
                            const char *units,
                            const char *histogramUnits);

    // The blob cache, in a directory of the story's own
    std::string mStorageDir;
    std::string mBlobCacheDir;

    // Salts the story's shaders, so that driver shader caches, such as Mesa's, have never seen them
    uint64_t mShaderSalt = 0;

    // Partial's results are recorded for a story of their own
    std::string mPartialStory;
    std::unique_ptr<perf_test::PerfResultReporter> mPartialReporter;

    PhaseResult mPhaseResult;
    bool mWarnedResident = false;
    GLuint mFramebuffer  = 0;
    // Frames are presented on a fixed schedule from the start of the phase
    Timer mFrameTimer;
    double mNextPresentSeconds = 0;
};

VulkanPipelineBinaryBenchmark::VulkanPipelineBinaryBenchmark()
    : ANGLERenderTest("VulkanPipelineBinary", GetParam(), "ms")
{
    // The phases are the measurement and run in initializeBenchmark().  The harness's timed trials
    // still run, as the test runner requires their wall_time, but for a single empty step each.
    mStepsToRun = 1;

    // Without the leading '_', as the harness does with the test's own story
    mPartialStory = GetParam().partialStory().substr(1);
    mPartialReporter =
        std::make_unique<perf_test::PerfResultReporter>(mName + mBackend, mPartialStory);
    for (CachePhase phase : kPhases)
    {
        perf_test::PerfResultReporter *reporter = getPhaseReporter(phase);
        const std::string prefix                = std::string(".") + PhaseName(phase) + "_";
        // Only a workload that links every program before drawing has a step that does just that
        if (LinksAllFirst(GetParam().workload))
        {
            reporter->RegisterImportantMetric(prefix + "time_to_link_all", "ms");
        }
        reporter->RegisterImportantMetric(prefix + "time_to_first_draw", "ms");
        reporter->RegisterImportantMetric(prefix + "time_to_all_draws", "ms");
        reporter->RegisterFyiMetric(prefix + "resident_memory", "sizeInBytes");
        reporter->RegisterImportantMetric(prefix + "disk_cache_size", "sizeInBytes");
        reporter->RegisterFyiMetric(prefix + "disk_cache_files", "count");
        reporter->RegisterFyiMetric(prefix + "blob_cache_hits", "count");
        reporter->RegisterFyiMetric(prefix + "blob_cache_stores", "count");
    }

    // A driver's own shader cache outlives the process, and other stories draw the same programs,
    // so the story's shaders are salted with the time it started, in nanoseconds
    mShaderSalt = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                            std::chrono::system_clock::now().time_since_epoch())
                                            .count());

    // The storage has to be in place before the Display is created
    mStorageDir = CreateStorageDirectory();
    if (mStorageDir.empty())
    {
        failTest("No directory to store the cache in");
        return;
    }
    mBlobCacheDir = mStorageDir + "/blob_cache";
}

VulkanPipelineBinaryBenchmark::~VulkanPipelineBinaryBenchmark()
{
    RemoveTestBlobCacheDirectory(mStorageDir);
}

// Opening the directory again is what a process restart looks like to the test blob cache.
void VulkanPipelineBinaryBenchmark::registerBlobCache()
{
    TestBlobCacheLimits limits;
    limits.maxTotalSize    = kBlobCacheMaxTotalSize;
    limits.maxTotalEntries = kBlobCacheMaxTotalEntries;
    InitTestBlobCache(static_cast<EGLDisplay>(getGLWindow()->getCurrentDisplay()),
                      eglSetBlobCacheFuncsANDROID, mBlobCacheDir, limits);
}

// Drops the blob cache from the page cache, so the phase reads its entries from storage.
void VulkanPipelineBinaryBenchmark::evictBlobCache()
{
    std::error_code ec;
    std::filesystem::directory_iterator it(mBlobCacheDir, ec);
    bool resident = false;
    for (std::filesystem::directory_iterator end; !ec && it != end; it.increment(ec))
    {
        bool fileResident = false;
        ASSERT_NO_FATAL_FAILURE(EvictFromPageCache(it->path(), &fileResident));
        resident = resident || fileResident;
    }
    ASSERT_FALSE(ec) << "Failed to list " << mBlobCacheDir << ": " << ec.message();
    if (resident && !mWarnedResident)
    {
        std::cout << "Part of the blob cache is still resident after eviction, is " << mBlobCacheDir
                  << " in memory backed storage?" << std::endl;
        mWarnedResident = true;
    }
}

void VulkanPipelineBinaryBenchmark::setUpDrawResources()
{
    // A quad as {x, y, u, v}
    constexpr std::array<float, 24> kQuad = {
        -1, -1, 0, 0, 1, -1, 1, 0, -1, 1, 0, 1, -1, 1, 0, 1, 1, -1, 1, 0, 1, 1, 1, 1,
    };
    GLuint vertexArray = 0;
    GLuint buffer      = 0;
    glGenVertexArrays(1, &vertexArray);
    glGenBuffers(1, &buffer);
    glBindVertexArray(vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kQuad), kQuad.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<const void *>(2 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    // The render target
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, kTargetSize, kTargetSize);
    glGenFramebuffers(1, &mFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    ASSERT_EQ(static_cast<GLenum>(GL_FRAMEBUFFER_COMPLETE),
              glCheckFramebufferStatus(GL_FRAMEBUFFER));

    // Not black, so the window shows the benchmark is alive before the first frame is due
    glClearColor(0.239f, 0.863f, 0.518f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    present();
}

// Copies the draws' target to the window.
void VulkanPipelineBinaryBenchmark::showDrawsInWindow()
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, mFramebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, kTargetSize, kTargetSize, 0, 0, GetParam().windowWidth,
                      GetParam().windowHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
}

void VulkanPipelineBinaryBenchmark::present()
{
    showDrawsInWindow();
    getGLWindow()->swap();
}

// Presents on a fixed schedule, so the timing of the calls doesn't drift the frame rate.  Frames
// missed during a stall are skipped rather than presented back to back.
void VulkanPipelineBinaryBenchmark::presentIfFrameElapsed()
{
    const double now = mFrameTimer.getElapsedWallClockTime();
    if (now >= mNextPresentSeconds)
    {
        present();
        mNextPresentSeconds += kFrameSeconds;
        while (mNextPresentSeconds <= now)
        {
            mNextPresentSeconds += kFrameSeconds;
        }
    }
}

// Partial draws only some of the programs, so its results go to a story of their own.
perf_test::PerfResultReporter *VulkanPipelineBinaryBenchmark::getPhaseReporter(
    CachePhase phase) const
{
    if (phase == CachePhase::Partial)
    {
        return mPartialReporter.get();
    }
    ASSERT(phase == CachePhase::Cold || phase == CachePhase::Warm);
    return mReporter.get();
}

const std::string &VulkanPipelineBinaryBenchmark::getPhaseStory(CachePhase phase) const
{
    if (phase == CachePhase::Partial)
    {
        return mPartialStory;
    }
    ASSERT(phase == CachePhase::Cold || phase == CachePhase::Warm);
    return mStory;
}

// Records a phase's metric for the console and for the perf dashboard.
void VulkanPipelineBinaryBenchmark::recordPhaseTime(CachePhase phase,
                                                    const char *metric,
                                                    double milliseconds)
{
    const std::string name = std::string(".") + PhaseName(phase) + "_" + metric;
    recordDoubleMetricForStory(getPhaseReporter(phase), getPhaseStory(phase), name.c_str(),
                               milliseconds, "ms");
    addHistogramSampleForStory(getPhaseStory(phase), name.c_str(), milliseconds,
                               "msBestFitFormat_smallerIsBetter");
}

void VulkanPipelineBinaryBenchmark::recordPhaseInteger(CachePhase phase,
                                                       const char *metric,
                                                       size_t value,
                                                       const char *units,
                                                       const char *histogramUnits)
{
    const std::string name = std::string(".") + PhaseName(phase) + "_" + metric;
    recordIntegerMetricForStory(getPhaseReporter(phase), getPhaseStory(phase), name.c_str(), value,
                                units);
    addHistogramSampleForStory(getPhaseStory(phase), name.c_str(), static_cast<double>(value),
                               histogramUnits);
}

// Links and draws the phase's working set, timing how long it takes.
void VulkanPipelineBinaryBenchmark::runPhase(CachePhase phase)
{
    const PipelineBinaryPerfParams &param = GetParam();
    const std::vector<size_t> workingSet  = param.workingSet(phase);
    const bool linkAllFirst               = LinksAllFirst(param.workload);
    mPhaseResult                          = {};

    // Generating sources is the application's own work, so it isn't timed
    std::vector<ProgramSource> sources(workingSet.size());
    for (size_t i = 0; i < workingSet.size(); ++i)
    {
        // Every phase of the story uses the same salt, so later phases find what cold stored
        sources[i] = GenerateProgramSource(workingSet[i], mShaderSalt);
    }

    std::vector<GLuint> programs(workingSet.size(), 0);
    Timer timer;
    timer.start();
    mFrameTimer.restart();
    mNextPresentSeconds = kFrameSeconds;

    // Link every program before drawing any, as applications do while loading
    if (linkAllFirst)
    {
        for (size_t i = 0; i < workingSet.size(); ++i)
        {
            programs[i] = CompileAndLinkProgram(sources[i]);
            sources[i]  = {};
            presentIfFrameElapsed();
        }
        // A program that failed to link would draw nothing, and report a very good time
        for (size_t i = 0; i < workingSet.size(); ++i)
        {
            ASSERT_NE(0u, CheckLinkStatusAndReturnProgram(programs[i], true))
                << "Program " << workingSet[i] << " failed to link";
        }
        mPhaseResult.timeToLinkAllMs = timer.getElapsedWallClockTime() * 1000.0;
        timer.restart();
    }

    // Draw every program once, waiting for the GPU at the first draw and the last
    for (size_t i = 0; i < workingSet.size(); ++i)
    {
        // Link each program just before drawing it
        if (!linkAllFirst)
        {
            programs[i] = CompileAndLinkProgram(sources[i]);
            sources[i]  = {};
        }
        glUseProgram(programs[i]);
        glUniform4f(glGetUniformLocation(programs[i], "u_param"), 0.5f, 0.5f, 1.0f, 0.0f);

        const GLsizei cell = static_cast<GLsizei>(i % kCellCount);
        glViewport((cell % kCellsPerRow) * kCellSize, (cell / kCellsPerRow) * kCellSize, kCellSize,
                   kCellSize);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        if (i == 0)
        {
            glFinish();
            mPhaseResult.timeToFirstDrawMs = timer.getElapsedWallClockTime() * 1000.0;
        }
        if (i + 1 == workingSet.size())
        {
            glFinish();
            mPhaseResult.timeToAllDrawsMs = timer.getElapsedWallClockTime() * 1000.0;
        }
        presentIfFrameElapsed();

        if (!linkAllFirst)
        {
            // The draw has resolved the link, and the program's one pipeline is drawn
            ASSERT_NE(0u, CheckLinkStatusAndReturnProgram(programs[i], true))
                << "Program " << workingSet[i] << " failed to link";
        }
        glDeleteProgram(programs[i]);
    }

    // The application keeps running after its last new pipeline
    for (int frame = 0; frame < kIdleFrames; ++frame)
    {
        const double remaining = mNextPresentSeconds - mFrameTimer.getElapsedWallClockTime();
        if (remaining > 0)
        {
            std::this_thread::sleep_for(std::chrono::duration<double>(remaining));
        }
        presentIfFrameElapsed();
    }

    mPhaseResult.residentMemory = GetProcessMemoryUsageKB() * 1000;
    ASSERT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
}

// Records the phase once its Display is gone, so whatever ANGLE was still writing has landed.
void VulkanPipelineBinaryBenchmark::recordPhase(CachePhase phase)
{
    DiskUsage usage;
    ASSERT_NO_FATAL_FAILURE(MeasureDiskUsage(mBlobCacheDir, &usage));
    const TestBlobCacheStats stats = GetTestBlobCacheStats();

    // A later phase draws nothing the cold phase didn't, so whatever it stores it didn't find
    ASSERT_TRUE(phase == CachePhase::Cold || stats.stores == 0)
        << PhaseName(phase) << " phase stored " << stats.stores
        << " entries the cold phase should have, so it measured a cold cache";

    if (LinksAllFirst(GetParam().workload))
    {
        recordPhaseTime(phase, "time_to_link_all", mPhaseResult.timeToLinkAllMs);
    }
    recordPhaseTime(phase, "time_to_first_draw", mPhaseResult.timeToFirstDrawMs);
    recordPhaseTime(phase, "time_to_all_draws", mPhaseResult.timeToAllDrawsMs);
    recordPhaseInteger(phase, "resident_memory", mPhaseResult.residentMemory, "sizeInBytes",
                       "sizeInBytes_smallerIsBetter");
    recordPhaseInteger(phase, "disk_cache_size", usage.bytes, "sizeInBytes",
                       "sizeInBytes_smallerIsBetter");
    recordPhaseInteger(phase, "disk_cache_files", usage.files, "count", "count");
    recordPhaseInteger(phase, "blob_cache_hits", static_cast<size_t>(stats.hits), "count", "count");
    recordPhaseInteger(phase, "blob_cache_stores", static_cast<size_t>(stats.stores), "count",
                       "count");

    if (phase == CachePhase::Cold)
    {
        ASSERT_GT(stats.stores, 0u) << "The cold phase stored nothing";
    }
    else
    {
        ASSERT(phase == CachePhase::Warm || phase == CachePhase::Partial);
        ASSERT_GT(stats.hits, 0u) << PhaseName(phase) << " phase read nothing from the cache";
    }
}

// Runs every phase, each on a new Display, and stops at the first failure.  The last Display stays
// for the harness to tear down.
void VulkanPipelineBinaryBenchmark::initializeBenchmark()
{
    for (CachePhase phase : kPhases)
    {
        registerBlobCache();
        ASSERT_NO_FATAL_FAILURE(evictBlobCache());
        ASSERT_NO_FATAL_FAILURE(setUpDrawResources());
        ASSERT_NO_FATAL_FAILURE(runPhase(phase));

        destroyGLWindow();
        ASSERT_NO_FATAL_FAILURE(recordPhase(phase));
        if (!createGLWindow())
        {
            return;
        }
    }
}

// The step of the harness's timed trials, which has nothing to do.
void VulkanPipelineBinaryBenchmark::drawBenchmark() {}

// Runs the story's phases and reports how quickly each gets its pipelines back.
TEST_P(VulkanPipelineBinaryBenchmark, Run)
{
    run();
}

class VulkanPipelineBinaryLargeBenchmark : public VulkanPipelineBinaryBenchmark
{};

// The same for stories too large to run by default.
TEST_P(VulkanPipelineBinaryLargeBenchmark, DISABLED_Run)
{
    run();
}

// Adds a story for each workload.
void AddStories(size_t programCount, std::vector<PipelineBinaryPerfParams> *stories)
{
    constexpr std::array<Workload, 2> kWorkloads = {Workload::DrawImmediately,
                                                    Workload::DeferredDraw};
    for (Workload workload : kWorkloads)
    {
        stories->push_back(PipelineBinaryPerfParams(workload, programCount));
    }
}

std::vector<PipelineBinaryPerfParams> PipelineBinaryPerfStories()
{
    std::vector<PipelineBinaryPerfParams> stories;
    AddStories(100, &stories);
    AddStories(1000, &stories);
    return stories;
}

std::vector<PipelineBinaryPerfParams> PipelineBinaryLargePerfStories()
{
    std::vector<PipelineBinaryPerfParams> stories;
    AddStories(10000, &stories);
    AddStories(20000, &stories);
    return stories;
}

ANGLE_INSTANTIATE_TEST_ARRAY(VulkanPipelineBinaryBenchmark, PipelineBinaryPerfStories());
ANGLE_INSTANTIATE_TEST_ARRAY(VulkanPipelineBinaryLargeBenchmark, PipelineBinaryLargePerfStories());

}  // anonymous namespace
