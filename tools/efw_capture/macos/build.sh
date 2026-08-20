#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec clang -fobjc-arc -framework Foundation -framework IOKit \
  -o "$script_dir/efw_capture_readonly" \
  "$script_dir/efw_capture_readonly.m"
