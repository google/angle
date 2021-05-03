#!/usr/bin/env lucicfg
#
# Copyright 2021 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# main.star: lucicfg configuration for ANGLE's standalone builers.

lucicfg.config(
    fail_on_warnings = True,
    lint_checks = [
        "default",
        "-module-docstring",
        "-function-docstring",
    ],
)

# Enable LUCI Realms support.
lucicfg.enable_experiment("crbug.com/1085650")
# Launch 0% of Swarming tasks for builds in "realms-aware mode"
# TODO(https://crbug.com/1204972): ramp up to 100%.
# luci.builder.defaults.experiments.set({"luci.use_realms": 0})

# Enable LUCI Realms support.
lucicfg.enable_experiment("crbug.com/1085650")

# Launch 25% of Swarming tasks for builds in "realms-aware mode"
# TODO(https://crbug.com/1204972): ramp up to 100%.
luci.builder.defaults.experiments.set({"luci.use_realms": 25})

luci.project(
    name = "angle",
    buildbucket = "cr-buildbucket.appspot.com",
    logdog = "luci-logdog.appspot.com",
    milo = "luci-milo.appspot.com",
    notify = "luci-notify.appspot.com",
    scheduler = "luci-scheduler.appspot.com",
    swarming = "chromium-swarm.appspot.com",
    acls = [
        acl.entry(
            roles = [
                acl.PROJECT_CONFIGS_READER,
                acl.LOGDOG_READER,
                acl.BUILDBUCKET_READER,
                acl.SCHEDULER_READER,
            ],
            groups = "all",
        ),
        acl.entry(
            roles = [
                acl.SCHEDULER_OWNER,
            ],
            groups = "project-angle-admins",
        ),
        acl.entry(
            roles = [
                acl.LOGDOG_WRITER,
            ],
            groups = "luci-logdog-angle-writers",
        ),
    ],
    bindings = [
        luci.binding(
            roles = "role/swarming.poolOwner",
            groups = ["project-angle-owners", "mdb/chrome-troopers"],
        ),
        luci.binding(
            roles = "role/swarming.poolViewer",
            groups = "all",
        ),
        # Allow any Angle build to trigger a test ran under testing accounts
        # used on shared chromium tester pools.
        luci.binding(
            roles = "role/swarming.taskServiceAccount",
            users = [
                "chromium-tester@chops-service-accounts.iam.gserviceaccount.com",
                "chrome-gpu-gold@chops-service-accounts.iam.gserviceaccount.com",
            ],
        ),
    ],
)

# Swarming permissions
luci.realm(name = "pools/ci")
luci.realm(name = "pools/try")

# Allow Angle owners and Chrome troopers to run tasks directly for testing and
# development on all Angle bots. E.g. via `led` tool or "Debug" button in Swarming Web UI.
luci.binding(
    realm = "@root",
    roles = "role/swarming.poolUser",
    groups = ["project-angle-owners", "mdb/chrome-troopers"],
)
luci.binding(
    realm = "@root",
    roles = "role/swarming.taskTriggerer",
    groups = ["project-angle-owners", "mdb/chrome-troopers"],
)

def _generate_project_pyl(ctx):
    ctx.output["project.pyl"] = "\n".join([
        "# This is a non-LUCI generated file",
        "# This is consumed by presubmit checks that need to validate the config",
        repr(dict(
            # We don't validate matching source-side configs for simplicity.
            validate_source_side_specs_have_builder = False,
        )),
        "",
    ])

lucicfg.generator(_generate_project_pyl)

luci.milo(
    logo = "https://storage.googleapis.com/chrome-infra/OpenGL%20ES_RGB_June16.svg",
    monorail_project = "angleproject",
    monorail_components = ["Infra"],
)

luci.logdog(gs_bucket = "chromium-luci-logdog")

# The category for an os: a more generic grouping than specific OS versions that
# can be used for computing defaults
os_category = struct(
    ANDROID = "Android",
    LINUX = "Linux",
    MAC = "Mac",
    WINDOWS = "Windows",
)

def os_enum(dimension, category, console_name):
    return struct(dimension = dimension, category = category, console_name = console_name)

os = struct(
    ANDROID = os_enum("Ubuntu", os_category.ANDROID, "android"),
    LINUX = os_enum("Ubuntu", os_category.LINUX, "linux"),
    MAC = os_enum("Mac", os_category.MAC, "mac"),
    WINDOWS = os_enum("Windows", os_category.WINDOWS, "win"),
)

# Recipes

_RECIPE_NAME_PREFIX = "recipe:"
_DEFAULT_BUILDERLESS_OS_CATEGORIES = [os_category.LINUX, os_category.WINDOWS]
_GOMA_RBE_PROD = {
    "server_host": "goma.chromium.org",
    "rpc_extra_params": "?prod",
}

