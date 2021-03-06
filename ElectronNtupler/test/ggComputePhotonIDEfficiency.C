#include "TVector3.h"
#include "TH2D.h"
#include "TCut.h"
#include "TROOT.h"
#include "TLatex.h"
#include "TH1D.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TLegend.h"
#include "TTree.h"
#include "TString.h"

#include <vector>

// A flag to print some info when processing the first photon
bool firstBarrelPhoton = true;
bool firstEndcapPhoton = true;

// Use only events when there is one and only one photon matching MC truth?
// (like Savvas does)
bool restrictToOnePhoton = false;

const int nWP = 3;
enum WpType { WP_LOOSE = 0,
	      WP_MEDIUM, 
	      WP_TIGHT};
const TString wpName[nWP] = 
  {"Loose", "Medium", "Tight"};

enum IsoType {CH_ISO, NH_ISO, PH_ISO};

float computeIso(IsoType isoType, float isoVal, float eta, float rho); 

// Use here one of the WpType values
const WpType wp = WP_TIGHT;

const TString treename = "ggNtuplizer/EventTree";
// const TString fname1 = "~/workspace/ntuples/ggNtuple_GJets_HT-100to200_singleFile.root";
//const TString fname1 = "/tmp/rslu/job_phys14_gjet_pt40_20bx25.root";
const TString fname1 = "root://eoscms.cern.ch//eos/cms/store/group/phys_egamma/ikrav/common_ntuples/PHYS14/job_phys14_gjet_pt40_20bx25.root";

bool verbose = false;
bool smallEventCount = false;

const int nEtaBins = 2;

const float ptMin = 30;
const float ptMax = 200;
const float barrelEtaLimit = 1.479;

bool passWorkingPoint(WpType wp, bool isBarrel, float pt,
		      float hOverE, float full5x5_sigmaIetaIeta, 
		      float chIso, float nhIso, float phIso);

bool isMatched(float pEta, float pPhi,
	       std::vector<int> *mcPID,
	       std::vector<int> *mcMomPID,
	       std::vector<int> *mcStatus,
	       std::vector<float> *mcEta,
	       std::vector<float> *mcPhi);

bool isMatchedV2(float pEta, float pPhi,
		 std::vector<int> *mcPID,
		 std::vector<int> *mcMomPID,
		 std::vector<int> *mcStatus,
		 std::vector<float> *mcEta,
		 std::vector<float> *mcPhi);

