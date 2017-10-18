#include "Cabling/Cable.hh"
#include "Cabling/DTC.hh"


Cable::Cable(const int id, const double phiSectorWidth, const int phiSectorRef, const Category& type, const int slot, const bool isPositiveCablingSide) : 
  phiSectorWidth_(phiSectorWidth),
  phiSectorRef_(phiSectorRef),
  type_(type),
  slot_(slot),
  isPositiveCablingSide_(isPositiveCablingSide)
{ 
  myid(id);
  // ASSIGN A SERVICESCHANNEL TO THE CABLE
  const std::tuple<int, ChannelSection, int>& servicesChannelInfo = computeServicesChannel(phiSectorRef, type, slot, isPositiveCablingSide);
  servicesChannel_ = std::get<0>(servicesChannelInfo);
  servicesChannelSection_ = std::get<1>(servicesChannelInfo);
  servicesChannelPlotColor_ = std::get<2>(servicesChannelInfo);

  // BUILD DTC ASOCIATED TO THE CABLE
  buildDTC(phiSectorWidth, phiSectorRef, type, slot, isPositiveCablingSide);  
};


Cable::~Cable() {
  delete myDTC_;       // TO DO: switch to smart pointers and remove this!
  myDTC_ = nullptr;
}


/* Compute services channels.
 * They are the channels where the optical cables are routed when they exit the tracker.
 * They are closely related to the phiSector ref.
 */
