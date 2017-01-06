#!/bin/sh

# runs astyle on our code to conform to project standards
# contributors should run this prior to PRs

# a handful of subprojects (BCM headers, libfdt, etc.) are included in-tree
# to avoid create merge conflicts when pulling upstream, only style our code
FILES="*.c *.cc *.h arm_chainloader/*.c arm_chainloader/*.cc arm_chainloader/*.h arm_chainloader/drivers/*.cc arm_chainloader/drivers/*.hpp"

# run astyle with tabs and attached braces
astyle --style=attach --indent=tab $FILES
