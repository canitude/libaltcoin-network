###############################################################################
#  Copyright (c) 2014-2015 libbitcoin-network developers (see COPYING).
#
#         GENERATED SOURCE CODE, DO NOT EDIT EXCEPT EXPERIMENTALLY
#
###############################################################################

# Automake settings.
#==============================================================================
# Look for macros in the m4 subdirectory.
#------------------------------------------------------------------------------
ACLOCAL_AMFLAGS = -I m4


# Distribute, make and install products.
#==============================================================================
# files => ${pkgconfigdir}
#------------------------------------------------------------------------------
pkgconfig_DATA = \
    libbitcoin-network.pc

# files => ${docdir}
#------------------------------------------------------------------------------
doc_DATA = \
    AUTHORS \
    COPYING \
    ChangeLog \
    INSTALL \
    NEWS \
    README

# src/libaltcoin-network.la => ${libdir}
#------------------------------------------------------------------------------
lib_LTLIBRARIES = src/libaltcoin-network.la
src_libaltcoin_network_la_CPPFLAGS = -I${srcdir}/include ${bitcoin_CPPFLAGS}
src_libaltcoin_network_la_LIBADD = ${bitcoin_LIBS}
src_libaltcoin_network_la_SOURCES = \
    src/acceptor.cpp \
    src/channel.cpp \
    src/connector.cpp \
    src/hosts.cpp \
    src/message_subscriber.cpp \
    src/p2p.cpp \
    src/proxy.cpp \
    src/settings.cpp \
    src/protocols/protocol.cpp \
    src/protocols/protocol_address_31402.cpp \
    src/protocols/protocol_events.cpp \
    src/protocols/protocol_ping_31402.cpp \
    src/protocols/protocol_ping_60001.cpp \
    src/protocols/protocol_reject_70002.cpp \
    src/protocols/protocol_seed_31402.cpp \
    src/protocols/protocol_timer.cpp \
    src/protocols/protocol_version_31402.cpp \
    src/protocols/protocol_version_70002.cpp \
    src/sessions/session.cpp \
    src/sessions/session_batch.cpp \
    src/sessions/session_inbound.cpp \
    src/sessions/session_manual.cpp \
    src/sessions/session_outbound.cpp \
    src/sessions/session_seed.cpp

# local: test/libaltcoin_network_test
#------------------------------------------------------------------------------
if WITH_TESTS

TESTS = libbitcoin_network_test_runner.sh

check_PROGRAMS = test/libbitcoin_network_test
test_libbitcoin_network_test_CPPFLAGS = -I${srcdir}/include ${bitcoin_CPPFLAGS}
test_libbitcoin_network_test_LDADD = src/libaltcoin-network.la ${boost_unit_test_framework_LIBS} ${bitcoin_LIBS}
test_libbitcoin_network_test_SOURCES = \
    test/main.cpp \
    test/p2p.cpp

endif WITH_TESTS

# files => ${includedir}/bitcoin
#------------------------------------------------------------------------------
include_altcoindir = ${includedir}/altcoin
include_altcoin_HEADERS = \
    include/altcoin/network.hpp

include_altcoin_networkdir = ${includedir}/altcoin/network
include_altcoin_network_HEADERS = \
    include/altcoin/network/acceptor.hpp \
    include/altcoin/network/channel.hpp \
    include/altcoin/network/connector.hpp \
    include/altcoin/network/define.hpp \
    include/altcoin/network/hosts.hpp \
    include/altcoin/network/message_subscriber.hpp \
    include/altcoin/network/p2p.hpp \
    include/altcoin/network/proxy.hpp \
    include/altcoin/network/settings.hpp \
    include/altcoin/network/version.hpp

include_altcoin_network_protocolsdir = ${includedir}/altcoin/network/protocols
include_altcoin_network_protocols_HEADERS = \
    include/altcoin/network/protocols/protocol.hpp \
    include/altcoin/network/protocols/protocol_address_31402.hpp \
    include/altcoin/network/protocols/protocol_events.hpp \
    include/altcoin/network/protocols/protocol_ping_31402.hpp \
    include/altcoin/network/protocols/protocol_ping_60001.hpp \
    include/altcoin/network/protocols/protocol_reject_70002.hpp \
    include/altcoin/network/protocols/protocol_seed_31402.hpp \
    include/altcoin/network/protocols/protocol_timer.hpp \
    include/altcoin/network/protocols/protocol_version_31402.hpp \
    include/altcoin/network/protocols/protocol_version_70002.hpp

include_altcoin_network_sessionsdir = ${includedir}/altcoin/network/sessions
include_altcoin_network_sessions_HEADERS = \
    include/altcoin/network/sessions/session.hpp \
    include/altcoin/network/sessions/session_batch.hpp \
    include/altcoin/network/sessions/session_inbound.hpp \
    include/altcoin/network/sessions/session_manual.hpp \
    include/altcoin/network/sessions/session_outbound.hpp \
    include/altcoin/network/sessions/session_seed.hpp

impl_altcoin_networkdir = ${includedir}/altcoin/src/network
impl_altcoin_network_HEADERS = \
    src/acceptor.cpp \
    src/channel.cpp \
    src/connector.cpp \
    src/hosts.cpp \
    src/message_subscriber.cpp \
    src/p2p.cpp \
    src/proxy.cpp \
    src/settings.cpp

impl_altcoin_network_protocolsdir = ${includedir}/altcoin/src/network/protocols
impl_altcoin_network_protocols_HEADERS = \
    src/protocols/protocol.cpp \
    src/protocols/protocol_address_31402.cpp \
    src/protocols/protocol_events.cpp \
    src/protocols/protocol_ping_31402.cpp \
    src/protocols/protocol_ping_60001.cpp \
    src/protocols/protocol_reject_70002.cpp \
    src/protocols/protocol_seed_31402.cpp \
    src/protocols/protocol_timer.cpp \
    src/protocols/protocol_version_31402.cpp \
    src/protocols/protocol_version_70002.cpp

impl_altcoin_network_sessionsdir = ${includedir}/altcoin/src/network/sessions
impl_altcoin_network_sessions_HEADERS = \
    src/sessions/session.cpp \
    src/sessions/session_batch.cpp \
    src/sessions/session_inbound.cpp \
    src/sessions/session_manual.cpp \
    src/sessions/session_outbound.cpp \
    src/sessions/session_seed.cpp
