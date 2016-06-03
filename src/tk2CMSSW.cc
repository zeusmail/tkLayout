/**
 * @file tk2CMSSW.cc
 * @brief This class provides the interface to analyse a tracker and write the results to XML files for CMSSW
 */

#include <SvnRevision.h>
#include <tk2CMSSW.h>





#include<iomanip>
#include <boost/version.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <string>
using boost::property_tree::ptree;
using boost::property_tree::write_xml;
using boost::property_tree::xml_writer_settings;
using boost::property_tree::xml_writer_make_settings;




namespace insur {

  void tk2CMSSW::setTrackerDependantStrings(bool isPixelTracker) {

    td_.isPixelTracker = isPixelTracker;

    if (!td_.isPixelTracker) {
      if (wt) td_.nspace = xml_newfileident;
      else td_.nspace = xml_fileident;

      td_.trackerfile = xml_OT_trackerfile;
      td_.topologyfile = xml_topologyfile;
      td_.prodcutsfile = xml_prodcutsfile;
      td_.trackersensfile = xml_trackersensfile;
      td_.recomatfile = xml_recomatfile;

      td_.bar = xml_OT_bar;
      td_.spec_bar = xml_OT_bar;
      td_.fwd = xml_OT_fwd;
      td_.value_bar = xml_OT_bar;
      td_.tracker = xml_OT;
      td_.value_layer = xml_OT_value_layer;
      td_.barrel_prefix = xml_tob_prefix;
      td_.endcaps_prefix = xml_tid_prefix;
    }

    else {
      td_.nspace = xml_PX_fileident;

      td_.trackerfile = xml_PX_trackerfile;
      td_.topologyfile = xml_PX_topologyfile;
      td_.prodcutsfile = xml_PX_prodcutsfile;
      td_.trackersensfile = xml_PX_trackersensfile;
      td_.recomatfile = xml_PX_recomatfile;

      td_.bar = xml_PX_bar;
      td_.spec_bar = xml_PX_bar;
      td_.fwd = xml_PX_fwd;  
      td_.value_bar = xml_PX_value_bar;
      td_.tracker = "";
      td_.value_layer = xml_PX_value_layer;
      td_.barrel_prefix = xml_PX_barrel_prefix;
      td_.endcaps_prefix = xml_PX_endcaps_prefix;
    }
  }