void ggComputePhotonIDEfficiency()
{

  // This statement below should not be needed, but in one particular node I had to
  // add it, somehow the vector header was not loaded automatically there.
  gROOT->ProcessLine("#include <vector>"); 

  //
  // Find the tree
  //
  // TFile *file1 = new TFile(fname1); // this doesn't seem to work with EOS and xrood...
  TFile *file1 = TFile::Open(fname1);
  if( !file1 )
    assert(0);
  TTree *tree = (TTree*)file1->Get(treename);
  if( !tree )
    assert(0);
  printf("Found the tree\n"); fflush(stdout);

  TH1D *phoEtCheckHist = new TH1D("phoEtCheckHist","",185, 15, 200);
  TH1D *phoSCEtaCheckHist = new TH1D("phoSCEtaCheckHist","",200, -5, 5);
  TH2D *phoTotCheck = new TH2D("phoTotCheck","", 200, -5, 5, 185, 15, 200);

  // Event-level variables:
  int nPho;
  float rho;
  // Per-photon variables
  // Kinematics
  std::vector <float> *phoEt = 0;     
  std::vector <float> *phoSCEta = 0;    
  std::vector <float> *phoPhi = 0;    
  // ID related variables
  std::vector <float> *phoSigmaIEtaIEta_2012 = 0; 
  std::vector <float> *phoHoverE = 0;    
  std::vector <float> *phoPFPhoIso = 0;    
  std::vector <float> *phoPFChIso = 0;    
  std::vector <float> *phoPFNeuIso = 0;    
  std::vector <int> *phohasPixelSeed = 0;    
  // MC variables  
  std::vector <int> *mcPID = 0;     
  std::vector <int> *mcMomPID = 0;     
  std::vector <int> *mcStatus = 0;     
  std::vector <float> *mcEta = 0;     
  std::vector <float> *mcPhi = 0;     

  // Declare branches
  TBranch *b_nPho = 0;
  TBranch *b_rho = 0;
  TBranch *b_phoEt = 0;
  TBranch *b_phoSCEta = 0;
  TBranch *b_phoPhi = 0;
  TBranch *b_phoSigmaIEtaIEta_2012 = 0;
  TBranch *b_phoHoverE = 0;
  TBranch *b_phoPFPhoIso = 0;
  TBranch *b_phoPFChIso = 0;
  TBranch *b_phoPFNeuIso = 0;
  TBranch *b_phohasPixelSeed = 0;
  //
  TBranch *b_mcPID;
  TBranch *b_mcMomPID;
  TBranch *b_mcStatus;
  TBranch *b_mcEta;
  TBranch *b_mcPhi;

  // Connect variables and branches to the tree with the data
  tree->SetBranchAddress("nPho", &nPho, &b_nPho);
  tree->SetBranchAddress("rho", &rho, &b_rho);
  tree->SetBranchAddress("phoEt", &phoEt, &b_phoEt);
  tree->SetBranchAddress("phoEta", &phoSCEta, &b_phoSCEta);
  tree->SetBranchAddress("phoPhi", &phoPhi, &b_phoPhi);
  tree->SetBranchAddress("phoSigmaIEtaIEta_2012", &phoSigmaIEtaIEta_2012, &b_phoSigmaIEtaIEta_2012); 
  tree->SetBranchAddress("phoHoverE", &phoHoverE, &b_phoHoverE);
  tree->SetBranchAddress("phoPFPhoIso", &phoPFPhoIso, &b_phoPFPhoIso);
  tree->SetBranchAddress("phoPFChIso", &phoPFChIso, &b_phoPFChIso);
  tree->SetBranchAddress("phoPFNeuIso", &phoPFNeuIso, &b_phoPFNeuIso);
  tree->SetBranchAddress("phohasPixelSeed", &phohasPixelSeed, &b_phohasPixelSeed);
  //
  tree->SetBranchAddress("mcPID", &mcPID, &b_mcPID);
  tree->SetBranchAddress("mcMomPID", &mcMomPID, &b_mcMomPID);
  tree->SetBranchAddress("mcStatus", &mcStatus, &b_mcStatus);
  tree->SetBranchAddress("mcEta", &mcEta, &b_mcEta);
  tree->SetBranchAddress("mcPhi", &mcPhi, &b_mcPhi);

  //
  // The first loop is to fill the weight histograms Weight histograms
  //
  TH2D *hSignal = new TH2D("hSignal","",200,-5,5,185,15,200);
  TH2D *hBackground = new TH2D("hBackground","",200,-5,5,185,15,200);
  int nEvents = 0;
  int nEventsWithOnePhotonMatched = 0;
  int nPhotons = 0;
  int nPhotonsWithOnePhotonMatched = 0;

  UInt_t maxEvents = tree->GetEntries();
  if( smallEventCount )
    maxEvents = 100000;
  if(verbose)
    printf("Start loop over events for WEIGHTS, total events = %lld\n", 
           tree->GetEntries() );
  for(UInt_t ievent = 0; ievent < maxEvents; ievent++){
    
    if( ievent%100000 == 0){
      printf("."); fflush(stdout);
    }
    Long64_t tentry = tree->LoadTree(ievent);
    nEvents++;

    // Load the value of the number of the photons in the event    
    b_nPho->GetEntry(tentry);

    // Get data for all photons in this event, only vars of interest
    b_phoEt->GetEntry(tentry);
    b_phoSCEta->GetEntry(tentry);
    b_phoPhi->GetEntry(tentry);
    b_phohasPixelSeed->GetEntry(tentry);

    b_mcPID->GetEntry(tentry);
    b_mcMomPID->GetEntry(tentry);
    b_mcStatus->GetEntry(tentry);
    b_mcEta->GetEntry(tentry);
    b_mcPhi->GetEntry(tentry);

    // Loop over photons #1: discard all events that contain more than 1 matched photon
    int nMatched = 0;
    for(int ipho = 0; ipho < nPho; ipho++){
      nPhotons++;
      
      // // Preselection
      // if( !(phoEt->at(ipho) > ptMin && phoEt->at(ipho) < ptMax ) ) continue;
      // if( fabs(phoSCEta->at(ipho))>2.5) continue;
      bool isTrue = isMatchedV2( phoSCEta->at(ipho), phoPhi->at(ipho),
				 mcPID, mcMomPID, mcStatus,
				 mcEta, mcPhi);

      if( isTrue ) 
	nMatched++;
    }// end loop over photons

    if( restrictToOnePhoton && (nMatched != 1) ) continue;
    nEventsWithOnePhotonMatched++;

    // Loop over photons #2: fill the weights histograms
    for(int ipho = 0; ipho < nPho; ipho++){
      nPhotonsWithOnePhotonMatched++;
      
      // Preselection
      if( !(phoEt->at(ipho) > ptMin && phoEt->at(ipho) < ptMax ) ) continue;
      if( fabs(phoSCEta->at(ipho))>1.4442 && fabs(phoSCEta->at(ipho))<1.566) continue;
      if( fabs(phoSCEta->at(ipho))>2.5) continue;
      // Do not apply electron veto, as agreed
      // if( !( phohasPixelSeed->at(ipho) == 0 ) ) continue;
      // Match to MC truth
      bool isTrue = isMatchedV2( phoSCEta->at(ipho), phoPhi->at(ipho),
				 mcPID, mcMomPID, mcStatus,
				 mcEta, mcPhi);

      if( isTrue ) {
	hSignal->Fill( phoSCEta->at(ipho), phoEt->at(ipho) );
      }else{
	hBackground->Fill( phoSCEta->at(ipho), phoEt->at(ipho) );
      }
    }// end loop over photons
  } // end loop over events

  //
  // Prepare for computing efficiencies
  //
  double sumSignalDenomEB = 0;
  double sumSignalNumEB   = 0;
  double sumSignalDenomEE = 0;
  double sumSignalNumEE   = 0;
  double sumBackDenomEB = 0;
  double sumBackNumEB   = 0;
  double sumBackDenomEE = 0;
  double sumBackNumEE   = 0;

  double sumSignalDenomEBErr2 = 0;
  double sumSignalNumEBErr2   = 0;
  double sumSignalDenomEEErr2 = 0;
  double sumSignalNumEEErr2   = 0;
  double sumBackDenomEBErr2 = 0;
  double sumBackNumEBErr2   = 0;
  double sumBackDenomEEErr2 = 0;
  double sumBackNumEEErr2   = 0;

  int nSigBarrel = 0;
  double nSigBarrelWeighted = 0;
  int nBgBarrel  = 0;
  double nBgBarrelWeighted  = 0;

  // 
  // The second loop over events is for efficiency computation
  //
  if(verbose)
    printf("Start loop over events for EFFICIENCY caluclation, total events = %lld\n", 
           tree->GetEntries() );
  for(UInt_t ievent = 0; ievent < maxEvents; ievent++){

    if( ievent%100000 == 0){
      printf("."); fflush(stdout);
    }
    Long64_t tentry = tree->LoadTree(ievent);

    // Load the value of the number of the photons in the event    
    b_nPho->GetEntry(tentry);
    b_rho->GetEntry(tentry);
    if(verbose)
      printf("Event %d, number of photons %u\n", ievent, nPho);

    // Get data for all photons in this event, only vars of interest
    b_phoEt->GetEntry(tentry);
    b_phoSCEta->GetEntry(tentry);
    b_phoPhi->GetEntry(tentry);
    b_phoSigmaIEtaIEta_2012->GetEntry(tentry);
    b_phoHoverE->GetEntry(tentry);
    b_phoPFPhoIso->GetEntry(tentry);
    b_phoPFChIso->GetEntry(tentry);
    b_phoPFNeuIso->GetEntry(tentry);
    b_phohasPixelSeed->GetEntry(tentry);

    b_mcPID->GetEntry(tentry);
    b_mcMomPID->GetEntry(tentry);
    b_mcStatus->GetEntry(tentry);
    b_mcEta->GetEntry(tentry);
    b_mcPhi->GetEntry(tentry);

    // Loop over photons #1: discard all events that contain more than 1 matched photon
    int nMatched = 0;
    for(int ipho = 0; ipho < nPho; ipho++){
      
      // // Preselection
      // if( !(phoEt->at(ipho) > ptMin && phoEt->at(ipho) < ptMax ) ) continue;
      // if( fabs(phoSCEta->at(ipho))>2.5) continue;
      bool isTrue = isMatchedV2( phoSCEta->at(ipho), phoPhi->at(ipho),
				 mcPID, mcMomPID, mcStatus,
				 mcEta, mcPhi);

      if( isTrue ) 
	nMatched++;
    }// end loop over photons

    if( restrictToOnePhoton && (nMatched != 1) ) continue;

    // Loop over photons #2: compute the pass/total sums
    for(int ipho = 0; ipho < nPho; ipho++){
      
      // Preselection
      if( !(phoEt->at(ipho) > ptMin && phoEt->at(ipho) < ptMax ) ) continue;
      if( fabs(phoSCEta->at(ipho))>1.4442 && fabs(phoSCEta->at(ipho))<1.566) continue;
      if( fabs(phoSCEta->at(ipho))>2.5) continue;
      // Do not apply electron veto, as agreed
      // if( !( phohasPixelSeed->at(ipho) == 0 ) ) continue;

      bool isBarrel = (fabs(phoSCEta->at(ipho)) < barrelEtaLimit);
      // Correct for pile-up
      float chIsoWithEA = computeIso(CH_ISO, 
				     phoPFChIso->at(ipho), 
				     phoSCEta->at(ipho), rho);
      float nhIsoWithEA = computeIso(NH_ISO, 
				     phoPFNeuIso->at(ipho), 
				     phoSCEta->at(ipho), rho);
      float phIsoWithEA = computeIso(PH_ISO, 
				     phoPFPhoIso->at(ipho), 
				     phoSCEta->at(ipho), rho);

      // Compute ID decision
      bool pass = passWorkingPoint( wp, isBarrel, phoEt->at(ipho),
				    phoHoverE->at(ipho),
				    phoSigmaIEtaIEta_2012->at(ipho),
				    chIsoWithEA, nhIsoWithEA, phIsoWithEA);

      // Match to MC truth
      bool isTrue = isMatchedV2( phoSCEta->at(ipho), phoPhi->at(ipho),
				 mcPID, mcMomPID, mcStatus,
				 mcEta, mcPhi);
      
      // We reweight signal and background (separately) to have 
      // a flat pt and eta distribution. This step is a matter of definition.
      double binContent = 0;
      TH2D *hEvents = hSignal;
      if( !isTrue )
	hEvents = hBackground;
      binContent = hEvents->GetBinContent
	( hEvents->FindBin( phoSCEta->at(ipho), phoEt->at(ipho) ) );
      double weight = 1;
      if( binContent == 0 ){
      	printf("Error! Zero! pt=%f, eta=%f\n", phoEt->at(ipho), phoSCEta->at(ipho));
      }else{
      	weight = 1./binContent;
      }
      
      if( isTrue ) {
	phoEtCheckHist->Fill(phoEt->at(ipho), weight);
	phoSCEtaCheckHist->Fill(phoSCEta->at(ipho), weight);
	phoTotCheck->Fill(phoSCEta->at(ipho), phoEt->at(ipho), weight);
	  	
	if( isBarrel ) {
	  nSigBarrel++;
	  nSigBarrelWeighted += weight;
	  sumSignalDenomEB += weight;
	  sumSignalDenomEBErr2 += weight*weight;
	  if( pass ) {
	    sumSignalNumEB += weight;
	    sumSignalNumEBErr2 += weight*weight;
	  }
	} else {
	  sumSignalDenomEE += weight;
	  sumSignalDenomEEErr2 += weight*weight;	  
	  if( pass ) {
	    sumSignalNumEE += weight;
	    sumSignalNumEEErr2 += weight*weight;
	  }

	}// end barrel / endcap
      } // end if signal

      if( !isTrue ) {
	
	if( isBarrel ) {
	  nBgBarrel++;
	  nBgBarrelWeighted += weight;
	  
	  sumBackDenomEB += weight;
	  sumBackDenomEBErr2 += weight*weight;
	  // if( phoEt->at(ipho) > 55.250197 && phoEt->at(ipho) < 55.250199 ) // 55.250198
	  //   printf("Mismatched case: pt= %f decision= %d HoE= %f  see= %f  ch= %f  nh= %f  ph= %f \n", 
	  // 	   phoEt->at(ipho), pass,  phoHoverE->at(ipho) ,
	  // 	   phoSigmaIEtaIEta_2012->at(ipho) ,
	  // 	   chIsoWithEA, nhIsoWithEA, phIsoWithEA);
	  if( pass ) {
	    sumBackNumEB += weight;
	    sumBackNumEBErr2 += weight*weight;
	  }
	  // else {
	  //   printf("Bg barrel photon failed cuts: pt=%f   w=%f\n", phoEt->at(ipho), weight);
	  // }
	} else {
	  sumBackDenomEE += weight;
	  sumBackDenomEEErr2 += weight*weight;	  
	  if( pass ) {
	    sumBackNumEE += weight;
	    sumBackNumEEErr2 += weight*weight;
	  }

	}// end barrel / endcap
      } // end if background
	
    } // end loop over photons
      
      
  }// end loop over events  

  printf("barrel signal a=%f   b=%f\n", sumSignalNumEB, sumSignalDenomEB);
  printf("barrel back a=%f    b=%f\n", sumBackNumEB, sumBackDenomEB);

  printf("\nEfficiencies for the working point %s\n", wpName[wp].Data());

  // Compute signal efficiencies
  double effSignalEB = sumSignalNumEB / sumSignalDenomEB;
  double effSignalEBErr = sqrt( sumSignalDenomEBErr2 
				* effSignalEB*(1-effSignalEB)
				/(sumSignalDenomEB*sumSignalDenomEB) );
  printf("Signal barrel efficiency: %5.1f +- %5.1f %%\n", 
	 effSignalEB*100, effSignalEBErr*100 );

  double effSignalEE = sumSignalNumEE / sumSignalDenomEE;
  double effSignalEEErr = sqrt( sumSignalDenomEEErr2 
				* effSignalEE*(1-effSignalEE)
				/(sumSignalDenomEE*sumSignalDenomEE) );
  printf("Signal endcap efficiency: %5.1f +- %5.1f %%\n", 
	 effSignalEE*100, effSignalEEErr*100 );

  // Compute background efficiencies
  double effBackEB = sumBackNumEB / sumBackDenomEB;
  double effBackEBErr = sqrt( sumBackDenomEBErr2 
				* effBackEB*(1-effBackEB)
				/(sumBackDenomEB*sumBackDenomEB) );
  printf("Background barrel efficiency: %5.1f +- %5.1f %%\n", 
	 effBackEB*100, effBackEBErr*100 );

  double effBackEE = sumBackNumEE / sumBackDenomEE;
  double effBackEEErr = sqrt( sumBackDenomEEErr2 
				* effBackEE*(1-effBackEE)
				/(sumBackDenomEE*sumBackDenomEE) );
  printf("Background endcap efficiency: %5.1f +- %5.1f %%\n", 
	 effBackEE*100, effBackEEErr*100 );

  printf("\n");
  printf(" signal photons: %d    weighted:   %.1f\n", nSigBarrel, nSigBarrelWeighted);
  printf(" backgr photons: %d    weighted:   %.1f\n", nBgBarrel, nBgBarrelWeighted);
  printf(" nEvents                      = %d\n", nEvents);
  printf(" nPhotons                     = %d\n", nPhotons);
  printf(" nEventsWithOnePhotonMatched  = %d\n", nEventsWithOnePhotonMatched);
  printf(" nPhotonsWithOnePhotonMatched = %d\n", nPhotonsWithOnePhotonMatched);
  
  TCanvas *c2 = new TCanvas("c2","c2",10,10,900, 600);
  c2->Divide(3,1);
  c2->cd(1);
  phoEtCheckHist->Draw();
  c2->cd(2);
  phoSCEtaCheckHist->Draw();
  c2->cd(3);
  phoTotCheck->Draw("colz");
}

