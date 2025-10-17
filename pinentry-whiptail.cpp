#include <iostream>
#include <string>
#include <stdexcept>
#include <unistd.h>
#include <sys/wait.h>
#include <gpg-error.h>

#define ERR(e) gpg_err_make(GPG_ERR_SOURCE_PINENTRY, e)

struct options {
	std::string desc;
	std::string error_msg;
	std::string tty_name;
	std::string tty_type;
};

static std::string build_message(struct options &options)
{
	std::string msg = options.desc;
	if (options.error_msg != "") {
		msg += "\n\n" + options.error_msg;
	}

	return msg;
}

static inline std::string resolve_tty_file(struct options &options)
{
	return options.tty_name.empty() ? "/dev/tty" : options.tty_name;
}

static std::string run_whiptail_password(struct options &options)
{
	std::string tty = resolve_tty_file(options);
	std::string msg = "", pin = "";
	FILE *tty_in, *tty_out;
	int pipefd[2], status = 0, n;
	char buf[256];

	tty_in = fopen(tty.c_str(), "r");
	tty_out = fopen(tty.c_str(), "w");
	if (tty_in == nullptr || tty_out == nullptr) {
		throw std::runtime_error("failed to open tty for whiptail");
	}

	if (pipe(pipefd) != 0) {
		fclose(tty_in);
		fclose(tty_out);
		throw std::runtime_error("pipe() failed");
	}

	pid_t pid = fork();
	if (pid == 0) {
		// child process
		dup2(fileno(tty_in), 0);
		dup2(fileno(tty_out), 1);
		dup2(pipefd[1], 2); // whiptail writes result to stderr
		close(pipefd[0]);
		close(pipefd[1]);

		if (options.tty_type != "") {
			setenv("TERM", options.tty_type.c_str(), 1);
		}

		msg = build_message(options);
		execlp("whiptail", "whiptail", "--passwordbox", msg.c_str(),
			"10", "60", (char *) nullptr);
		exit(127);
	}

	close(pipefd[1]);

	while ((n = read(pipefd[0], buf, sizeof(buf))) > 0) {
		pin.append(buf, n);
	}

	close(pipefd[0]);
	waitpid(pid, &status, 0);
	fclose(tty_in);
	fclose(tty_out);

	if (pin.empty() == false && pin.back() == '\n') {
		pin.pop_back();
	}

	return pin;
}

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
				options.error_msg = line.substr(9);
				std::cout << "OK" << std::endl;

			} else if (line.starts_with("OPTION") == true) {
				parse_option(options, line.substr(7));
				std::cout << "OK" << std::endl;

			} else if (line == "GETPIN") {
				std::string pin = run_whiptail_password(options);
				if (pin.empty() == true) {
					std::cout << "ERR " << ERR(GPG_ERR_CANCELED);
					std::cout << " Operation cancelled <Pinentry>";
					std::cout << std::endl;
				} else {
					std::cout << "D " << pin << std::endl;
					std::cout << "OK" << std::endl;
				}
				std::cout << std::flush;

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
