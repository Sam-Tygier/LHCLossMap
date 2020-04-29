# LHCLossMap

This is an example to be used with Merlin++
http://www.accelerators.manchester.ac.uk/merlin/

Contains several example programs. They can be used as the starting point for simulations with Merlin++.

optics_check: Loads a lattice, outputs lattice parameters and aperture survey
poincare: tracks a particle for many turns in order to create phase space stability plots
tracks: tracks a single particle and outputs its coordinates at each element
loss_map: runs a collimation simulation and outputs loss locations

## Build instructions

```
git clone https://github.com/Sam-Tygier/LHCLossMap.git
cd LHCLossMap
mkdir build
cd build
cmake -DMERLIN_ROOT_DIR=/path/to/Merlin/build ..
make

```

## Running

Each program will read settings from a text file and can take additional settings as arguments.

There is an example input in examples/2018/

Run `./create.sh` which will the optics and aperture files by running madx. Note this assumes you are running on a system with access to CERN's AFS.

Then run

```
optics_check lhc_2018_b1_h.merlin
```

or with additional options

```
loss_map lhc_2018_b1_h.merlin --seed=1 --npart=10000
```

## Thanks

Thanks to the Merlin++ users and developers who have handed down scripts and snippets that these scripts have grown from. Especially James Molson, Haroon Rafique and Maurizio Serluca.
