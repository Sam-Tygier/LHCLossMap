#include <iostream>
#include <fstream>

#include "MerlinVersion.h"
#include "settings.h"
#include "accelerator_sim.h"

#include "ParticleTracker.h"
#include "SymplecticIntegrators.h"
#include "ProtonBunch.h"
//#include "IonBunch.h"
#include "ParticleDistributionGenerator.h"



using namespace std;
using namespace ParticleTracking;

class PoincareDistributionGenerator: public ParticleDistributionGenerator
{
public:

	PoincareDistributionGenerator(double min, double step_size, char plane, vector<double> * al):
	pmin(min), pstep_size(step_size), pplane(plane), amplitude_list(al)
	{
		if(pplane != 'x' && pplane != 'y')
		{
			cout << "PoincareDistributionGenerator: plane must be 'x' or 'y'" <<endl;
			exit(1);
		}
		amplitude_list->clear();
		amplitude_list->push_back(0);
	}

	virtual PSvector GenerateFromDistribution() const;
private:
	double pmin, pstep_size;
	char pplane;
	mutable int count = 1;
	vector<double> * amplitude_list;
};

PSvector PoincareDistributionGenerator::GenerateFromDistribution() const
{
	PSvector p(0);
	double pos = pmin + pstep_size * count;
	cout << "PDG: count = "<< count <<  "  pos = " << pos << endl;

	if(pplane == 'x')
	{
		p.x() = pos;
	}
	else if (pplane == 'y')
	{
		p.y() = pos;
	}

	amplitude_list->push_back(pos);
	count += 1;
	return p;
}


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

	int nturns = settings.get_int("nturns");
	cout << "nturns: " << nturns<< endl;

	auto accsim = std::make_unique<AcceleratorSim>(&settings);

	accsim->build_lattice();
	accsim->set_start_element(start_element);

	cout << "Found start element '" << start_element << "' at  position " << accsim->start_element_number << endl;
	accsim->get_lattice_functions();

	AcceleratorModel::RingIterator bline = accsim->model->GetRing(accsim->start_element_number);


	BeamData mybeam = accsim->get_beam_data();
	mybeam.charge = accsim->beam_charge;

	double poincare_min =  settings.get_double("poincare_min", 0);
	double poincare_max =  settings.get_double("poincare_max");
	double poincare_steps =  settings.get_int("poincare_steps");
	cout << "poincare_min = " << poincare_min << endl;
	cout << "poincare_max = " << poincare_max << endl;
	cout << "poincare_steps = " << poincare_steps << endl;

	double poincare_step_size = (poincare_max - poincare_min) / (poincare_steps - 1);
	cout << "poincare_step_size = " << poincare_step_size << endl;

	for(char plane: {'x', 'y'})
	{
		vector<double> amplitude_list;
		auto myBunch = make_unique<ParticleBunch>(poincare_steps,
	          PoincareDistributionGenerator(poincare_min, poincare_step_size, plane, &amplitude_list), mybeam, nullptr);

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

		string pout_fname = result_dir+"/poincare_"+ plane + "_" + run_name + ".dat";
		ofstream pout(pout_fname);
		pout << "#turn id x xp y yp ct dp init_amp\n";
		pout << "#poincare min:"<< poincare_min << " max:"<< poincare_max << " steps:"<< poincare_steps <<"\n";

		pout << "#beam alpha_x:"<< mybeam.alpha_x << " beta_x:"<< mybeam.beta_x << " gamma_x:"<< mybeam.gamma_x() << " emit_x:" << mybeam.emit_x<< " alpha_y:"<< mybeam.alpha_y << " beta_y:"<< mybeam.beta_y << " gamma_y:"<< mybeam.gamma_y() << " emit_y:" << mybeam.emit_y <<"\n";

		for(int n = 0; n < nturns; n++)
		{
			for(auto &p: myBunch->GetParticles())
			{
				pout.precision(15);
				pout << n << " " << p.id() << " " <<p.x() << " " <<p.xp() << " "<<p.y() << " "<<p.yp() << " "<<p.ct() << " "<<p.dp() << " " << amplitude_list.at(p.id()) <<endl;
			}
			tracker->Track(myBunch.get());
		}
		cout << "Wrote " << pout_fname <<endl;
	}
}
