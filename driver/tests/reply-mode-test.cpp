#include "ReplyMode.hpp"
#include <cassert>
#include <cstdio>

int main() {
    dbhw::ReplyMode reply;
    using dbhw::ReplyKind;
    assert(reply.select(false, false) == ReplyKind::Simple);
    assert(reply.select(true, false) == ReplyKind::Simple);
    reply.bootstrapReadFinished(ReplyKind::Simple, true);
    assert(reply.select(true, false) == ReplyKind::Simple);
    reply = dbhw::ReplyMode{};
    assert(reply.select(true, false) == ReplyKind::Simple);
    reply.bootstrapReadFinished(ReplyKind::Simple, false);
    assert(reply.select(false, false) == ReplyKind::Simple);
    assert(reply.select(true, false) == ReplyKind::Framed);
    auto workingFallback = reply;
    reply.bootstrapReadFinished(ReplyKind::Framed, false);
    assert(reply.select(true, false) == ReplyKind::Simple);
    reply.bootstrapReadFinished(ReplyKind::Simple, false);
    assert(reply.select(true, false) == ReplyKind::Simple);

    reply = workingFallback;
    reply.bootstrapReadFinished(ReplyKind::Framed, true);
    assert(reply.select(true, false) == ReplyKind::Framed);
    assert(reply.select(false, false) == ReplyKind::Simple);
    assert(reply.select(true, true) == ReplyKind::Framed);
    assert(reply.select(false, true) == ReplyKind::Unavailable);
    assert(dbhw::ReplyMode{}.select(true, false) == ReplyKind::Simple);
    std::puts("reply mode: simple baseline, advertised fallback, failed fallback, and verified binding passed");
}