const std::tuple<int, ChannelSection, int> Cable::computeServicesChannel(const int phiSectorRef, const Category& type, const int slot, const bool isPositiveCablingSide) const {
  int servicesChannel = 0;
  ChannelSection servicesChannelSection = ChannelSection::UNKNOWN;

  if (type == Category::PS10G) {
    if (phiSectorRef == 0) { servicesChannel = 1; servicesChannelSection = ChannelSection::B; }
    else if (phiSectorRef == 1) { servicesChannel = 2; servicesChannelSection = ChannelSection::B; }
    else if (phiSectorRef == 2) { servicesChannel = 3; servicesChannelSection = ChannelSection::B; }
    else if (phiSectorRef == 3) { servicesChannel = 5; servicesChannelSection = ChannelSection::B; }
    else if (phiSectorRef == 4) { servicesChannel = 6; servicesChannelSection = ChannelSection::B; }
    else if (phiSectorRef == 5) { servicesChannel = 8; servicesChannelSection = ChannelSection::B; }
    else if (phiSectorRef == 6) { servicesChannel = 9; servicesChannelSection = ChannelSection::B; }
    else if (phiSectorRef == 7) { servicesChannel = 11; servicesChannelSection = ChannelSection::B; }
    else if (phiSectorRef == 8) { servicesChannel = 12; servicesChannelSection = ChannelSection::B; }   
  }

  else if (type == Category::PS5G) {
    if (slot == 3) {
      if (phiSectorRef == 0) { servicesChannel = 3; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 1) { servicesChannel = 4; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 2) { servicesChannel = 5; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 3) { servicesChannel = 6; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 4) { servicesChannel = 7; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 5) { servicesChannel = 8; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 6) { servicesChannel = 9; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 7) { servicesChannel = 10; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 8) { servicesChannel = 11; servicesChannelSection = ChannelSection::B; }
    }
    else {
      if (phiSectorRef == 0) { servicesChannel = 1; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 1) { servicesChannel = 2; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 2) { servicesChannel = 4; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 3) { servicesChannel = 5; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 4) { servicesChannel = 7; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 5) { servicesChannel = 8; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 6) { servicesChannel = 10; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 7) { servicesChannel = 11; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 8) { servicesChannel = 12; servicesChannelSection = ChannelSection::B; }
    }
  }

  else if (type == Category::SS) {
    if (slot == 1 || slot == 2) {
      if (phiSectorRef == 0) { servicesChannel = 3; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 1) { servicesChannel = 4; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 2) { servicesChannel = 5; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 3) { servicesChannel = 6; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 4) { servicesChannel = 7; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 5) { servicesChannel = 8; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 6) { servicesChannel = 9; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 7) { servicesChannel = 10; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 8) { servicesChannel = 11; servicesChannelSection = ChannelSection::B; }
    }
    else if (slot == 3) {
      if (phiSectorRef == 0) { servicesChannel = 1; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 1) { servicesChannel = 2; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 2) { servicesChannel = 3; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 3) { servicesChannel = 5; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 4) { servicesChannel = 6; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 5) { servicesChannel = 8; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 6) { servicesChannel = 9; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 7) { servicesChannel = 11; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 8) { servicesChannel = 12; servicesChannelSection = ChannelSection::B; }
    }
    else {
      if (phiSectorRef == 0) { servicesChannel = 1; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 1) { servicesChannel = 2; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 2) { servicesChannel = 3; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 3) { servicesChannel = 4; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 4) { servicesChannel = 6; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 5) { servicesChannel = 7; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 6) { servicesChannel = 9; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 7) { servicesChannel = 10; servicesChannelSection = ChannelSection::B; }
      else if (phiSectorRef == 8) { servicesChannel = 12; servicesChannelSection = ChannelSection::B; }
    }
  }


  // VERY IMPORTANT!
  // COMPUTE SERVICES CHANNEL PLOT COLOR, FOR BOTH SIDES, BEFORE THE CHANNEL NUMBERING IS MIRRORED FOR (-Z) SIDE.
  // THIS IS BECAUSE A GIVEN SERVICE CHANNEL IS IDENTICAL ALL ALONG Z (going from (+Z) to (-Z) side).
  // HENCE, EVEN IF THE CHANNEL NUMBERING CAN BE DIFFERENT, WE DONT CARE, AND WE ACTUALLY WANT A GIVEN CHANNEL COLORED BY A UNIQUE COLOR!!
  int servicesChannelPlotColor = computeServicesChannelPlotColor(servicesChannel, servicesChannelSection);


  // NEGATIVE CABLING SIDE.
  // A given services channel is simplified as a straight line all along (Z).
  // THIS DEFINES THE CHANNEL NUMBERING ON THE (-Z) SIDE.

  // OPTION A: 
  // Channel 1A on (+Z) side becomes -1A on the (-Z) side, 1C on (+Z) side becomes -1C on (-Z) side, and so on.
  if (!isPositiveCablingSide) {
    servicesChannel *= -1;
  }

  // OPTION B (NOT PRESENTLY RETAINED)
  /*
  // This is the following transformation:
  // 1 -> 6
  // 2 -> 5
  // 3 -> 4
  // 7 -> 12
  // 8 -> 11
  // 9 -> 10
  // This is so that the numbering follows a rotation of 180 degrees around CMS_Y for the negative cabling side.
  // The services channel is then set to negative on negative cabling side.
  if (!isPositiveCablingSide) {
  double pivot = (servicesChannel <= 6 ? 3.5 : 9.5);
  servicesChannel = servicesChannel + round( 2. * (pivot - servicesChannel) );
  servicesChannel *= -1;
  servicesChannelSection = (servicesChannelSection == ChannelSection::A ? ChannelSection::C : ChannelSection::A);
  }*/

  return std::make_tuple(servicesChannel, servicesChannelSection, servicesChannelPlotColor);
}


/* Compute color associated to services channel.
 * If section A, +12 is added so that the same color is used as scetion C, but that color can be set as transparent if desired.
 */
const int Cable::computeServicesChannelPlotColor(const int servicesChannel, const ChannelSection& servicesChannelSection) const {
  int plotColor = 0;
  plotColor = servicesChannel;
  if (servicesChannelSection == ChannelSection::A) plotColor += 12;
  return plotColor;
}


