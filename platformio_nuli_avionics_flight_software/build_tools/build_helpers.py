# SPDX-License-Identifier: MIT

import ast
import json
import os
import re

Import("env")


FIRMWARE_VERSION_ENV = "AVIONICS_FIRMWARE_VERSION"
FIRMWARE_VERSION_PATTERN = re.compile(r"^[A-Za-z0-9._+-]{1,19}$")

TARGETS = {
    "board": env.BoardConfig(),
}


def parse_value(value):
    if isinstance(value, list):
        return [parse_value(item) for item in value]
    if isinstance(value, tuple):
        return tuple(parse_value(item) for item in value)

    text = str(value).strip()
    lower_text = text.lower()

    if lower_text == "true":
        return True
    if lower_text == "false":
        return False
    if lower_text in ("none", "null"):
        return None

    if text[:1] in ("[", "{", '"', "'"):
        try:
            return json.loads(text)
        except ValueError:
            return ast.literal_eval(text)

    return text


def iter_config_overrides():
    for option_name, option_value in env.GetProjectOptions():
        target_name, separator, config_path = option_name.partition(".")
        if not separator or target_name not in TARGETS:
            continue
        if not config_path:
            raise ValueError(f"{option_name} must include a config path")
        yield target_name, config_path, parse_value(option_value)


applied_overrides = []

for target_name, config_path, value in iter_config_overrides():
    TARGETS[target_name].update(config_path, value)
    applied_overrides.append(f"{target_name}.{config_path}")

if applied_overrides:
    print("Applied config overrides: " + ", ".join(applied_overrides))


firmware_version = os.environ.get(FIRMWARE_VERSION_ENV, "").strip()

if firmware_version:
    if not FIRMWARE_VERSION_PATTERN.fullmatch(firmware_version):
        raise ValueError(
            f"{FIRMWARE_VERSION_ENV} must be 1-19 chars using letters, "
            "numbers, '.', '_', '+', or '-'"
        )

    env.Append(
        BUILD_FLAGS=[
            f'-D AVIONICS_ARGUMENT_firmwareVersion=\\"{firmware_version}\\"',
        ]
    )
    print(f"Applied firmware version: {firmware_version}")
