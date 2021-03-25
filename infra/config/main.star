#!/usr/bin/env lucicfg
#
# Copyright 2021 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# main.star: lucicfg configuration for ANGLE's standalone builers.

lucicfg.config(fail_on_warnings = True)

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
            groups = "project-angle-admins"
        ),
        acl.entry(
            roles = [
                acl.LOGDOG_WRITER,
            ],
            groups = "luci-logdog-angle-writers",
        ),
    ]
)

luci.milo(
    logo = "https://storage.googleapis.com/chrome-infra/OpenGL%20ES_RGB_June16.svg",
    monorail_project = "angleproject",
    monorail_components = ["Infra"],
)

luci.logdog(gs_bucket = "chromium-luci-logdog")

# The category for an os: a more generic grouping than specific OS versions that
# can be used for computing defaults
os_category = struct(
    LINUX = "Linux",
    MAC = "Mac",
    WINDOWS = "Windows",
)

def os_enum(dimension, category, console_name):
    return struct(dimension = dimension, category = category, console_name = console_name)

os = struct(
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
    name = "recipe:run_presubmit"
)

def get_os_from_name(name):
    if name.startswith("linux"):
        return os.LINUX
    if name.startswith("win"):
        return os.WINDOWS
    if name.startswith("mac"):
        return os.MAC
    return os.MAC

