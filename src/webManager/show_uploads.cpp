#include "../../includes/Server.hpp"
#include <dirent.h>
#include <sys/stat.h>

static std::string escape_json(const std::string &input)
{
	std::string out;
	for (size_t i = 0; i < input.size(); ++i)
	{
		char c = input[i];
		if (c == '\\') out += "\\\\";
		else if (c == '"') out += "\\\"";
		else if (c == '\n') out += "\\n";
		else if (c == '\r') out += "\\r";
		else if (c == '\t') out += "\\t";
		else out += c;
	}
	return out;
}

std::string build_uploads_json(const std::string &www_root)
{
	std::string uploads_dir;
	if (www_root.empty())
		uploads_dir = "www/uploads";
	else
		uploads_dir = www_root + "/uploads";
	DIR *dir = opendir(uploads_dir.c_str());

	if (!dir)
		return "{\"files\":[]}";

	std::ostringstream json;
	json << "{\"files\":[";

	bool first = true;
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		std::string name = entry->d_name;
		if (name == "." || name == "..")
			continue;

		std::string full_path = uploads_dir + "/" + name;
		struct stat st;
		if (stat(full_path.c_str(), &st) != 0 || !S_ISREG(st.st_mode))
			continue;

		if (!first)
			json << ",";
		first = false;

		json << "{\"name\":\"" << escape_json(name) << "\",\"size\":" << st.st_size << "}";
	}

	closedir(dir);
	json << "]}";
	return json.str();
}
