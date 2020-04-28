#ifndef SETTINGS_H
#define SETTINGS_H

#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>


/*** Class to read configuration settings from a text file
 *
 * File should contain lines like:
 *
 *     keyword1=value1
 *     keyword2=value2
 *
 * '#' can be used for comments
 *
 */
class Settings
{
	private:
		std::unordered_map<std::string, std::string> setting_store;

	public:
		/// Read settings from file
		Settings(const std::string& fname);

		void parse_arguments(int argc, char** argv);


		/// Access value by keyword
		std::string operator[](const std::string& k) const;

		/// Check if a keyword exists
		bool has_key(const std::string& k) const;

		/// Get a value by keyword
		std::string get(const std::string& k) const;
		/// Get a value by keyword, converted to double
		double get_double(const std::string& k) const;
		/// Get a value by keyword, converted to int
		int get_int(const std::string& k) const;
		/// Get a value by keyword, converted to bool
		bool get_bool(const std::string& k) const;
		/// Get a value by keyword, converted to vector of doubles
		std::vector<double> get_dv(const std::string& k) const;

		/// Get a value by keyword. Return dflt if keyword is not set.
		std::string get(const std::string& k, const std::string& dflt) const;
		/// Get a value by keyword, converted to double. Return dflt if keyword is not set.
		double get_double(const std::string& k, const double dflt) const;
		/// Get a value by keyword, converted to int. Return dflt if keyword is not set.
		int get_int(const std::string& k, const int dflt) const;
		/// Get a value by keyword, converted to bool. Return dflt if keyword is not set.
		bool get_bool(const std::string& k, const bool dflt) const;
		/// Get a value by keyword, converted to vector of doubles. Return dflt if keyword is not set.
		std::vector<double> get_dv(const std::string& k, const std::vector<double> dflt) const;

		/// Output the current settings
		friend std::ostream& operator << (std::ostream& os, const Settings& ob);
};


#endif