// Effective area corrections
const int nEtaBinsEA = 7;
const float etaBinLimits[nEtaBinsEA+1] = {
  0.0, 1.0, 1.479, 2.0, 2.2, 2.3, 2.4, 2.5};

// const float areaPhotons[nEtaBinsEA] = {
//   0.0894, 0.0750, 0.0423, 0.0561, 0.0882, 0.1144, 0.1684
// };
// const float areaNeutralHadrons[nEtaBinsEA] = {
//   0.049, 0.0108, 0.0019, 0.0037, 0.0062, 0.0130, 0.1699
// };
// const float areaChargedHadrons[nEtaBinsEA] = {
//   0.0089, 0.0062, 0.0086, 0.0041, 0.0113, 0.0085, 0.0039
// };

const float areaPhotons[nEtaBinsEA] = {
  0.0894361 ,  0.0750406 ,  0.0422692 ,  0.0561305 ,  0.0881777 ,  0.114391  , 0.168394    
};
const float areaNeutralHadrons[nEtaBinsEA] = {
  0.00491696 ,  0.0108215  ,  0.00185819 ,  0.0036691  ,  0.00619475 ,  0.0130409  ,  0.169902   
};
const float areaChargedHadrons[nEtaBinsEA] = {
  0.00885603 ,  0.00615045 ,  0.00861475 ,  0.00412423 ,  0.0113191  ,  0.00846383 ,  0.0038625  
};

