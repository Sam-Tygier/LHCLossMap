#ifndef ACCELERATOR_SIM_H
#define ACCELERATOR_SIM_H

#include <string>
#include <memory>
#include <map>

#include "BeamData.h"
#include "PhysicalConstants.h"


class CollimatorDatabase;
class AcceleratorModel;
class LatticeFunctionTable;
//class Dispersion;
class Settings;


enum loss_map_mode_t {HORIZONTAL_LOSS, VERTICAL_LOSS};

class AcceleratorSim
{
public:
	std::string log_dir, input_data_dir, result_dir;
	std::string optics_file, aperture_file, collimator_file;
	std::string start_element;

	size_t start_element_number;
	loss_map_mode_t loss_plane;

	std::unique_ptr<CollimatorDatabase> collimator_db;
	std::unique_ptr<AcceleratorModel> model;
	std::unique_ptr<LatticeFunctionTable> twiss;

	double halo_size_sig, normalized_emittance, emittance;
	double h_offset_entrance, JawSize_entrance, JawPosition_entrance;

	Settings* settings;

	std::string particle_type;
	double beam_energy;
	double beam_momentum;
	double mass;
	double mass_mev;
	double charge;
	double beam_charge; // total beam charge
	double impact_factor;
	double beam_kinetic_energy, beta, gamma;
	double beam_rigidity;

	AcceleratorSim(Settings *settings_);
	~AcceleratorSim();

	void build_lattice();
	void get_lattice_functions();

	void set_start_element(std::string start_element_);
	void setup_collimators();
	void set_halo_size_from_aperture(std::string element_name);
	void setup_apertures();
	BeamData get_beam_data();

private:
	void set_emittance();
};


#endif
