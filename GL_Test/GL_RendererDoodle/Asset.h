#pragma once

#include <string>
#include <map>

std::string searchForAsset(std::string fname);

void getNameAndExtension(std::string& out_dir, std::string& out_name, std::string& out_ext, const std::string fpath);

std::string preprocessShader(const std::string& fname, const std::string& outName, const std::map<std::string, std::string>& defines);