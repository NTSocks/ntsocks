#!/bin/sh
# SPDX-License-Identifier: BSD-3-Clause
# Copyright(c) 2010-2014 Intel Corporation

# Auto-generate a Makefile in build directory
# Args:
#   $1: path of project src root

echo "# Automatically generated by gen-build-mk.sh"
echo
echo "ifdef O"
echo "ifeq (\"\$(origin O)\", \"command line\")"
echo "\$(error \"Cannot specify O= as you are already in a build directory\")"
echo "endif"
echo "endif"
echo
echo "MAKEFLAGS += --no-print-directory"
echo
echo "all:"
echo "	@\$(MAKE) -C $1 O=\$(CURDIR)"
echo
echo "%::"
echo "	@\$(MAKE) -C $1 O=\$(CURDIR) \$@"
