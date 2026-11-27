#pragma once

namespace meow_ir {
enum class LearnedResultAction { None, Back, Retry, SendFailed, Matches, Wait, ContinueNow, Advance };
struct LearnedResultInput {
    bool back=false,retry=false,down=false,right=false;
    bool pending=false,checking=false,hasMatches=false,partial=false;
    bool txBusy=false,txSucceeded=true;
};
inline LearnedResultAction learnedResultAction(const LearnedResultInput& in) {
    if(in.back) return LearnedResultAction::Back;
    if(in.retry) return LearnedResultAction::Retry;
    if(!in.txBusy && !in.txSucceeded) return LearnedResultAction::SendFailed;
    if(in.down) return LearnedResultAction::Matches;
    if(in.right && in.pending) return LearnedResultAction::ContinueNow;
    if(in.right && in.checking) return LearnedResultAction::Wait;
    if(in.pending && !in.checking && (in.hasMatches || in.partial)) return LearnedResultAction::Matches;
    if(in.right || (in.pending && !in.checking)) return LearnedResultAction::Advance;
    return LearnedResultAction::None;
}
} // namespace meow_ir