void Cable::assignPowerServicesChannels() {
  /*std::vector<std::pair<int, double> > myBundlesPhis;
    for (auto& myBundle : bundles_) {
    const int bundleId = myBundle.myid();
    const double bundlePhi = myBundle.meanPhi();
    myBundlesPhis.push_back(std::make_pair(bundleId, bundlePhi));
    }
    std::sort(myBundlesPhis.begin(), myBundlesPhis.end(), 
    [] (std::pair<int, double> a, std::pair<int, double> b) { 
    return (moduloComp(a.second, b.second, 2.*M_PI)); 
    } 
    );*/


  //for (auto& myBundle : bundles_) {
  /*const int bundleId = myBundle.myid();
    const int phiIndex = std::distance(myBundlesPhis.begin(), it);
    const int semiPhiRegionIndex = (phiIndex <= 2 ? 0 : 1);
    const int semiPhiRegionRef = 2 * phiSectorRef_ + semiPhiRegionIndex;
    std::pair<int, ChannelSection> powerServicesChannel = computePowerServicesChannel(semiPhiRegionRef, isPositiveCablingSide_);
    myBundle.setPowerServicesChannel(powerServicesChannel);
    /*} 
    else {
    std::cout << "ERROR: bundle Id not found in my own bundles vector!" << std::endl;
    }    */



  for (auto& myBundle : bundles_) {
    const double bundlePhi = myBundle.meanPhi();

    const int phiIndex = 
    const int semiPhiRegionIndex = (phiIndex <= 2 ? 0 : 1);

    const int semiPhiRegionRef = 2 * phiSectorRef_ + semiPhiRegionIndex;
    std::pair<int, ChannelSection> powerServicesChannel = computePowerServicesChannel(semiPhiRegionRef, isPositiveCablingSide_);
    myBundle.setPowerServicesChannel(powerServicesChannel);
    /*} 
      else {
      std::cout << "ERROR: bundle Id not found in my own bundles vector!" << std::endl;
      }    */
  }
}


