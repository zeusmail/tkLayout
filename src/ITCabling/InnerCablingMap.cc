#include "ITCabling/InnerCablingMap.hh"
#include <Tracker.hh>


InnerCablingMap::InnerCablingMap(Tracker* tracker) {
  try {
    // CONNECT MODULES TO SERIAL POWER CHAINS
    connectModulesToPowerChains(tracker);

    // CONNECT MODULES TO LPGBTS
    connectModulesToGBTs(powerChains_, GBTs_);
    
    // CONNECT LPGBTS TO BUNDLES
    connectGBTsToBundles(GBTs_, bundles_);

    // CONNECT BUNDLES TO DTCs
    connectBundlesToDTCs(bundles_, DTCs_);
  }

  catch (PathfulException& pe) { pe.pushPath(fullid(*this)); throw; }
}


/* MODULES TO SERIAL POWER CHAINS CONNECTIONS.
 */
void InnerCablingMap::connectModulesToPowerChains(Tracker* tracker) {
  ModulesToPowerChainsConnector powerChainsBuilder;
  tracker->accept(powerChainsBuilder);
  powerChainsBuilder.postVisit();
  powerChains_ = powerChainsBuilder.getPowerChains();
}


/* MODULES TO GBTS CONNECTIONS.
 */
void InnerCablingMap::connectModulesToGBTs(std::map<int, PowerChain*>& powerChains, std::map<std::string, GBT*>& GBTs) {

  for (auto& it : powerChains) {
    // COLLECT ALL INFORMATION NEEDED TO BUILD GBTS
    
    PowerChain* myPowerChain = it.second;

    const bool isBarrel = myPowerChain->isBarrel();
    const std::string subDetectorName = myPowerChain->subDetectorName();
    const int layerNumber = myPowerChain->layerDiskNumber();
    const int ringNumber = myPowerChain->ringNumber();
    const int numModulesInPowerChain = myPowerChain->numModules();

    const int layerOrRingNumber = (isBarrel ? layerNumber : ringNumber);
    const int numELinksPerModule = inner_cabling_functions::computeNumELinksPerModule(subDetectorName, layerOrRingNumber);

    const std::pair<int, int> gbtsInPowerChain = computeMaxNumModulesPerGBTInPowerChain(numELinksPerModule, numModulesInPowerChain, isBarrel);
    const int maxNumModulesPerGBTInPowerChain = gbtsInPowerChain.first;
    const int numGBTsInPowerChain = gbtsInPowerChain.second;
    

    for (auto& m : myPowerChain->modules()) {
      m.setNumELinks(numELinksPerModule);

      const int powerChainId = myPowerChain->myid();
      const bool isBarrelLong = myPowerChain->isBarrelLong();
      const int ringRef = (isBarrelLong ? m.uniRef().ring - 1 : m.uniRef().ring - 2);
      const int phiRefInPowerChain = m.getPhiRefInPowerChain();
      //std::cout << "phiRefInPowerChain = " << phiRefInPowerChain << std::endl;
      
      const std::pair<int, int> myGBTIndexes = computeGBTPhiIndex(isBarrel, ringRef, phiRefInPowerChain, maxNumModulesPerGBTInPowerChain, numGBTsInPowerChain);
      const int myGBTIndex = myGBTIndexes.first;
      const int myGBTIndexColor = myGBTIndexes.second;
      const std::string myGBTId = computeGBTId(powerChainId, myGBTIndex);

      // BUILD GBTS AND STORE THEM
      createAndStoreGBTs(myPowerChain, m, myGBTId, myGBTIndex, myGBTIndexColor, numELinksPerModule, GBTs);
    }
  }

  // CHECK GBTS
  checkModulesToGBTsCabling(GBTs);
}



// MODULES TO GBTS !!!!!

