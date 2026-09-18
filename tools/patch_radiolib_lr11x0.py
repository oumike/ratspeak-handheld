"""Allow RadioLib to initialize LR1110 modules with pre-0x0308 firmware."""

Import("env")

import os


MARKER = "ratspeak-lr1110-oldfw-patch"
OLD = """  state = this->driveDiosInSleepMode(true);
  RADIOLIB_ASSERT(state);"""
NEW = """  state = this->driveDiosInSleepMode(true);
  // ratspeak-lr1110-oldfw-patch: old LR1110 firmware does not implement
  // DriveDiosInSleepMode. It is optional, so ignore only that command error.
  if(state == RADIOLIB_ERR_SPI_CMD_INVALID) {
    state = RADIOLIB_ERR_NONE;
  }
  RADIOLIB_ASSERT(state);"""

path = os.path.join(
    env.subst("$PROJECT_LIBDEPS_DIR"),
    env.subst("$PIOENV"),
    "RadioLib",
    "src",
    "modules",
    "LR11x0",
    "LR11x0.cpp",
)


def apply_patch(source_path=path):
    with open(source_path, encoding="utf-8") as source_file:
        source = source_file.read()
    if MARKER in source:
        return None
    if OLD not in source:
        return "RadioLib LR11x0::config() no longer matches the pinned patch"
    with open(source_path, "w", encoding="utf-8") as source_file:
        source_file.write(source.replace(OLD, NEW, 1))
    print("[patch_radiolib_lr11x0] enabled old LR1110 firmware support")
    return None


if os.path.isfile(path):
    error = apply_patch()
    if error:
        print(f"[patch_radiolib_lr11x0] ERROR: {error}")
        env.Exit(1)


def patch_before_compile(source_node):
    source_path = source_node.srcnode().get_abspath()
    error = apply_patch(source_path)
    if error:
        raise RuntimeError(f"[patch_radiolib_lr11x0] {error}")
    return source_node


env.AddBuildMiddleware(
    patch_before_compile,
    "*/RadioLib/src/modules/LR11x0/LR11x0.cpp",
)


def verify_patched(target, source, env):
    if not os.path.isfile(path):
        print(f"[patch_radiolib_lr11x0] ERROR: RadioLib source missing: {path}")
        return 1
    with open(path, encoding="utf-8") as source_file:
        if MARKER in source_file.read():
            return 0
    error = apply_patch()
    if error:
        print(f"[patch_radiolib_lr11x0] ERROR: {error}")
        return 1
    print("[patch_radiolib_lr11x0] ERROR: source was not patched before compilation")
    return 1


env.AddPreAction("$BUILD_DIR/${PROGNAME}.elf", verify_patched)