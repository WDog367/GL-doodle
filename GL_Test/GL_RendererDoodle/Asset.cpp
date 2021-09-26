#include "Asset.h"

#include "PROJECT_OPTIONS.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <map>

std::string searchForAsset(std::string fname) {
	static std::string prefixes[] = { "/", "/Assets/" };
	static std::string subdirs[] = { "", "models/", "shaders/", "textures/", "scenes/" };
	static std::string exec_dir = RESOURCE_DIR;//".";

	if (std::fstream(fname)) {
		std::cerr << "Warning: using absolute path: " + fname << std::endl;
		return fname;
	}

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

// adds preprocessor defines to shader, so ubershader can be used
std::string preprocessShader(const std::string& fname, const std::string& outName, const std::map<std::string, std::string>& defines) {
	std::string fpath = searchForAsset(fname);

	if (fpath == "") {
		std::cerr << "Warning: Shader \'" << fname << "\' not found" << std::endl;
		return "";
	}

	std::string dir, name, ext;
	getNameAndExtension(dir, name, ext, fpath);
	std::string outpath = dir + ".generated." + name + "." + outName + "." + ext;

	std::fstream shader_in(fpath, std::ios_base::in);
	std::fstream shader_out(outpath, std::ios_base::out);

	if (!shader_in) {
		std::cerr << "failed to open shader output file" << std::endl;
	}

	if (!shader_out) {
		std::cerr << "failed to open shader output file" << std::endl;
	}

	int glslVersion = 0;

	while (!shader_in.eof()) {
		std::string line;
		getline(shader_in, line);

		std::istringstream ls(line);
		std::string token;
		ls >> token;


		// add defines right after version spec
		if (token == "#version") {
			shader_out << line << "\n";

			for (const std::pair<std::string, std::string>& pair : defines) {
				shader_out << "#define " << pair.first << " " << pair.second << "\n";
			}
		}
		else {
			shader_out << line << "\n";
		}
	}

	shader_in.close();
	shader_out.close();

	return outpath;
}
