TESTS		=	t0001 t0002 t0003 t0004 t0005 t0006 t0007 t0008 t0009 \
			connect_fail ct_options lang_ct_param array_bind cs_diag \
			get_send_data rpc_ct_param rpc_ct_setparam ct_diagclient \
			ct_diagserver ct_diagall cs_config
check_PROGRAMS	=	$(TESTS)

t0001_SOURCES	=	t0001.c common.c common.h
t0002_SOURCES	=	t0002.c common.c common.h
t0003_SOURCES	=	t0003.c common.c common.h
t0004_SOURCES	=	t0004.c common.c common.h
t0005_SOURCES	=	t0005.c common.c common.h
t0006_SOURCES	=	t0006.c
t0007_SOURCES	=	t0007.c common.c common.h
t0008_SOURCES	=	t0008.c common.c common.h
t0009_SOURCES	=	t0009.c common.c common.h
connect_fail_SOURCES = 	connect_fail.c common.c common.h
ct_options_SOURCES =	ct_options.c common.c common.h
lang_ct_param_SOURCES =	lang_ct_param.c common.c common.h
array_bind_SOURCES =	array_bind.c common.c common.h
cs_diag_SOURCES =	cs_diag.c common.c common.h
get_send_data_SOURCES =	get_send_data.c common.c common.h
rpc_ct_param_SOURCES =	rpc_ct_param.c common.c common.h
rpc_ct_setparam_SOURCES =	rpc_ct_setparam.c common.c common.h
ct_diagclient_SOURCES = ct_diagclient.c common.c common.h
ct_diagserver_SOURCES = ct_diagserver.c common.c common.h
ct_diagall_SOURCES = ct_diagall.c common.c common.h
cs_config_SOURCES = cs_config.c common.c common.h

LIBS		=	../libct.la @NETWORK_LIBS@
INCLUDES	=	-I$(top_srcdir)/include
CLEANFILES	=	tdsdump.out