const std::pair<int, int> InnerCablingMap::computeMaxNumModulesPerGBTInPowerChain(const int numELinksPerModule, const int numModulesInPowerChain, const bool isBarrel) {
  //std::cout << "STARTTTTTTTTTTTTTTT InnerCablingMap::computeMaxNumModulesPerGBTInPowerChain  " << " numELinksPerModule = " <<  numELinksPerModule << ", numModulesInPowerChain = " << numModulesInPowerChain << ", isBarrel = " << isBarrel << std::endl;

  int numModules = numModulesInPowerChain;
  if (isBarrel) {
    if (numModules % 2 == 1) logERROR(any2str("Found an odd number of modules in BPIX power chain, which is not supported."));
    else numModules /= 2;  // Divide by 2 because in BPIX, the GBTs assignment works by rod.
                           // This is becasue it makes the powering of the GBTs much easier.
  }
  //std::cout << "numModules = " << numModules << std::endl;

  const int numELinks = numELinksPerModule * numModules;
  //std::cout << "numELinks = " << numELinks << std::endl;

  const double numGBTsExact = static_cast<double>(numELinks) / inner_cabling_maxNumELinksPerGBT;
  //std::cout << "numGBTsExact = " << numGBTsExact << std::endl;
  const int numGBTs = (fabs(numGBTsExact - round(numGBTsExact)) < inner_cabling_roundingTolerance ? 
		       round(numGBTsExact) 
		       : std::ceil(numGBTsExact)
		       );

  if (numGBTs == 0) logERROR(any2str("Power chain has ") + any2str(numModules) 
			     + any2str(" modules, but found numGBTs == ") +  any2str(numGBTs) + any2str(", that's not enough!!")
			     );

  const double maxNumModulesPerGBTExact = static_cast<double>(numModules) / numGBTs;
  const int maxNumModulesPerGBTInPowerChain = (fabs(maxNumModulesPerGBTExact - round(maxNumModulesPerGBTExact)) < inner_cabling_roundingTolerance ? 
					       round(maxNumModulesPerGBTExact) 
					       : std::ceil(maxNumModulesPerGBTExact)
					       );
  // TO DO: MINOR ISSUE OF TAKING THE ceil IS THAT it doesnt handle optimally the (RARE) case where we have 3 + 2 + 2
  // Since it will lead to following repartition: 3 + 3 + 1
  // One could take the floor, and then look at the number of remaining modules in power chain.
  // This case does not appear in practice, but can be worked on if needed.

  //std::cout << "numGBTs = " << numGBTs << std::endl;
  //std::cout << "maxNumModulesPerGBTInPowerChain = " << maxNumModulesPerGBTInPowerChain << std::endl;

  return std::make_pair(maxNumModulesPerGBTInPowerChain, numGBTs);
}


/* Compute the phi index associated to each GBT.
 */
const std::pair<int, int> InnerCablingMap::computeGBTPhiIndex(const bool isBarrel, const int ringRef, const int phiRefInPowerChain, const int maxNumModulesPerGBTInPowerChain, const int numGBTsInPowerChain) const {
  //std::cout << "InnerCablingMap::computeGBTPhiIndex " << std::endl;

  const int moduleRef = (isBarrel ? ringRef : phiRefInPowerChain);
  //std::cout << "moduleRef = " << moduleRef << std::endl;

  if (maxNumModulesPerGBTInPowerChain == 0) logERROR(any2str("Found maxNumModulesPerGBTInPowerChain == 0."));

  const double myGBTIndexExact = static_cast<double>(moduleRef) / maxNumModulesPerGBTInPowerChain;
  //std::cout << "myGBTIndexExact = " << myGBTIndexExact << std::endl;
  int myGBTIndex = (fabs(myGBTIndexExact - round(myGBTIndexExact)) < inner_cabling_roundingTolerance ? 
		    round(myGBTIndexExact) 
		    : std::floor(myGBTIndexExact)
		    );
  if (isBarrel && phiRefInPowerChain == 1) myGBTIndex += numGBTsInPowerChain;
  //std::cout << "myGBTIndex = " << myGBTIndex << std::endl;

  int myGBTIndexColor = myGBTIndex;
  if (isBarrel && phiRefInPowerChain == 1 && femod(numGBTsInPowerChain, 2) == 0) myGBTIndexColor += 1;
  myGBTIndexColor = femod(myGBTIndexColor, 2);

  return std::make_pair(myGBTIndex, myGBTIndexColor);
}