    // public
    /**
     * This is the main translation function if a tkgeometry tracker model needs to be represented in an XML form that CMSSW
     * can understand. It splits the work into three main parts: filesystem gymnastics related to input and output, analysis of the
     * tkgeometry model, and generation of XML output. Analysis and output generation are delegated to internal instances of
     * <i>Extractor</i> and <i>XMLWriter</i>, respectively. This function deals mainly with the file system and makes sure
     * that the generated output files go where they are supposed to go. The user may specify the name of a subdirectory for the
     * output files. If that directory exists, all of its contents will be overwritten by the new files. If no name is given, a default is
     * used instead.
     * @mt A refernce to the global material table
     * @mb A reference to an existing material budget that serves as input to the translation
     * @xmlOutDirectoryName A string with the name of a subfolder for the output; empty by default.
     */
  void tk2CMSSW::translate(MaterialTable& mt, MaterialBudget& mb, std::string xmlGeneralPath, std::string xmlOutDirectoryPath, std::string xmlOutDirectoryName, bool wt) {

        // analyse tracker system and build up collection of elements, composites, hierarchy, shapes, positions, algorithms and topology
        // ex is an instance of Extractor class
	ex.setTrackerSpecificStrings(td_);
        ex.analyse(mt, mb, data, td_.isPixelTracker, wt);

        std::stringstream simpleHeaderStream;
	std::string metdataFileName = xmlOutDirectoryName + "_" + currentDateTime(false) + ".cfg";
        writeSimpleHeader(simpleHeaderStream, metdataFileName);
	wr.setTrackerSpecificStrings(td_);
        wr.setSimpleHeader(simpleHeaderStream.str());
        
        // translate collected information to XML and write to buffers
        std::ifstream instream;
        std::ofstream outstream;

	    if (!td_.isPixelTracker) {
	      outstream.open((xmlOutDirectoryPath + metdataFileName).c_str());
	      if (outstream.fail()) throw std::runtime_error("Error opening XML generation metadata file for writing.");
	      writeMetadata(outstream);
	      if (outstream.fail()) throw std::runtime_error("Error writing to XML generation metadata file.");
	      outstream.close();
	      outstream.clear();
	      std::cout << "CMSSW XML generation metadata has been written to " << xmlOutDirectoryPath << metdataFileName << std::endl;

	      if (!wt) {
		instream.open((xmlGeneralPath + "/" + xml_pixbarfile).c_str());
		outstream.open((xmlOutDirectoryPath + xml_pixbarfile).c_str());
		if (instream.fail() || outstream.fail()) throw std::runtime_error("Error opening one of the pixbar files.");
		//writeSimpleHeader(outstream);
		wr.pixbar(data.shapes, instream, outstream);
		if (outstream.fail()) throw std::runtime_error("Error writing to pixbar file.");
		instream.close();
		instream.clear();
		outstream.close();
		outstream.clear();
		std::cout << "CMSSW modified pixel barrel has been written to " << xmlOutDirectoryPath << xml_pixbarfile << std::endl;

		instream.open((xmlGeneralPath + "/" + xml_pixfwdfile).c_str());
		outstream.open((xmlOutDirectoryPath + xml_pixfwdfile).c_str());
		if (instream.fail() || outstream.fail()) throw std::runtime_error("Error opening one of the pixfwdn files.");
		//writeSimpleHeader(outstream);
		wr.pixfwd(data.shapes, instream, outstream);
		if (outstream.fail()) throw std::runtime_error("Error writing to pixfwd file.");
		instream.close();
		instream.clear();
		outstream.close();
		outstream.clear();
		std::cout << "CMSSW modified pixel endcap has been written to " << xmlOutDirectoryPath << xml_pixfwdfile << std::endl;
	      }
	    }

	    if (wt) outstream.open((xmlOutDirectoryPath + xml_newtrackerfile).c_str());
	    else outstream.open((xmlOutDirectoryPath + td_.trackerfile).c_str());
            if (outstream.fail()) throw std::runtime_error("Error opening tracker file for writing.");
            std::ifstream trackerVolumeTemplate((xmlGeneralPath + "/" + xml_trackervolumefile).c_str());
            wr.tracker(data, outstream, trackerVolumeTemplate, wt);
            if (outstream.fail()) throw std::runtime_error("Error writing to tracker file.");
            outstream.close();
            outstream.clear();
            std::cout << "CMSSW tracker geometry output has been written to " << xmlOutDirectoryPath << (wt ? xml_newtrackerfile : td_.trackerfile) << std::endl;

	    if (wt) instream.open((xmlGeneralPath + "/" + xml_newtopologyfile).c_str());
	    else instream.open((xmlGeneralPath + "/" + xml_topologyfile).c_str());
	    outstream.open((xmlOutDirectoryPath + td_.topologyfile).c_str());
            if (instream.fail() || outstream.fail()) throw std::runtime_error("Error opening one of the topology files.");
            //writeSimpleHeader(outstream);
            wr.topology(data.specs, instream, outstream);
            if (outstream.fail()) throw std::runtime_error("Error writing to topology file.");
            instream.close();
            instream.clear();
            outstream.close();
            outstream.clear();
	    std::cout << "CMSSW topology output has been written to " << xmlOutDirectoryPath << td_.topologyfile << std::endl;

	    instream.open((xmlGeneralPath + "/" + td_.prodcutsfile).c_str());
	    outstream.open((xmlOutDirectoryPath + td_.prodcutsfile).c_str());
            if (instream.fail() || outstream.fail()) throw std::runtime_error("Error opening one of the prodcuts files.");
            //writeSimpleHeader(outstream);
            wr.prodcuts(data.specs, instream, outstream);
            if (outstream.fail()) throw std::runtime_error("Error writing to prodcuts file.");
            instream.close();
            instream.clear();
            outstream.close();
            outstream.clear();
            std::cout << "CMSSW prodcuts output has been written to " << xmlOutDirectoryPath << td_.prodcutsfile << std::endl;

	    instream.open((xmlGeneralPath + "/" + td_.trackersensfile).c_str());
	    outstream.open((xmlOutDirectoryPath + td_.trackersensfile).c_str());
            if (instream.fail() || outstream.fail()) throw std::runtime_error("Error opening one of the trackersens files.");
            //writeSimpleHeader(outstream);
            wr.trackersens(data.specs, instream, outstream);
            if (outstream.fail()) throw std::runtime_error("Error writing trackersens to file.");
            instream.close();
            instream.clear();
            outstream.close();
            outstream.clear();
            std::cout << "CMSSW sensor surface output has been written to " << xmlOutDirectoryPath << td_.trackersensfile << std::endl;

	    if (wt) instream.open((xmlGeneralPath + "/" + xml_newrecomatfile).c_str());
	    else instream.open((xmlGeneralPath + "/" + xml_recomatfile).c_str());
	    outstream.open((xmlOutDirectoryPath + td_.recomatfile).c_str());
            if (instream.fail() || outstream.fail()) throw std::runtime_error("Error opening one of the recomaterial files.");
            //writeSimpleHeader(outstream);
            wr.recomaterial(data.specs, data.lrilength, instream, outstream, wt);
            if (outstream.fail()) throw std::runtime_error("Error writing recomaterial to file.");
            instream.close();
            instream.clear();
            outstream.close();
            outstream.clear();
            std::cout << "CMSSW reco material output has been written to " << xmlOutDirectoryPath << td_.recomatfile << std::endl;
    }
    
