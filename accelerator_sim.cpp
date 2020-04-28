#include <fstream>

#include "accelerator_sim.h"
#include "settings.h"

#include "MaterialData.h"
#include "CollimatorDatabase.h"
#include "MADInterface.h"
#include "LatticeFunctions.h"
#include "PhysicalUnits.h"
#include "PhysicalConstants.h"
#include "CollimatorAperture.h"
#include "ApertureConfiguration.h"
#include "Collimator.h"


using namespace std;
using namespace PhysicalUnits;
using namespace PhysicalConstants;
using namespace ParticleTracking;

AcceleratorSim::AcceleratorSim(Settings* settings_)
	: settings(settings_)
{
	log_dir = settings->get("log_dir", "logs/");
	input_data_dir = settings->get("input_data_dir", "commondata/");
	result_dir = settings->get("result_dir", "result/");
	cout << "log_dir = " << log_dir << endl;
	cout << "input_data_dir = " << input_data_dir << endl;
	cout << "result_dir = " << result_dir << endl;

	optics_file = settings->get("optics");
	collimator_file = settings->get("collimators", "");
	aperture_file = settings->get("apertures", "");
	cout << "optics_file = " << optics_file << endl;
	cout << "collimator_file = " << collimator_file << endl;
	cout << "aperture_file = " << aperture_file << endl;

	mass = ProtonMass;
	mass_mev = ProtonMassMeV;
	charge = 1;
	cout << "mass :" << mass << " kg (" << mass_mev << " MeV)" << endl;
	cout << "charge :" << charge << endl;

	beam_energy =  settings->get_double("beam_energy");
	cout << "beam_energy = " << beam_energy << endl;
	beam_momentum = sqrt(pow(beam_energy,2) - pow(mass_mev*MeV,2));
	beam_kinetic_energy = beam_energy - mass_mev*MeV;
	gamma = beam_energy/mass_mev/MeV;
	beta = sqrt(1.0-(1.0/pow(gamma,2)));
	beam_rigidity = beam_momentum/eV /SpeedOfLight / charge;

	set_emittance();
	cout << "normalized_emittance = " << normalized_emittance << endl;
	cout << "emittance = " << emittance << endl;
	cout << "beam_momentum: " << beam_momentum << endl;
	cout << "beam_kinetic_energy: " << beam_kinetic_energy << endl;
	cout << "gamma: " << gamma << endl;
	cout << "beta: " << beta << endl;
	cout << "rigidity: " << beam_rigidity << endl;

	beam_charge =  settings->get_double("beam_charge", 1);
	cout << "beam_charge = " << beam_charge << endl;
}

AcceleratorSim::~AcceleratorSim()
{}

void AcceleratorSim::set_emittance()
{
	if(settings->has_key("normalized_emittance") == settings->has_key("emittance"))
	{
		cout << "One of normalized_emittance or emittance must be set." <<endl;
		exit(1);
	}
	if(settings->has_key("normalized_emittance"))
	{
		normalized_emittance = settings->get_double("normalized_emittance");
		emittance = normalized_emittance/(gamma*beta);
	}else{
		emittance = settings->get_double("emittance");
		normalized_emittance = emittance*(gamma*beta);
	}

}

void AcceleratorSim::build_lattice()
{
	std::unique_ptr<MADInterface> myMADinterface(new MADInterface(optics_file, beam_momentum));

	myMADinterface->TreatTypeAsDrift("RFCAVITY");

	model.reset(myMADinterface->ConstructModel());
}


void AcceleratorSim::get_lattice_functions()
{
	if(!model){cout << "No model, run build_lattice()" << endl; exit(1);}
	twiss.reset(new LatticeFunctionTable(model.get(), beam_momentum));

	twiss->AddFunction(2,2,1);
	twiss->AddFunction(4,4,2);

	twiss->SetForceLongitudinalStability(true);
	twiss->Calculate();

	if(0)
	{
		string fname = result_dir+"LatticeFunctions.dat";
		ofstream latticeFunctionLog(fname);
		if (!latticeFunctionLog){cerr << "Failed to open: " << fname << endl; exit(1);}

		latticeFunctionLog << "#S X PX Y PY T DELTAP BETX ALFX BETY ALFY LF163 LF263 LF363 LF463 LF663 GAMX GAMY" <<endl;

		latticeFunctionLog.precision(16);
		twiss->PrintTable(latticeFunctionLog);
		cout << "Wrote " << fname << endl;
	}
}

void AcceleratorSim::set_start_element(std::string start_element_)
{
	if(!model){cout << "No model, run build_lattice()" << endl; exit(1);}
	start_element = start_element_;
	start_element_number = model->GetIndexes("*"+start_element).at(0);
}

void AcceleratorSim::setup_collimators()
{
	if(!model){cout << "No model, run build_lattice()" << endl; exit(1);}
	if(!twiss){cout << "No twiss, run get_lattice_functions()" << endl; exit(1);}
	if(start_element == ""){cout << "No start element, run set_start_element()" << endl; exit(1);}

	impact_factor = settings->get_double("impact_factor", 1e-6);

	if(collimator_file != ""){
		collimator_db.reset(new CollimatorDatabase(collimator_file, new StandardMaterialData, true));
	}
	else
	{
		cout << "No collimator file set in settings. Exiting" << endl;
		exit(1);
	}

	collimator_db->MatchBeamEnvelope(false);
	collimator_db->EnableJawAlignmentErrors(false);
	collimator_db->SelectImpactFactor(start_element, impact_factor);
	collimator_db->ConfigureCollimators(model.get(), emittance, emittance, twiss.get());
}

