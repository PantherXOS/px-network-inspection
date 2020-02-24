#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <argp.h>
#include <stdbool.h>

const char *argp_program_version = "px-network-inspection";
const char *argp_program_bug_address = "<s.mahmood@pantherx.org>";

/* Program documentation. */
static char doc[] = "A utility to collect network routing details of local machine.";

/* A description of the arguments we accept. */
static char args_doc[] = "";

/* The options we understand. */
static struct argp_option options[] = {
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
static error_t parse_opt (int key, char *arg, struct argp_state *state)
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
static struct argp argp = { options, parse_opt, args_doc, doc};

int main (int argc, char **argv)
{
	struct arguments arguments;

	/* Default values. */

	/* Parse our arguments; every option seen by parse_opt will
	   be reflected in arguments. */
	argp_parse (&argp, argc, argv, 0, 0, &arguments);

	printf("path: %s and format: %s\n", arguments.output_file, arguments.format == JSON ? "JSON" : "CXX");
	exit (0);
}