/* Compute the Id associated to each GBT.
 */
const std::string InnerCablingMap::computeGBTId(const int powerChainId, const int myGBTIndex) const {
  std::ostringstream GBTIdStream;
  GBTIdStream << powerChainId << "_" << myGBTIndex;
  const std::string GBTId = GBTIdStream.str();
  return GBTId;
}


/* Create a GBT, if does not exist yet.
 * Store it in the GBTs container.
 */
void InnerCablingMap::createAndStoreGBTs(PowerChain* myPowerChain, Module& m, const std::string myGBTId, const int myGBTIndex, const int myGBTIndexColor, const int numELinksPerModule, std::map<std::string, GBT*>& GBTs) {

  auto found = GBTs.find(myGBTId);
  if (found == GBTs.end()) {
    GBT* myGBT = GeometryFactory::make<GBT>(myPowerChain, myGBTId, myGBTIndex, myGBTIndexColor, numELinksPerModule);
    connectOneModuleToOneGBT(m, myGBT);
    GBTs.insert(std::make_pair(myGBTId, myGBT));  
  }
  else {
    connectOneModuleToOneGBT(m, found->second);
  }
}


/* Connect module to GBT and vice-versa.
 */
void InnerCablingMap::connectOneModuleToOneGBT(Module& m, GBT* myGBT) const {
  myGBT->addModule(&m);
  m.setGBT(myGBT);
}


/* Check bundles-cables connections.
 */

void InnerCablingMap::checkModulesToGBTsCabling(const std::map<std::string, GBT*>& GBTs) const {
  for (const auto& it : GBTs) {
    const std::string myGBTId = it.first;
    const GBT* myGBT = it.second;

    // CHECK THE NUMBER OF ELINKS PER GBT.
    const int numELinks = myGBT->numELinks();

    if (numELinks > inner_cabling_maxNumELinksPerGBT) {
      logERROR(any2str("GBT ")  + any2str(myGBTId) + any2str(" is connected to ") + any2str(numELinks) + any2str(" ELinks. ")
	       + "Maximum number of ELinks per GBT allowed is " + any2str(inner_cabling_maxNumELinksPerGBT)
	       );
    }
  }
}






// GBTs to BUNDLES !!!!


/* GBTS TO BUNDLES CONNECTIONS.
 */
void InnerCablingMap::connectGBTsToBundles(std::map<std::string, GBT*>& GBTs, std::map<int, InnerBundle*>& bundles) {

  for (auto& it : GBTs) {
    // COLLECT ALL INFORMATION NEEDED TO BUILD BUNDLES   
    GBT* myGBT = it.second;

    const std::string subDetectorName = myGBT->subDetectorName();
    
    const int layerDiskNumber = myGBT->layerDiskNumber();
    const int powerChainPhiRef = myGBT->powerChainPhiRef();
    const int ringNumber = myGBT->ringNumber();

    //std::cout << "subDetectorName = " << subDetectorName << std::endl;
    //std::cout << "layerDiskNumber = " << layerDiskNumber << std::endl;
    //std::cout << "powerChainPhiRef = " << powerChainPhiRef << std::endl;
        
    const int myBundleIndex = computeBundleIndex(subDetectorName, layerDiskNumber, powerChainPhiRef, ringNumber);

    const bool isPositiveZEnd = myGBT->isPositiveZEnd();
    const bool isPositiveXSide = myGBT->isPositiveXSide();
    const int myBundleId = computeBundleId(isPositiveZEnd, isPositiveXSide, subDetectorName, layerDiskNumber, myBundleIndex);

    // BUILD BUNDLES AND STORE THEM
    createAndStoreBundles(myGBT, bundles, myBundleId, isPositiveZEnd, isPositiveXSide, subDetectorName, layerDiskNumber, myBundleIndex);    
  }

  // CHECK BUNDLES
  checkGBTsToBundlesCabling(bundles);
}


