/*
  ROUTINE TO READ IN 2PT/3PT CORRELATORS COMPUTED IN DISTILLATION,
  WHERE 3PT FUNCTION ARE MATELEMS OF PSEUDO-PDF FORMALISM

  SUMMATION OF OPERATOR INSERTION IS PERFORMED INTERNALLY,
  ALLOWING FOR SUMMATION METHOD TO EXTRACT MATRIX ELEMENT
*/
#include<vector>
#include<fstream>
#include<math.h>
#include<string>
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

#include "operators.h"
#include "pseudo_utils.h"
#include "corr_utils.h"
#include "fit_util.h"
#include "threept_tr.h"
#include "rotations.h"
#include "shortcuts_gsl.h"

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

typedef std::complex<double> cd;

void getSVs(Eigen::MatrixXcd *h)
{
  Eigen::JacobiSVD<Eigen::MatrixXcd> s(*h,Eigen::ComputeFullU | Eigen::ComputeFullV);
  std::cout << s.singularValues() << std::endl;
}

int main(int argc, char *argv[])
{
  if ( argc != 8 )
    {
      std::cout << "\nUsage: $0 <px> <py> <pz> <dx> <dy> <dz> <mu>\n" << std::endl;
      exit(1);
    }
  // Register all the necessary factories for relating lattice matrix elements and helicity amplitudes
  Hadron::IrrepsCubicEnv::registerAll();
  Hadron::IrrepsCubicOctEnv::registerAll();
  Hadron::IrrepsCubicHelicityEnv::registerAll();
  Hadron::SubduceTablesOctEnv::registerAll();
  Hadron::SubduceTablesLgEnv::registerAll();

  int px, py, pz, dx, dy, dz, mu;

  std::stringstream ss;
  // Get ints from CL
  ss << argv[1]; ss >> px; ss.clear();
  ss << argv[2]; ss >> py; ss.clear();
  ss << argv[3]; ss >> pz; ss.clear();
  ss << argv[4]; ss >> dx; ss.clear();
  ss << argv[5]; ss >> dy; ss.clear();
  ss << argv[6]; ss >> dz; ss.clear();
  ss << argv[7]; ss >> mu; ss.clear();
  

  double mass = 0.535;
  int Lx      = 32;

  XMLArray::Array<int> mom;
  mom.resize(3);
  mom[0] = px; mom[1] = py; mom[2] = pz;

  double E    = sqrt( pow(mass,2) + pow(2*PI/Lx,2)*(mom*mom) );

  
  // std::string opName = "NucleonMG1g1MxD0J0S_J1o2_G1g1";
  std::string opName = "NucleonMG1g1MxD0J0S_J1o2_H1o2D4E1";

  std::vector<int> disp = { dx, dy, dz, 0 };


  Spinor s(opName,mom,E,mass,Lx);
  s.buildSpinors();


  // K.assembleBig(mu,true,mass,&s,disp);

  // std::cout << "Kinematic matrix evaluates to: " << std::endl;
  // std::cout << K.mat << std::endl;

  // // Singular values
  // getSVs(&K.mat);


  // Hard code some svd check
  Eigen::MatrixXcd h(12,3);
  // Eigen::MatrixXcd h(8,3);
  // Eigen::MatrixXcd h(6,3);

  // p = (0,0,1)
  h(0,0) = cd(0,0);        h(0,1) = cd(0,0);       h(0,2) = cd(0,0);
  h(1,0) = cd(-1.13979,0); h(1,1) = cd(0,1.13979); h(1,2) = cd(-0.326235,0);
  h(2,0) = cd(-1.13979,0); h(2,1) = cd(0,1.13979); h(2,2) = cd(-0.326235,0);
  h(3,0) = cd(0,0);        h(3,1) = cd(0,0);       h(3,2) = cd(0,0);

  // p = (0,1,1)
  h(4,0) = cd(0,0);        h(4,1) = cd(0,0);       h(4,2) = cd(0,0);
  h(5,0) = cd(-0.852445,0.756604);  h(5,1)=cd(0.756604,0.852445);  h(5,2)=cd(-0.243991,0.216559);
  h(6,0) = cd(-0.852445,-0.756604); h(6,1)=cd(-0.756604,0.852445); h(6,2)=cd(-0.243991,-0.216559);
  h(7,0) = cd(0,0);        h(7,1) = cd(0,0);       h(7,2) = cd(0,0);

  // // p = (1,1,1)
  h(4,0) = cd(0.873651,0);   h(4,1) = cd(0,-0.873651); h(4,2) = cd(0.250061,0);
  h(5,0) = cd(-0.732015,0);  h(5,1) = cd(0,0.732015);  h(5,2) = cd(-0.209521,0);
  h(6,0) = cd(-0.732015,0);  h(6,1) = cd(0,0.732015);  h(6,2) = cd(-0.209521,0);
  h(7,0) = cd(-0.873651,0);  h(7,1) = cd(0,0.873651);  h(7,2) = cd(-0.250061,0);

  std::cout << "For big matrix, SVs = " << std::endl;
  getSVs(&h);


  // Hard code another
  Eigen::MatrixXcd hsmall(6,3);

  h(0,0) = cd(-1.13979,0); h(0,1) = cd(0,1.13979); h(0,2) = cd(-0.326235,0);
  h(1,0) = cd(-1.13979,0); h(1,1) = cd(0,1.13979); h(1,2) = cd(-0.326235,0);
  h(2,0) = cd(-0.852445,0.756604); h(2,1)=cd(0.756604,0.852445); h(2,2)=cd(-0.243991,0.216559);
  h(3,0) = cd(-0.852445,-0.756604); h(3,1)=cd(-0.756604,0.852445); h(3,2)=cd(-0.243991,-0.216559);
  h(4,0) = cd(-0.732015,0); h(4,1)=cd(0,0.732015); h(4,2)=cd(-0.209521,0);
  h(5,0) = cd(-0.732015,0);  h(5,1) = cd(0,0.732015); h(5,2) = cd(-0.209521,0);

  std::cout << "For small matrix, SVs = " << std::endl;
  getSVs(&hsmall);



  kinMatPDF_t V(2,2,current::VECTOR,mu,disp);
  kinMatPDF_t A(2,2,current::AXIAL,mu,disp);
  kinMatPDF_t T(2,2,current::TENSOR,mu,4,disp);
  

  V.assemble(true,mass,&s,&s);
  std::cout << "Vector = " << V.mat << std::endl;

  A.assemble(true,mass,&s,&s);
  std::cout << "Axial = " << A.mat << std::endl;

  T.assemble(true,mass,&s,&s);
  std::cout << "Tensor = " << T.mat << std::endl;
  
  

  return 0;
}
