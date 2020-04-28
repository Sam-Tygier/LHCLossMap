#include <iostream>
#include <fstream>

#include "MerlinVersion.h"
#include "settings.h"
#include "accelerator_sim.h"
#include "TrackingOutputASCII.h"

#include "ParticleTracker.h"
#include "SymplecticIntegrators.h"
#include "ProtonBunch.h"
#include "NANCheckProcess.h"


using namespace std;
using namespace ParticleTracking;

int main(int argc, char** argv)
{
	cout << merlin_version_info();

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


	Settings settings(settings_file);
	settings.parse_arguments(argc, argv);
	cout << "Settings:" << endl;

	string run_name = settings[ "run_name" ];
	cout << "run_name = " << run_name << endl;

	const string log_dir = settings.get("log_dir", "logs/");
	const string input_data_dir = settings.get("input_data_dir", "commondata/");
	const string result_dir = settings.get("result_dir", "results/");
	cout << "log_dir = " << log_dir << endl;
	cout << "input_data_dir = " << input_data_dir << endl;
	cout << "result_dir = " << result_dir << endl;

	string start_element = settings["start_element"];
	cout << "start_element = " << start_element << endl;
	const string tracking_integrator = settings.get("tracking_integrator", "symplectic");
	cout << "tracking_integrator = " << tracking_integrator << endl;

	auto accsim = std::make_unique<AcceleratorSim>(&settings);

	const string particle_type = settings.get("particle", "p");
	cout << "particle = " << particle_type << endl;

	unique_ptr<ProtonBunch> myBunch;

	myBunch = make_unique<ProtonBunch>(accsim->beam_momentum, 1);

	accsim->build_lattice();
	accsim->set_start_element(start_element);

	AcceleratorModel::RingIterator bline = accsim->model->GetRing(accsim->start_element_number);
	ParticleTracker* tracker = new ParticleTracker(bline,myBunch.get());
	tracker->SetLogStream(std::cout);

	if (tracking_integrator == "symplectic")
	{
		tracker->SetIntegratorSet(new ParticleTracking::SYMPLECTIC::StdISet());
	}
	else if (tracking_integrator == "transport")
	{
		tracker->SetIntegratorSet(new ParticleTracking::TRANSPORT::StdISet());
	}
	else if (tracking_integrator == "thinlens")
	{
		tracker->SetIntegratorSet(new ParticleTracking::THIN_LENS::StdISet());
	}
	else
	{
		cerr << "Unknown tracking_integrator:" << tracking_integrator <<endl;
		exit(1);
	}

	auto nancheck = new NANCheckProcess;
	nancheck->SetDetailed(1);
	nancheck->SetHaltNAN(1);
	tracker->AddProcess(nancheck);


	string trackoutasciifile = result_dir+ "/merlin_track_"+run_name+".txt";
	auto trackoutascii = new TrackingOutputASCII(trackoutasciifile);
	trackoutascii->output_all = true;
	tracker->SetOutput(trackoutascii);
	cout << "Wrote: " <<  trackoutasciifile << endl;

	Particle p(0);
	p.x() = settings.get_double("x", 0);
	p.xp() = settings.get_double("xp", 0);
	p.y() = settings.get_double("y", 0);
	p.yp() = settings.get_double("yp", 0);
	p.ct() = settings.get_double("ct", 0);
	p.dp() = settings.get_double("dp", 0);
	cout << "Initial particle: " << p << endl;


	myBunch->AddParticle(p);


	cout << "Tracking" << endl;
	tracker->Track(myBunch.get());
	cout << "Final particle: " << *(myBunch->begin()) << endl;

	cout << "Finished.\tParticle number: " << myBunch->size() << endl;


}