float computeIso(IsoType isoType, float isoVal, float eta, float rho){

  const float *areas = 0;
  if( isoType == CH_ISO )
    areas = areaChargedHadrons;
  else if (isoType == NH_ISO)
    areas = areaNeutralHadrons;
  else if (isoType == PH_ISO)
    areas = areaPhotons;
  else
    assert(0);
  // Compute isolation with effective area correction for PU
  // Find eta bin first. If eta>2.5, the last eta bin is used.
  int etaBin = 0; 
  while ( etaBin < nEtaBinsEA-1 
	  && fabs( eta ) > etaBinLimits[nEtaBinsEA+1] )
    { ++etaBin; };

  float isoValWithEA =  std::max( (float)0.0, isoVal - rho * areas[etaBin] );
  return isoValWithEA;				  
}


// ID cuts and code
// const float hOverECut[2][nWP] = 
//   { { 0.032, 0.020, 0.012 },
//     { 0.023, 0.011, 0.011 } };

// const float sieieCut[2][nWP] = 
//   { {0.0100, 0.0099, 0.0098},
//     {0.0270, 0.0269, 0.0264} };

// const float chIsoCut[2][nWP] = 
//   { {2.94, 2.62, 1.91},
//     {3.07, 1.40, 1.26} };

// const float nhIso_A[2][nWP] = 
//   { {3.16, 2.69, 2.55},
//     {17.16, 4.92, 2.71} };

