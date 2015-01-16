deps = {
  "third_party/gyp":
      "http://gyp.googlecode.com/svn/trunk@1987",

  # TODO(kbr): figure out how to better stay in sync with Chromium's
  # versions of googletest and googlemock.
  "tests/third_party/googletest":
      "http://googletest.googlecode.com/svn/trunk@629",

  "tests/third_party/googlemock":
      "http://googlemock.googlecode.com/svn/trunk@410",

  "tests/third_party/rapidjson":
      "https://github.com/miloyip/rapidjson.git@24dd7ef839b79941d129e833368e913aa9c045da",
}

hooks = [
  {
    # A change to a .gyp, .gypi, or to GYP itself should run the generator.
    "pattern": ".",
    "action": ["python", "build/gyp_angle"],
  },
]
