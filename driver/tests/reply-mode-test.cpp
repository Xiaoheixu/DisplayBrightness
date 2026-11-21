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
    assert(reply.select(true, false) == ReplyKind::Framed);

    // A transient failure of both modes must not permanently exclude the
    // framed mode that becomes responsive on a later bounded startup visit.
    dbhw::ReplyMode transient;unsigned visits=0;bool recovered=false;
    for(;visits<6&&!recovered;++visits){
        auto kind=transient.select(true,false);
        recovered=visits>=3&&kind==ReplyKind::Framed;
        transient.bootstrapReadFinished(kind,recovered);
    }
    assert(recovered&&visits==4);
    reply = workingFallback;
    reply.bootstrapReadFinished(ReplyKind::Framed, true);
    assert(reply.select(true, false) == ReplyKind::Framed);
    assert(reply.select(false, false) == ReplyKind::Simple);
    assert(reply.select(true, true) == ReplyKind::Framed);
    assert(reply.select(false, true) == ReplyKind::Unavailable);
    assert(dbhw::ReplyMode{}.select(true, false) == ReplyKind::Simple);
    std::puts("reply mode: simple baseline, advertised fallback, failed fallback, and verified binding passed");
}
