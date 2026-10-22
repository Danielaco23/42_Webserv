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



static std::string get_header_value(const std::string &headers, const std::string &header_name)
{
    std::string search;
    size_t pos;
    size_t end;
    std::string value;

    search = header_name + ":";
    pos = headers.find(search);

    if (pos == std::string::npos)
        return "";

    pos += search.size();

    while (pos < headers.size()
        && (headers[pos] == ' ' || headers[pos] == '\t'))
        pos++;

    end = headers.find("\r\n", pos);
    if (end == std::string::npos)
        end = headers.size();

    value = headers.substr(pos, end - pos);

    while (!value.empty()
        && (value[value.size() - 1] == ' '
        || value[value.size() - 1] == '\t'))
        value.erase(value.size() - 1);

    return value;
}