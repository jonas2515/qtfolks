AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"$(PACKAGE)\" \
	$(QTCORE_CFLAGS) \
	$(QTDECLARATIVE_CFLAGS) \
	$(QTTEST_CFLAGS) \
	$(QTCONTACTS_CFLAGS) \
	$(TP_QT4_CFLAGS) \
	$(FOLKS_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(NULL)

AM_CXXFLAGS = \
	$(ERROR_CXXFLAGS) \
	$(NULL)

LDADD = \
	$(QTCORE_LIBS) \
	$(QTDECLARATIVE_LIBS) \
	$(QTTEST_LIBS) \
	$(QTCONTACTS_LIBS) \
	$(TP_QT4_LIBS) \
	$(FOLKS_LIBS) \
	$(NULL)

backend_store_key_file=$(top_srcdir)/tests/data/backend-store-key-file.ini
backend_key_file=$(top_srcdir)/tests/data/backend-key-file--sample-contacts.ini
TESTS_ENVIRONMENT = \
	QT_PLUGIN_PATH=$(QT_FOLKS_PLUGINS_UNINST_PATH) \
	FOLKS_BACKEND_STORE_KEY_FILE_PATH=$(backend_store_key_file) \
	FOLKS_BACKEND_KEY_FILE_PATH=$(backend_key_file) \
	$(NULL)

noinst_PROGRAMS = \
	contact-retrieval \
	$(NULL)

BUILT_SOURCES = \
	$(nodist_contact_retrieval_SOURCES) \
	$(NULL)

TESTS = $(noinst_PROGRAMS)

contact_retrieval_SOURCES = \
	contact-retrieval.cpp \
	contact-retrieval.h \
	utils.h \
	$(NULL)

nodist_contact_retrieval_SOURCES = \
	_gen/contact-retrieval.moc.hpp \
	$(NULL)

_gen/%.moc.hpp: %.h
	$(mkdir_p) _gen
	$(MOC) $(AM_CPPFLAGS) -i $< -o $@

EXTRA_DIST = \
	$(backend_store_key_file) \
	$(backend_key_file) \
	$(NULL)

CLEANFILES = \
	$(BUILT_SOURCES) \
	$(NULL)

MAINTAINERCLEANFILES = \
	_gen \
	Makefile.in \
	$(NULL)

-include $(top_srcdir)/git.mk
