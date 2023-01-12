/*
  ROUTINE TO READ IN 2PT/3PT CORRELATORS COMPUTED IN DISTILLATION,
  WHERE 3PT FUNCTION ARE MATELEMS OF PSEUDO-GPD FORMALISM

  2PT & 3PT FUNCTIONS ARE COMBINED ACCORDING TO KINEMATICS/DISPLACEMENTS

  DATA WRITTEN TO FILE ACCORDING TO:
     + 2-STATE FITS: 2PT/3PT WRITTEN SEPARATELY FOR FITTING
     + SUMMATION: SUMMED 3PT/2PT RATIO WRITTEN TO FILE FOR FITTING
*/
#include<vector>
#include<fstream>
#include<math.h>
#include<string>
#include<tuple>
#include<sstream>
#include<stdlib.h>
#include<iomanip>
#include<iostream>
#include<algorithm>
#include<ctype.h>
#include<omp.h>

// Headers to manage xml input
#include<libxml/xmlreader.h>
#include<libxml/xpath.h>
#include<libxml/parser.h>

// #include "summation.h"
#include "operators.h"
#include "pseudo_utils.h"
// #include "pseudo_structs.h"

#include "corr_utils.h"
#include "fit_util.h"
#include "threept_tr.h"

// Bring in neccesary adat headers
#include "AllConfStoreDB.h"
#include "DBFunc.h"
#include "adat/map_obj.h"
#include "hadron/hadron_sun_npart_npt_corr.h"
#include "hadron/ensem_filenames.h"
#include "hadron/clebsch.h"
#include "io/key_val_db.h"
#include "io/adat_xmlio.h"
#include "io/adat_xml_group_reader.h"
#include "adat/handle.h"

#include "hadron/irreps_cubic_factory.h"
#include "hadron/irreps_cubic_oct_factory.h"
#include "hadron/irreps_cubic_helicity_factory.h"
#include "hadron/subduce_tables_oct_factory.h"
#include "hadron/subduce_tables_lg_factory.h"
#include "hadron/subduce_tables.h"
#include "hadron/subduce_tables_factory.h"
#include "hadron/single_hadron_coeffs.h"


using namespace ADATXML;
using namespace Pseudo;
using namespace NCOR;


typedef XMLArray::Array<XMLArray::Array<int> > XML2D;

// Define a struct for global props
global_t global;

// Define some structs to help determine what edbs to read
domain_t temporal3pt, temporal2ptFin, temporal2ptIni, temporal2ptRest;
info3pt db3ptInfo;
info2pt db2ptFinInfo, db2ptIniInfo, db2ptRestInfo;

// Constants
const std::complex<double> redFact(sqrt(2),0);


// /*
//   Convert between lattice LG rows and helicity amplitudes
// */
// std::complex<double> lgToHelicityAmps(const Hadron::KeyHadronSUNNPartNPtCorr_t *k)
// {
//   // Returned complex weight
//   std::complex<double> weight;

//   // Construct snk/src operator subduction info
//   subduceInfo snkOp(k->npoint[1].irrep.op.ops[1].name,k->npoint[1].irrep.irrep_mom.mom);
//   subduceInfo srcOp(k->npoint[3].irrep.op.ops[1].name,k->npoint[3].irrep.irrep_mom.mom);
//   // Instantiate canonical rotation structs
//   Hadron::CubicCanonicalRotation_t snkRot, srcRot;


//   // Get the Euler angles, setting all to null for rest case
//   if ( shortMom(k->npoint[1].irrep.irrep_mom.mom,"") != "000" )
//     snkRot = Hadron::cubicCanonicalRotation(k->npoint[1].irrep.irrep_mom.mom);
//   else
//     {
//       snkRot.alpha=0; snkRot.beta=0; snkRot.gamma=0;
//     }
//   if ( shortMom(k->npoint[3].irrep.irrep_mom.mom,"") != "000" )
//     srcRot = Hadron::cubicCanonicalRotation(k->npoint[3].irrep.irrep_mom.mom);
//   else
//     {
//       srcRot.alpha=0; srcRot.beta=0; srcRot.gamma=0;
//     }

// #if VERBOSITY>0
//   // Irrep stuff
//   std::cout << "SNK/SRC IRREP DIMS = " << snkOp.irrep_dim << "/" << srcOp.irrep_dim << std::endl;
//   std::cout << "Snk Euler Angles:"
// 	    << "        alpha = " << snkRot.alpha
// 	    << "        beta  = " << snkRot.beta
// 	    << "        gamma = " << snkRot.gamma << std::endl;
//   std::cout << "Src Euler Angles:"
// 	    << "        alpha = " << srcRot.alpha
// 	    << "        beta  = " << srcRot.beta
// 	    << "        gamma = " << srcRot.gamma << std::endl;
// #endif

//   // Inner subduction/Wigner-D matrix mults
//   for ( int a = 1; a <= snkOp.irrep_dim; ++a )
//     {
//       int twoJZ_a = pow((-1),a-1);
//       for ( int b = 1; b <= snkOp.irrep_dim; ++b )
// 	{
// 	  int twoJZ_b = pow((-1),b-1);
// 	  for ( int c = 1; c <= srcOp.irrep_dim; ++c )
// 	    {
// 	      int twoJZ_c = pow((-1),c-1);

// #if VERBOSITY>2
// 	      std::cout << "NEW COMBO" << std::endl;
// 	      std::cout << "Snk Wigner-D: " << Hadron::Wigner_D(1,twoJZ_a,twoJZ_b,snkRot.alpha,snkRot.beta,snkRot.gamma) << std::endl;
// 	      std::cout << "Snk Subduction: " << (*snkOp.H).operator()(k->npoint[1].irrep.irrep_mom.row,a) << std::endl;
// 	      std::cout << "Src Wigner-D: " << Hadron::Wigner_D(1,twoJZ_b,twoJZ_c,srcRot.alpha,srcRot.beta,srcRot.gamma) << std::endl;
// 	      std::cout << "Src Subduction: " << (*srcOp.H).operator()(c,k->npoint[3].irrep.irrep_mom.row) << std::endl;
// #endif

// 	      // Build the weight
// 	      weight += (*snkOp.H).operator()(k->npoint[1].irrep.irrep_mom.row,a)*
// 		Hadron::Wigner_D(1,twoJZ_a,twoJZ_b,
// 				 snkRot.alpha,snkRot.beta,snkRot.gamma)*
// 		Hadron::Wigner_D(1,twoJZ_b,twoJZ_c,
// 				 srcRot.alpha,srcRot.beta,srcRot.gamma)*
// 		(*srcOp.H).operator()(c,k->npoint[3].irrep.irrep_mom.row);


// 	    } // c
// 	} // b
//     } // a

// #if VERBOSITY>0
//   std::cout << "WEIGHT = " << weight << std::endl;
// #endif

//   return weight;			
// }