    // private
    /**
     * This prints the contents of the internal CMSSWBundle collection; used for debugging.
     */
    void tk2CMSSW::print() {
        std::cout << "tm2CMSSW internal status:" << std::endl;
        std::cout << "elements: " << data.elements.size() << " entries." << std::endl;
        for (unsigned int i = 0; i < data.elements.size(); i++) {
            std::cout << "entry " << i << ": tag = " << data.elements.at(i).tag << ", density = " << data.elements.at(i).density << ", atomic number = ";
            std::cout << data.elements.at(i).atomic_number << ", atomic weight = " << data.elements.at(i).atomic_weight << std::endl;
        }
        std::cout << "composites: " << data.composites.size() << " entries." << std::endl;
        for (unsigned int i = 0; i < data.composites.size(); i++) {
            std::cout << "entry " << i << ": name = " << data.composites.at(i).name << ", density = " << data.composites.at(i).density << ", method = ";
            switch (data.composites.at(i).method) {
                case wt: std::cout << "fraction by weight";
                break;
                case vl: std::cout << "fraction by volume";
                break;
                case ap: std::cout << "fraction by atomic proportion";
                break;
                default: std::cout << "unknown method";
            }
            std::cout << std::endl << "elements: ";
            std::vector<std::pair<std::string, double> >& elems = data.composites.at(i).elements;
            for (unsigned int j = 0; j < elems.size(); j++) std::cout << "(" << elems.at(j).first << ", " << elems.at(j).second << ") ";
            std::cout << std::endl;
        }
        std::cout << "rotations: " << data.rots.size() << " entries." << std::endl;
        for (auto const &it : data.rots) {
            std::cout << "name = " << it.second.name << ", thetax = " << it.second.thetax << ", phix = ";
            std::cout << it.second.phix << ", thetay = " << it.second.thetay << ", phiy = " << it.second.phiy;
            std::cout << ", thetaz = " << it.second.thetaz << ", phiz = " << it.second.phiz << std::endl;
        }
        std::cout << "logic: " << data.logic.size() << " entries." << std::endl;
        for (unsigned int i = 0; i < data.logic.size(); i++) {
            std::cout << "name_tag = " << data.logic.at(i).name_tag << ", shape_tag = " << data.logic.at(i).shape_tag;
            std::cout << ", material_tag = " << data.logic.at(i).material_tag << std::endl;
        }
        std::cout << "shapes: " << data.shapes.size() << " entries." << std::endl;
        for (unsigned int i = 0; i < data.shapes.size(); i++) {
            std::cout << "name_tag = " << data.shapes.at(i).name_tag << ", type = ";
            switch (data.shapes.at(i).type) {
                case bx: std::cout << "box, dx = " << data.shapes.at(i).dx << ", dy = " << data.shapes.at(i).dy << ", dz = ";
                std::cout << data.shapes.at(i).dz;
                break;
                case tb: std::cout << "tube, rmin = " << data.shapes.at(i).rmin << ", rmax = " << data.shapes.at(i).rmax;
		std::cout << ", dz = " << data.shapes.at(i).dz;
		break;
	        case co: std::cout << "cone, rmin1 = " << data.shapes.at(i).rmin1 << ", rmax1 = " << data.shapes.at(i).rmax1;
	        std::cout << ", rmin2 = " << data.shapes.at(i).rmin2 << ", rmax2 = " << data.shapes.at(i).rmax2;
	        std::cout << ", dz = " << data.shapes.at(i).dz;
	        break;
                case tp: std::cout << "trapezoid, dx = " << data.shapes.at(i).dx << ", dy = " << data.shapes.at(i).dy;
                std::cout << ", dyy = " << data.shapes.at(i).dyy << ", dz = " << data.shapes.at(i).dz;
                break;
                default: std::cout << "unknown shape";
            }
            std::cout << std::endl;
        }
        std::cout << "operations on shapes: " << data.shapeOps.size() << " entries." << std::endl;
        for (unsigned int i = 0; i < data.shapeOps.size(); i++) {
            std::cout << "name_tag = " << data.shapeOps.at(i).name_tag << ", type = ";
            switch (data.shapeOps.at(i).type) {
	        case uni: std::cout << "union, rSolid1 = " << data.shapeOps.at(i).rSolid1 << ", rSolid2 = " << data.shapeOps.at(i).rSolid2;
	        break;
	        case intersec: std::cout << "intersection, rSolid1 = " << data.shapeOps.at(i).rSolid1 << ", rSolid2 = " << data.shapeOps.at(i).rSolid2;
	        break;
	        default: std::cout << "unknown operation";
            }
            std::cout << std::endl;
        }
        std::cout << "positions: " << data.positions.size() << " entries." << std::endl;
        for (unsigned int i = 0; i < data.positions.size(); i++) {
            std::cout << "parent_tag = " << data.positions.at(i).parent_tag << ", child_tag = " << data.positions.at(i).child_tag;
            std::cout << ", rotref = " << (data.positions.at(i).rotref.empty() ? "[no name]": data.positions.at(i).rotref) << ", ";
            std::cout << ", translation = (" << data.positions.at(i).trans.dx << ", " << data.positions.at(i).trans.dy << ", ";
            std::cout << data.positions.at(i).trans.dz << ")" << std::endl;
        }
        std::cout << "algorithms: " << data.algos.size() << " entries." << std::endl;
        for (unsigned int i = 0; i < data.algos.size(); i++ ) {
            std::cout << "name = " << data.algos.at(i).name << ", parent = " << data.algos.at(i).parent << std::endl;
            std::cout << "parameters:" << std::endl;
            for (unsigned int j = 0; j < data.algos.at(i).parameters.size(); j++) std::cout << data.algos.at(i).parameters.at(j) << std::endl;
        }
        SpecParInfo spec;
        std::cout << "topology: " << data.specs.size() << " entries." << std::endl;
        for (unsigned int i = 0; i < data.specs.size(); i++) {
            std::cout << "name = " << data.specs.at(i).name << std::endl << "partselectors:" << std::endl;
            for (unsigned int j = 0; j < data.specs.at(i).partselectors.size(); j++) std::cout << data.specs.at(i).partselectors.at(j) << std::endl;
            std::cout << "parameter = (" << data.specs.at(i).parameter.first << ", " << data.specs.at(i).parameter.second << ")" << std::endl;
        }
    }



