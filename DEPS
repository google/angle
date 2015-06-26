deps = {
  "third_party/gyp":
      "http://chromium.googlesource.com/external/gyp@b4781fc38236b0fb1238969c918a75a200cfffdb",

  # TODO(kbr): figure out how to better stay in sync with Chromium's
  # versions of googletest and googlemock.
  "src/tests/third_party/googletest":
      "http://chromium.googlesource.com/external/googletest.git@23574bf2333f834ff665f894c97bef8a5b33a0a9",

  "src/tests/third_party/googlemock":
      "http://chromium.googlesource.com/external/googlemock.git@b2cb211e49d872101d991201362d7b97d7d69910",

  "third_party/deqp/src":
      "https://android.googlesource.com/platform/external/deqp@92f7752da82925ca5e7288c5b4814efa7a381d89",

  "third_party/libpng":
      "https://android.googlesource.com/platform/external/libpng@094e181e79a3d6c23fd005679025058b7df1ad6c",

  "third_party/zlib":
      "https://chromium.googlesource.com/chromium/src/third_party/zlib@afd8c4593c010c045902f6c0501718f1823064a3",
}

hooks = [
  {
    # A change to a .gyp, .gypi, or to GYP itself should run the generator.
    "pattern": ".",
    "action": ["python", "build/gyp_angle"],
  },
]