/*
  SOME CALLS TO HELP ADAT READ
*/
// Read a correlator edb
std::vector<NCOR::corrEquivalence>
getCorrs(const std::vector<std::string>& dbases, std::vector<Hadron::KeyHadronSUNNPartNPtCorr_t>& fetch)
{
  typedef Hadron::KeyHadronSUNNPartNPtCorr_t       K;
  typedef ENSEM::VectorComplex                     V;

  // Initialize map returned to main associating keys with correlators datatypes
  // n.b. cannot use a std::map here as I haven't created a comparator
  // ADAT::MapObject<Hadron::KeyHadronSUNNPartNPtCorr_t, correlators> retMap;

  // vector of corrEquivalences forall 3pt tseps to return
  // std::vector<NCOR::corrEquivalence> ret( temporal3pt.numT() );

  // NEW
  std::vector<NCOR::corrEquivalence> RET(4);


  // Make a vector large enough to hold just desired keys in fetch
  std::vector<ADATIO::SerialDBKey<K> > keyFetch(fetch.size());
  std::cout << "Will search for " << keyFetch.size() << " keys from databases" << std::endl;
  std::vector< std::vector<ADATIO::SerialDBData<V> > > dataFetch(keyFetch.size());

  // Now run over the keys
  for ( std::vector<ADATIO::SerialDBKey<K> >::iterator k = keyFetch.begin(); k != keyFetch.end(); ++k )
    {
      int cnt = k-keyFetch.begin();
      (*k).key() = fetch[cnt]; // set the key

      int ret;
      // Run over the databases
      for ( std::vector<std::string>::const_iterator dbase = dbases.begin(); dbase != dbases.end(); ++dbase)
	{

	  // Initialize arbitrary DB structure
	  FILEDB::AllConfStoreDB< ADATIO::SerialDBKey<K>,  ADATIO::SerialDBData<V> > database;
	  
	  // Check the current db even exists
	  if (database.open(*dbase, O_RDONLY, 0400) != 0)
	    {
	      std::cerr << __func__ << ": error opening dbase= " << *dbase << std::endl;
	      exit(1);
	    }

	  ret = database.get(*k,dataFetch[cnt]);
	  // Attempt to access the key in the current dbase
	  if ( ret == 0 )
	    {
	      std::cout << "Successfully fetched key = " << k->key() << std::endl;
	      database.close();
	      break;
	    }
	  else
	    continue;
	  database.close();
	} // end dbase


      // Abort if all dbs have been searched and current key has not been found
      if ( ret != 0 )
	{      
	  std::cerr << __func__ << ": key not found\n" << k->key();
	  exit(1);
	}
    } // end k


  // Hold the number of cnfgs in ensemble
  int enscfgs = dataFetch[0].size();
  // String stream for interpreting ENSEM::peekObs data
  std::stringstream sIn;

  // With all the keys found and SerialDBData obtained, parse into correlators form
  for ( int k = 0; k < keyFetch.size(); ++k )
    {
      // Make some array to associate with this key
      NCOR::correlator ensAdat;

      ensAdat.ensemble.ens.resize(enscfgs);
      // ensAdat.ncor[0].imag.resize(enscfgs);
      // Parse data for all configs for this key
      for ( int g = 0; g < dataFetch[k].size(); ++g )
	{
	  // Loop over the data elems for the config of this key
	  for ( int dd = 0; dd < dataFetch[k][g].data().numElem(); ++dd )
	    {
	      double r_, i_;
	      sIn << ENSEM::peekObs(ENSEM::real(dataFetch[k][g].data()),dd);
	      sIn >> r_; sIn.clear(); sIn.str(std::string());
	      sIn << ENSEM::peekObs(ENSEM::imag(dataFetch[k][g].data()),dd);
	      sIn >> i_; sIn.clear(); sIn.str(std::string());

	      
	      std::complex<double> dc(r_,i_);

	      ensAdat.ensemble.ens[g].push_back(dc);
		// .real[g].push_back(r_);
	      // ensAdat.ncor[0].imag[g].push_back(i_);
	    }
	}
      // // Insert this key-correlator combo
      // RET[ (keyFetch[k].key().npoint[1].t_slice - temporal3pt.min)/temporal3pt.step ].
      // 	keyCorrMap.insert(keyFetch[k].key(),ensAdat);


      // NEW - Something hacky
      Hadron::KeyHadronSUNNPartNPtCorr_t tmp = keyFetch[k].key(); //convenience
      if ( tmp.npoint[1].irrep.irrep_mom.row == 1 && tmp.npoint[3].irrep.irrep_mom.row == 1 )
	RET[0].keyCorrMap.insert(keyFetch[k].key(),ensAdat);
      if ( tmp.npoint[1].irrep.irrep_mom.row == 1 && tmp.npoint[3].irrep.irrep_mom.row == 2 )
	RET[1].keyCorrMap.insert(keyFetch[k].key(),ensAdat);
      if ( tmp.npoint[1].irrep.irrep_mom.row == 2 && tmp.npoint[3].irrep.irrep_mom.row == 1 )
	RET[2].keyCorrMap.insert(keyFetch[k].key(),ensAdat);
      if ( tmp.npoint[1].irrep.irrep_mom.row == 2 && tmp.npoint[3].irrep.irrep_mom.row == 2 )
	RET[3].keyCorrMap.insert(keyFetch[k].key(),ensAdat);
      
    }

  return RET;
}


// Reader for global properties
void read(XMLReader& xml, const std::string path, global_t& g)
{
  try {
    read(xml, path+"/ensem", g.ensem);
    read(xml, path+"/cfgs", g.cfgs);
    read(xml, path+"/t2ptRows", g.t2ptRows);
    read(xml, path+"/observable", g.observable);
    read(xml, path+"/state", g.state);
    read(xml, path+"/projection", g.projector);
    read(xml, path+"/insertion/gamma", g.chromaGamma);
    read(xml, path+"/nvec", g.nvec);
    read(xml, path+"/Lt", g.Lt);
    read(xml, path+"/Lx", g.Lx);
    read(xml, path+"/rest", g.rest);
    read(xml, path+"/pi", g.pi);
    read(xml, path+"/pf", g.pf);
    read(xml, path+"/dispNegate", g.dispNegate);
    read(xml, path+"/momNegate", g.momNegate);
    read(xml, path+"/OpMap", g.opMomXML);
  } catch ( std::string &e ) {
    std::cerr << "Unable to parse global properties " << e << std::endl;
  }
}

// Reader for temporal domains of data & any fits
void read(XMLReader& xml, const std::string path, domain_t& t)
{
  try {
    read(xml, path+"/tmin", t.min);
    read(xml, path+"/tstep", t.step);
    read(xml, path+"/tmax", t.max);
  } catch ( std::string &e ) {
    std::cerr << "Unable to parse domain_t struct from ini xml " << e << std::endl;
  }
}

void setType(const std::string type, NCOR::fitInfo_t& I)
{
  try {
    std::cout << "Read type string = " << type << std::endl;
    auto it = table.find(type);
    if ( it != table.end() )
      I.type = it->second;
  } catch ( std::string &e ) {
    std::cerr << "Whoops" << e << std::endl;
  }
}

// Reader for fit functions
void read(XMLReader& xml, const std::string path, NCOR::fitInfo_t& I)
{
  try {
    std::string type;
    read(xml, path+"/funcType", type); // Access type
    setType(type, I);
    // read(xml, path+"/funcType", type, I);

    read(xml, path+"/range", I.range);
    read(xml, path+"/bayes", I.bayesianFit);
    read(xml, path+"/imposeNonLinParamHierarchy", I.imposeNonLinParamHierarchy);
    read(xml, path+"/priors/prior", I.strParamValMaps.strPriorMap);
    read(xml, path+"/priors/width", I.strParamValMaps.strWidthMap);
    read(xml, path+"/minimizerProps/maxIters", I.initFitParams.maxIters);
    read(xml, path+"/minimizerProps/tolerance", I.initFitParams.tolerance);
    read(xml, path+"/minimizerProps/start", I.strParamValMaps.strStartMap);
    read(xml, path+"/minimizerProps/step", I.strParamValMaps.strStepMap);
  } catch ( std::string &e ) {
    std::cerr << "Unable to parse fitInfo_t from ini xml " << e << std::endl;
  }
}

