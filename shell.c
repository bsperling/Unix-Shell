//C Libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Unix Libraries
#include <dirent.h> 
#include <unistd.h>
#include <errno.h>

//Lex Stuff
typedef struct yy_buffer_state * YY_BUFFER_STATE;
extern int yyparse();
extern void yy_switch_to_buffer (YY_BUFFER_STATE  new_buffer);
extern YY_BUFFER_STATE yy_scan_string(char * str);
extern YY_BUFFER_STATE get_current_buffer();
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);

//My Stuff
#include "shell.h"
#include "aliaslist.h"

extern char **environ;

/* Shell Interfacing */

/* Cleanup for when a new line has been entered - printing stuff, etc. */
void handle_new_line() {
	char current_directory[512];
	getcwd(current_directory, sizeof(current_directory));

	char *path_extension = strrchr(current_directory, '/') + 1;

	char current_user[100];
	getlogin_r(current_user, sizeof(current_user));

	printf("%s:%s $ ", current_user, path_extension);
}

/* Handle Syntax */
//TODO: Allow this to take a string so we can print out what the actual issue is
void syntax_error_found() {
	//printf("Syntax Error\n");
}


/* System Function Calls */

void list_files() {
	DIR *d;
	struct dirent *dir;
	d = opendir(".");

	if (d) {
    	while ((dir = readdir(d)) != NULL) {
      		printf("%s\n", dir->d_name);
    	}
    	closedir(d);
  	}
  	else {
  		printf("directory empty\n");
  	}
}

void change_directory(char * new_directory) {
	int success = chdir(new_directory);
	if (success != 0) {
		printf("%s: %s\n", new_directory, strerror(errno));
	}
	else {
		char *var_name = "PWD";
		char current_directory[512]; 
		getcwd(current_directory, sizeof(current_directory));
		set_environment_variable(var_name, current_directory);
	}
}

void change_directory_home(void) {
	change_directory(getenv("HOME"));
}

/* Environment Variables */

void set_environment_variable(char *var_name, char* new_value)
{
	if (setenv(var_name, new_value, 1) == 0) {
		printf("setenv - variable: \"%s\"\tvalue: \"%s\"\n", var_name, new_value);		
	}
	else {
		printf("error: setenv: could not set \"%s\" to \"%s\" : %s\n", var_name, new_value, strerror(errno));
	}
}

void unset_environment_variable(char *var_name) {
	if (unsetenv(var_name) == 0) {
		printf("unsetenv - \"%s\"\n", var_name);	
	}
	else {
		printf("error: unsetenv: could not unset \"%s\" : %s\n", var_name, strerror(errno));
	}
}

void print_environment_variables(void)
{
	char ** env;
	for (env = environ; *env; ++env)
	{
    	printf("%s\n", *env);
    }
}

/* Aliasing */

void list_aliases(void) {
	//Check for success
	print_aliases();
}

void create_alias(char *alias_name, char *full_command) {
	char * temp_alias = malloc(strlen(alias_name)+1);
	char * temp_command = malloc(strlen(full_command)+1);

	strcpy(temp_alias, alias_name);
	strcpy(temp_command, full_command);

	struct alias* a = malloc(sizeof(struct alias));
	a->alias_name = temp_alias;
	a->full_command = temp_command;

	//Check for success
	add_alias(a);

	//assume succesfull
	printf("alias - \"%s\" : \"%s\"\n", alias_name, full_command);
}


void remove_alias(char *alias_name) {
	//Check for success
	if (remove_alias_with_name(alias_name) == 0) {
		printf("alias removed : \"%s\"\n", alias_name);
	}
	else {
		printf("error: unalias: could not remove \"%s\" : alias not found\n", alias_name);
	}
}

void check_aliases(char *alias_name) {
	struct alias* tempalias = get_alias_with_name(alias_name);
	if (tempalias != NULL) {
		parse_string(tempalias->full_command);
	}
	else {
		printf("error: \"%s\": command not found\n", alias_name);
	}
}

char * check_environment_variables(char *buffer)
{
	char *start_pointer;
	char *end_pointer;
	char environ_buffer[1024];

	char *return_buffer;
	char *return_buffer_reference;

	
	if((start_pointer = strstr(buffer, "${")) != NULL)
	{
		if((end_pointer = strchr(start_pointer+2, '}')) != NULL)
		{

			int size = end_pointer - start_pointer-2;
			//initialize environ_buffer to 0 before strncopy
			memset(environ_buffer, '\0', 1024);
			strncpy(environ_buffer, start_pointer+2, size);
			puts(environ_buffer);

			if(getenv(environ_buffer) != NULL)
			{
				printf("%s\n", getenv(environ_buffer));
				return_buffer = calloc(1024,sizeof(char));
				int before_var_size = start_pointer - buffer + 1;
				strncpy(return_buffer, buffer, before_var_size-1);
				strcat(return_buffer, getenv(environ_buffer));

				return_buffer_reference = &return_buffer[strlen(return_buffer)-1];
				strncpy(return_buffer_reference+1, end_pointer+1, strlen(end_pointer)-1);

				puts(return_buffer);

				return return_buffer;
			}
			else
			{
				printf("Environment variable \"%s\" not found.\n", environ_buffer);
				return NULL;			
			}
			
		}
	}
	else
	{
		//printf("Did not find environment variable.\n");
		return NULL;
	}
}

char * replace_environ_vars_and_aliases(char* buffer)
{
	char * temp_buffer = check_environment_variables(buffer);
	puts(buffer);
	if(temp_buffer != NULL)
	{
	    puts(temp_buffer);
	    while((temp_buffer != NULL) && (strcmp(temp_buffer,buffer) != 0))
	    {
		strcpy(buffer,temp_buffer);
		free(temp_buffer);
		temp_buffer = check_environment_variables(buffer);
	    }
	}
	char *returned_buffer = malloc(1024);
	memcpy(returned_buffer, buffer, 1024);
	
	return returned_buffer;
}

/* Reads the entire command line from the terminal, parses environment variables, and aliases. */
void preparse(char * true_buffer) {
	char buffer[1024];
	fgets(buffer,1024,stdin);
	char * return_buffer = replace_environ_vars_and_aliases(buffer);
	strcpy(true_buffer, return_buffer);
	//free(return_buffer); SUPER MAJOR ISSUE!!!!
}

/* Lex/Yacc */
void parse_string(char * input) {
	//puts(input);
	//YY_BUFFER_STATE cur = get_current_buffer();
	YY_BUFFER_STATE buffer = yy_scan_string(input);
    yyparse();
   	//yy_switch_to_buffer(cur);
   	//yy_delete_buffer(buffer); //MAJOR ISSUE!!!
}

void parse_file(char * input_file_name) {
	/*
	FILE *in = fopen(input_file_name, "r");
	if (in == NULL) {
		printf("error: invalid file: %s\n", input_file_name);
		return;
	}

    fseek(in, 0, SEEK_END);
    unsigned long len = (unsigned long)ftell(in);

	YY_BUFFER_STATE cur = get_current_buffer();
	YY_BUFFER_STATE buffer = yy_scan_buffer(in, len);
    yyparse();
    yy_switch_to_buffer(cur);
    yy_delete_buffer(buffer);
    */
}




