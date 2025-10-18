/**
 * This file is a part of pinentry-whiptail.
 *
 * Copyright (c) 2025, Vlasta Vesely <vlastavesely@proton.me>
 *
 * Redistribution and use in any form, with or without modification,
 * are permitted.
 *
 * There's ABSOLUTELY NO WARRANTY, express or implied.
 */
#ifndef __DEBUG_H
#define __DEBUG_H

// XXX define this macro to enable a debug log
//#define DEBUG 1

#ifdef DEBUG
	#include <fstream>
	static std::ofstream logf("/tmp/pinentry-whiptail.log", std::ios::app);
	static void log(const std::string &msg) {
		logf << msg << std::endl;
		logf.flush();
	}
	#define LOG(m) log(m)
#else
	#define LOG(m) /* noop */
#endif

#endif /*  __DEBUG_H */

