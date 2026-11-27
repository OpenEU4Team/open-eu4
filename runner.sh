#!/bin/bash
export LD_PRELOAD=./libeu4_override.so
exec ./eu4 "$@"