// Reader for db3ptInfo struct
void read(XMLReader& xml, const std::string path, info3pt& d)
{
  try {
    read(xml, path+"/base", d.base);
    read(xml, path+"/momTag", d.momTag);
    read(xml, path+"/tsnkTag", d.tsnkTag);
    read(xml, path+"/t0Tag", d.t0Tag);
    read(xml, path+"/zTag", d.zTag);
    read(xml, path+"/rows", d.rows);
    // read(xml, path+"/rowinfo", d.rowWeights);
  } catch ( std::string &e ) {
    std::cerr << "Unable to parse db3ptInfo struct from ini xml " << e << std::endl;
    exit(1);
  }
}

// Reader for db2ptInfo struct
void read(XMLReader& xml, const std::string path, info2pt& d)
{
  try {
    read(xml, path+"/base", d.base);
    read(xml, path+"/momTag", d.momTag);
    read(xml, path+"/t0Tag", d.t0Tag);
    read(xml, path+"/rows", d.rows);
    // read(xml, path+"/rowinfo", d.rowWeights);
  } catch ( std::string &e ) {
    std::cerr << "Unable to parse db2ptInfo struct from ini xml " << e << std::endl;
    exit(1);
  }
}


/*
  Modify the src/snk row structure of passed key to allow a second RowCombo (RC2) to be accessed
*/
void makeKeyRC2(Hadron::KeyHadronSUNNPartNPtCorr_t &kRC2, XMLArray::Array<int> &newRC)
{
  // newRC2 = db2ptInfo.rows[1]; i.e. an XML::Array<int> object
  kRC2.npoint[1].irrep.irrep_mom.row = newRC[0]; // new snk row
  kRC2.npoint[2].irrep.irrep_mom.row = newRC[1]; // new src row
}


/*
  Accept a std::vector<Hadron::KeyHadronSUNNPartNPtCorr_t> of keys, and
  row/sign modifiers and create new keys to append
*/
void expandRowCombinations(std::vector<Hadron::KeyHadronSUNNPartNPtCorr_t>& v,
			   XMLArray::Array<XMLArray::Array<int> >& r)
{
  // Make r.size local copies of the vector of keys
  std::vector<std::vector<Hadron::KeyHadronSUNNPartNPtCorr_t> > modV(r.size(),v);

  for ( int i = 0; i < r.size(); i++ )
    {

      // modV[i][0].npoint[1].irrep.irrep_mom.row = r[i][0];
      // modV[i][0].npoint[3].irrep.irrep_mom.row = r[i][1];

      // }
      


  // exit(10);

  // // Iterate over the *NEW* (starting at idx1) different row combinations
  // auto ri = r.begin();
  // while ( ri != r.end() )
  //   {

      for ( int j = 0; j < modV[0].size(); j++ )
	{

  // Perform a quick check on first key in &v to see if this row combo (ri) exists,
  // if so skip inputting this row combo
	  if ( modV[0][j].npoint[1].irrep.irrep_mom.row == r[i][0]
	       && modV[0][j].npoint[3].irrep.irrep_mom.row == r[i][1] )
	    {
	      continue;
	    }
	  else {
	    modV[i][j].npoint[1].irrep.irrep_mom.row = r[i][0];
	    modV[i][j].npoint[3].irrep.irrep_mom.row = r[i][1];
	    
	    // int idx = std::distance(r.begin(),ri);
	    // for ( auto i = modV[idx].begin(); i != modV[idx].end(); ++i )
	    //   {
	    //     // Modify the 1st npoint struct
	    //     i->npoint[1].irrep.irrep_mom.row = ri->first[0];
	    //     // Modify the 2nd npoint struct, unless we are iterating through a 3pt key
	    //     if ( i->npoint.size() == 2 ) { i->npoint[2].irrep.irrep_mom.row = ri->first[1]; }
	    //     else { i->npoint[3].irrep.irrep_mom.row = ri->first[1]; }
	    //   }
	    
	  }
	  
	}

      if ( i != 0 )
	// Now tack these modified keys onto the end of v
	v.insert(v.end(), modV[i].begin(), modV[i].end());
	
      // } // end check for existing row combo in first entry of &v
      
      // ri++; // iterate
    }
}
  

/*
  Utility to make 2pt db list
*/
std::vector<std::string> makeDBList(global_t& g, info2pt& I, Hadron::KeyHadronSUNNPartNPtCorr_t *kTemplate)
{
  std::vector<std::string> s;
  
  // Get the momentum of the template key
  Array<int> thisMom = kTemplate->npoint[1].irrep.irrep_mom.mom;

  // A temporary db to set
  std::string tmp_db_plus = I.base+"/"+g.ensem+"/"+I.t0Tag+"/momXYZ."+shortMom(thisMom,".") \
    +"/EDB/"+g.ensem+"."+g.state+"_p"+shortMom(thisMom,"")+".n"+g.nvec+"."+I.t0Tag+".edb";
      
  // Push to db list
  s.push_back(tmp_db_plus);

  // Make a db name for potentially negated momentum combinations
  if ( shortMom(thisMom,"") != "000" && global.momNegate )
    {
      Array<int> thisMomMinus = thisMom*-1;
      // A temporary db to set
      std::string tmp_db_minus = I.base+"/"+g.ensem+"/"+I.t0Tag+"/momXYZ."+shortMom(thisMomMinus,".") \
	+"/EDB/"+g.ensem+"."+g.state+"_p"+shortMom(thisMomMinus,"")+".n"+g.nvec+"."+I.t0Tag+".edb";
      // Push to db list
      s.push_back(tmp_db_minus);
    }
  return s;
}

/*
  Utility to make 3pt db list
*/
std::vector<std::string> makeDBList(global_t& g, info3pt& I, domain_t& t, Hadron::KeyHadronSUNNPartNPtCorr_t *kTemplate)
{
  std::vector<std::string> s;
  for ( int ti = t.min; ti <= t.max; ti+=t.step )
    {
      // A temporary db to set
      std::string tmp_db_plus = I.base[0]+"/"+g.ensem+I.base[1]+ti+"/snk"+shortMom(g.pf,".") \
	+"_src"+shortMom(g.pi,".")+"/EDB/"+g.ensem+"."+g.state+"."+g.observable+"_pf"+shortMom(g.pf,"") \
	+"_pi"+shortMom(g.pi,"")+".n"+g.nvec+"."+I.t0Tag+"_"+
	I.tsnkTag+ti+"."+I.zTag+".edb";
	  
      // Push to db list
      s.push_back(tmp_db_plus);
      // Check for any non-trivial 3pt momenta and make new db file names
      if ( global.momNegate && ( shortMom(g.pf,"") != "000" || shortMom(g.pi,"") != "000" ) )
	{
	  // A temporary db to set
	  std::string tmp_db_minus = I.base[0]+"/"+g.ensem+I.base[1]+ti+"/snk"+shortMom(g.pf*-1,".") \
	    +"_src"+shortMom(g.pi*-1,".")+"/EDB/"+g.ensem+"."+g.state+"."+g.observable+"_pf"+shortMom(g.pf*-1,"") \
	    +"_pi"+shortMom(g.pi*-1,"")+".n"+g.nvec+"."+I.t0Tag+"_"+
	    I.tsnkTag+ti+"."+I.zTag+".edb";
	  // Push to db list
	  s.push_back(tmp_db_minus);
	}
    }
  std::cout << "  Made 3pt db list" << std::endl;
  return s;
}


