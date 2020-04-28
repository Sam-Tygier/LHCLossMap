#include <iostream>
#include <fstream>

#include "settings.h"
#include "accelerator_sim.h"

#include "RandomNG.h"
#include "PhysicalUnits.h"
#include "PhysicalConstants.h"
#include "ProtonBunch.h"
#include "HaloParticleDistributionGenerator.h"
#include "BunchFilter.h"
#include "ParticleTracker.h"
#include "SymplecticIntegrators.h"
#include "CollimatorAperture.h"
#include "CollimateProtonProcess.h"
#include "LossMapCollimationOutput.h"
#include "DetailedCollimationOutput.h"
#include "ScatteringModelsMerlin.h"
#include "MonitorProcess.h"
#include "MerlinVersion.h"

using namespace std;
using namespace PhysicalUnits;
using namespace PhysicalConstants;
using namespace ParticleTracking;

int main(int argc, char* argv[])
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
	//cout << settings << endl;

	unsigned int seed1 = settings.get_int("seed");
	unsigned int seed2 = settings.get_int("seed2", 0);

	RandomNG::init({seed1, seed2});
	cout << "Seeds " << seed1 << " " << seed2 << endl;

	string run_name = settings[ "run_name" ]+"_"+to_string(seed1);

	int npart = settings.get_int("npart");
	int nturns = settings.get_int("nturns");
	cout << "npart: " << npart<< endl;
	cout << "nturns: " << nturns<< endl;

	string loss_plane_s = settings["loss_plane"];

	const string log_dir = settings.get("log_dir", "logs/");
	const string result_dir = settings.get("result_dir", "results/");

	auto accsim = std::make_unique<AcceleratorSim>(&settings);

	loss_map_mode_t loss_plane;
	if (loss_plane_s == "H" || loss_plane_s == "h") loss_plane = HORIZONTAL_LOSS;
	else if (loss_plane_s == "V" || loss_plane_s == "v") loss_plane = VERTICAL_LOSS;
	else {cout << "Unknown loss_plane" << endl; exit(1);}
	cout << "loss_plane_s: " << loss_plane_s<< endl;

	string start_element = settings["start_element"];
	const string tracking_integrator = settings.get("tracking_integrator", "symplectic");
	const string scattering_model = settings.get("scattering_model", "merlin");

	accsim->build_lattice();
	accsim->set_start_element(start_element);

	cout << "Found start element '" << start_element << "' at  position " << accsim->start_element_number << endl;

	accsim->get_lattice_functions();
	accsim->loss_plane = loss_plane;

	accsim->setup_apertures();
	accsim->setup_collimators();
	accsim->set_halo_size_from_aperture(start_element);

	BeamData mybeam = accsim->get_beam_data();
	mybeam.charge = accsim->beam_charge/npart;

	ProtonBunch* myBunch;
	switch (loss_plane){
		case HORIZONTAL_LOSS:
		{
			HorizontalHaloParticleBunchFilter bunchfilter_h;
			bunchfilter_h.SetHorizontalLimit(accsim->JawSize_entrance);
			bunchfilter_h.SetHorizontalOrbit(accsim->h_offset_entrance);
			myBunch = new ProtonBunch(npart, HorizonalHalo2ParticleDistributionGenerator(accsim->halo_size_sig), mybeam, &bunchfilter_h);
			break;
		}
		case VERTICAL_LOSS:
		{
			VerticalHaloParticleBunchFilter bunchfilter_v;
			bunchfilter_v.SetVerticalLimit(accsim->JawSize_entrance);
			myBunch = new ProtonBunch(npart, VerticalHalo2ParticleDistributionGenerator(accsim->halo_size_sig), mybeam, &bunchfilter_v);
			break;
		}
		default:
			cerr << "Unknown loss_plane" << endl;
			exit(1);
	}

	myBunch->SetMacroParticleCharge(mybeam.charge);

	if(0)
	{
		ofstream bunchout("init_bunch_"+run_name+".dat");
		if (!bunchout.good()){cerr << "Failed to open bunchout" << endl; return 1;}
		myBunch->Output(bunchout);
	}

	AcceleratorModel::RingIterator bline = accsim->model->GetRing(accsim->start_element_number);
	ParticleTracker* tracker = new ParticleTracker(bline,myBunch);
	tracker->SetLogStream(std::cout);

	if (tracking_integrator == "symplectic")
	{
		tracker->SetIntegratorSet(new ParticleTracking::SYMPLECTIC::StdISet());
	}
	else if (tracking_integrator == "transport")
	{
		tracker->SetIntegratorSet(new ParticleTracking::TRANSPORT::StdISet());
	}
	else
	{
		cerr << "Unknown tracking_integrator:" << tracking_integrator <<endl;
		exit(1);
	}

	double output_bin_size = 0.1;
	LossMapCollimationOutput* myLossOutput = new LossMapCollimationOutput(tencm);
	CollimateProtonProcess* myCollimateProcess;
	myCollimateProcess=new CollimateProtonProcess(2,4);
	myCollimateProcess->SetCollimationOutput(myLossOutput);

	DetailedCollimationOutput* myDetailedOutput;
	if(settings.has_key("detailed_loss_output"))
	{
		myDetailedOutput = new DetailedCollimationOutput();
		myDetailedOutput->AddIdentifier(settings[ "detailed_loss_output" ]);
		myCollimateProcess->SetCollimationOutput(myDetailedOutput);
	}

	ScatteringModel* myScatter;
	if (scattering_model == "merlin")
	{
		myScatter = new ScatteringModelMerlin;
	}
	else if(scattering_model == "sixtrack")
	{
		myScatter = new ScatteringModelSixTrack;
	}
	else if(scattering_model == "sixtrackelastic")
	{
		myScatter = new ScatteringModelSixTrackElastic;
	}
	else if(scattering_model == "sixtrackioniz")
	{
		myScatter = new ScatteringModelSixTrackIoniz;
	}
	else if(scattering_model == "sixtracksd")
	{
		myScatter = new ScatteringModelSixTrackSD;
	}
	else
	{
		cerr << "Unknown scattering_model:" << scattering_model <<endl;
		exit(1);
	}

	myCollimateProcess->SetScatteringModel(myScatter);
	stringstream loststr;

	myCollimateProcess->ScatterAtCollimator(true);

	// Sets maximum allowed loss percentage at a single collimator.
	myCollimateProcess->SetLossThreshold(101.0);

	//Add Collimation process to the tracker.
	myCollimateProcess->SetOutputBinSize(output_bin_size);
	tracker->AddProcess(myCollimateProcess);

	int initial_turn = 1;
	for (int turn=initial_turn; turn<=nturns; turn++)
	{
		cout << "Turn " << turn <<"\tParticle number: " << myBunch->size() << endl;

		tracker->Track(myBunch);

		if(myBunch->size() <= 1)
		{
			break;
		}
	}

	cout << "npart: " << npart << endl;
	cout << "left: " << myBunch->size() << endl;
	cout << "absorbed: " << npart - myBunch->size() << endl;

	myLossOutput->Finalise();
	string col_output_name = result_dir+ "/losses_"+run_name+".txt";
	ofstream* col_output = new ofstream(col_output_name);
	if(!col_output->good())
	{
		std::cerr << "Could not open collimation loss file: " << col_output_name << std::endl;
		exit(EXIT_FAILURE);
	}
	*col_output << "# From merlin npart="<< npart << " nturns="<< nturns <<endl;
	myLossOutput->Output(col_output);

	if(settings.has_key("detailed_loss_output"))
	{
		string col_output_name2 = result_dir+ "/detailed_losses_"+run_name+".txt";
		ofstream* col_output2 = new ofstream(col_output_name2);
		if(!col_output->good())
		{
			std::cerr << "Could not open detailed collimation loss file: "<< col_output_name2 << std::endl;
			exit(EXIT_FAILURE);
		}
		*col_output2 << "# From merlin npart="<< npart << " nturns="<< nturns <<endl;
		myDetailedOutput->Output(col_output2);
	}

	delete tracker;
	return 0;
}
