// aCTUALMENTE NO LOS USO O NO LOS USAS ando retirando cosas para ver q falla



static bool starts_with(const std::string &value, const std::string &prefix)
{
	return value.compare(0, prefix.size(), prefix) == 0;
}

static std::string int_to_string(int value)
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}



static std::string get_header_value(const std::string &headers, const std::string &name)
{
	size_t pos = headers.find(name + ":");
	if (pos == std::string::npos)
		return "";

	pos += name.size() + 1;
	while (pos < headers.size() && (headers[pos] == ' ' || headers[pos] == '\t'))
		++pos;

	size_t end = headers.find("\r\n", pos);
	if (end == std::string::npos)
		end = headers.find('\n', pos);
	if (end == std::string::npos)
		end = headers.size();

	return headers.substr(pos, end - pos);
}