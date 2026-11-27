#include "../../src/app/app_09/infrared_result_flow.h"
#include <cassert>
#include <cstdio>

using namespace meow_ir;
enum class Scene { Result,Discard,Failed,Matches,Name };
struct CaptureFlow {
    Scene scene=Scene::Result;
    bool handlesOpen=true,partial=false;
    unsigned learnedCommand=0xB27BE0;
    void frame(LearnedResultInput input) {
        // The last cooperative scan step completes after input was sampled.
        const auto action=learnedResultAction(input);
        switch(action) {
            case LearnedResultAction::Back:
            case LearnedResultAction::Retry: scene=Scene::Discard;handlesOpen=false;break;
            case LearnedResultAction::SendFailed:scene=Scene::Failed;handlesOpen=false;break;
            case LearnedResultAction::Matches:scene=Scene::Matches;break;
            case LearnedResultAction::ContinueNow:partial=input.checking||input.partial;handlesOpen=false;scene=Scene::Name;break;
            case LearnedResultAction::Advance:handlesOpen=false;scene=Scene::Name;break;
            default:break;
        }
    }
};
int main() {
    LearnedResultInput last;
    last.pending=true;last.hasMatches=true;last.checking=false;
    {
        CaptureFlow flow;last.back=true;last.txSucceeded=false;flow.frame(last);
        assert(flow.scene==Scene::Discard && !flow.handlesOpen); // B beats completion and failed TX.
        assert(flow.learnedCommand==0xB27BE0);
    }
    {
        CaptureFlow flow;last.back=false;last.txSucceeded=false;flow.frame(last);
        assert(flow.scene==Scene::Failed); // Failed A send beats automatic match navigation.
    }
    {
        CaptureFlow flow;last.txSucceeded=true;last.retry=true;flow.frame(last);
        assert(flow.scene==Scene::Discard);
    }
    {
        CaptureFlow flow;last.retry=false;flow.frame(last);assert(flow.scene==Scene::Matches);
    }
    {
        CaptureFlow flow;LearnedResultInput in;in.right=true;in.checking=true;
        assert(learnedResultAction(in)==LearnedResultAction::Wait);
        in.pending=true;flow.frame(in);
        assert(flow.scene==Scene::Name && flow.partial && !flow.handlesOpen);
        assert(flow.learnedCommand==0xB27BE0); // Continue keeps the current learned payload.
    }
    {
        LearnedResultInput in;in.pending=true;in.partial=true;
        assert(learnedResultAction(in)==LearnedResultAction::Matches);
        in={};in.right=true;assert(learnedResultAction(in)==LearnedResultAction::Advance);
        in={};assert(learnedResultAction(in)==LearnedResultAction::None);
    }
    {
        // Explicit second Right was sampled before the final step found a match.
        CaptureFlow flow;LearnedResultInput in;
        in.right=true;in.pending=true;in.checking=false;in.hasMatches=true;
        flow.frame(in);assert(flow.scene==Scene::Name && !flow.partial);
        flow={};in.partial=true;flow.frame(in);
        assert(flow.scene==Scene::Name && flow.partial && flow.learnedCommand==0xB27BE0);
    }
    puts("IR result flow checks passed (Back/retry/TX priority, completion, immediate Continue)");
}