def _recipe_for_package(cipd_package):
    def recipe(*, name, cipd_version = None, recipe = None, use_bbagent = False):
        # Force the caller to put the recipe prefix rather than adding it
        # programatically to make the string greppable
        if not name.startswith(_RECIPE_NAME_PREFIX):
            fail("Recipe name {!r} does not start with {!r}"
                .format(name, _RECIPE_NAME_PREFIX))
        if recipe == None:
            recipe = name[len(_RECIPE_NAME_PREFIX):]
        return luci.recipe(
            name = name,
            cipd_package = cipd_package,
            cipd_version = cipd_version,
            recipe = recipe,
            use_bbagent = use_bbagent,
        )

    return recipe

build_recipe = _recipe_for_package(
    "infra/recipe_bundles/chromium.googlesource.com/chromium/tools/build",
)

build_recipe(
    name = "recipe:angle",
)

build_recipe(
    name = "recipe:run_presubmit",
)

def get_os_from_name(name):
    if name.startswith("android"):
        return os.ANDROID
    if name.startswith("linux"):
        return os.LINUX
    if name.startswith("win"):
        return os.WINDOWS
    if name.startswith("mac"):
        return os.MAC
    return os.MAC

# Adds both the CI and Try standalone builders.
def angle_standalone_builder(name, debug, cpu, toolchain = "clang", uwp = False, trace_tests = False):
    properties = {
        "builder_group": "angle",
    }
    config_os = get_os_from_name(name)
    dimensions = {}
    dimensions["os"] = config_os.dimension

    goma_props = {}
    goma_props.update(_GOMA_RBE_PROD)

    if config_os.category in _DEFAULT_BUILDERLESS_OS_CATEGORIES:
        dimensions["builderless"] = "1"
        goma_props["enable_ats"] = True

    isolated_props = {"server": "https://isolateserver.appspot.com"}

    properties["$build/goma"] = goma_props
    properties["$recipe_engine/isolated"] = isolated_props
    properties["platform"] = config_os.console_name
    properties["toolchain"] = toolchain

    if trace_tests:
        properties["test_mode"] = "trace_tests"
    elif toolchain == "gcc":
        properties["test_mode"] = "checkout_only"
    elif debug or toolchain == "msvc" or config_os.category == os_category.ANDROID:
        properties["test_mode"] = "compile_only"
    else:
        properties["test_mode"] = "compile_and_test"

    luci.builder(
        name = name,
        bucket = "ci",
        triggered_by = ["master-poller"],
        executable = "recipe:angle",
        service_account = "angle-ci-builder@chops-service-accounts.iam.gserviceaccount.com",
        properties = properties,
        dimensions = dimensions,
        build_numbers = True,
    )

    luci.builder(
        name = name,
        bucket = "try",
        executable = "recipe:angle",
        service_account = "angle-try-builder@chops-service-accounts.iam.gserviceaccount.com",
        properties = properties,
        dimensions = dimensions,
        build_numbers = True,
    )

    # Trace tests are only included automatically if files in the capture folder change.
    if trace_tests:
        config = "trace"
        location_regexp = [
            ".+/[+]/src/libANGLE/capture/.+",
            ".+/[+]/src/tests/capture.+",
        ]
    else:
        config = "angle"
        location_regexp = None

    if uwp:
        os_name = "winuwp"
    else:
        os_name = config_os.console_name

    short_name = "dbg" if debug else "rel"

    luci.console_view_entry(
        console_view = "ci",
        builder = "ci/" + name,
        category = config + "|" + os_name + "|" + toolchain + "|" + cpu,
        short_name = short_name,
    )

    luci.list_view_entry(
        list_view = "try",
        builder = "try/" + name,
    )

    # Include all bots in the CQ by default except GCC and Android configs.
    if toolchain != "gcc":
        luci.cq_tryjob_verifier(
            cq_group = "master",
            builder = "angle:try/" + name,
            location_regexp = location_regexp,
        )

luci.bucket(
    name = "ci",
    acls = [
        acl.entry(
            acl.BUILDBUCKET_TRIGGERER,
            users = [
                "angle-ci-builder@chops-service-accounts.iam.gserviceaccount.com",
            ],
        ),
    ],
)

luci.bucket(
    name = "try",
    acls = [
        acl.entry(
            acl.BUILDBUCKET_TRIGGERER,
            groups = [
                "project-angle-tryjob-access",
                "service-account-cq",
            ],
        ),
    ],
)

luci.builder(
    name = "presubmit",
    bucket = "try",
    executable = "recipe:run_presubmit",
    service_account = "angle-try-builder@chops-service-accounts.iam.gserviceaccount.com",
    build_numbers = True,
    dimensions = {
        "os": os.LINUX.dimension,
    },
    properties = {
        "repo_name": "angle",
        "runhooks": True,
    },
)

luci.gitiles_poller(
    name = "master-poller",
    bucket = "ci",
    repo = "https://chromium.googlesource.com/angle/angle",
    refs = [
        "refs/heads/master",
    ],
    schedule = "with 10s interval",
)