std::pair<int, ChannelSection> Cable::computePowerServicesChannel(const int semiPhiRegionRef, const bool isPositiveCablingSide) {

  int servicesChannel = 0;
  ChannelSection servicesChannelSection = ChannelSection::B;

  if (isPositiveCablingSide) {
    if (semiPhiRegionRef == 0) { servicesChannel = 1; servicesChannelSection = ChannelSection::C; }
    else if (semiPhiRegionRef == 1) { servicesChannel = 2; servicesChannelSection = ChannelSection::A; }
    else if (semiPhiRegionRef == 2) { servicesChannel = 2; servicesChannelSection = ChannelSection::C; }
    else if (semiPhiRegionRef == 3) { servicesChannel = 3; servicesChannelSection = ChannelSection::C; }
    else if (semiPhiRegionRef == 4) { servicesChannel = 4; servicesChannelSection = ChannelSection::A; }
    else if (semiPhiRegionRef == 5) { servicesChannel = 4; servicesChannelSection = ChannelSection::C; }
    else if (semiPhiRegionRef == 6) { servicesChannel = 5; servicesChannelSection = ChannelSection::C; }
    else if (semiPhiRegionRef == 7) { servicesChannel = 6; servicesChannelSection = ChannelSection::A; }
    else if (semiPhiRegionRef == 8) { servicesChannel = 6; servicesChannelSection = ChannelSection::C; }
    else if (semiPhiRegionRef == 9) { servicesChannel = 7; servicesChannelSection = ChannelSection::C; }
    else if (semiPhiRegionRef == 10) { servicesChannel = 8; servicesChannelSection = ChannelSection::A; }
    else if (semiPhiRegionRef == 11) { servicesChannel = 8; servicesChannelSection = ChannelSection::C; }
    else if (semiPhiRegionRef == 12) { servicesChannel = 9; servicesChannelSection = ChannelSection::C; }
    else if (semiPhiRegionRef == 13) { servicesChannel = 10; servicesChannelSection = ChannelSection::A; }
    else if (semiPhiRegionRef == 14) { servicesChannel = 10; servicesChannelSection = ChannelSection::C; }
    else if (semiPhiRegionRef == 15) { servicesChannel = 11; servicesChannelSection = ChannelSection::C; }
    else if (semiPhiRegionRef == 16) { servicesChannel = 12; servicesChannelSection = ChannelSection::A; }
    else if (semiPhiRegionRef == 17) { servicesChannel = 12; servicesChannelSection = ChannelSection::C; }
    else { std::cout << "ERROR: semiPhiRegionRef = " << semiPhiRegionRef << std::endl; }
  }

  // This is the following transformation:
  // 1 -> 6
  // 2 -> 5
  // 3 -> 4
  // 7 -> 12
  // 8 -> 11
  // 9 -> 10
  // This is so that the numbering follows a rotation of 180 degrees around CMS_Y for the negative cabling side.
  // The services channel is then set to negative on negative cabling side.
  /*if (!isPositiveCablingSide) {
    const double pivot = (servicesChannel <= 6 ? 3.5 : 9.5);
    servicesChannel = servicesChannel + round( 2. * (pivot - servicesChannel) );
    servicesChannel *= -1;
    servicesChannelSection = (servicesChannelSection == ChannelSection::A ? ChannelSection::C : ChannelSection::A);
    }*/

  else {
    if (semiPhiRegionRef == 0) { servicesChannel = -1; servicesChannelSection = ChannelSection::A; }
    else if (semiPhiRegionRef == 1) { servicesChannel = -1; servicesChannelSection = ChannelSection::C; }
    else if (semiPhiRegionRef == 2) { servicesChannel = -2; servicesChannelSection = ChannelSection::A; }
    else if (semiPhiRegionRef == 3) { servicesChannel = -3; servicesChannelSection = ChannelSection::A; }
    else if (semiPhiRegionRef == 4) { servicesChannel = -3; servicesChannelSection = ChannelSection::C; }
    else if (semiPhiRegionRef == 5) { servicesChannel = -4; servicesChannelSection = ChannelSection::A; }
    else if (semiPhiRegionRef == 6) { servicesChannel = -5; servicesChannelSection = ChannelSection::A; }
    else if (semiPhiRegionRef == 7) { servicesChannel = -5; servicesChannelSection = ChannelSection::C; }
    else if (semiPhiRegionRef == 8) { servicesChannel = -6; servicesChannelSection = ChannelSection::A; }
    else if (semiPhiRegionRef == 9) { servicesChannel = -7; servicesChannelSection = ChannelSection::A; }
    else if (semiPhiRegionRef == 10) { servicesChannel = -7; servicesChannelSection = ChannelSection::C; }
    else if (semiPhiRegionRef == 11) { servicesChannel = -8; servicesChannelSection = ChannelSection::A; }
    else if (semiPhiRegionRef == 12) { servicesChannel = -9; servicesChannelSection = ChannelSection::A; }
    else if (semiPhiRegionRef == 13) { servicesChannel = -9; servicesChannelSection = ChannelSection::C; }
    else if (semiPhiRegionRef == 14) { servicesChannel = -10; servicesChannelSection = ChannelSection::A; }
    else if (semiPhiRegionRef == 15) { servicesChannel = -11; servicesChannelSection = ChannelSection::A; }
    else if (semiPhiRegionRef == 16) { servicesChannel = -11; servicesChannelSection = ChannelSection::C; }
    else if (semiPhiRegionRef == 17) { servicesChannel = -12; servicesChannelSection = ChannelSection::A; }
    else { std::cout << "ERROR: semiPhiRegionRef = " << semiPhiRegionRef << std::endl; }
  }

  return std::make_pair(servicesChannel, servicesChannelSection);
}


/* Build DTC asociated to the cable.
 */
void Cable::buildDTC(const double phiSectorWidth, const int phiSectorRef, const Category& type, const int slot, const bool isPositiveCablingSide) {
  std::string dtcName = computeDTCName(phiSectorRef, type, slot, isPositiveCablingSide);
  DTC* dtc = GeometryFactory::make<DTC>(dtcName, phiSectorWidth, phiSectorRef, type, slot, isPositiveCablingSide);
  dtc->addCable(this);
  myDTC_ = dtc;
}


/* Compute DTC name.
 */
const std::string Cable::computeDTCName(const int phiSectorRef, const Category& type, const int slot, const bool isPositiveCablingSide) const {
  std::ostringstream dtcNameStream;
  if (!isPositiveCablingSide) dtcNameStream << cabling_negativePrefix << "_";
  dtcNameStream << phiSectorRef << "_" << any2str(type) << "_" << slot;
  const std::string dtcName = dtcNameStream.str();
  return dtcName;
}