// const float nhIso_B[2][nWP] = 
//   { {0.0023, 0.0023, 0.0023},
//     {0.0116, 0.0116, 0.0116} };

// const float phIso_A[2][nWP] = 
//   { {4.43, 1.35, 1.29},
//     {2.11, 2.11, 1.91} };

// const float phIso_B[2][nWP] = 
//   { {0.0004, 0.0004, 0.0004},
//     {0.0037, 0.0037, 0.0037} };

const float hOverECut[2][nWP] = 
  { { 0.0322882, 0.0195331, 0.0115014 },
    { 0.0226555, 0.0108988, 0.0107019 } };

const float sieieCut[2][nWP] = 
  { {0.00995483, 0.00993551, 0.00984631},
    {0.0269592, 0.0269045, 0.026399} };

const float chIsoCut[2][nWP] = 
  { {2.94279, 2.61706, 1.90634},
    {3.07267, 1.40267, 1.2556} };

const float nhIso_A[2][nWP] = 
  { {3.15819, 2.69467, 2.5482},
    {17.1632, 4.91651, 2.70834} };

const float nhIso_B[2][nWP] = 
  { {0.0023, 0.0023, 0.0023},
    {0.0116, 0.0116, 0.0116} };

const float phIso_A[2][nWP] = 
  { {4.43365, 1.34528, 1.29427},
    {2.10842, 2.10055, 1.90084} };

