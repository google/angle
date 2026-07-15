This directory is now effectively unused. Test specs now entirely handled via
Starlark in //infra/config/. This directory still exists because:

1. A default (non-builder-specific) test spec directory needs to be defined for
   a Chromium config.
2. The Starlark-generated GN args need to be pointed to by an entry in
   `angle_mb_config.pyl`
3. `trybot_analyze_config.json` is not generated, so there is not much reason to
   move it under //infra/config/.