# name, clang, debug, cpu, uwp, trace_tests
angle_standalone_builder("android-arm-dbg", debug = True, cpu = "arm")
angle_standalone_builder("android-arm-rel", debug = False, cpu = "arm")
angle_standalone_builder("android-arm64-dbg", debug = True, cpu = "arm64")
angle_standalone_builder("android-arm64-rel", debug = False, cpu = "arm64")
angle_standalone_builder("linux-clang-dbg", debug = True, cpu = "x64")
angle_standalone_builder("linux-clang-rel", debug = False, cpu = "x64")
angle_standalone_builder("linux-gcc-dbg", debug = True, cpu = "x64", toolchain = "gcc")
angle_standalone_builder("linux-gcc-rel", debug = False, cpu = "x64", toolchain = "gcc")
angle_standalone_builder("linux-trace-rel", debug = False, cpu = "x64", trace_tests = True)
angle_standalone_builder("mac-dbg", debug = True, cpu = "x64")
angle_standalone_builder("mac-rel", debug = False, cpu = "x64")
angle_standalone_builder("win-clang-x86-dbg", debug = True, cpu = "x86")
angle_standalone_builder("win-clang-x86-rel", debug = False, cpu = "x86")
angle_standalone_builder("win-clang-x64-dbg", debug = True, cpu = "x64")
angle_standalone_builder("win-clang-x64-rel", debug = False, cpu = "x64")
angle_standalone_builder("win-msvc-x86-dbg", debug = True, cpu = "x86", toolchain = "msvc")
angle_standalone_builder("win-msvc-x86-rel", debug = False, cpu = "x86", toolchain = "msvc")
angle_standalone_builder("win-msvc-x64-dbg", debug = True, cpu = "x64", toolchain = "msvc")
angle_standalone_builder("win-msvc-x64-rel", debug = False, cpu = "x64", toolchain = "msvc")
angle_standalone_builder("win-trace-rel", debug = False, cpu = "x64", trace_tests = True)
angle_standalone_builder("winuwp-x64-dbg", debug = True, cpu = "x64", toolchain = "msvc", uwp = True)
angle_standalone_builder("winuwp-x64-rel", debug = False, cpu = "x64", toolchain = "msvc", uwp = True)

# Views

luci.console_view(
    name = "ci",
    title = "ANGLE CI Builders",
    repo = "https://chromium.googlesource.com/angle/angle",
    refs = ["refs/heads/master"],
)

luci.list_view(
    name = "try",
    title = "ANGLE Try Builders",
)

luci.list_view_entry(
    list_view = "try",
    builder = "try/presubmit",
)

# CQ

luci.cq(
    status_host = "chromium-cq-status.appspot.com",
    submit_max_burst = 4,
    submit_burst_delay = 480 * time.second,
)

luci.cq_group(
    name = "master",
    watch = cq.refset(
        "https://chromium.googlesource.com/angle/angle",
        refs = [r"refs/heads/master", r"refs/heads/main"],
    ),
    acls = [
        acl.entry(
            acl.CQ_COMMITTER,
            groups = "project-angle-committers",
        ),
        acl.entry(
            acl.CQ_DRY_RUNNER,
            groups = "project-angle-tryjob-access",
        ),
    ],
    verifiers = [
        luci.cq_tryjob_verifier(
            builder = "angle:try/presubmit",
            disable_reuse = True,
        ),
        luci.cq_tryjob_verifier(
            builder = "chromium:try/android_angle_deqp_rel_ng",
        ),
        luci.cq_tryjob_verifier(
            builder = "chromium:try/android_angle_rel_ng",
        ),
        luci.cq_tryjob_verifier(
            builder = "chromium:try/android_angle_vk64_deqp_rel_ng",
        ),
        luci.cq_tryjob_verifier(
            builder = "chromium:try/android_angle_vk64_rel_ng",
        ),
        luci.cq_tryjob_verifier(
            builder = "chromium:try/fuchsia-angle-try",
        ),
        luci.cq_tryjob_verifier(
            builder = "chromium:try/linux-angle-chromium-try",
        ),
        luci.cq_tryjob_verifier(
            builder = "chromium:try/linux-angle-try",
        ),
        luci.cq_tryjob_verifier(
            builder = "chromium:try/linux-swangle-try-tot-angle-x64",
        ),
        luci.cq_tryjob_verifier(
            builder = "chromium:try/mac-angle-chromium-try",
        ),
        luci.cq_tryjob_verifier(
            builder = "chromium:try/mac-angle-try",
        ),
        luci.cq_tryjob_verifier(
            builder = "chromium:try/win-angle-chromium-x64-try",
        ),
        luci.cq_tryjob_verifier(
            builder = "chromium:try/win-angle-chromium-x86-try",
        ),
        luci.cq_tryjob_verifier(
            builder = "chromium:try/win-angle-x64-try",
        ),
        luci.cq_tryjob_verifier(
            builder = "chromium:try/win-angle-x86-try",
        ),
        luci.cq_tryjob_verifier(
            builder = "chromium:try/win-swangle-try-tot-angle-x86",
        ),
    ],
)
