// Receives typed structural input through the production queue and replay path.
// Gathering uses actual steering; rejection, same-boundary loss, retained frames
// and exact state are checked without a second transition implementation.
#include <crucible/runtime/InspectorSession.hpp>
#include <crucible/presentation/ScenarioSnapshot.hpp>
#include <crucible/simulation.hpp>
#include <array>
#include <stdexcept>
#include <iostream>
using namespace crucible;
using namespace crucible::runtime;
namespace {
void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
Simulation::ScenarioOptions Options() {
    return {{64,32,1},4,SteeringSettings{},ResourceSettings{},StructuralSettings{}};
}
void ReceiveCommands() {
    Simulation simulation{2048,Options()};
    HeadlessSession session{simulation,{8,16}};
    presentation::ScenarioSnapshot retained{2048,4,2048}, final{2048,4,2048};
    const auto fuse = BoundaryCommand::Fuse();
    Require(session.GetIngress().TryAdmitCommands(std::span{&fuse,1}).status == CommandIngress::AdmissionStatus::accepted,"fuse admission");
    Require(session.TryStep().status == HeadlessSession::StepStatus::advanced,"normal refusal advances");
    Require(session.GetTrace()[0].result == StructuralCommandResult::insufficient_mass,"startup49 below64");
    const FieldEdit gather{FieldEditKind::set,0,{48.5F,16.5F},8,4};
    Require(session.GetIngress().TryAdmit(std::span{&gather,1}).status == CommandIngress::AdmissionStatus::accepted,"gather admission");
    for (int index=0;index<60;++index) Require(session.TryStep().status == HeadlessSession::StepStatus::advanced,"gather tick");
    Require(simulation.GetStructuralState()->eligible_mobile >= 64,"gather opportunity");
    Require(session.GetIngress().TryAdmitCommands(std::span{&fuse,1}).status == CommandIngress::AdmissionStatus::accepted,"actual fuse admission");
    Require(session.TryStep().status == HeadlessSession::StepStatus::advanced && retained.TryCapture(simulation),"fuse capture");
    const auto held = retained.GetInfo()->structural;
    Require(held->occupied && held->generation == 1 && held->hold_ticks == 1,"fusion boundary hold");
    const std::array commands{BoundaryCommand::Shatter(0),BoundaryCommand::Shatter(1),BoundaryCommand::Fuse()};
    Require(session.GetIngress().TryAdmitCommands(commands).status == CommandIngress::AdmissionStatus::accepted,"ordered batch");
    Require(session.TryStep().status == HeadlessSession::StepStatus::advanced && final.TryCapture(simulation),"shatter/refuse boundary");
    const auto trace=session.GetTrace();
    Require(trace[3].result == StructuralCommandResult::stale_generation && trace[4].result == StructuralCommandResult::applied,"stale then valid shatter");
    const auto ledger=*final.GetInfo()->biomass;
    Require(ledger.lost_mass==16 && ledger.mobile_mass+ledger.structure_mass==2032,"loss conserved");
    Require(retained.GetInfo()->structural->occupied && retained.GetInfo()->biomass->lost_mass==0,"retained frame owned");
    Simulation oracle{2048,Options()}; HeadlessSession replay{oracle,{8,16}};
    presentation::ScenarioSnapshot expected{2048,4,2048};
    Require(replay.TryReplay(trace,session.GetCompletedTick()).status == HeadlessSession::StepStatus::advanced &&
        expected.TryCapture(oracle) && final.HasEqualState(expected),"complete structural replay");
    auto changed=std::vector<AppliedCommand>(trace.begin(),trace.end()); changed[0].result=StructuralCommandResult::applied;
    Simulation mismatch{2048,Options()};HeadlessSession rejected{mismatch,{8,16}};
    Require(rejected.TryReplay(changed,session.GetCompletedTick()).status==HeadlessSession::StepStatus::application_failed,"replay verifies actual refusal");
}
void ReceiveBoundaries() {
    Simulation simulation{2048,Options()};HeadlessSession session{simulation,{2,0}};
    const auto fuse=BoundaryCommand::Fuse();
    Require(session.GetIngress().TryAdmitCommands(std::span{&fuse,1}).status==CommandIngress::AdmissionStatus::accepted,"trace-full admit");
    Require(session.TryStep().status==HeadlessSession::StepStatus::trace_full && session.GetCompletedTick()==0 &&
        session.GetIngress().GetStatistics().pending==1 && simulation.GetStructuralState()->generation==0,"trace full preserves boundary");
    InspectorSession run{2048,{8,16},ReclamationMissionSettings{1780,1},true};
    Require(run.TryPump(std::chrono::nanoseconds{16'666'667}).advanced_ticks==1 &&
        run.GetMission()->outcome==ReclamationMissionOutcome::lost,"structural deadline");
    Require(run.TryFuseRelay().status==CommandIngress::AdmissionStatus::closed,"terminal rejects input");
    run.Restart();Require(run.GetMission()->outcome==ReclamationMissionOutcome::active &&
        run.GetSnapshot().GetInfo()->biomass->lost_mass==0 && run.GetTrace().empty(),"fresh structural restart");
}
}
int main(){try{ReceiveCommands();ReceiveBoundaries();std::cout<<"Structural commands/rejection/replay received\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