/*
  Utility to make 2pt key list
*/
std::vector<Hadron::KeyHadronSUNNPartNPtCorr_t> makeKeyList(Hadron::KeyHadronSUNNPartNPtCorr_t &kTemplate)
{
  std::vector<Hadron::KeyHadronSUNNPartNPtCorr_t> s;

  s.push_back(kTemplate);
  // Default to row1-row1 combo (from template key); add the row2-row2 combo if t2ptRows = 0
  if ( global.t2ptRows == 0 )
    {
      // Assuming only a single 2pt key populates s, and that it is a row1-row1 piece
      s.push_back(kTemplate);
      s[1].npoint[1].irrep.irrep_mom.row = 2;
      s[1].npoint[2].irrep.irrep_mom.row = 2;
    }

  // Potentially include negated momentum combination
  if ( global.momNegate )
    checkKeyMomNegate(s,global);
  return s;
}


/*
  Utility to make 3pt key list
*/
std::vector<Hadron::KeyHadronSUNNPartNPtCorr_t> makeKeyList(domain_t& t, Hadron::KeyHadronSUNNPartNPtCorr_t *kTemplate)
{
  std::vector<Hadron::KeyHadronSUNNPartNPtCorr_t> s;

  // With template 3pt key passed as pointer, iterate over tseps and make remaining 3pt keys
  for ( int ti = t.min; ti <= t.max; ti+=t.step )
    {
      // Make a new key from the template
      Hadron::KeyHadronSUNNPartNPtCorr_t tmp_k = *kTemplate;
      
      // Change sink t_slice
      tmp_k.npoint[1].t_slice=ti;
      // Push this tmp key
      s.push_back(tmp_k);

      if ( global.dispNegate )
	{
	  // Make a key for opposing displacement, if disp_list != ''
	  if ( tmp_k.npoint[2].irrep.op.ops[1].disp_list.size() > 0 )
	    {
	      for ( int d = 0; d < tmp_k.npoint[2].irrep.op.ops[1].disp_list.size(); ++d )
		{
		  tmp_k.npoint[2].irrep.op.ops[1].disp_list[d]*=-1;
		}
	      // insert this key with opposite displacement
	      s.push_back(tmp_k);
	    }
	} // global.dispNegate
    }

  // Potentially include negated momentum combination
  if ( global.momNegate )
    checkKeyMomNegate(s,global);
  std::cout << "  Set all 3pt keys from template key" << std::endl;
  return s;
}


// #if 0
// void rowAvg(std::vector<corrEquivalence> &tC, ADAT::MapObject<XMLArray::Array<int>, double> &r)
// {
//   std::cout << "New corrEquivalence to average" << std::endl;
//   // Iterate over std::vec components - i.e. time slices
//   for ( std::vector<corrEquivalence>::iterator it = tC.begin(); it != tC.end(); ++it )
//     {
//       // For this tsep, organize keys within each pz/z channel
//       Hadron::KeyHadronSUNNPartNPtCorr_t kAvg = it->keyCorrMap.begin()->first;
//       ADAT::MapObject<Hadron::KeyHadronSUNNPartNPtCorr_t, correlators> cAvg;

//       std::cout << "--- Running through this corrEquivalence" << std::endl;
//       // Hold z's & p's as temporaries
//       XMLArray::Array<int> _pi, _pf, _q;
//       std::vector<int> _d;
//       _pf = kAvg.npoint[1].irrep.irrep_mom.mom;
//       _q  = kAvg.npoint[2].irrep.irrep_mom.mom;
//       _pi = kAvg.npoint[3].irrep.irrep_mom.mom;
//       _d  = kAvg.npoint[2].irrep.op.ops[1].disp_list;

//       // Try to negate momentum & z's, and fetch new keys
//       for ( int i = 1; i > -2; i-=2 )
// 	{
// 	  for ( int j = 1; j > -2; j-=2 )
// 	    {
// 	      std::cout << "------ Swap pair = ( " << i << " , " << j << " )" << std::endl;
// 	      kAvg.npoint[1].irrep.irrep_mom.mom = _pf * i; // allow swap of snk mom
// 	      kAvg.npoint[2].irrep.irrep_mom.mom = _q * i; // allow swap of ins mom
// 	      kAvg.npoint[3].irrep.irrep_mom.mom = _pi * i; // allow swap of src mom
// 	      // Since we are accessing a key from the tc[*].keyCorrMap map (unordered)
// 	      // We have no guarantee the operator name is correct after setting opposing momenta
// 	      // So we enforce the correct name here...
// 	      kAvg.npoint[1].irrep.op.ops[1].name =
// 		global.opMomXML[shortMom(kAvg.npoint[1].irrep.irrep_mom.mom,"")];
// 	      kAvg.npoint[3].irrep.op.ops[1].name =
// 		global.opMomXML[shortMom(kAvg.npoint[3].irrep.irrep_mom.mom,"")];
	      

// 	      // Allow swap of z
// 	      for ( int d = 0; d != _d.size(); ++d )
// 		{ 
// 		  kAvg.npoint[2].irrep.op.ops[1].disp_list[d] = _d[d] * j;
// 		}

	      
// 	      // Collect the correlators to average
// 	      correlators toAvg(r.size());

// 	      for ( auto rows = r.begin(); rows != r.end(); ++rows )
// 		{
// 		  kAvg.npoint[1].irrep.irrep_mom.row = rows->first[0];
// 		  kAvg.npoint[3].irrep.irrep_mom.row = rows->first[1];

// 		  std::cout << "*****Key = " << kAvg << std::endl;

// 		  // Map matelem btwn lattice LG rows to helicity amplitudes
// 		  std::complex<double> weight = lgToHelicityAmps(&kAvg);

// 		  std::cout << "*****Weight = " << weight << std::endl;
// 		  std::cout << "\n\n";

// 		  // // Fetch this correlator, applying weight in line
// 		  // toAvg.ncor[std::distance(r.begin(),rows)] = rows->second * it->keyCorrMap[kAvg].ncor[0];
// 		}
	      
// 	      // Set row values to "-60" to indicate averaging
// 	      kAvg.npoint[1].irrep.irrep_mom.row = -60;
// 	      kAvg.npoint[3].irrep.irrep_mom.row = -60;
// 	      // Insert this average key and result of merged correlators
// 	      cAvg.insert(kAvg, mergeCorrelators(toAvg));
// 	    } // for zswap (j)
// 	} // for 3-mom swap (i)

//       // With all swapping done, clear std::vector<corrEquivalence>[*it].keyCorrMap and refill it with "cAvg"
//       it->keyCorrMap.clear();
//       it->keyCorrMap = cAvg;

//     } // for std::vector<corrEquivalence> iterator
// }
// #endif


