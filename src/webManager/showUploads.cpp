#include "Server.hpp"

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

std::string generateDirectoryListing(std::string path, std::string route) {
    DIR* dir;
    struct dirent* entry;
    std::stringstream html;

    html << "<html><head><title>Index of " << route << "</title>";
    html << "<style>"
         << "body { font-family: sans-serif; padding: 20px; }"
         << "table { width: 100%; border-collapse: collapse; }"
         << "th, td { text-align: left; padding: 8px; border-bottom: 1px solid #ddd; }"
         << "tr:hover { background-color: #f5f5f5; }"
         << ".btn-del { color: white; background: #ff4444; border: none; padding: 5px 10px; cursor: pointer; border-radius: 4px; }"
         << "</style></head><body>";
    
    html << "<h1>Index of " << route << "</h1>";
    html << "<table><thead><tr><th>Name</th><th>Action</th></tr></thead><tbody>";

    if ((dir = opendir(path.c_str())) != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            std::string name = entry->d_name;
            if (name == ".") continue; // Ignorar directorio actual

            html << "<tr>";
            // Enlace para VER/DESCARGAR
            html << "<td><a href=\"" << route << "/" << name << "\">" << name << "</a></td>";
            
            // Botón para ELIMINAR (usando JavaScript para enviar un método DELETE)
            if (name != "..") {
                html << "<td>"
                     << "<button class='btn-del' onclick=\"deleteFile('" << route << "/" << name << "')\">Delete</button>"
                     << "</td>";
            } else {
                html << "<td></td>";
            }
            html << "</tr>";
        }
        closedir(dir);
    }

    html << "</tbody></table>";

    // Script para manejar la eliminación
    html << "<script>"
         << "function deleteFile(path) {"
         << "  if (confirm('Are you sure?')) {"
         << "    fetch(path, { method: 'DELETE' })"
         << "    .then(res => { if(res.ok) location.reload(); else alert('Error deleting file'); });"
         << "  }"
         << "}"
         << "</script>";
    
    html << "</body></html>";
    return html.str();
}