const int InnerCablingMap::computeBundleIndex(const std::string subDetectorName, const int layerNumber, const int powerChainPhiRef, const int ringNumber) const {
  int myBundleIndex = 0;

  if (subDetectorName == inner_cabling_tbpx) {

    // TO DO: THIS SHOULD BE COMPUTED AS A FUNCTION OF LAYER NUMBER, NOT HARCODED!!!
    const int maxNumPowerChainsPerBundleBarrelLayer1 = 1;
    const int maxNumPowerChainsPerBundleBarrelLayer2 = 3;
    const int maxNumPowerChainsPerBundleBarrelLayer3 = 3;
    const int maxNumPowerChainsPerBundleBarrelLayer4 = 4;

    int maxNumPowerChainsPerBundleBarrelLayer = 0;
    if (layerNumber == 1) maxNumPowerChainsPerBundleBarrelLayer = maxNumPowerChainsPerBundleBarrelLayer1;
    else if (layerNumber == 2) maxNumPowerChainsPerBundleBarrelLayer = maxNumPowerChainsPerBundleBarrelLayer2;
    else if (layerNumber == 3) maxNumPowerChainsPerBundleBarrelLayer = maxNumPowerChainsPerBundleBarrelLayer3;
    else if (layerNumber == 4) maxNumPowerChainsPerBundleBarrelLayer = maxNumPowerChainsPerBundleBarrelLayer4;
    else logERROR("Did not find supported layer number.");

    if (maxNumPowerChainsPerBundleBarrelLayer == 0) logERROR(any2str("Found maxNumPowerChainsPerBundleBarrelLayer == 0."));

    //std::cout << "maxNumPowerChainsPerBundleBarrelLayer = " << maxNumPowerChainsPerBundleBarrelLayer << std::endl;

    const double myBundleIndexExact = static_cast<double>(powerChainPhiRef) / maxNumPowerChainsPerBundleBarrelLayer;
    myBundleIndex = (fabs(myBundleIndexExact - round(myBundleIndexExact)) < inner_cabling_roundingTolerance ? 
			 round(myBundleIndexExact) 
			 : std::floor(myBundleIndexExact)
			 );
    //std::cout << "myBundleIndexExact = " << myBundleIndexExact << std::endl;
    //std::cout << "myBundleIndex = " << myBundleIndex << std::endl;
  }
  else if (subDetectorName == inner_cabling_tfpx || subDetectorName == inner_cabling_tepx) {
    myBundleIndex = (femod(ringNumber, 2) == 1 ? 0 : 1);
  }
  else logERROR("Unsupported detector name.");

  return myBundleIndex;
}


/* Compute the Id associated to each bundle.
 */
const int InnerCablingMap::computeBundleId(const bool isPositiveZEnd, const bool isPositiveXSide, const std::string subDetectorName, const int layerDiskNumber, const int myBundleIndex) const {

  const int innerTrackerQuarterIndex = inner_cabling_functions::computeInnerTrackerQuarterIndex(isPositiveZEnd, isPositiveXSide);
  const int subdetectorIndex = inner_cabling_functions::computeSubDetectorIndex(subDetectorName);

  const int bundleId = innerTrackerQuarterIndex * 1000 + subdetectorIndex * 100 + layerDiskNumber * 10 + myBundleIndex;
  return bundleId;
}