/*
  Try a new version of rowAvg
     - this will collapse keyCorrMap member of each corrEquivalence class to a single entry
     - single entry will have src/snk rows set to '0' (to denote average) and correlators averaged
     - this Adat MapObject will replace original
*/
void rowAvg(std::vector<NCOR::corrEquivalence>& v)
{
  for ( auto a = v.begin(); a != v.end(); ++a )
    {
      
      // Access the first key in map to store info
      Hadron::KeyHadronSUNNPartNPtCorr_t tmp = a->keyCorrMap.begin()->first;
      
      // Obtain the snk/src subduction info
      subduceInfo snkOp(tmp.npoint[1].irrep.op.ops[1].name,tmp.npoint[1].irrep.irrep_mom.mom);
      subduceInfo srcOp(tmp.npoint[3].irrep.op.ops[1].name,tmp.npoint[3].irrep.irrep_mom.mom);
      


      // Determine if +/-zsep need to be stored
      int disp2Store(1);
      if ( ! tmp.npoint[2].irrep.op.ops[1].disp_list.empty() )
	disp2Store = 2;

      
      // Collect the correlators to average
      std::vector<NCOR::VVC> toAvg(snkOp.irrep_dim * srcOp.irrep_dim * disp2Store);
      

      // For this corrEquivance instance, access each correlator and pack into 'toAvg'
      for ( auto m = a->keyCorrMap.begin(); m != a->keyCorrMap.end(); ++m )
	{
	  int idx = std::distance(a->keyCorrMap.begin(),m);

	  // Get Ioffe-time
	  int ioffe(0);
	  if ( ! m->first.npoint[2].irrep.op.ops[1].disp_list.empty() )
	    ioffe = m->first.npoint[2].irrep.op.ops[1].disp_list.back();

	  // Conjugate if Ioffe-time is negative
	  NCOR::VVC dum = m->second.ensemble.ens;
#if 1
	  if ( ioffe < 0 )
	    {
	      std::cout << "Ioffe = " << ioffe << " for key = ";
	      std::cout << m->first << std::endl;
	      NCOR::conj(&dum);
	    }
#endif
	    	  
	  // Pack this correlator
	  toAvg[idx] = dum;
	}


      // Modify the tmp key to represent final row averaged correlator
      tmp.npoint[1].irrep.irrep_mom.row = 0;
      tmp.npoint[3].irrep.irrep_mom.row = 0;
      

      if ( !tmp.npoint[2].irrep.op.ops[1].disp_list.empty()
	   && tmp.npoint[2].irrep.op.ops[1].disp_list.back() < 0 )
	tmp.npoint[2].irrep.op.ops[1].disp_list *= -1;


      // Erase the original keyCorrMap
      a->keyCorrMap.clear();
      
      // Remake the keyCorrMap
      ADAT::MapObject<Hadron::KeyHadronSUNNPartNPtCorr_t, NCOR::correlator> cavg;
      NCOR::correlator mergeCorr = NCOR::mergeCorrs(toAvg);
      cavg.insert(tmp, mergeCorr);

      a->keyCorrMap = cavg;
    }
}



/*
  MUST INCLUDE THIS DUMB READER FOR INSTANCES WHERE 2PT AND 3PT CORRELATORS WERE MADE
  WITH DEVEL AND DEVEL-PDF BRANCHES OF ADAT, RESPECTIVELY!
*/
#ifndef HAVE_DISPLIST_2PT

bool checkExist(const std::string n)
{
  std::ifstream in(n);
  return in.good();
}


void Reads2pt(std::ifstream &inFile, NCOR::correlator& dum, info2pt& I, Hadron::KeyHadronSUNNPartNPtCorr_t *h)
{
  // // Look and check for existence of potentially several files
  // for ( auto d = I.base.begin(); d != I.base.end(); ++d )
  //   {  --> *d
      std::string dumb = I.base+"/"+I.momTag+"."+shortMom(h->npoint[1].irrep.irrep_mom.mom,".")
	+"/ENS/"+Hadron::ensemFileName(*h);


      NCOR::read(inFile,dumb,dum.ensemble);
      // arr_print(dum.ncor[0]);
}
#endif


// /*
//   Utility to ensure data has been parsed correctly from dbs
// */
// // void parseCheck(ADAT::MapObject<Hadron::KeyHadronSUNNPartNPtCorr_t, correlators> A)
// void parseCheck(std::vector<corrEquivalence> T)
// {
//   for ( auto tt = T.begin(); tt != T.end(); ++tt )
//     {
//       for ( auto kc = tt->keyCorrMap.begin(); kc != tt->keyCorrMap.end(); ++kc )
// 	{
// 	  std::cout << "Performing simple ensemble average check on key = \n"
// 		    << "     " << kc->first << std::endl;
	  
// 	  corrFunc funcDum;
// 	  funcDum.data = kc->second;
// 	  std::cout << "We have dimensions: " << funcDum.data.ncor.size() << " X "
// 		    << funcDum.data.ncor[0].real.size() << " X "
// 		    << funcDum.data.ncor[0].real[0].size() << std::endl;
// 	  // arr_print(funcDum.data.ncor[0]);
// 	  ensemble funcDumAvg = corrAvg(funcDum.data);
// 	  std::cout << " Now the average" << std::endl;
// 	  arr_print(funcDumAvg);
// 	}
//     }
// }

