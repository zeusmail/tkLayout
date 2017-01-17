#ifndef IRRADIATIONPOWER_H
#define IRRADIATIONPOWER_H

#include <string>
#include <map>
#include <vector>
#include <utility>

#include "global_constants.h"
#include "Units.h"
#include "Tracker.h"
#include "SimParms.h"
#include "Visitor.h"
#include "SummaryTable.h"

class IrradiationPowerVisitor : public GeometryVisitor {
  double timeIntegratedLumi;
  double referenceTemp;
  double operatingTemp;
  double alphaParam;
  double biasVoltage;
  const IrradiationMapsManager* irradiationMap_;

public:
  MultiSummaryTable sensorsIrradiationPowerSummary;
  void preVisit();
  void visit(SimParms& sp);
  void visit(Barrel& b);
  void visit(Endcap& e);
  void visit(DetectorModule& m);
};


#endif