void InnerCablingMap::createAndStoreBundles(GBT* myGBT, std::map<int, InnerBundle*>& bundles, const int bundleId, const bool isPositiveZEnd, const bool isPositiveXSide, const std::string subDetectorName, const int layerDiskNumber, const int myBundleIndex) {

  auto found = bundles.find(bundleId);
  if (found == bundles.end()) {
    InnerBundle* myBundle = GeometryFactory::make<InnerBundle>(bundleId, isPositiveZEnd, isPositiveXSide, subDetectorName, layerDiskNumber, myBundleIndex);
    connectOneGBTToOneBundle(myGBT, myBundle);
    bundles.insert(std::make_pair(bundleId, myBundle));  
  }
  else {
    connectOneGBTToOneBundle(myGBT, found->second);
  }
}


/* Connect GBT to Bundle and vice-versa.
 */
void InnerCablingMap::connectOneGBTToOneBundle(GBT* myGBT, InnerBundle* myBundle) const {
  myBundle->addGBT(myGBT);
  myGBT->setBundle(myBundle);
}


/* Check GBTs-Bundle connections.
 */

void InnerCablingMap::checkGBTsToBundlesCabling(const std::map<int, InnerBundle*>& bundles) const {
  for (const auto& it : bundles) {
    const int myBundleId = it.first;
    const InnerBundle* myBundle = it.second;

    // CHECK THE NUMBER OF GBTs per Bundle
    const int numGBTs = myBundle->numGBTs();

    if (numGBTs > inner_cabling_maxNumGBTsPerBundle) {
      logERROR(any2str("InnerBundle ")  + any2str(myBundleId) + any2str(" is connected to ") + any2str(numGBTs) + any2str(" GBTs. ")
	       + "Maximum number of GBTs per Bundle allowed is " + any2str(inner_cabling_maxNumGBTsPerBundle)
	       );
    }
  }
}








// BUNDLES TO DTCS !!!!!!

void InnerCablingMap::connectBundlesToDTCs(std::map<int, InnerBundle*>& bundles, std::map<int, InnerDTC*>& DTCs) {

 for (auto& it : bundles) {
    // COLLECT ALL INFORMATION NEEDED TO BUILD DTCS   
    InnerBundle* myBundle = it.second;

    const std::string subDetectorName = myBundle->subDetectorName();   
    const int layerDiskNumber = myBundle->layerDiskNumber();
        
    const bool isPositiveZEnd = myBundle->isPositiveZEnd();
    const bool isPositiveXSide = myBundle->isPositiveXSide();
    const int myDTCId = computeDTCId(isPositiveZEnd, isPositiveXSide, subDetectorName, layerDiskNumber);

    // BUILD DTCS AND STORE THEM
    createAndStoreDTCs(myBundle, DTCs, myDTCId, isPositiveZEnd, isPositiveXSide);    
  }

  // CHECK DTCS
  checkBundlesToDTCsCabling(DTCs);
}


/* Compute the Id associated to each DTC.
 */
const int InnerCablingMap::computeDTCId(const bool isPositiveZEnd, const bool isPositiveXSide, const std::string subDetectorName, const int layerDiskNumber) const {
  int myDTCId = 0;

  if (subDetectorName == inner_cabling_tbpx) myDTCId = layerDiskNumber;

  else if (subDetectorName == inner_cabling_tfpx) {
    if (layerDiskNumber == 1) myDTCId = 5;
    else if (layerDiskNumber == 2) myDTCId = 4;
    else if (layerDiskNumber == 3) myDTCId = 3;
    else if (layerDiskNumber == 4) myDTCId = 5;
    else if (layerDiskNumber == 5) myDTCId = 4;
    else if (layerDiskNumber == 6) myDTCId = 3;
    else if (layerDiskNumber == 7) myDTCId = 2;
    else if (layerDiskNumber == 8) myDTCId = 5;
    else logERROR(any2str("Unexpected diskNumber in FPX : ") + any2str(layerDiskNumber));
  }

  else if (subDetectorName == inner_cabling_tepx) {
    if (layerDiskNumber == 1) myDTCId = 6;
    else if (layerDiskNumber == 2) myDTCId = 6;
    else if (layerDiskNumber == 3) myDTCId = 7;
    else if (layerDiskNumber == 4) myDTCId = 7;
    else logERROR(any2str("Unexpected diskNumber in EPX : ") + any2str(layerDiskNumber));
  }

  const int innerTrackerQuarterIndex = inner_cabling_functions::computeInnerTrackerQuarterIndex(isPositiveZEnd, isPositiveXSide);
  myDTCId += innerTrackerQuarterIndex * 10;

  return myDTCId;
}