  void tk2CMSSW::writeSimpleHeader(std::ostream& os, std::string& metdataFileName) {
      os << "<!--" << std::endl;
      os << "============= XML GENERATION METADATA HEADER =============" << std::endl;
      os << "tkLayout revision: " << SvnRevision::revisionNumber << std::endl;
      os << "generated by: " << fullUserName() << std::endl;
      os << "generation date: " << currentDateTime(true) << std::endl;
      os << "note: see " << metdataFileName << " for full config files" << std::endl;
      os << "=========== END XML GENERATION METADATA HEADER ===========" << std::endl;
      os << "-->" << std::endl;
    }

  void tk2CMSSW::writeMetadata(std::ofstream& out) {
    out << "============= XML GENERATION METADATA =============" << std::endl;
    out << "tkLayout revision: " << SvnRevision::revisionNumber << std::endl;
    out << "generated by: " << fullUserName() << std::endl;
    out << "generation date: " << currentDateTime(true) << std::endl;
    for (std::vector<ConfigFile>::const_iterator it = getConfigFiles().begin(); it != getConfigFiles().end(); ++it) {
      out << std::endl << std::endl;
      out << "CONFIG FILE: " << it->name << std::endl;
      out << it->content << std::endl; 
    }
    out << "=========== END XML GENERATION METADATA ===========" << std::endl;
  }

    std::string tk2CMSSW::currentDateTime(bool withTime) const {
      time_t     now = time(0);
      struct tm  tstruct;
      char       buf[80];
      tstruct = *localtime(&now);
      if (withTime) strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
      else strftime(buf, sizeof(buf), "%Y_%m_%d", &tstruct);
      return buf;
    }

    std::string tk2CMSSW::fullUserName() const {
      struct passwd* pwd = getpwuid(getuid());
      char hn[256];
      hn[255] = 0;
      gethostname(hn, 255); // if hostname is too long it gets truncated and the string might or might not contain a terminating null byte, therefore we force the last char to be null by construction
      return std::string(pwd->pw_gecos) + " (" + pwd->pw_name + " on " + hn + ")";
    }







