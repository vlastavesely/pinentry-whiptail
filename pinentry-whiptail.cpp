/**
 * This file is a part of pinentry-whiptail.
 *
 * Copyright © 2025, Vlasta Vesely <vlastavesely@proton.me>
 *
 * Redistribution and use in any form, with or without modification,
 * are permitted.
 *
 * There's ABSOLUTELY NO WARRANTY, express or implied.
 */
#include <iostream>
#include <string>
#include <numeric>
#include <stdexcept>
#include <unistd.h>
#include <sys/wait.h>
#include <gpg-error.h>
#include "debug.h"

#define ERR(e) gpg_err_make(GPG_ERR_SOURCE_PINENTRY, e)

struct options {
	std::string desc;
	std::string error_msg;
	std::string tty_name;
	std::string tty_type;
};

// Since the Assuan protocol is line-based, some characters (like the newline
// are escaped similarly to the URL encoding.
static std::string decode_percent_string(const std::string &text)
{
	std::string out;

	for (auto it = text.begin(); it != text.end(); it++) {
		char c = *it;
		switch (c) {
		case '%':
			if (text.end() != it + 1 && text.end() != it + 2) {
				const std::string s{it[1], it[2]};
				out += std::stol(s, nullptr, 16);
				it += 2;
			}

			break;
		default:
			out += c;
		}
	}

	return out;
}

// GnuPG does not pass random env variables but it passes a selection of them.
// The one that can be used for configuration of the pinentry program is
// PINENTRY_USER_DATA. At this point it just expects the colour scheme for
// Whiptail.
static void configure_whiptail()
{
	if (getenv("PINENTRY_USER_DATA") != nullptr) {
		setenv("NEWT_COLORS", getenv("PINENTRY_USER_DATA"), 1);
	}
}

static std::string trim_newline(const std::string &in)
{
	std::string s = in;
	if (s.empty() == false && s.back() == '\n') {
		s.pop_back();
	}

	return s;
}

static inline std::string build_message(struct options &options)
{
	std::string msg;

	msg = decode_percent_string(options.desc);
	msg = trim_newline(msg);
	if (options.error_msg != "") {
		msg += "\n\n" + options.error_msg;
	}

	return msg;
}

static inline std::string resolve_tty_file(struct options &options)
{
	return options.tty_name.empty() ? "/dev/tty" : options.tty_name;
}

static unsigned int count_newlines(const std::string &s)
{
	return std::accumulate(s.cbegin(), s.cend(), 0, [](unsigned int prev, char c) {
		return c != '\n' ? prev : prev + 1;
	});
}

static std::string calculate_whiptail_height(const std::string &msg)
{
	// 7 = borders, spaces, the entry and the buttons
	return std::to_string(count_newlines(msg) + 8);
}

static std::string run_whiptail_password(struct options &options)
{
	std::string tty = resolve_tty_file(options);
	std::string msg = "", pin = "";
	FILE *tty_in, *tty_out;
	int pipefd[2], status = 0, n;
	char buf[256];
	pid_t pid;

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

	pid = fork();
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

		configure_whiptail();
		msg = build_message(options);
		execlp("whiptail", "whiptail", "--passwordbox",
			"--title", "GPG Pinentry", msg.c_str(),
			calculate_whiptail_height(msg).c_str(),
			"75", (char *) nullptr);
		exit(127);
	}

	close(pipefd[1]);

	while (1) {
		n = read(pipefd[0], buf, sizeof(buf));
		if (n < 1) {
			break;
		}

		pin.append(buf, n);
	}

	close(pipefd[0]);
	waitpid(pid, &status, 0);
	fclose(tty_in);
	fclose(tty_out);

	pin = trim_newline(pin);

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

	std::cout << "OK Pleased to meet you" << std::endl;

	while (std::cin.good()) {
		try {
			getline(std::cin, line);
			if (line.length() == 0) {
				break;
			}

			LOG(line);
			if (line.starts_with("SETDESC") == true) {
				options.desc = line.substr(8);
				std::cout << "OK" << std::endl;

			} else if (line.starts_with("SETERROR") == true) {
				options.error_msg = line.length() > 9
					? line.substr(9) : "";
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

			} else if (line == "BYE") {
				std::cout << "OK closing" << std::endl;
				break;

			} else {
				std::cout << "OK" << std::endl;
			}

			std::cout << std::flush;

		} catch (const std::runtime_error &e) {
			std::cout << "ERR " << ERR(GPG_ERR_UNEXPECTED);
			std::cout << " " << e.what() << std::endl;
			return -1;
		}
	}

	return 0;
}