void InnerCablingMap::createAndStoreDTCs(InnerBundle* myBundle, std::map<int, InnerDTC*>& DTCs, const int DTCId, const bool isPositiveZEnd, const bool isPositiveXSide) {

  auto found = DTCs.find(DTCId);
  if (found == DTCs.end()) {
    InnerDTC* myDTC = GeometryFactory::make<InnerDTC>(DTCId, isPositiveZEnd, isPositiveXSide);
    connectOneBundleToOneDTC(myBundle, myDTC);
    DTCs.insert(std::make_pair(DTCId, myDTC));  
  }
  else {
    connectOneBundleToOneDTC(myBundle, found->second);
  }
}


/* Connect Bundle to DTC and vice-versa.
 */
void InnerCablingMap::connectOneBundleToOneDTC(InnerBundle* myBundle, InnerDTC* myDTC) const {
  myDTC->addBundle(myBundle);
  myBundle->setDTC(myDTC);
}


/* Check Bundles-DTC connections.
 */

void InnerCablingMap::checkBundlesToDTCsCabling(const std::map<int, InnerDTC*>& DTCs) const {
  for (const auto& it : DTCs) {
    const int myDTCId = it.first;
    const InnerDTC* myDTC = it.second;

    // CHECK THE NUMBER OF Bundles per DTC
    const int numBundles = myDTC->numBundles();

    if (numBundles > inner_cabling_maxNumBundlesPerCable) {
      logERROR(any2str("InnerDTC ")  + any2str(myDTCId) + any2str(" is connected to ") + any2str(numBundles) + any2str(" Bundles. ")
	       + "Maximum number of Bundles per DTC allowed is " + any2str(inner_cabling_maxNumBundlesPerCable)
	       );
    }
  }
}





/* BUNDLES TO POWER SERVICE CHANNELS CONNECTIONS.
 * VERY IMPORTANT: connection scheme from modules to optical bundles = connection scheme from modules to power cables.
 * As a result, 1 single Bundle object is used for both schemes.
 * Regarding the connections to services channels, each Bundle is then assigned:
 * - 1 Optical Services Channel Section (considering the Bundle as an optical Bundle);
 * - 1 Power Services Channels section (making as if the Bundle is a power cable);
 * 
 * The optical channel mapping is done so that all bundles connected to the same DTC,
 * are routed through the same channel.
 *
 * The assignments of power cables to services channels must be done after the bundles to DTCs connections are established.
 * Indeed, the power channel mapping is done so that all modules connected to the same DTC, 
 * have their power cables routed through 2 consecutive channels sections at most.
 */
/*
void InnerCablingMap::computePowerServicesChannels() {
  for (bool isPositiveCablingSide : { true, false }) {

    // BARREL ONLY: IN VIEW OF THE POWER CHANNEL ASSIGNMENT, SPLIT EACH NONANT INTO 2 SEMI-NONANTS
    routeBarrelBundlesPoweringToSemiNonants(isPositiveCablingSide);
 
    // ASSIGN POWER SERVICES CHANNELS
    std::map<int, Cable*>& cables = (isPositiveCablingSide ? cables_ : negCables_);
    for (auto& c : cables) {
      c.second->assignPowerChannelSections();
    }

    // CHECK POWER SERVICES CHANNELS
    const std::map<int, Bundle*>& bundles = (isPositiveCablingSide ? bundles_ : negBundles_);
    checkBundlesToPowerServicesChannels(bundles);
  }
}
*/