const float phIso_B[2][nWP] = 
  { {0.0004, 0.0004, 0.0004},
    {0.0037, 0.0037, 0.0037} };


bool passWorkingPoint(WpType iwp, bool isBarrel, float pt,
		      float hOverE, float full5x5_sigmaIetaIeta, 
		      float chIso, float nhIso, float phIso){

  int ieta = 0;
  if( !isBarrel )
    ieta = 1;

  bool result = 1
    && hOverE < hOverECut[ieta][iwp]
    && full5x5_sigmaIetaIeta > 0 // in case miniAOD sets this to zero due to pre-selection of storage
    && full5x5_sigmaIetaIeta < sieieCut[ieta][iwp]
    && chIso < chIsoCut[ieta][iwp]
    && nhIso < nhIso_A[ieta][iwp] + pt * nhIso_B[ieta][iwp]
    && phIso < phIso_A[ieta][iwp] + pt * phIso_B[ieta][iwp] ;

 // Print information the first time we apply cuts to a photon
  bool toPrint = firstBarrelPhoton || firstEndcapPhoton;
  if( toPrint ){
    TString ecalPart = "BARREL";
    if( !isBarrel )
      ecalPart = "ENDCAP";
    printf("\nProcessing the first photon. Working point %s. %s. Cut values:\n",
	   ecalPart.Data(), wpName[iwp].Data());
    printf("   H/E   < %f \n", hOverECut[ieta][iwp]);
    printf("   Sieie < %f \n", sieieCut[ieta][iwp]);
    printf("   charged isolation < %f \n", chIsoCut[ieta][iwp]);
    printf("   neu had isolation < %f + pt * %f\n", nhIso_A[ieta][iwp],
	   nhIso_B[ieta][iwp]);
    printf("   photon  isolation < %f + pt * %f\n", phIso_A[ieta][iwp],
	   phIso_B[ieta][iwp]);
    if( isBarrel )
      firstBarrelPhoton = false;
    else
      firstEndcapPhoton = false;
  }
   
  return result;
}


