#include <iostream>
#include <fstream>
#include <vector>

#include "settings.h"
#include "accelerator_sim.h"

#include "LatticeFunctions.h"
#include "PhysicalUnits.h"
#include "PhysicalConstants.h"

#include "ParticleTracker.h"
#include "SymplecticIntegrators.h"
#include "CollimatorAperture.h"
#include "CollimateProtonProcess.h"
#include "LossMapCollimationOutput.h"
#include "DetailedCollimationOutput.h"
#include "ApertureSurvey.h"

using namespace std;
using namespace PhysicalUnits;
using namespace PhysicalConstants;
using namespace ParticleTracking;

int main(int argc, char* argv[])
{
	cout << "Loss map" << endl;
	string settings_file;
	if (argc >= 2)
	{
		settings_file = argv[1];
	}else
	{
		cout << "Please give settings file" << endl;
		return 1;
	}

	cout << "Optics check" << endl;
	Settings settings(settings_file);
	settings.parse_arguments(argc, argv);

	cout << "Settings:" << endl;

	auto accsim = std::make_unique<AcceleratorSim>(&settings);

	string run_name = settings[ "run_name" ];

	const string log_dir = settings.get("log_dir", "logs/");
	const string input_data_dir = settings.get("input_data_dir", "commondata/");
	const string result_dir = settings.get("result_dir", "results/");

	string start_element = settings["start_element"];
	const string tracking_integrator = settings.get("tracking_integrator", "symplectic");
	const string scattering_model = settings.get("scattering_model", "merlin");

	accsim->build_lattice();
	accsim->set_start_element(start_element);

	cout << "Found start element '" << start_element << "' at  position " << accsim->start_element_number << endl;

	accsim->get_lattice_functions();

	string latttice_outfile = result_dir+"LatticeFunctions_" + run_name + ".dat";
	ofstream latticeFunctionLog(latttice_outfile);
	if (!latticeFunctionLog){cerr << "Failed to open: " << latttice_outfile << endl; return 1;}

	latticeFunctionLog << "#S X PX Y PY T DELTAP BETX ALFX BETY ALFY LF163 LF263 LF363 LF463 LF663 GAMX GAMY" <<endl;

	latticeFunctionLog.precision(16);
	accsim->twiss->PrintTable(latticeFunctionLog);
	cout << "Done: " << latttice_outfile <<endl;

	cout << "At start element: "<< start_element <<endl;
	cout.precision(12);
	cout << "Beta x,y: " << accsim->twiss->Value(1,1,1,accsim->start_element_number)*meter << " " << accsim->twiss->Value(3,3,2,accsim->start_element_number)*meter << endl;
	cout << "Alpha x,y: " << -accsim->twiss->Value(1,2,1,accsim->start_element_number)*meter << " " << -accsim->twiss->Value(3,4,2,accsim->start_element_number)*meter << endl;
	cout << "Gamma x,y: " << accsim->twiss->Value(2,2,1,accsim->start_element_number)*meter << " " << accsim->twiss->Value(4,4,2,accsim->start_element_number)*meter << endl;

	if (accsim->aperture_file != ""){
		accsim->setup_apertures();

		string aperture_outfile = result_dir+"ApertureSurvey_" + run_name + ".dat";
		ofstream* ap_test_log = new ofstream(aperture_outfile);
		//ApertureSurvey::ApertureSurvey(accsim->model.get(), ap_test_log, ApertureSurvey::abs_distance, 0.1);
		ApertureSurvey::ApertureSurvey(accsim->model.get(), ap_test_log, ApertureSurvey::points_per_element, 3);
		cout << "Done: " << aperture_outfile <<endl;
	}

	return 0;
}
