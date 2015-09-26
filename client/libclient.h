
#ifndef LIBCLIENT_H
#define LIBCLIENT_H

#define BUFFER_SIZE 50
#define LEFT_STRIP(str)			while (*(++str) == ' ')
#define MAX_ARGS 				10
#define CMD_BUFFER_SIZE 		2*MAX_ARGS

#define CHECK_ARGC(argc, expected) 	if (argc != expected) {\
										printf("Cantidad de argumentos invalida.\n");\
										FREE_ARGV(cmd);\
										return NULL;\
									}

#define DUMP_ARGS(argc, argv)	{\
									for (int i = 0; i < argc; i++) {\
										printf("<%s> ", argv[i]);\
									}\
									printf("\n");\
								}

#define FREE_ARGV(cmd)  {\
									for (int i = 0; i < cmd.argc; i++) {\
										free(cmd.argv[i]);\
									}\
									free(cmd.argv);\
								}


#define COLOR_GREEN   "\x1b[32m"
#define COLOR_RESET   "\x1b[0m"


struct shell_cmd {
	int argc;
	char** argv;
};

struct shell_cmd parse_command(char* command);
void parse_answer(DB_DATAGRAM* datagram);
DB_DATAGRAM* command_to_datagram(char* command);
void print_prompt();

#endif
