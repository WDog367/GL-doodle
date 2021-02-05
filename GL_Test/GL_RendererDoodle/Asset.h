#pragma once

#include <string>

std::string searchForAsset(std::string fname);

void getNameAndExtension(std::string& out_dir, std::string& out_name, std::string& out_ext, const std::string fpath);