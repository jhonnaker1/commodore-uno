# Repository-wide targets. Each port also builds on its own -- `cd c64 && make`
# -- and nothing here replaces that; this is the cross-port view.
#
# `check` is STATIC and fast (a second or two), so there is no excuse to skip
# it. `build-all` needs six toolchains and takes minutes, so it is separate:
# a gate nobody can afford to run is a gate nobody runs.

.PHONY: help check build-all clean-all

help:
	@echo "make check      - verify the shared-source invariants (fast, static)"
	@echo "make build-all  - clean-build every port and alternate target (slow)"
	@echo "make clean-all  - clean every port"

check:
	@python3 tools/check_shared.py

build-all:
	@python3 tools/build_all.py

clean-all:
	@for d in */Makefile; do $(MAKE) -C `dirname $$d` clean >/dev/null 2>&1 || true; done
	@echo "cleaned"