/* Barrel only: in view of the power channel assignment, split each nonant into 2 semi-nonants.
   The cooling pipes design should be invariant by rotation of 180° around CMS_Y, to avoid different cooling designs on both (Z) side.
   The cooling pipes and power cables are assigned to similar channels slots (A or C).
   As a result, the channel assignmennt of power cables need to follow the same symmetry as the cooling pipes.
   Hence, the CHANNEL ASSIGNMENNT OF POWER CABLES NEED TO BE INVARIANT BY ROTATION OF 180DEG AROUND CMS_Y.
   This is a priori not trivial, since the power cables scheme follow the bundles scheme, hence the optical map mirror symmetry.
   This is only possible if, for each phi nonant, one define a semi-nonant Phi boundary in a certain way.
   This is what is done here: the semi-nonant Phi boundaries are defined
   so that ALL NONANTS AND SEMI-NONANTS PHI BOUNDARIES ARE INVARIANT BY ROTATION OF 180° AROUND CMS_Y.
 */
/*
void InnerCablingMap::routeBarrelBundlesPoweringToSemiNonants(const bool isPositiveInnerCablingSide) {
  std::map<int, Bundle*>& bundles = (isPositiveCablingSide ? bundles_ : negBundles_);
  const std::map<int, Bundle*>& stereoBundles = (isPositiveCablingSide ? negBundles_ : bundles_);

  // phiSectorRefMarker keeps track of the Phi nonant we are in.
  int phiSectorRefMarker = -1;
  // phiSectorRefMarker keeps track of the stereo Phi nonant we are in.
  // 'stereo' means on the other cabling side, by a rotation of 180° around CMS_Y.
  int stereoPhiSectorRefMarker = -1;

  // TILTED PART OF TBPS + TB2S
  // Loop on all bundles of a given cabling side (sorted by their BundleIds).
  for (auto& b : bundles) {
    Bundle* myBundle = b.second;
    const bool isBarrel = myBundle->isBarrel();
    const bool isBarrelPSFlatPart = myBundle->isBarrelPSFlatPart();

    // Only for Barrel: tilted TBPS, or TB2S.
    if (isBarrel && !isBarrelPSFlatPart) {
      // Should the bundle be assigned to the lower or upper semi-nonant ?
      // 'lower' and 'upper' are defined by 'smaller' or 'bigger' Phi, 
      // in the trigonometric sense in the (XY) plane in CMS global frame of reference.
      bool isLower; // what we want to compute!

      // Identifier of the Phi nonant we are in.
      const int phiSectorRef = myBundle->getCable()->phiSectorRef();
      // In case of a switch to a different Phi nonant, initialize variables.
      if (phiSectorRef != phiSectorRefMarker) {	
	phiSectorRefMarker = phiSectorRef;
	// Starts by assigning to bundle to the lower semi-nonant.
	isLower = true;
	stereoPhiSectorRefMarker = -1;	
      }

      // Get the bundle located on the other cabling side, by a rotation of 180° around CMS_Y.
      const int stereoBundleId = myBundle->stereoBundleId();
      auto found = stereoBundles.find(stereoBundleId);
      if (found != stereoBundles.end()) {
	const Bundle* myStereoBundle = found->second;
	// Get the Phi nonant in which the stereoBundle is located.
	const int stereoPhiSectorRef = myStereoBundle->getCable()->phiSectorRef();
	
	// Decisive point!! 
	// As soon as a change in the identifier of the stereoBundle Phi nonant is detected,
	// one assigns the bundle to the upper semi-nonant.
	if (stereoPhiSectorRefMarker != -1 && stereoPhiSectorRefMarker != stereoPhiSectorRef) isLower = false;

	// Lastly, assign the semi-nonant attribution decision to the bundle.
	myBundle->setIsPowerRoutedToBarrelLowerSemiNonant(isLower);

	// Keeps track of the Phi nonant in which the stereoBundle is located.
	stereoPhiSectorRefMarker = stereoPhiSectorRef;
      }
      else {
	logERROR(any2str("Could not find stereo bundle, id ") 
		    + any2str(stereoBundleId)
		    );
      }
    }

    // NB: All this is possible because the bundles and stereoBundles are sorted by their Ids.
    // One relies on 2 characteristics of the BundleId scheme:
    // - all Bundles connected to the same DTC will have consecutive Ids.
    // - the Id increment is in Phi, in the (XY) plane in CMS global frame of reference.
  }


  // FLAT PART OF TBPS
  // For a given bundle, connected to untilted modules:
  // Take the same semi-nonant assignment as the bundle located at the same Phi and connected to the tilted modules.

  // Loop on all bundles of a given cabling side (sorted by their BundleIds).
  for (auto& b : bundles) {
    Bundle* myBundle = b.second;
    const bool isBarrelPSFlatPart = myBundle->isBarrelPSFlatPart();

    // Only for Barrel: flat part of TBPS
    if (isBarrelPSFlatPart) {
      const int tiltedBundleId = myBundle->tiltedBundleId();
 
      // Get the bundle located at the same Phi, but connected to the tilted modules.
      auto found = bundles.find(tiltedBundleId);
      if (found != bundles.end()) {
	const Bundle* myTiltedBundle = found->second;

	// Decisive point!!
	// Get the semi-nonant attribution of the tiltedBundle.
	const bool isLower = myTiltedBundle->isPowerRoutedToBarrelLowerSemiNonant();

	// Lastly, assign the semi-nonant attribution decision to the bundle.
	myBundle->setIsPowerRoutedToBarrelLowerSemiNonant(isLower);
      }
      else {
	logERROR(any2str("Could not find bundle connected to tilted modules, id ") 
		 + any2str(tiltedBundleId)
		 );
      }
    }
  }

}
*/


