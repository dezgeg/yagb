MAKEFLAGS += -rR

all: _force_qmake
	@$(MAKE) --no-print-directory -f Makefile.qt all

define gen-target
$(1): _force_qmake
	@$$(MAKE) --no-print-directory -f Makefile.qt $(1)
endef

$(foreach target,$(filter-out all,$(MAKECMDGOALS)),$(eval $(call gen-target,$(target))))

_force_qmake:
	qmake -o Makefile.qt