# Adds both the CI and Try standalone builders.
def angle_standalone_builder(name, clang, debug, cpu, uwp = False, trace_tests = False):
    properties = {
        "debug": debug,
        "target_cpu": cpu,
    }
    os = get_os_from_name(name)
    dimensions = {}
    dimensions["os"] = os.dimension

    goma_props = {}
    goma_props.update(_GOMA_RBE_PROD)

    if os.category in _DEFAULT_BUILDERLESS_OS_CATEGORIES:
        dimensions["builderless"] = "1"
        goma_props["enable_ats"] = True

    properties["$build/goma"] = goma_props

    caches = []
    if os.category == os_category.WINDOWS:
        caches += [swarming.cache(name = "win_toolchain", path = "win_toolchain")]

    if os.category == os_category.MAC:
        # Cache for mac_toolchain tool and XCode.app
        caches += [swarming.cache(name = "osx_sdk", path = "osx_sdk")]
        properties["$depot_tools/osx_sdk"] = {
            "sdk_version": "12D4e"
        }

    if not clang:
        properties["clang"] = False

    if uwp:
        properties["uwp"] = True

    if trace_tests:
        properties["trace_tests"] = True

    luci.builder(
        name = name,
        bucket = "ci",
        triggered_by = ["master-poller"],
        executable = "recipe:angle",
        service_account = "angle-ci-builder@chops-service-accounts.iam.gserviceaccount.com",
        properties = properties,
        dimensions = dimensions,
        caches = caches,
        build_numbers = True,
    )

    luci.builder(
        name = name,
        bucket = "try",
        executable = "recipe:angle",
        service_account = "angle-try-builder@chops-service-accounts.iam.gserviceaccount.com",
        properties = properties,
        dimensions = dimensions,
        caches = caches,
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

    if clang:
        compiler = "clang"
    elif os.category == os_category.WINDOWS:
        compiler = "msvc"
    else:
        compiler = "gcc"

    if uwp:
        os = "winuwp"
    else:
        os = os.console_name

    short_name = "dbg" if debug else "rel"

    luci.console_view_entry(
        console_view = "ci",
        builder = "ci/" + name,
        category = config + "|" + os + "|" + compiler + "|" + cpu,
        short_name = short_name,
    )

    luci.list_view_entry(
        list_view = "try",
        builder = "try/" + name,
    )

    # Include all bots in the CQ by default except GCC configs.
    if compiler != "gcc":
        luci.cq_tryjob_verifier(
            cq_group = 'master',
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
                "luci-scheduler@appspot.gserviceaccount.com",
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
angle_standalone_builder("linux-clang-dbg", clang = True, debug = True, cpu = "x64")
angle_standalone_builder("linux-clang-rel", clang = True, debug = False, cpu = "x64")
angle_standalone_builder("linux-gcc-dbg", clang = False, debug = True, cpu = "x64")
angle_standalone_builder("linux-gcc-rel", clang = False, debug = False, cpu = "x64")
angle_standalone_builder("linux-trace-rel", clang = True, debug = False, cpu = "x64", trace_tests = True)
angle_standalone_builder("mac-dbg", clang = True, debug = True, cpu = "x64")
angle_standalone_builder("mac-rel", clang = True, debug = False, cpu = "x64")
angle_standalone_builder("win-clang-x86-dbg", clang = True, debug = True, cpu = "x86")
angle_standalone_builder("win-clang-x86-rel", clang = True, debug = False, cpu = "x86")
angle_standalone_builder("win-clang-x64-dbg", clang = True, debug = True, cpu = "x64")
angle_standalone_builder("win-clang-x64-rel", clang = True, debug = False, cpu = "x64")
angle_standalone_builder("win-msvc-x86-dbg", clang = False, debug = True, cpu = "x86")
angle_standalone_builder("win-msvc-x86-rel", clang = False, debug = False, cpu = "x86")
angle_standalone_builder("win-msvc-x64-dbg", clang = False, debug = True, cpu = "x64")
angle_standalone_builder("win-msvc-x64-rel", clang = False, debug = False, cpu = "x64")
angle_standalone_builder("win-trace-rel", clang = True, debug = False, cpu = "x64", trace_tests = True)
angle_standalone_builder("winuwp-x64-dbg", clang = False, debug = True, cpu = "x64", uwp = True)
angle_standalone_builder("winuwp-x64-rel", clang = False, debug = False, cpu = "x64", uwp = True)

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
    status_host = 'chromium-cq-status.appspot.com',
    submit_max_burst = 4,
    submit_burst_delay = 480 * time.second,
)

luci.cq_group(
    name = 'master',
    watch = cq.refset('https://chromium.googlesource.com/angle/angle'),
    acls = [
        acl.entry(
                acl.CQ_COMMITTER,
            groups = 'project-angle-committers',
        ),
        acl.entry(
            acl.CQ_DRY_RUNNER,
            groups = 'project-angle-tryjob-access',
        ),
    ],
    verifiers = [
        luci.cq_tryjob_verifier(
            builder = 'angle:try/presubmit',
            disable_reuse = True,
        ),
        luci.cq_tryjob_verifier(
            builder = 'chromium:try/android_angle_deqp_rel_ng',
        ),
        luci.cq_tryjob_verifier(
            builder = 'chromium:try/android_angle_rel_ng',
        ),
        luci.cq_tryjob_verifier(
            builder = 'chromium:try/android_angle_vk64_deqp_rel_ng',
        ),
        luci.cq_tryjob_verifier(
            builder = 'chromium:try/android_angle_vk64_rel_ng',
        ),
        luci.cq_tryjob_verifier(
            builder = 'chromium:try/fuchsia-angle-rel',
        ),
        luci.cq_tryjob_verifier(
            builder = 'chromium:try/linux_angle_deqp_rel_ng',
        ),
        luci.cq_tryjob_verifier(
            builder = 'chromium:try/linux_angle_ozone_rel_ng',
        ),
        luci.cq_tryjob_verifier(
            builder = 'chromium:try/linux-angle-rel',
        ),
        luci.cq_tryjob_verifier(
            builder = 'chromium:try/linux-swangle-try-tot-angle-x64',
        ),
        luci.cq_tryjob_verifier(
            builder = 'chromium:try/mac-angle-chromium-try',
        ),
        luci.cq_tryjob_verifier(
            builder = 'chromium:try/mac-angle-try',
        ),
        luci.cq_tryjob_verifier(
            builder = 'chromium:try/win-angle-chromium-x64-try',
        ),
        luci.cq_tryjob_verifier(
            builder = 'chromium:try/win-angle-chromium-x86-try',
        ),
        luci.cq_tryjob_verifier(
            builder = 'chromium:try/win-angle-x64-try',
        ),
        luci.cq_tryjob_verifier(
            builder = 'chromium:try/win-angle-x86-try',
        ),
        luci.cq_tryjob_verifier(
            builder = 'chromium:try/win-swangle-try-tot-angle-x86',
        ),
    ],
)