/* Check services channels sections containing power cables.
 */
/*
void InnerCablingMap::checkBundlesToPowerServicesChannels(const std::map<int, Bundle*>& bundles) {
  std::map<std::pair<const int, const ChannelSlot>, int > channels;

  for (auto& b : bundles) {
    const ChannelSection* mySection = b.second->powerChannelSection();
    const int myChannelNumber = mySection->channelNumber();
    const ChannelSlot& myChannelSlot = mySection->channelSlot();

    // Check channel number.
    if (fabs(myChannelNumber) == 0 || fabs(myChannelNumber) > cabling_numServicesChannels) {
      logERROR(any2str("Invalid channel number = ")
	       + any2str(myChannelNumber)
	       + any2str(". Should not be 0 or have abs value > ")
	       + any2str(cabling_numServicesChannels) + any2str(".")
	       );
    }
    // Check channel slot.
    if (myChannelSlot != ChannelSlot::A && myChannelSlot != ChannelSlot::C) {
      logERROR(any2str("Power cable: Invalid channel slot = ") 
	       + any2str(myChannelSlot)
	       + any2str(". Should be ") + any2str(ChannelSlot::A)
	       + any2str(" or ") + any2str(ChannelSlot::C) + any2str(".")
	       );
    }

    // Compute number of power cables per channel section.
    std::pair<const int, const ChannelSlot> myChannel = std::make_pair(myChannelNumber, myChannelSlot);
    channels[myChannel] += 1;
  }

  // Check number of power cables per channel section.
  for (const auto& c : channels) { 
    if (c.second > cabling_maxNumPowerCablesPerChannel) {
      logERROR(any2str("Services channel ") + any2str(c.first.first) 
	       + any2str(" section ") + any2str(c.first.second) 
	       + any2str(" has " ) + any2str(c.second) + any2str(" power cables.")
	       + any2str(" Max number of power cables per channel section is ") 
	       + any2str(cabling_maxNumPowerCablesPerChannel)
	       );
    }
  }
}
*/
