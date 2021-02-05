#include "Asset.h"

#include "PROJECT_OPTIONS.h"

#include <fstream>
#include <iostream>

std::string searchForAsset(std::string fname) {
	static std::string prefixes[] = { "/", "/Assets/" };
	static std::string subdirs[] = { "", "models/", "shaders/", "textures/", "scenes/" };
	static std::string exec_dir = RESOURCE_DIR;//".";

	for (const std::string& prefix : prefixes) {
		for (const std::string& subdir : subdirs) {
			std::string fpath = exec_dir + prefix + subdir + fname;

			std::fstream testfile(fpath);

			if (testfile) {
				return fpath;
			}
		}
	}

	std::cerr << "Warning: Asset \'" << fname << "\' not found" << std::endl;
	return "";
}

void getNameAndExtension(std::string& out_dir, std::string& out_name, std::string& out_ext, const std::string fpath) {
	int last_dot = fpath.rfind('.');
	int last_slash = fpath.rfind('/');

	out_dir = fpath.substr(0, last_slash + 1);
	out_name = fpath.substr(last_slash + 1, last_dot);
	out_ext = fpath.substr(last_dot + 1, fpath.length());
}