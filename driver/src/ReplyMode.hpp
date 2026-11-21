#pragma once

namespace dbhw {
enum class ReplyKind { Simple, Framed, Unavailable };

// A failed startup read can change the next round, never add an I/O to
// the current one. After publication, keep the mode that was verified.
class ReplyMode {
    ReplyKind preferred = ReplyKind::Simple;

public:
    ReplyKind select(bool framedAdvertised, bool initialized) const {
        if (preferred == ReplyKind::Simple || framedAdvertised)
            return preferred;
        return initialized ? ReplyKind::Unavailable : ReplyKind::Simple;
    }

    void bootstrapReadFinished(ReplyKind attempted, bool valid) {
        if (valid) {
            preferred = attempted;
            return;
        }
        preferred = attempted == ReplyKind::Simple ? ReplyKind::Framed : ReplyKind::Simple;
    }
};
}