//
// MC truth matching
//

// ggNtuple arrays needed:
//   mcStatus, mcPID, mcMomPID, mcEta, mcPhi 
bool isMatched(float pEta, float pPhi,
	       std::vector<int> *mcPID,
	       std::vector<int> *mcMomPID,
	       std::vector<int> *mcStatus,
	       std::vector<float> *mcEta,
	       std::vector<float> *mcPhi){

  bool isMatched = false;
  // double genPt = -1;   
  if(verbose) printf("Check match for photon eta= %f   phi= %f\n", pEta, pPhi);
  for(uint imc = 0; imc < (*mcPID).size(); imc++){

    if(verbose) printf("   Check next particle: pid= %d  status= %d   mom= %d  (eta,phi)=(%f,%f)\n",
		       (*mcPID)[imc], (*mcStatus)[imc], (*mcMomPID)[imc], (*mcEta)[imc], (*mcPhi)[imc]);
    double msts = (*mcStatus)[imc];
    if((msts != 1)||((*mcPID)[imc] != 22))continue;
    if(verbose)printf("      passed pid and status\n");

    if((fabs((*mcMomPID)[imc]) !=21) 
       &&(fabs((*mcMomPID)[imc]) !=1)
       &&(fabs((*mcMomPID)[imc]) !=2)
       &&(fabs((*mcMomPID)[imc]) !=3)
       &&(fabs((*mcMomPID)[imc]) !=4)
       &&(fabs((*mcMomPID)[imc]) !=5)
       &&(fabs((*mcMomPID)[imc]) !=6))continue;    
    if(verbose) printf("       passed mother\n");
    
    double meta = (*mcEta)[imc];
    double mphi = (*mcPhi)[imc];
    
    TVector3 mcphoton;
    TVector3 recoPHOTOn;
    mcphoton.SetPtEtaPhi(1.0,meta,mphi);
    recoPHOTOn.SetPtEtaPhi(1.0,pEta,pPhi);
    
    double DR = mcphoton.DrEtaPhi(recoPHOTOn);
    if(verbose) printf("        dR=%f\n", DR);
    if(DR < 0.1 ){
      isMatched = true;
      if(verbose)printf("         PASSE ALL\n");
      // genPt = (*mcPt)[imc];
      break;
    }  
  }

  return isMatched;
}

