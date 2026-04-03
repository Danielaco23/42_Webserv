#include "../includes/Server.hpp"
#include <fstream>
#include <sys/stat.h>

/**
 * @brief Reads the full body of a POST request from the socket.
 * @param client_fd Socket file descriptor.
 * @param content_length Expected number of bytes to read.
 * @return Body content as a string.
 */
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
		if (n <= 0)
			break;
		body.append(buffer, n);
		remaining -= n;
	}
	return body;
}

/**
 * @brief Saves uploaded file to the filesystem.
 * @param www_root Document root directory.
 * @param filename Original filename from upload.
 * @param content File content data.
 * @return true if saved successfully, false otherwise.
 */
bool save_uploaded_file(const std::string &www_root, const std::string &filename, const std::string &content)
{
	std::string uploads_dir;
	if (www_root.empty())
		uploads_dir = "www/uploads";
	else
		uploads_dir = www_root + "/uploads";
	
	// Ensure uploads directory exists
	struct stat st = {};
	if (stat(uploads_dir.c_str(), &st) == -1)
		mkdir(uploads_dir.c_str(), 0755);
	
	std::string file_path = uploads_dir + "/" + filename;
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
bool extract_multipart_file(const std::string &part, std::string &filename, std::string &content)
{
	// Find Content-Disposition header to extract filename
	size_t disp_pos = part.find("Content-Disposition:");
	if (disp_pos == std::string::npos)
		return false;
	
	size_t filename_pos = part.find("filename=\"", disp_pos);
	if (filename_pos == std::string::npos)
		return false;
	
	filename_pos += 10;
	size_t filename_end = part.find("\"", filename_pos);
	if (filename_end == std::string::npos)
		return false;
	
	filename = part.substr(filename_pos, filename_end - filename_pos);
	
	// Find the blank line separating headers from content
	size_t body_start = part.find("\r\n\r\n", disp_pos);
	if (body_start == std::string::npos)
		body_start = part.find("\n\n", disp_pos);
	else
		body_start += 4;
	
	if (body_start == std::string::npos)
		return false;
	
	// Content ends at the last CRLF before the boundary
	size_t body_end = part.rfind("\r\n");
	if (body_end == std::string::npos)
		body_end = part.rfind("\n");
	if (body_end == std::string::npos)
		body_end = part.size();
	
	content = part.substr(body_start, body_end - body_start);
	return true;
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
			if (save_uploaded_file(www_root, filename, content))
				++count;
		}
		
		part_start = part_end;
	}
	return count;
}
