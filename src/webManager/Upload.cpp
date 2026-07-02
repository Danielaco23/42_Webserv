#include "Server.hpp"
#include <fstream>
#include <sys/stat.h>
#include <cctype>
#include <cerrno>

static std::string to_lower_copy(const std::string &value)
{
    std::string lowered = value;
    for (size_t i = 0; i < lowered.size(); ++i)
        lowered[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lowered[i])));
    return lowered;
}

static std::string trim_copy(const std::string &value)
{
    size_t start = 0;
    while (start < value.size() && (value[start] == ' ' || value[start] == '\t' || value[start] == '\r' || value[start] == '\n'))
        ++start;

    size_t end = value.size();
    while (end > start && (value[end - 1] == ' ' || value[end - 1] == '\t' || value[end - 1] == '\r' || value[end - 1] == '\n'))
        --end;

    return value.substr(start, end - start);
}

static bool extract_header_value(const std::string &headers_part, const std::string &name, std::string &value)
{
    std::string lower_headers = to_lower_copy(headers_part);
    std::string lower_name = to_lower_copy(name);
    size_t header_pos = lower_headers.find(lower_name + ":");
    if (header_pos == std::string::npos)
        return false;

    size_t colon_pos = headers_part.find(':', header_pos);
    if (colon_pos == std::string::npos)
        return false;

    size_t start = colon_pos + 1;
    while (start < headers_part.size() && (headers_part[start] == ' ' || headers_part[start] == '\t'))
        ++start;

    size_t end = headers_part.find("\r\n", start);
    if (end == std::string::npos)
        end = headers_part.find("\n", start);
    if (end == std::string::npos)
        end = headers_part.size();

    value = trim_copy(headers_part.substr(start, end - start));
    return !value.empty();
}

static std::string basename_from_upload_name(const std::string &name)
{
	size_t slash = name.find_last_of('/');
	size_t backslash = name.find_last_of('\\');
	size_t pos = std::string::npos;

	if (slash != std::string::npos && backslash != std::string::npos)
		pos = (slash > backslash) ? slash : backslash;
	else if (slash != std::string::npos)
		pos = slash;
	else
		pos = backslash;

	if (pos == std::string::npos)
		return name;
	return name.substr(pos + 1);
}

static bool is_safe_upload_filename(const std::string &name)
{
	if (name.empty() || name == "." || name == "..")
		return false;
	if (name.find("..") != std::string::npos)
		return false;
	if (name.find('/') != std::string::npos || name.find('\\') != std::string::npos)
		return false;

	for (size_t i = 0; i < name.size(); ++i)
	{
		unsigned char c = static_cast<unsigned char>(name[i]);
		if (c < 32 || c == 127)
			return false;
	}
	return true;
}

static bool is_suspicious_raw_filename(const std::string &raw_name)
{
	if (raw_name.empty())
		return true;
	if (raw_name.find("..") != std::string::npos)
		return true;
	for (size_t i = 0; i < raw_name.size(); ++i)
	{
		unsigned char c = static_cast<unsigned char>(raw_name[i]);
		if (c < 32)
			return true;
	}
	return false;
}

/**
 * @brief Reads the full body of a POST request from the socket.
 * @param client_fd Socket file descriptor.
 * @param content_length Expected number of bytes to read.
 * @return Body content as a string.

std::string read_request_body(int client_fd, size_t content_length)
{
	std::string body;
	const size_t chunk_size = 8192;
	char buffer[chunk_size];
	size_t remaining = content_length;

	while (remaining > 0)
	{
		size_t to_read = chunk_size;
		if (remaining < chunk_size)
			to_read = remaining;
		ssize_t n = recv(client_fd, buffer, to_read, 0);
		if (n < 0)
		{
			if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			break;
		}
		if (n == 0)
			break;
		body.append(buffer, n);
		remaining -= n;
	}
	return body;
}*/

/**
 * @brief Saves uploaded file to the filesystem.
 * @param www_root Document root directory.
 * @param filename Original filename from upload.
 * @param content File content data.
 * @return true if saved successfully, false otherwise.
 */
bool save_uploaded_file(const std::string &www_root, const std::string &filename, const std::string &content)
{
	if (is_suspicious_raw_filename(filename))
		return false;

	std::string safe_name = basename_from_upload_name(filename);
	if (!is_safe_upload_filename(safe_name))
		return false;

	std::string uploads_dir;
	if (www_root.empty())
		uploads_dir = "www/uploads";
	else
		uploads_dir = www_root + "/uploads";
	
	// Ensure uploads directory exists
	struct stat st = {};
	if (stat(uploads_dir.c_str(), &st) == -1)
		mkdir(uploads_dir.c_str(), 0755);
	
	std::string file_path = uploads_dir + "/" + safe_name;
	std::ofstream out(file_path.c_str(), std::ios::binary);
	if (!out)
		return false;
	
	out.write(content.data(), content.size());
	out.close();
	std::cout << "Saved upload: " << file_path << " (" << content.size() << " bytes)" << std::endl;
	return true;
}

/**
 * @brief Extracts filename and content from a multipart form part.
 * @param part Raw multipart part data.
 * @param filename Output filename extracted from Content-Disposition.
 * @param content Output file content.
 * @return true if successfully extracted, false otherwise.
 */
bool extract_multipart_file(
    const std::string &part,
    std::string &filename,
    std::string &content
)
{
    filename.clear();
    content.clear();

    size_t sep = part.find("\r\n\r\n");
    if (sep == std::string::npos)
        return false;

    std::string headers = part.substr(0, sep);
    content = part.substr(sep + 4);

    std::string cd;
    if (!extract_header_value(headers, "Content-Disposition", cd))
        return false;

    std::string lower_cd = to_lower_copy(cd);
    if (lower_cd.find("form-data") == std::string::npos)
        return false;

    size_t fn_pos = lower_cd.find("filename=");
    if (fn_pos == std::string::npos)
        return false;

    fn_pos += 9;
    while (fn_pos < cd.size() && (cd[fn_pos] == ' ' || cd[fn_pos] == '\t'))
        ++fn_pos;

    size_t fn_end = cd.find(';', fn_pos);
    if (fn_end == std::string::npos)
        fn_end = cd.size();

    filename = trim_copy(cd.substr(fn_pos, fn_end - fn_pos));
    if (filename.size() >= 2 && filename[0] == '"' && filename[filename.size() - 1] == '"')
    {
        filename = filename.substr(1, filename.size() - 2);
    }
    return (!filename.empty());
}

/**
 * @brief Processes multipart form data and saves all uploaded files.
 * @param body Complete POST request body.
 * @param boundary Multipart boundary string from Content-Type header.
 * @param www_root Document root directory.
 * @return Number of files successfully saved.
 */
int process_uploads(const std::string &body, const std::string &boundary, const std::string &www_root)
{
	int count = 0;
	if (boundary.empty())
		return count;
	
	size_t part_start = body.find(boundary);
	while (part_start != std::string::npos)
	{
		part_start += boundary.size();
		size_t part_end = body.find(boundary, part_start);
		if (part_end == std::string::npos)
			break;
		
		std::string part = body.substr(part_start, part_end - part_start);
		std::string filename, content;
		if (extract_multipart_file(part, filename, content))
		{
			std::cout << "Processing uploaded file: " << filename << " (" << content.size() << " bytes)" << std::endl;
			if (save_uploaded_file(www_root, filename, content))
				++count;
		}
		
		part_start = part_end;
	}
	return count;
}