void AcceleratorSim::set_halo_size_from_aperture(string element_name)
{
	vector<Collimator*> TCP;
	int siz = model->ExtractTypedElements(TCP,element_name);
	if(!siz)
	{
		cerr << "Can't set halo size from " << element_name << ". It is not found or not a collimator." << endl;
		exit(1);
	}

	Aperture *ap = (TCP[0])->GetAperture();
	if(!ap)
	{
		cout << "Could not get tcp ap" << endl;
		exit(1);
	}

	CollimatorAperture* CollimatorJaw = dynamic_cast<CollimatorAperture*>(ap);
	if(!CollimatorJaw)
	{
		cout << "Could not cast. "<< (TCP[0])->GetQualifiedName() << "does not have a CollimatorAperture" << endl;
		abort();
	}

	double col_element_number = model->GetIndexes("*"+element_name).at(0);

	h_offset_entrance = twiss->Value(1,0,0,col_element_number);


	JawSize_entrance = CollimatorJaw->GetFullEntranceWidth() / 2.0;
	JawPosition_entrance = CollimatorJaw->GetEntranceXOffset();

	cout << "h_offset entrance: " << h_offset_entrance << endl;
	cout << "Jaw position entrance: " << JawPosition_entrance << endl;

	double beta_x_start_el = twiss->Value(1,1,1,col_element_number)*meter;
	double beta_y_start_el = twiss->Value(3,3,2,col_element_number)*meter;

	double beta_loss_plane = loss_plane==HORIZONTAL_LOSS ? beta_x_start_el : beta_y_start_el;

	double sig_start_el = sqrt(beta_loss_plane * emittance);

	halo_size_sig = (JawSize_entrance + impact_factor) / sig_start_el;
}


void AcceleratorSim::setup_apertures()
{
	if(!model){cout << "No model, run build_lattice()" << endl; exit(1);}
	if(aperture_file == "") {cout << "No aperture file path" << endl; exit(1);}
	ApertureConfiguration* apc = new ApertureConfiguration(aperture_file);

	if (settings->has_key("aperture_log"))
	{
		ofstream* ApertureConfigurationLog = new ofstream(log_dir+settings->get("aperture_log"));
		if (!(*ApertureConfigurationLog)){cerr << "Failed to open ApertureConfigurationLog" << endl; exit(1);}
		apc->SetLogFile(*ApertureConfigurationLog);
		apc->EnableLogging(true);
	}

	bool enable_apertures = true;
	if (enable_apertures){
		apc->ConfigureElementApertures(model.get());
	}
	cout << "aperture load finished" << endl<< "start twiss" << endl;;
	delete apc;
}

BeamData AcceleratorSim::get_beam_data()
{
	if(!twiss){cout << "No twiss, run get_lattice_functions()" << endl; exit(1);}

	BeamData mybeam;
	mybeam.p0 = beam_momentum;

	mybeam.beta_x = twiss->Value(1,1,1,start_element_number)*meter;
	mybeam.beta_y = twiss->Value(3,3,2,start_element_number)*meter;
	mybeam.alpha_x = -twiss->Value(1,2,1,start_element_number);
	mybeam.alpha_y = -twiss->Value(3,4,2,start_element_number);

	//Dispersion
	mybeam.Dx=twiss->Value(1, 6, 3, start_element_number) / twiss->Value(6, 6, 3, start_element_number);
	mybeam.Dy=twiss->Value(3, 6, 3, start_element_number) / twiss->Value(6, 6, 3, start_element_number);
	mybeam.Dxp=twiss->Value(2, 6, 3, start_element_number) / twiss->Value(6, 6, 3, start_element_number);
	mybeam.Dyp=twiss->Value(4, 6, 3, start_element_number) / twiss->Value(6, 6, 3, start_element_number);

	mybeam.emit_x = emittance * meter;
	mybeam.emit_y = emittance * meter;

	mybeam.sig_dp = 0.0;
	mybeam.sig_z = 0.0;

	//Beam centroid
	mybeam.x0=twiss->Value(1,0,0,start_element_number);
	mybeam.xp0=twiss->Value(2,0,0,start_element_number);
	mybeam.y0=twiss->Value(3,0,0,start_element_number);
	mybeam.yp0=twiss->Value(4,0,0,start_element_number);
	mybeam.ct0=twiss->Value(5,0,0,start_element_number);

	//X-Y coupling
	mybeam.c_xy=0.0;
	mybeam.c_xyp=0.0;
	mybeam.c_xpy=0.0;
	mybeam.c_xpyp=0.0;

	//Check if beam parameters are ok.
	cout.precision(16);
	cout << "Beta x\t" << mybeam.beta_x << endl;
	cout << "Beta y\t" << mybeam.beta_y << endl;
	cout << "Alpha x\t" << mybeam.alpha_x << endl;
	cout << "Alpha y\t" << mybeam.alpha_y << endl;
	cout << "Gamma x\t" << mybeam.gamma_x() << endl;
	cout << "Gamma y\t" << mybeam.gamma_y() << endl;
	cout << "Emittance x\t" << mybeam.emit_x << endl;
	cout << "Emittance y\t" << mybeam.emit_y << endl;
	cout << "Extent x\t" << sqrt(mybeam.emit_x * mybeam.beta_x) << endl;
	cout << "Extent y\t" << sqrt(mybeam.emit_y * mybeam.beta_y) << endl;

	if(!mybeam.ok())
	{
		cerr << "Bad beam parameters: Check emittance and beta." << endl;
		exit(EXIT_FAILURE);
	}
	cout << "Beam parameters ok." << endl;

	return mybeam;
}