  void tk2CMSSW::printXml(CMSSWBundle pixelData, std::string outsubdir) {
   
    std::string xmlpath = mainConfiguration.getXmlDirectory() + "/" + outsubdir + "/";
    std::cout<< "Xmls to be produced here=" << xmlpath<< std::endl;

    std::vector<ShapeInfo>& shapes = pixelData.shapes;
    std::vector<LogicalInfo>& logic = pixelData.logic;
    std::vector<PosInfo>& positions = pixelData.positions;
    std::vector<AlgoInfo>& algos = pixelData.algos;
    std::vector<Composite>& composites = pixelData.composites;
    std::vector<Element>& elements = pixelData.elements;

    ptree tree;
    tree.add("DDDefinition.<xmlattr>.xmlns", "http://www.cern.ch/cms/DDL");
    tree.add("DDDefinition.<xmlattr>.xmlns:xsi", "http://www.cern.ch/www.w3.org/2001/XMLSchema-instance");
    tree.add("DDDefinition.<xmlattr>.xsi:schemaLocation",
        "http://www.cern.ch/cms/DDL ../../../DetectorDescription/Schema/DDLSchema.xsd");

    ptree& matSec = tree.add("DDDefinition.MaterialSection", "");
    matSec.add("<xmlattr>.label", "pixel.xml");

    for( auto& e: elements ) {
      ptree& elem = matSec.add("ElementaryMaterial","");
      elem.add("<xmlattr>.name",e.tag);
      elem.add("<xmlattr>.symbol",e.tag);
      elem.add("<xmlattr>.atomicNumber",e.atomic_number);
      std::stringstream ss;
      ss << e.atomic_weight << "*g/mole";
      elem.add("<xmlattr>.atomicWeight",ss.str());
      ss.str("");
      ss << e.density << "*g/cm3";
      elem.add("<xmlattr>.density",ss.str());
    }

    for( auto& c: composites ) {
      ptree& comp = matSec.add("CompositeMaterial", "");
      comp.add("<xmlattr>.name",c.name);
      std::stringstream ss;
      ss << std::setprecision(3) << c.density << "*g/cm3";
      comp.add("<xmlattr>.density",ss.str());
      ss.str("");
      comp.add("<xmlattr>.method","mixture by weight");

          for( auto& e: c.elements ) {
          ptree& elem = comp.add("MaterialFraction", "");
          elem.add("<xmlattr>.fraction",e.second);
          ptree& m = elem.add("rMaterial", "");
          m.add("<xmlattr>.name", xml_phaseII_Pixelnamespace + e.first);
    
          }
    }

    ptree& solidSec = tree.add("DDDefinition.SolidSection", "");
    solidSec.add("<xmlattr>.label", "pixel.xml");
    /*//Defining the pixel mother volume
    ptree& mSolid = solidSec.add("UnionSolid", "");
    mSolid.add("<xmlattr>.name",xml_phaseII_pixmotherVolume);
    mSolid.add("<xmlattr>.firstSolid",xml_pixfwdident + ":" + xml_phaseII_pixecap);
    mSolid.add("<xmlattr>.secondSolid",xml_pixbarident + ":" + xml_phaseII_pixbar);
    ptree& mTranslation = mSolid.add("Translation","");
    mTranslation.add("<xmlattr>.x","0*cm");
    mTranslation.add("<xmlattr>.y","0*cm");
    mTranslation.add("<xmlattr>.z","0*cm");*/
    
    for( auto& s: shapes) {
        if( s.type == ShapeType::bx ) {
        ptree& solid = solidSec.add("Box", "");
        solid.add("<xmlattr>.name",s.name_tag);
        std::stringstream ss;
        ss << std::setprecision(3) << s.dx << "*mm";
        solid.add("<xmlattr>.dx",ss.str());
        ss.str("");
        ss << std::setprecision(3) << s.dy << "*mm";
        solid.add("<xmlattr>.dy",ss.str());
        ss.str("");
        ss << std::setprecision(3) << s.dz << "*mm";
        solid.add("<xmlattr>.dz",ss.str());
      } else  if( s.type == ShapeType::tb ) {
        ptree& solid = solidSec.add("Tubs", "");
        solid.add("<xmlattr>.name",s.name_tag);
        std::stringstream ss;
        ss << std::setprecision(4) << s.rmin << "*mm";
        solid.add("<xmlattr>.rMin",ss.str());
        ss.str("");
        ss << std::setprecision(4) << s.rmax << "*mm";
        solid.add("<xmlattr>.rMax",ss.str());
        ss.str("");
        ss << std::setprecision(4) << s.dz << "*mm";
        solid.add("<xmlattr>.dz",ss.str());
        solid.add("<xmlattr>.startPhi","0*deg");
        solid.add("<xmlattr>.deltaPhi","360*deg");
      }
    }

    ptree& rotSec = tree.add("DDDefinition.RotationSection", "");
    rotSec.add("<xmlattr>.label", "pixel.xml");
    
    ptree& rotation1 = rotSec.add("Rotation","");
    rotation1.add("<xmlattr>.name","HCZ2YX");
    rotation1.add("<xmlattr>.thetaX","90*deg");
    rotation1.add("<xmlattr>.phiX","270*deg");
    rotation1.add("<xmlattr>.thetaY","180*deg");
    rotation1.add("<xmlattr>.phiY","0*deg");
    rotation1.add("<xmlattr>.thetaZ","90*deg");
    rotation1.add("<xmlattr>.phiZ","0*deg");
 
    ptree& rotation2 = rotSec.add("Rotation","");
    rotation2.add("<xmlattr>.name","FlippedHCZ2YX");
    rotation2.add("<xmlattr>.thetaX","90*deg");
    rotation2.add("<xmlattr>.phiX","270*deg");
    rotation2.add("<xmlattr>.thetaY","0*deg");
    rotation2.add("<xmlattr>.phiY","0*deg");
    rotation2.add("<xmlattr>.thetaZ","90*deg");
    rotation2.add("<xmlattr>.phiZ","180*deg");

    ptree& rotation3 = rotSec.add("Rotation","");
    rotation3.add("<xmlattr>.name","FLIP");
    rotation3.add("<xmlattr>.thetaX","90*deg");
    rotation3.add("<xmlattr>.phiX","180*deg");
    rotation3.add("<xmlattr>.thetaY","90*deg");
    rotation3.add("<xmlattr>.phiY","90*deg");
    rotation3.add("<xmlattr>.thetaZ","180*deg");
    rotation3.add("<xmlattr>.phiZ","0*deg");
      
    ptree& logicSec = tree.add("DDDefinition.LogicalPartSection", "");
    logicSec.add("<xmlattr>.label", "pixel.xml");
    
    /*ptree& mLogic = logicSec.add("LogicalPart", "");
    mLogic.add("<xmlattr>.name", xml_phaseII_pixmotherVolume);
    mLogic.add("<xmlattr>.category", "unspecified");
    ptree& mRsolid = mLogic.add("rSolid","");
    mRsolid.add("<xmlattr>.name", xml_phaseII_Pixelnamespace + xml_phaseII_pixmotherVolume);
    ptree& mRmat = mLogic.add("rMaterial","");
    mRmat.add("<xmlattr>.name","materials:Air");*/

    for( auto& l: logic) {
      ptree& logical = logicSec.add("LogicalPart","");
      logical.add("<xmlattr>.name",l.name_tag);
      logical.add("<xmlattr>.category", "unspecified");

      ptree& rsolid = logical.add("rSolid","");
      rsolid.add("<xmlattr>.name",l.shape_tag); 

      ptree& rmat = logical.add("rMaterial","");
      rmat.add("<xmlattr>.name",l.material_tag); 

    }

    ptree& posSec = tree.add("DDDefinition.PosPartSection", "");
    posSec.add("<xmlattr>.label", "pixel.xml");
    
    /*ptree& mPosition = posSec.add("PosPart","");
    mPosition.add("<xmlattr>.copyNumber","1");
      
    ptree& mParent = mPosition.add("rParent","");
    mParent.add("<xmlattr>.name","tracker:Tracker");
      
    ptree& mChild = mPosition.add("rChild","");
    mChild.add("<xmlattr>.name",xml_phaseII_Pixelnamespace + xml_phaseII_pixmotherVolume);*/

    for( auto& p: positions) {

      ptree& position = posSec.add("PosPart","");
      position.add("<xmlattr>.copyNumber",p.copy);

      ptree& parent = position.add("rParent","");
      parent.add("<xmlattr>.name",p.parent_tag);

      ptree& child = position.add("rChild","");
      child.add("<xmlattr>.name",p.child_tag);

      if( p.rotref != "" ) {
        ptree& rot = position.add("rRotation","");
        rot.add("<xmlattr>.name",p.rotref);
      }

      if( p.trans.dx != 0 || p.trans.dy != 0 || p.trans.dz != 0) {
        ptree& translation = position.add("Translation","");
        std::stringstream ss;
        ss << std::setprecision(3) << p.trans.dx << "*mm";
        translation.add("<xmlattr>.x",ss.str());
        ss.str("");
        ss << std::setprecision(3) << p.trans.dy << "*mm";
        translation.add("<xmlattr>.y",ss.str());
        ss.str("");
        ss << std::setprecision(3) << p.trans.dz << "*mm";
        translation.add("<xmlattr>.z",ss.str());
      }
    }

    for( auto& a: algos ) {
      ptree& algo = posSec.add("Algorithm","");
      algo.add("<xmlattr>.name",a.name);

      ptree& parent = algo.add("rParent","");
      parent.add("<xmlattr>.name",a.parent);
      if( !a.parameter_map.empty() ) {
        for( auto& p: a.parameter_map ) { 
          std::string ptype; 
          if( p.second.second == AlgoPartype::st ) 
            ptype = "String";
          else if( p.second.second == AlgoPartype::num )
            ptype = "Numeric";
          ptree& algoPar = algo.add(ptype,"");
          algoPar.add("<xmlattr>.name",p.first);
          algoPar.add("<xmlattr>.value",p.second.first);
        } 
      }
      if( a.vecpar.name != "" ) {
        ptree& algovPar = algo.add("Vector","");
        algovPar.add("<xmlattr>.name",a.vecpar.name);
        algovPar.add("<xmlattr>.type",a.vecpar.type);
        algovPar.add("<xmlattr>.nEntries",a.vecpar.nEntries);
        std::stringstream ss;
        for( unsigned int i = 0; i<a.vecpar.values.size() - 1; i++ )
          ss << a.vecpar.values[i] << ",";
        ss << a.vecpar.values[a.vecpar.values.size() - 1];
        algovPar.add("<xmltext>",ss.str());

      }
    }

#if BOOST_VERSION >= 105600
    xml_writer_settings<std::string> settings(' ', 1);
#else
    xml_writer_settings<char> settings(' ', 1);
#endif
    write_xml(xmlpath+"pixel_test.xml", tree, std::locale(), settings);

 
    ///////////////writing pixel structure Topology////////////////////
    std::vector<SpecParInfo>& specs = pixelData.specs;
    ptree tree_topo;
    tree_topo.add("DDDefinition.<xmlattr>.xmlns", "http://www.cern.ch/cms/DDL");
    tree_topo.add("DDDefinition.<xmlattr>.xmlns:xsi", "http://www.cern.ch/www.w3.org/2001/XMLSchema-instance");
    tree_topo.add("DDDefinition.<xmlattr>.xsi:schemaLocation",
        "http://www.cern.ch/cms/DDL ../../../DetectorDescription/Schema/DDLSchema.xsd");
    ptree& specParSec = tree_topo.add("DDDefinition.SpecParSection", "");
    specParSec.add("<xmlattr>.label", xml_specpars_label);//xml_specpars_label=spec-p```ars2.xml

    ptree& spec = specParSec.add("SpecPar","");
    spec.add("<xmlattr>.name","FullTrackerPar");

    ptree& partSel = spec.add("PartSelector","");
    partSel.add("<xmlattr>.path","//Tracker");

    ptree& param = spec.add("Parameter","");
    param.add("<xmlattr>.name","TkDDDStructure");
    param.add("<xmlattr>.value","FullTracker");

    for( auto& s: specs ) {
      if( s.name.find("Module") != std::string::npos )      continue;
      // if( s.name.find("ROUHits") != std::string::npos)      continue;
      ptree& spec = specParSec.add("SpecPar","");
      spec.add("<xmlattr>.name",s.name);  
      for( auto& p: s.partselectors ) {
        ptree& partSel = spec.add("PartSelector","");
        partSel.add("<xmlattr>.path","//"+ p);
      }
      ptree& param = spec.add("Parameter","");
      param.add("<xmlattr>.name",s.parameter.first);
      param.add("<xmlattr>.value",s.parameter.second);
    }

    std::vector<std::string> barrel_partselectors;
    std::vector<std::string> endcap_partselectors;

    for( auto& s: specs ) {
      if( s.name.find("Module") == std::string::npos )                 continue;
      else if( s.name.find("BModule") != std::string::npos )      
        barrel_partselectors.push_back(xml_phaseII_Pixelnamespace + s.name);
      else if( s.name.find("EModule") != std::string::npos )      
        endcap_partselectors.push_back(xml_phaseII_Pixelnamespace + s.name);
      ptree& spec = specParSec.add("SpecPar","");
      spec.add("<xmlattr>.name",s.name);  
      for( auto& p: s.partselectors ) {
        ptree& partSel = spec.add("PartSelector","");
        partSel.add("<xmlattr>.path","//"+ p);
      }
      ptree& param = spec.add("Parameter","");
      param.add("<xmlattr>.name",s.parameter.first);
      param.add("<xmlattr>.value",s.parameter.second);
      for(auto m: s.moduletypes){
        ptree& param1 = spec.add("Parameter","");
        param1.add("<xmlattr>.name", "PixelROCRows");
        param1.add("<xmlattr>.value", m.rocrows);
        ptree& param2 = spec.add("Parameter","");
        param2.add("<xmlattr>.name", "PixelROCCols");
        param2.add("<xmlattr>.value", m.roccols);
        ptree& param3 = spec.add("Parameter","");
        param3.add("<xmlattr>.name", "PixelROC_X");
        param3.add("<xmlattr>.value", m.rocx);
        ptree& param4 = spec.add("Parameter","");
        param4.add("<xmlattr>.name", "PixelROC_Y");
        param4.add("<xmlattr>.value", m.rocy);
      } 
    }
    write_xml(xmlpath+"pixelStructureTopology_test.xml", tree_topo, std::locale(), settings);

    //sensor portion
    ptree tree_sense;
    tree_sense.add("DDDefinition.<xmlattr>.xmlns", "http://www.cern.ch/cms/DDL");
    tree_sense.add("DDDefinition.<xmlattr>.xmlns:xsi", "http://www.cern.ch/www.w3.org/2001/XMLSchema-instance");
    tree_sense.add("DDDefinition.<xmlattr>.xsi:schemaLocation",
        "http://www.cern.ch/cms/DDL ../../../DetectorDescription/Schema/DDLSchema.xsd");
    ptree& specParSensorSec = tree_sense.add("DDDefinition.SpecParSection", "");
    specParSensorSec.add("<xmlattr>.label", xml_specpars_label);//xml_specpars_label=spec-pars2.xml
    //Barrel Part 
    ptree& barrel_specSensor = specParSensorSec.add("SpecPar","");
    barrel_specSensor.add("<xmlattr>.name","ROUHitsTracker" + xml_phaseII_pixbar);  
    for( auto& p: barrel_partselectors ) {
      ptree& partSel = barrel_specSensor.add("PartSelector","");
      p.erase(p.end()-3,p.end());
      partSel.add("<xmlattr>.path","//"+ p);
    }
    ptree& barrel_param1 = barrel_specSensor.add("Parameter","");
    barrel_param1.add("<xmlattr>.name","SensitiveDetector");
    barrel_param1.add("<xmlattr>.value","TkAccumulatingSensitiveDetector");

    ptree& barrel_param2 = barrel_specSensor.add("Parameter","");
    barrel_param2.add("<xmlattr>.name","ReadOutName");
    //barrel_param2.add("<xmlattr>.value","TrackerHits" + xml_phaseII_pixbar);
    barrel_param2.add("<xmlattr>.value","TrackerHits" + xml_pixbar);//to be consistent with OscarProducer
   
    //Endcap Part 
    ptree& endcap_specSensor = specParSensorSec.add("SpecPar","");
    endcap_specSensor.add("<xmlattr>.name","ROUHitsTracker" + xml_phaseII_pixecap);  
    for( auto& p: endcap_partselectors ) {
      ptree& partSel = endcap_specSensor.add("PartSelector","");
      p.erase(p.end()-3,p.end());
      partSel.add("<xmlattr>.path","//"+ p);
    }
    ptree& endcap_param1 = endcap_specSensor.add("Parameter","");
    endcap_param1.add("<xmlattr>.name","SensitiveDetector");
    endcap_param1.add("<xmlattr>.value","TkAccumulatingSensitiveDetector");

    ptree& endcap_param2 = endcap_specSensor.add("Parameter","");
    endcap_param2.add("<xmlattr>.name","ReadOutName");
    endcap_param2.add("<xmlattr>.value","TrackerHitsPixelEndcap");////to be consistent with OscarProducer

    write_xml(xmlpath+"pixelsens_test.xml", tree_sense, std::locale(), settings);
    
    //Prodcut portion
    ptree tree_prodCut;
    tree_prodCut.add("DDDefinition.<xmlattr>.xmlns", "http://www.cern.ch/cms/DDL");
    tree_prodCut.add("DDDefinition.<xmlattr>.xmlns:xsi", "http://www.cern.ch/www.w3.org/2001/XMLSchema-instance");
    tree_prodCut.add("DDDefinition.<xmlattr>.xsi:schemaLocation",
        "http://www.cern.ch/cms/DDL ../../../DetectorDescription/Schema/DDLSchema.xsd");
    ptree& specParProdSec = tree_prodCut.add("DDDefinition.SpecParSection", "");
    specParProdSec.add("<xmlattr>.label", "trackerProdCuts.xml");//xml_specpars_label=spec-pars2.xml
    specParProdSec.add("<xmlattr>.eval","true");

    ptree& dead_spec = specParProdSec.add("SpecPar","");
    dead_spec.add("<xmlattr>.name","tracker-dead-pixel");

    ptree& barrel_partSel = dead_spec.add("PartSelector","");
    barrel_partSel.add("<xmlattr>.path","//pixbar:" + xml_phaseII_pixbar );

    ptree& endcap_partSel = dead_spec.add("PartSelector","");
    endcap_partSel.add("<xmlattr>.path","//pixfwd:" + xml_phaseII_pixecap);

    ptree& dead_param1 = dead_spec.add("Parameter","");
    dead_param1.add("<xmlattr>.name","CMSCutsRegion");
    dead_param1.add("<xmlattr>.value","TrackerPixelDeadRegion");
    dead_param1.add("<xmlattr>.eval","false");

    ptree& dead_param2 = dead_spec.add("Parameter","");
    dead_param2.add("<xmlattr>.name","ProdCutsForElectrons");
    dead_param2.add("<xmlattr>.value","1*mm");

    ptree& dead_param3 = dead_spec.add("Parameter","");
    dead_param3.add("<xmlattr>.name","ProdCutsForPositrons");
    dead_param3.add("<xmlattr>.value","1*mm");

    ptree& dead_param4 = dead_spec.add("Parameter","");
    dead_param4.add("<xmlattr>.name","ProdCutsForGamma");
    dead_param4.add("<xmlattr>.value","1*mm");


    ptree& sens_spec = specParProdSec.add("SpecPar","");
    sens_spec.add("<xmlattr>.name","tracker-sens-pixel");
    for( auto& p: barrel_partselectors ) {
      ptree& partSel = sens_spec.add("PartSelector","");
      partSel.add("<xmlattr>.path","//"+ p);
    }
    for( auto& p: endcap_partselectors ) {
      ptree& partSel = sens_spec.add("PartSelector","");
      partSel.add("<xmlattr>.path","//"+ p);
    }
    ptree& sens_param1 = sens_spec.add("Parameter","");
    sens_param1.add("<xmlattr>.name","CMSCutsRegion");
    sens_param1.add("<xmlattr>.value","TrackerPixelSensRegion");
    sens_param1.add("<xmlattr>.eval","false");

    ptree& sens_param2 = sens_spec.add("Parameter","");
    sens_param2.add("<xmlattr>.name","ProdCutsForElectrons");
    sens_param2.add("<xmlattr>.value","0.01*mm");

    ptree& sens_param3 = sens_spec.add("Parameter","");
    sens_param3.add("<xmlattr>.name","ProdCutsForPositrons");
    sens_param3.add("<xmlattr>.value","0.01*mm");

    ptree& sens_param4 = sens_spec.add("Parameter","");
    sens_param4.add("<xmlattr>.name","ProdCutsForGamma");
    sens_param4.add("<xmlattr>.value","0.01*mm");

    write_xml(xmlpath+"pixelProdCuts_test.xml", tree_prodCut, std::locale(), settings);
 
   //Reco Material
    ptree tree_recoMat;
    tree_recoMat.add("DDDefinition.<xmlattr>.xmlns", "http://www.cern.ch/cms/DDL");
    tree_recoMat.add("DDDefinition.<xmlattr>.xmlns:xsi", "http://www.cern.ch/www.w3.org/2001/XMLSchema-instance");
    tree_recoMat.add("DDDefinition.<xmlattr>.xsi:schemaLocation",
        "http://www.cern.ch/cms/DDL ../../../DetectorDescription/Schema/DDLSchema.xsd");
    ptree& specParRecoSec = tree_recoMat.add("DDDefinition.SpecParSection", "");
    specParRecoSec.add("<xmlattr>.label", "spec-pars2.xml");//xml_specpars_label=spec-pars2.xml
   
      std::string pixbarRecoSpeccommon = "TrackerRecMaterial" + xml_phaseII_pixbar + xml_layer;
      for( unsigned int l = 1; l<=3; l++){
          std::stringstream stemp;
          stemp << pixbarRecoSpeccommon << l;
          ptree& specReco = specParRecoSec.add("SpecPar","");
          specReco.add("<xmlattr>.name",stemp.str());
          specReco.add("<xmlattr>.eval","true");
	  /* for(auto& e:barrelRmatpath) {
            std::stringstream sl;
              sl << "Layer" << l;
              if( e.find(sl.str()) != std::string::npos ){
                 ptree& part = specReco.add("PartSelector","");
                 part.add("<xmlattr>.path",e);
              }
	      }*/
          for (const auto &rilength : pixelData.lrilength) {
            if (!rilength.barrel || rilength.index != l)
              continue;
            ptree &parRLength = specReco.add("Parameter","");
            parRLength.add("<xmlattr>.name",xml_recomat_radlength);
            parRLength.add("<xmlattr>.value",rilength.rlength);
            ptree &parILength = specReco.add("Parameter","");
            parILength.add("<xmlattr>.name",xml_recomat_xi);
            parILength.add("<xmlattr>.value",rilength.ilength);
          }
      }
      
      std::string pixfwdRecoSpeccommon = "TrackerRecMaterial" + xml_phaseII_pixecap + "Disk";
      /*for( unsigned int d = 0; d<discRingpair.size(); d++){
          std::stringstream stemp;
          stemp << pixfwdRecoSpeccommon << discRingpair[d].first << "Fw";
          ptree& specReco = specParRecoSec.add("SpecPar","");
          specReco.add("<xmlattr>.name",stemp.str());
          specReco.add("<xmlattr>.eval","true");
          for( unsigned int r = 0; r<discRingpair[d].second; r++){
            for(auto& e:ecapRmatpath) {
                std::stringstream sl;
                sl << "Ring" << r+1 << "Disc" << discRingpair[d].first;
                if( e.find(sl.str()) != std::string::npos ){
                    ptree& part = specReco.add("PartSelector","");
                    part.add("<xmlattr>.path",e);
                }
		}
          }
          for (const auto &rilength : pixelData.lrilength) {
            if (rilength.barrel || rilength.index != discRingpair[d].first)
              continue;
            ptree &parRLength = specReco.add("Parameter","");
            parRLength.add("<xmlattr>.name",xml_recomat_radlength);
            parRLength.add("<xmlattr>.value",rilength.rlength);
            ptree &parILength = specReco.add("Parameter","");
            parILength.add("<xmlattr>.name",xml_recomat_xi);
            parILength.add("<xmlattr>.value",rilength.ilength);
	    }
      }*/
      write_xml(xmlpath+"pixelRecoMaterial_test.xml", tree_recoMat, std::locale(), settings);
  }












    
}
