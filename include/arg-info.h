#ifndef ARG_INFO_H_
#define ARG_INFO_H_

#include <argp.h>
#include <stdbool.h>
#include <string.h>

///	@brief	version of the px-network-inspection
const char *argp_program_version = "0.0.12";

///	@brief	the bug report email address.
const char *argp_program_bug_address = "<s.mahmood@pantherx.org>";

///	@brief	Program documentation.
char doc[] = "A utility to collect network routing details of local machine.";

///	@brief	A description of the arguments we accept.
char args_doc[] = "";

///	@brief	The options we understand.
struct argp_option options[] =
{
	{"format", 'f', "Format <JSON:CXX>", 0, "File Format which has to be one of JSON or CXX" },
	{"output", 'o', "File", 0, "Output to FILE instead of standard output" },
	{ 0 }
};

/// @brief	the type of the output format
enum Format
{
	JSON,
	CXX
};

///	@brief	used by main to fetch the parsed arguments
struct arguments 
{
	///	@brief	the output format
	enum Format format;
	///	@brief	the output file address
	char *output_file;
};

/**
 *  @brief Parse a single option
 *
 *  @details
 *   The function is a call-back that is called by the arg-parser.
 *
 *  @see	https://wiki.pantherx.org
 *
 * 	@note	not to call from code.
 * 	@post	use inf struct argp.
 */
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

///	@brief	The argp parser.
struct argp argp = { options, parse_opt, args_doc, doc};

#endif
