// draw hits, and make some other related plots
// (cf. event_display.cpp for readout)
#include <cstdlib>
#include <iostream>
#include <fmt/format.h>

// ROOT
#include "TSystem.h"
#include "TStyle.h"
#include "TRegexp.h"
#include "TCanvas.h"
#include "TApplication.h"
#include "TBox.h"
#include "ROOT/RDataFrame.hxx"

// edm4hep
#include "edm4hep/MCParticleCollection.h"
#include "edm4hep/SimTrackerHitCollection.h"

// local
#include "WhichRICH.h"

using namespace ROOT;
using namespace ROOT::VecOps;
using namespace edm4hep;

TCanvas *CreateCanvas(TString name, Bool_t logx=0, Bool_t logy=0, Bool_t logz=0);

int main(int argc, char** argv) {

  // args
  TString infileN="out/sim.root";
  if(argc<=1) {
    fmt::print("\nUSAGE: {} [d/p] [simulation_output_file(optional)]\n\n",argv[0]);
    fmt::print("    [d/p]: d for dRICH\n");
    fmt::print("           p for pfRICH\n");
    fmt::print("    [simulation_output_file]: output from `npsim` (`simulate.py`)\n");
    fmt::print("                              default: {}\n",infileN);
    return 2;
  }
  std::string zDirectionStr = argv[1];
  if(argc>2) infileN = TString(argv[2]);
  WhichRICH wr(zDirectionStr);
  if(!wr.valid) return 1;

  // setup
  //TApplication mainApp("mainApp",&argc,argv); // keep canvases open
  //EnableImplicitMT();
  RDataFrame dfIn("events",infileN.Data());
  TString outfileN = infileN;
  outfileN(TRegexp("\\.root$"))=".";
  TFile *outfile = new TFile(outfileN+"plots.root","RECREATE");
  gStyle->SetOptStat(0);
  gStyle->SetPalette(55);


  /* lambdas
   * - most of these transform an `RVec<T1>` to an `RVec<T2>` using `VecOps::Map` or `VecOps::Filter`
   * - see NPdet/src/dd4pod/dd4hep.yaml for POD syntax
   */
  // calculate number of hits
  auto numHits = [](RVec<SimTrackerHitData> hits) { return hits.size(); };
  // calculate momentum magnitude for each particle (units=GeV)
  auto momentum = [](RVec<MCParticleData> parts){
    return Map(parts, [](auto p){
        auto mom = p.momentum;
        return sqrt( mom[0]*mom[0] + mom[1]*mom[1] + mom[2]*mom[2] );
        });
  };
  // filter for thrown particles
  auto isThrown = [](RVec<MCParticleData> parts){
    return Filter(parts, [](auto p){ return p.generatorStatus==1; } );
  };
  // get positions for each hit (units=cm)
  auto hitPos = [](RVec<SimTrackerHitData> hits){ return Map(hits,[](auto h){ return h.position; }); };
  auto hitPosX = [](RVec<Vector3d> v){ return Map(v,[](auto p){ return p.x/10; }); };
  auto hitPosY = [](RVec<Vector3d> v){ return Map(v,[](auto p){ return p.y/10; }); };
  auto hitPosZ = [](RVec<Vector3d> v){ return Map(v,[](auto p){ return p.z/10; }); };


  // transformations
  auto df1 = dfIn
    .Define("thrownParticles",isThrown,{"MCParticles"})
    .Define("thrownP",momentum,{"thrownParticles"})
    .Define("numHits",numHits,{wr.readoutName})
    .Define("hitPos",hitPos,{wr.readoutName})
    .Define("hitX",hitPosX,{"hitPos"})
    .Define("hitY",hitPosY,{"hitPos"})
    ;
  auto dfFinal = df1;


  // actions
  auto hitPositionHist = dfFinal.Histo2D(
      { "hitPositions",TString(wr.xRICH)+" hit positions (units=cm)",
      1000,-200,200, 1000,-200,200 },
      "hitX","hitY"
      );
  auto numHitsVsThrownP = dfFinal.Histo2D(
      { "numHitsVsThrownP","number of "+TString(wr.xRICH)+" hits vs. thrown momentum", 
      65,0,65, 100,0,400 },
      "thrownP","numHits"
      ); // TODO: cut opticalphotons (may not be needed, double check PID)


  // execution
  TCanvas *canv;
  canv = CreateCanvas("hits",0,0,1);
  hitPositionHist->Draw("colz");
  if(wr.zDirection>0) {
    hitPositionHist->GetXaxis()->SetRangeUser(100,300);
    hitPositionHist->GetYaxis()->SetRangeUser(-100,100);
  } else {
    hitPositionHist->GetXaxis()->SetRangeUser(-70,70);
    hitPositionHist->GetYaxis()->SetRangeUser(-70,70);
  }
  canv->Print(outfileN+"hits.png");
  canv->Write();
  //
  canv = CreateCanvas("photon_yield");
  numHitsVsThrownP->Draw("box");
  TProfile * aveHitsVsP;
  aveHitsVsP = numHitsVsThrownP->ProfileX("_pfx",1,-1,"i"); // TODO: maybe not the right errors, see TProfile::BuildOptions `i`
  aveHitsVsP->SetLineColor(kBlack);
  aveHitsVsP->SetLineWidth(3);
  aveHitsVsP->Draw("same");
  canv->Print(outfileN+"photon_count.png");
  canv->Write();
  aveHitsVsP->Write("aveHitsVsP");
  outfile->Close();


  // exit
  //fmt::print("\n\npress ^C to exit.\n\n");
  //mainApp.Run(); // keep canvases open
  return 0;
}


TCanvas *CreateCanvas(TString name, Bool_t logx, Bool_t logy, Bool_t logz) {
  TCanvas *c = new TCanvas("canv_"+name,"canv_"+name,800,600);
  c->SetGrid(1,1);
  if(logx) c->SetLogx(1);
  if(logy) c->SetLogy(1);
  if(logz) c->SetLogz(1);
  return c;
}

