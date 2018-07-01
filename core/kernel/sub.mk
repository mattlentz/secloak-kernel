srcs-y += assert.c
srcs-y += console.c
srcs-$(CFG_DT) += dt.c
srcs-y += tee_misc.c
srcs-y += panic.c
srcs-y += interrupt.c
cflags-remove-asan.c-y += $(cflags_kasan)