bool isMatchedV2(float pEta, float pPhi,
		 std::vector<int> *mcPID,
		 std::vector<int> *mcMomPID,
		 std::vector<int> *mcStatus,
		 std::vector<float> *mcEta,
		 std::vector<float> *mcPhi){
  
  bool isMatched = false;
  if(verbose) printf("Check match for photon eta= %f   phi= %f\n", pEta, pPhi);

  for(uint imc = 0; imc < (*mcPID).size(); imc++){

    if((*mcStatus)[imc] != 1)continue; 
    if((*mcPID)[imc] != 22)continue;	  
    if((fabs((*mcMomPID)[imc]) !=21) 
       && (fabs((*mcMomPID)[imc]) !=1)
       && (fabs((*mcMomPID)[imc]) !=2)
       && (fabs((*mcMomPID)[imc]) !=3)
       && (fabs((*mcMomPID)[imc]) !=4)
       && (fabs((*mcMomPID)[imc]) !=5)
       && (fabs((*mcMomPID)[imc]) !=6) )continue;
	
    double meta = (*mcEta)[imc];
    double mphi = (*mcPhi)[imc];	  
	  
    TVector3 mcphoton;
    TVector3 recoPHOTOn;
	  
    mcphoton.SetPtEtaPhi(1.0,meta,mphi);
    recoPHOTOn.SetPtEtaPhi(1.0,pEta,pPhi);			       
    double DR = mcphoton.DrEtaPhi(recoPHOTOn);
    if(DR < 0.1 ){
      isMatched = true;
    }
  } // end loop over MC particles

  return isMatched;
}
