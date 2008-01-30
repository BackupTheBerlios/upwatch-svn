if ENABLE_SERVER
SCR_SERVER = fill_probe_description.pl uw_maint.pl
endif

SCR_CLIENT = uw_fetch_regexes uw_start uw_stop

bin_SCRIPTS = $(SCR_SERVER) $(SCR_CLIENT)
EXTRA_DIST = $(bin_SCRIPTS)
