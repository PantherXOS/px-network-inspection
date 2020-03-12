#ifndef ARG_INFO_H_
#define ARG_INFO_H_

#include <argp.h>
#include <stdbool.h>
#include <string.h>

const char *argp_program_version = "0.0.6";
const char *argp_program_bug_address = "<s.mahmood@pantherx.org>";

/* Program documentation. */
char doc[] = "A utility to collect network routing details of local machine.";

/* A description of the arguments we accept. */
char args_doc[] = "";

/* The options we understand. */
struct argp_option options[] = {
	{"format", 'f', "Format <JSON:CXX>", 0, "File Format which has to be one of JSON or CXX" },
	{"output", 'o', "File", 0, "Output to FILE instead of standard output" },
	{ 0 }
};

enum Format
{
	JSON,
	CXX
};

/* Used by main to communicate with parse_opt. */
struct arguments 
{
	enum Format format;
	char *output_file;
};

/* Parse a single option. */
/* Parse a single option. */
error_t parse_opt (int key, char *arg, struct argp_state *state)
{
	/* Get the input argument from argp_parse, which we
	   know is a pointer to our arguments structure. */
	struct arguments *arguments = state->input;

	switch (key)
	{
		case 'f':
			if (!strcmp("JSON", arg))
				arguments->format = JSON;
			else if (!strcmp("CXX", arg))
				arguments->format = CXX;
			else
				argp_usage (state);
			break;
		case 'o':
			arguments->output_file = arg;
			break;

		case ARGP_KEY_ARG:
			break;
		case ARGP_KEY_END:
			break;

		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

/* Our argp parser. */
struct argp argp = { options, parse_opt, args_doc, doc};

#endif
