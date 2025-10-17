#include <iostream>
#include <string>
#include <stdexcept>
#include <gpg-error.h>

#define ERR(e) gpg_err_make(GPG_ERR_SOURCE_PINENTRY, e)

struct options {
	std::string desc = "";
	std::string error = "";
	std::string tty_name = "";
	std::string tty_type = "";
};

static void parse_option(struct options &options, const std::string &line)
{
	if (line.starts_with("ttyname") == true) {
		options.tty_name = line.substr(8);
	} else if (line.starts_with("ttytype") == true) {
		options.tty_type = line.substr(8);
	}
}

int main(int argc, const char **argv)
{
	struct options options;
	std::string line;

	while (std::cin.good()) {
		try {
			getline(std::cin, line);
			if (line.length() == 0) {
				break;
			}

			if (line.starts_with("SETDESC") == true) {
				options.desc = line.substr(8);
				std::cout << "OK" << std::endl;

			} else if (line.starts_with("SETERROR") == true) {
				options.error = line.substr(9);
				std::cout << "OK" << std::endl;

			} else if (line.starts_with("OPTION") == true) {
				parse_option(options, line.substr(7));
				std::cout << "OK" << std::endl;

			} else if (line == "GETPIN") {
				// TODO

			} else if (line == "BYE") {
				std::cout << "OK closing" << std::endl;
				std::cout << std::flush;
				break;

			} else {
				std::cout << "OK" << std::endl;
			}

		} catch (const std::runtime_error &e) {
			std::cout << "ERR " << ERR(GPG_ERR_UNEXPECTED);
			std::cout << " " << e.what() << std::endl;
			return -1;
		}
	}

	return 0;
}
