#include <cstdlib>
#include <iostream>

// ROOT
#include "TSystem.h"
#include "TFile.h"

// local
#include "WhichRICH.h"
#include "irtgeo/IrtGeoDRICH.h"
#include "irtgeo/IrtGeoPFRICH.h"

int main(int argc, char** argv) {

  // arguments
  std::string compactFileName = "";
  std::string irtAuxFileName  = "";
  if(argc<=1) {
    fmt::print("\nUSAGE: {} [d/p] [compact_file_name(optional)] [aux_file_name(optional)]\n\n",argv[0]);
    fmt::print("    [d/p]: d for dRICH\n");
    fmt::print("           p for pfRICH\n");
    return 2;
  }
  std::string zDirectionStr  = argv[1];
  if(argc>2) compactFileName = std::string(argv[2]);
  if(argc>3) irtAuxFileName  = std::string(argv[3]);

  // RICH-specific settings (also checks `zDirectionStr`)
  WhichRICH wr(zDirectionStr);
  if(!wr.valid) return 1;

  // start auxfile
  if(irtAuxFileName=="") irtAuxFileName = "geo/irt-"+wr.xrich+".root";
  auto irtAuxFile = new TFile(irtAuxFileName.c_str(),"RECREATE");

  // given DD4hep geometry from `compactFileName`, produce IRT geometry
  IrtGeo *IG;
  if(zDirectionStr=="d")      IG = new IrtGeoDRICH(compactFileName);
  else if(zDirectionStr=="p") IG = new IrtGeoPFRICH(compactFileName);
  else return 1;

  // write IRT auxiliary file
  IG->GetIrtGeometry()->Write();
  irtAuxFile->Close();
  fmt::print("\nWrote IRT Aux File: {}\n\n",irtAuxFileName);
}