int main(int argc, char *argv[])
{
  if ( argc != 2 )
    {
      std::cout << "\nUsage: $0 <ini xml>\n" << std::endl;
      exit(1);
    }

  // Instantiate a string streamer
  std::stringstream ss;

  XMLReader xmlSR(argv[1]);
  std::cout << "Reading from inixml = " << std::string(argv[1]) << std::endl;


  // Register all the necessary factories for relating lattice matrix elements and helicity amplitudes
  // Hadron::IrrepsCubicHelicityEnv::registerAll();
  Hadron::SubduceTablesOctEnv::registerAll();
  Hadron::SubduceTablesLgEnv::registerAll();


  /*
    Determine global properties
  */
  read(xmlSR, "/PGITD/global", global);
  /*
    Determine the db info - sets where/what edbs to search for keys
  */
  read(xmlSR, "/PGITD/dbInfo/threePt/tseries/range", temporal3pt);
  read(xmlSR, "/PGITD/dbInfo/threePt", db3ptInfo);
  read(xmlSR, "/PGITD/dbInfo/twoPtFin/tseries/range", temporal2ptFin);
  read(xmlSR, "/PGITD/dbInfo/twoPtFin", db2ptFinInfo);
  read(xmlSR, "/PGITD/dbInfo/twoPtIni/tseries/range", temporal2ptIni);
  read(xmlSR, "/PGITD/dbInfo/twoPtIni", db2ptIniInfo);
  read(xmlSR, "/PGITD/dbInfo/twoPtRest/tseries/range", temporal2ptRest);
  read(xmlSR, "/PGITD/dbInfo/twoPtRest", db2ptRestInfo);
  // dumpRowInfo(db3ptInfo.rows,db3ptInfo.signs,3);
  // dumpRowInfo(db2ptInfo.rows,db2ptInfo.signs,2);



  // Set the number of tseps once and for all
  const int nTSeps = temporal3pt.numT();

  // Structure to hold all desired 3pt/2pt motion/normalizing keys
  struct keydb_t
  {
    std::vector<Hadron::KeyHadronSUNNPartNPtCorr_t> keys;
    std::vector<std::string>                        dbs;
    // Map of keys and correlators structs returned
    // std::map<Hadron::KeyHadronSUNNPartNPtCorr_t, correlators> keycorrMap; --> need a comparator
    ADAT::MapObject<Hadron::KeyHadronSUNNPartNPtCorr_t, NCOR::correlator> keycorrMap;
  };

  struct nptKeysValues_t
  {
    keydb_t keydbs;
  } threePt, twoPi, twoPf, twoPtRest;


  struct keyTemplate_t
  {
    Hadron::KeyHadronSUNNPartNPtCorr_t key;
  } tempKey3pt, tempKey2Pi, tempKey2Pf, tempKeyRest;


  /*
    Read the template keys from the ini xml
  */
  try {
    read(xmlSR, "/PGITD/Ops/threePt/keyTemplate", tempKey3pt.key);
    read(xmlSR, "/PGITD/Ops/twoPt/keyTemplate", tempKey2Pi.key);
    read(xmlSR, "/PGITD/Ops/twoPtRest/keyTemplate", tempKeyRest.key);
    tempKey2Pf = tempKey2Pi;
  } catch ( std::string &e ) {
    std::cerr << "Unable to access key templates from ini xml " << e << std::endl;
  }
  std::cout << "READ THE TEMPLATE KEYS TO FETCH" << std::endl;

  /*
    Substitute correct momenta and src/snk operator names (based on passed global& global)
    into template 3pt & 2pt keys
  */
  // setOpsMoms(&tempKey3pt.key, tempKey2Pf.key, tempKey2Pi.key, global);
  setOpsMoms(&tempKey3pt.key, tempKey2Pf.key, tempKey2Pi.key, tempKeyRest.key, global);

  


  /*   Make the 3pt database structures to search   */
  threePt.keydbs.dbs    = makeDBList(global, db3ptInfo, temporal3pt, &tempKey3pt.key);
  // Make all 3pt keys from templates
  threePt.keydbs.keys   = makeKeyList(temporal3pt, &tempKey3pt.key);
  // Expand 3pt keys to include all desired row combinations
  expandRowCombinations(threePt.keydbs.keys, db3ptInfo.rows);
  /* Ensure 3pt keys conserve 3-momentum */
  conserveMom3PtKey(threePt.keydbs.keys);
  /* Print the 3pt dbs & keys to read */
  // dumpDBs(threePt.keydbs.dbs);
  // dumpKeys(threePt.keydbs.keys, 3);


  /*
    Access and store all three point functions
  */
  std::vector<NCOR::corrEquivalence> funcs3pt;
  try {
    funcs3pt = getCorrs(threePt.keydbs.dbs,threePt.keydbs.keys);
  }
  catch (std::string e) {
    std::cout << "Failed access of 3pt functions : " << e << std::endl;
    exit(1);
  }
  std::cout << "...........3pt Correlators   ---   SUCCESS!" << std::endl;

  /*
    Do some checks of the three-pt functions
  */
#if 0
  for ( auto i = funcs3pt.begin(); i != funcs3pt.end(); ++i )
    std::cout << " This {op,abs(z),abs(p)} corr type has " << i->keyCorrMap.size() << " keys" << std::endl;

  std::cout << "Number of tseps 3pt funcs are stored for = " << funcs3pt.size() << std::endl;
#if 0
  parseCheck(funcs3pt); std::cout << "\n";
#endif
  exit(8);
#endif


  /*
    NOW LOAD THE 2PT FUNCTIONS
  */
  twoPi.keydbs.dbs      = makeDBList(global, db2ptIniInfo, &tempKey2Pi.key);
  twoPi.keydbs.keys     = makeKeyList(tempKey2Pi.key);
  twoPf.keydbs.dbs      = makeDBList(global, db2ptFinInfo, &tempKey2Pf.key);
  twoPf.keydbs.keys     = makeKeyList(tempKey2Pf.key);
  twoPtRest.keydbs.dbs  = makeDBList(global, db2ptRestInfo, &tempKeyRest.key);
  twoPtRest.keydbs.keys = makeKeyList(tempKeyRest.key);


  /*
    Access and store all 2pt functions
  */
  // std::vector<NCOR::corrEquivalence> twoPtIni(1), twoPtFin(1);
  prop_t * propsIni = new prop_t(global.cfgs,temporal2ptIni,tempKey2Pi.key);
  propsIni->npt = 2;

#if 0
  std::cout << propsIni->cfgs << " " << propsIni->npt << " " << propsIni->Nt << " " << propsIni->gamma << " " << std::endl;
  std::cout << propsIni->key << std::endl;
  std::cout << propsIni->domain.min << " " << propsIni->domain.step << " " << propsIni->domain.max << " "<< propsIni->domain.numT() << std::endl;
  exit(90);
#endif

  
  NCOR::correlator twoPtIni(*propsIni);
  delete propsIni;

  prop_t * propsFin = new prop_t(global.cfgs,temporal2ptFin,tempKey2Pf.key);
  propsFin->npt = 2;
  NCOR::correlator twoPtFin(*propsFin);
  delete propsFin;

  prop_t * props = new prop_t(global.cfgs,temporal2ptRest,tempKeyRest.key);
  props->npt = 2;
  NCOR::correlator rest2pt(*props);
  delete props;


#ifdef HAVE_DISPLIST_2PT
#warning "Using getCorrs to read 2pt correlators"
  twoPtIni = getCorrs(twoPi.keydbs.dbs,twoPi.keydbs.keys);
  twoPtFin = getCorrs(twoPf.keydbs.dbs,twoPf.keydbs.keys);
#else
#warning ">>>>>>>>>>>  2pt correlators constructed w/o disp_list  -->  reverting to ascii reader"
  std::ifstream inFile;
  
  for ( auto k = twoPi.keydbs.keys.begin(); k != twoPi.keydbs.keys.end(); ++k )
    Reads2pt(inFile, twoPtIni, db2ptIniInfo, &(*k) );
  for ( auto k = twoPf.keydbs.keys.begin(); k != twoPf.keydbs.keys.end(); ++k )
    Reads2pt(inFile, twoPtFin, db2ptFinInfo, &(*k) );
  for ( auto k = twoPtRest.keydbs.keys.begin(); k != twoPtRest.keydbs.keys.end(); ++k )
      Reads2pt(inFile, rest2pt, db2ptRestInfo, &(*k) );
#endif




  twoPtIni.jackknife(); twoPtFin.jackknife(); rest2pt.jackknife();
  twoPtIni.ensAvg();    twoPtFin.ensAvg();    rest2pt.ensAvg();
  twoPtIni.Cov();       twoPtFin.Cov();       rest2pt.Cov();


  // LinAlg::printMat(twoPtFin.cov.dat["real"]);
  // LinAlg::printMat(twoPtFin.cov.inv["real"]);


  // Get the fit info for ini/fin 2pts & 3pt
  NCOR::fitInfo_t twoPtRestFitInfo, twoPtFinFitInfo, twoPtIniFitInfo, threePtFitInfo;
  read(xmlSR, "/PGITD/fitting/twoPtFin", twoPtFinFitInfo);
  read(xmlSR, "/PGITD/fitting/twoPtIni", twoPtIniFitInfo);
  read(xmlSR, "/PGITD/fitting/twoPtRest", twoPtRestFitInfo);
  read(xmlSR, "/PGITD/fitting/threePt", threePtFitInfo);

  // Parse the strParamValMaps
  twoPtFinFitInfo.parseParamMaps();
  twoPtIniFitInfo.parseParamMaps();
  twoPtRestFitInfo.parseParamMaps();
  threePtFitInfo.parseParamMaps();

  // To set up fit properly, pass the correlator's data covariance
  // Submatrix of covariance is internally grabbed, and its inverse computed
  twoPtFin.fit = NCOR::fitFunc_t(twoPtFinFitInfo,twoPtFin.cov.dat,temporal2ptFin);
  twoPtIni.fit = NCOR::fitFunc_t(twoPtIniFitInfo,twoPtIni.cov.dat,temporal2ptIni);
  rest2pt.fit = NCOR::fitFunc_t(twoPtRestFitInfo,rest2pt.cov.dat,temporal2ptRest);

  // Fire up the fits
  std::vector<std::string> components(2);
  components[0] = "real"; components[1] = "imag";
  NFIT::driver(&twoPtFin, components[0], true); fitResW(&twoPtFin, components[0]);
  NFIT::driver(&twoPtIni, components[0], true); fitResW(&twoPtIni, components[0]);
  NFIT::driver(&rest2pt, components[0], true);  fitResW(&rest2pt, components[0]);


  writeCorr(&twoPtFin);
  writeCorr(&twoPtIni);
  writeCorr(&rest2pt);


  /*
    Do some checks of 2pt functions
  */
#if 0
  std::cout << "2pt INI KEYS" << std::endl; dumpKeys(twoPi.keydbs.keys,2);
  std::cout << "2pt FIN KEYS" << std::endl; dumpKeys(twoPf.keydbs.keys,2);
  parseCheck(twoPtIni); std::cout << "\n";
  parseCheck(twoPtFin); std::cout << "\n";
  exit(9);
#endif


  /*
    Do some checks of the 2pt fit results
  */
#if 0
  std::cout << "PARAMS FOR 2PT INI:" << std::endl;
  for ( auto i = twoPtIni.res.params.begin(); i != twoPtIni.res.params.end(); ++i )
    {
      std::cout << "    " << i->first << " : ";
      for ( auto p = i->second.begin(); p != i->second.end(); ++p )
	{
	  std::cout << *p << " ";
	}
      std::cout << "\n";
    }

  std::cout << "PARAMS FOR 2PT FIN:" << std::endl;
  for ( auto i = twoPtFin.res.params.begin(); i != twoPtFin.res.params.end(); ++i )
    {
      std::cout << "    " << i->first << " : ";
      for ( auto p = i->second.begin(); p != i->second.end(); ++p )
        {
	  std::cout << *p << " ";
        }
      std::cout << "\n";
    }

  std::cout << "PARAMS FOR 2PT REST:" << std::endl;
  for ( auto i = rest2pt.res.params.begin(); i != rest2pt.res.params.end(); ++i )
    {
      std::cout << "    " << i->first << " : ";
      for ( auto p = i->second.begin(); p != i->second.end(); ++p )
        {
	  std::cout << *p << " ";
        }
      std::cout << "\n";
    }
  std::cout << "REST chi2: " << std::endl;
  for ( auto c = rest2pt.res.chi2.begin(); c != rest2pt.res.chi2.end(); ++c )
    std::cout << *c << " ";
  std::cout << "\n";
#endif



  // Global properties
  const int NUM_MATS           = 12;
  const int GPD_RANK           = 6;
  const int MATS_PER_INSERTION = 4;

  // Circular polarization vector
  polVecBasis_t BASIS;

  std::vector<Eigen::Matrix<std::complex<double>, NUM_MATS, 1> > MAT(global.cfgs);
  std::vector<kinMatGPD_t> KIN(global.cfgs);
  for ( auto k = KIN.begin(); k != KIN.end(); ++k )
    {
      int j = std::distance(KIN.begin(),k);
      
      // Initialize the final/initial state spinors for this jackknife sample
      Spinor finSpin(global.opMomXML[shortMom(global.pf,"")],
		     global.pf,twoPtFin.res.params["E0"][j],rest2pt.res.params["E0"][j],global.Lx);
      Spinor iniSpin(global.opMomXML[shortMom(global.pi,"")],
		     global.pi,twoPtIni.res.params["E0"][j],rest2pt.res.params["E0"][j],global.Lx);
      // Build the spinors
      finSpin.buildSpinors();
      iniSpin.buildSpinors();

      // Constant DISP & MASS variables for convenience
      const std::vector<int> DISP = shortZ(funcs3pt[0].keyCorrMap.begin()->first.npoint[2].irrep.op.ops[1].disp_list);
      const double MASS = rest2pt.res.params["E0"][j];


      // Init kinematic matrices for each of \gamma_4, \gamma_1, \gamma_2
      kinMatGPD_t GPD_4(MATS_PER_INSERTION,GPD_RANK,current::VECTOR,4,DISP);
      kinMatGPD_t GPD_1(MATS_PER_INSERTION,GPD_RANK,current::VECTOR,1,DISP);
      kinMatGPD_t GPD_2(MATS_PER_INSERTION,GPD_RANK,current::VECTOR,2,DISP);
      // Assemble the kinematic matrices
      GPD_4.assemble(true,MASS,&finSpin,&iniSpin);
      GPD_1.assemble(true,MASS,&finSpin,&iniSpin);
      GPD_2.assemble(true,MASS,&finSpin,&iniSpin);

      
      // Concatenate GPD_4,1,2 matrices into one large one for SVD
      // Eigen::Matrix<std::complex<double>, 3*MATS_PER_INSERTION, GPD_RANK> GPD;
      Eigen::MatrixXcd GPD(3*MATS_PER_INSERTION, GPD_RANK);
      std::cout << "GPD = " << GPD << std::endl;
      
      // Push GPD_4.mat into GPD
      for ( int i = 0; i < GPD_4.mat.rows(); ++i ) GPD.row(i) << GPD_4.mat.row(i);
      
      std::cout << "GPD = " << GPD << std::endl;
      // Form m=+/-1 components of \gamma_j's in circular basis
      for ( int m = 1; m >= -1; m-=2 )
	{
	  Eigen::Matrix<std::complex<double>, MATS_PER_INSERTION, GPD_RANK> TMP;
	  TMP = _I_*( BASIS.cirq(m)(0)*GPD_1.mat + BASIS.cirq(m)(1)*GPD_2.mat );

	  std::cout << "TMP = " << TMP << std::endl;

	  int offset = ( m > 0 ) ? 1 : 2; // ensure rows are mapped correctly w/ 'm'
	  for ( int i = offset*GPD_4.mat.rows(); i < (1+offset)*GPD_4.mat.rows(); ++i )
	    // Push TMP rows into GPD
	    GPD.row(i) << TMP.row(i - offset*GPD_4.mat.rows() );
	}

      std::cout << "FINAL GPD = " << GPD << std::endl;
      getSVs(&GPD);
      exit(8);

    }
  std::vector<Eigen::Matrix<std::complex<double>, GPD_RANK, 1> > AMP(global.cfgs);



  /*
    Loop over all equivalent 3pt functions --> diff. rows, but kinematically the same
  */
  for ( std::vector<NCOR::corrEquivalence>::iterator tsepItr = funcs3pt.begin();
	tsepItr != funcs3pt.end(); ++tsepItr )
    {
      // Convenience
      int rowf = tsepItr->keyCorrMap.begin()->first.npoint[1].irrep.irrep_mom.row;
      int rowi = tsepItr->keyCorrMap.begin()->first.npoint[3].irrep.irrep_mom.row;

      // Map the snk/src row combinations to a given element of correlator column vector
      std::map<std::pair<int,int>, int> matIDX;
      std::pair<int,int> rfri;

      rfri = std::make_pair(1,1); matIDX[rfri] = 0;
      rfri = std::make_pair(1,2); matIDX[rfri] = 1;
      rfri = std::make_pair(2,1); matIDX[rfri] = 2;
      rfri = std::make_pair(2,2); matIDX[rfri] = 3;

      /*
	For each correlator stored in each corrEquivalence,
	divide by 2pt function at same tsep
	Done per jackknife ensemble average
	Bias corrected after making ratio
	
	Sum up operator insertion time slice
      */
      std::vector<NCOR::correlator> ratio(nTSeps); // nTseps correlators of same rows/kinematics
      for ( auto it = tsepItr->keyCorrMap.begin(); it != tsepItr->keyCorrMap.end(); ++it )
	{
	  // tsepItr->keyCorrMap is not ordered by tsep yet --> form an index to store in ascending order
	  int idx = (it->first.npoint[1].t_slice - temporal3pt.min)/temporal3pt.step;
	  // Local copy of this tsep
	  const int TSEP = it->first.npoint[1].t_slice;


	  // Init this ratio --> REMEMBER, 3PTS HAVE TSLICES\IN[0,TSEP)
	  Pseudo::domain_t * d = new Pseudo::domain_t(0,1,it->first.npoint[1].t_slice-1);
	  prop_t * props = new prop_t(global.cfgs, *d, it->first);
	  props->npt     = 3;
	  props->gamma   = global.chromaGamma;

	  
	  // Now construct a new ratio
	  ratio[idx] = NCOR::correlator(*props, it->second.ensemble);

	  // Jackknife this ratio so 3pt/2pt ratio can be formed
	  ratio[idx].jackknife();
	  // ratio[idx].ensAvg();
	  


	  // Loop over insertion times for this 3pt TSEP
	  for ( auto tau = ratio[idx].ensemble.T.begin(); tau != ratio[idx].ensemble.T.end(); ++tau )
	    {
	      // Loop over the jackknife ensemble averages
	      // ratio[idx] is for fixed T with tau varying
	      for ( int j = 0; j < global.cfgs; ++j )
		{
		  ratio[idx].ensemble.ens[j][*tau] =
		    ( ratio[idx].jack[j].avg[*tau] / twoPtFin.jack[j].avg[TSEP].real() )
		    * sqrt(( twoPtIni.jack[j].avg[TSEP-*tau].real() * twoPtFin.jack[j].avg[*tau].real()
			     * twoPtFin.jack[j].avg[TSEP].real() )/
			   ( twoPtFin.jack[j].avg[TSEP-*tau].real() * twoPtIni.jack[j].avg[*tau].real()
			     * twoPtIni.jack[j].avg[TSEP].real() ));

		} // j
	    } // tau
	  


	  // Include standard kinematic prefactors arising in forming optimized 3pt/2pt ratio
	  for ( int j = 0; j < global.cfgs; ++j )
	    {
	      std::complex<double> commonKin(sqrt(4*twoPtFin.res.params["E0"][j]*twoPtIni.res.params["E0"][j]), 0.0);
	      
	      // Remove common kinematic factor & 1/\sqrt(2) from isovector current normalization
	      ratio[idx].ensemble.ens[j] *= (redFact*commonKin);
	    } // j


	  // Get ensemble avg so bias removal can proceed
	  ratio[idx].ensAvg();
#if 1
	  std::cout << "**************" << std::endl;
	  std::cout << "Ratio ens avg" << std::endl;
	  std::cout << ratio[idx] << std::endl;
	  std::cout << "**************" << std::endl;
#endif
	  // Correct for bias in forming ratio
	  ratio[idx].removeBias();


	  // Summation of operator insertion
	  ratio[idx].summation();

	  delete d;
	  delete props;
	} // it
      /*
	Now have summed ratio for a particular src/snk row combination
      */

      
      // Map the summed ratio data into correlator instance 'SR'
      // --> so covariance/fitting members can be used
      prop_t * xprops = new prop_t(global.cfgs, temporal3pt, ratio[0].key());
      xprops->npt     = 3;
      xprops->gamma   = global.chromaGamma;


      // Construct the single correlator instance 'SR'
      // ---> this will be fit
      NCOR::correlator SR(*xprops);
      
      for ( auto rptr = ratio.begin(); rptr != ratio.end(); ++rptr )
	{
	  int ridx = std::distance(ratio.begin(), rptr);
	  for ( auto gg = rptr->ensemble.ens.begin(); gg != rptr->ensemble.ens.end(); ++gg )
	    {
	      int gdx = std::distance(rptr->ensemble.ens.begin(), gg);
	      SR.ensemble.ens[gdx][ridx] = (*gg)[0];
	    } // gg
	} // rptr
      /*
	Now SR has been constructed
      */

      
      
      /*
	Perform linear fit to this summed ratio 'SR'
	--> exposes matrix element that will be fed into SVD to extract amplitudes
      */
      SR.jackknife();
      SR.ensAvg();

#if 1
      std::cout << "With key = " << SR.key() << " ..." << std::endl;
      std::cout << SR << std::endl;
#endif
#if 1

      // Make data covariance and initialize fit
      SR.Cov();
      SR.fit = NCOR::fitFunc_t(threePtFitInfo,SR.cov.dat,temporal3pt);

      writeCorr(&SR);
      /*
	Do the linear fits -- for both real/imag components
      */
      for ( auto f = components.begin(); f != components.end(); ++f )
	{
	  NFIT::driver(&SR, *f, false);

	  // Write out the fit results
	  fitResW(&SR, *f);


	  // Pipe fit results foreach jackknife sample into appropriate entry of MAT
	  for ( int g = 0; g < global.cfgs; ++g )
	    {
	      std::pair<int,int> lookUp = std::make_pair(rowf,rowi);
	      if ( *f == "real" )
		MAT[g](matIDX[lookUp]).real(SR.res.params["b"][g]);
	      if ( *f == "imag" )
		MAT[g](matIDX[lookUp]).imag(SR.res.params["b"][g]);
	    }

	  // Destroy the stored fits values since they've been written
	  SR.res.chi2.clear(); SR.res.params.clear();
	}
#endif
      delete xprops;

    } // funcs3pt iterator

  // With MAT populated per jackknife ensemble avg
  // Do the SVD per jackknife ensemble avg to extract amplitudes
  extAmplitudes(&MAT,&KIN,&AMP);
  std::cout << "What do these solutions look like?\n";
  
  // Put AMP results into a VectorXcd so writeAmplitudes can be reused
  std::vector<Eigen::VectorXcd> finalAMP(global.cfgs,Eigen::VectorXcd(GPD_RANK));
  for ( auto itr = AMP.begin(); itr != AMP.end(); ++itr )
    {
      int idx = std::distance(AMP.begin(), itr);
      std::cout << "A" << idx+1 << " = "  << (*itr)(idx) << "    ";
      finalAMP[idx](idx) = (*itr)(idx);
    }
  std::cout << "\n";


  // // Put AMP results into a VectorXcd so writeAmplitudes can be reused
  // std::vector<Eigen::VectorXcd> finalAMP(global.cfgs,Eigen::VectorXcd(4));
  // for ( auto itr = AMP.begin(); itr != AMP.end(); ++itr )
  //   {
  //     int idx = std::distance(AMP.begin(),itr);

  //     finalAMP[idx](0) = (*itr)(0);
  //     finalAMP[idx](1) = (*itr)(1);
  //     finalAMP[idx](2) = (*itr)(2);
  //     finalAMP[idx](3) = (*itr)(3);
  //   }

  // Write the extracted amplitudes to h5
  // -->clunky way to pass displacement
  writeAmplitudes(&finalAMP,&global,&threePtFitInfo,
		  &tempKey3pt.key.npoint[2].irrep.op.ops[1].disp_list);
  

  return 0;
}
