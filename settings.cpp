#include "settings.h"
#include <fstream>
#include <iostream>
#include <cctype>
#include <algorithm>

static std::string trim(std::string s)
{
	if (s.size() == 0) {return s;}
	int n1 =0, n2=s.size()-1;
	while(std::isspace(s[n1]) && n1<n2) n1++;
	while(std::isspace(s[n2]) && n2>n1) n2--;

	std::string ss = s.substr(n1, n2-n1+1);
	return ss;
}

Settings::Settings(const std::string& fname): setting_store()
{
	std::ifstream in_file(fname);
	if(!in_file.good())
	{
		std::cerr << "Could not open " << fname << std::endl;
		exit(1);
	}
	std::string line;
	int n = 1;
	while(getline(in_file, line))
	{
		line = line.substr(0, line.find("#"));
		if (line.size() == 0) continue;
		if (line.find("=") == std::string::npos)
		{
			std::cerr << "No '=' on line " << n << ":" << std::endl << line << std::endl;
			exit(1);
		}
		size_t eq_ch = line.find("=");
		std::string key = line.substr(0,eq_ch);
		std::string value = line.substr(eq_ch+1, std::string::npos);
		key = trim(key);
		value = trim(value);
		//cout <<""<<key << "="<< value <<""<< endl;
		setting_store[key] = value;
		n++;
	}
}

void Settings::parse_arguments(int argc, char** argv)
{
	for (int i = 1; i < argc; i++)
	{
		std::string arg = argv[i];
		if(arg.substr(0,2) == "--")
		{
			std::string key, value;
			if (arg.find("=") == std::string::npos)
			{
				key = arg.substr(2,std::string::npos);
				if (i == argc-1)
				{
					std::cerr << "No value for command line argument: " << arg  << std::endl;
					exit(1);
				}
				value = argv[i+1];
				i++;
			}
			else
			{
				size_t eq_ch = arg.find("=");
				key = arg.substr(2,eq_ch-2);
				value = arg.substr(eq_ch+1, std::string::npos);
			}
			key = trim(key);
			value = trim(value);
			setting_store[key] = value;
		}
	}
}

bool Settings::has_key(const std::string& k) const
{
	return setting_store.count(k);
}

std::string Settings::operator [](const std::string& k) const
{
	if(setting_store.count(k)){
		return setting_store.at(k);
	}
	else
	{
		std::cerr << "No key '"<< k <<"' in settings" << std::endl;
		exit(1);
	}
}

std::string Settings::get(const std::string& k) const
{
	return (*this)[k];
}

double Settings::get_double(const std::string& k) const
{
	return std::stod((*this)[k]);
}

int Settings::get_int(const std::string& k) const
{
	return std::stoi((*this)[k]);
}

bool Settings::get_bool(const std::string& k) const
{
	std::string val = (*this)[k];
	std::transform(val.begin(), val.end(), val.begin(), ::tolower);
	for(std::string true_val: {"true", "yes", "t"})
	{
		if(val == true_val) return true;
	}
	for(std::string false_val: {"false", "no", "f"})
	{
		if(val == false_val) return false;
	}

	std::cerr << "Value of '"<< k <<"' not recognised as boolean (true, false)" << std::endl;
	exit(1);
}

static std::vector<std::string> split_string(const std::string s, const std::string sep, const bool trim_string)
{
	std::vector<std::string> all_found;
	size_t pos = 0, new_pos;
	while(new_pos != std::string::npos)
	{
		new_pos = s.find(sep, pos);
		std::string found = s.substr(pos, new_pos-pos);
		if(trim_string) found = trim(found);
		all_found.push_back(found);
		pos = new_pos + sep.size();
	}
	return all_found;
}

#include <functional>
std::vector<double> Settings::get_dv(const std::string& k) const
{
	auto strings = split_string(this->get(k), ",", true);
	std::vector<double> vals;
	try
	{
		std::transform(strings.begin(), strings.end(), std::back_inserter(vals), [](std::string s){return std::stod(s);});
	}
	catch (std::invalid_argument &e)
	{
		throw std::invalid_argument("Could not convert key '" + k + "', value '"+this->get(k)+"' to vector of doubles");
	}

	return vals;
}

std::string Settings::get(const std::string& k, const std::string& dflt) const
{
	if(has_key(k))
	{
		return get(k);
	}
	else
	{
		return dflt;
	}
}
double Settings::get_double(const std::string& k, const double dflt) const
{
	if(has_key(k))
	{
		return get_double(k);
	}
	else
	{
		return dflt;
	}
}
int Settings::get_int(const std::string& k, const int dflt) const
{
	if(has_key(k))
	{
		return get_int(k);
	}
	else
	{
		return dflt;
	}
}
bool Settings::get_bool(const std::string& k, const bool dflt) const
{
	if(has_key(k))
	{
		return get_bool(k);
	}
	else
	{
		return dflt;
	}
}
std::vector<double> Settings::get_dv(const std::string& k, const std::vector<double> dflt) const
{
	if(has_key(k))
	{
		return get_dv(k);
	}
	else
	{
		return dflt;
	}
}

std::ostream& operator << (std::ostream& os, const Settings& ob)
{
	for (auto const &it: ob.setting_store)
	{
		os << it.first << " = " << it.second << std::endl;
	}
	return os;
}

