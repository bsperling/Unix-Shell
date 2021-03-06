%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "shell.h"
#include "aliaslist.h"
#include "global.h"
#include "preparser.h"

 
void yyerror (char *s) { syntax_error_found(); } 
int yylex();


%}

%union { char *cmd; char *arg; }

%type <arg> word;
%type <arg> quote_input;
%type <arg> quotes;
%type <arg> word_or_quote;

%start line
%token cd_command exit_command
%token setenv_command unsetenv_command printenv_command
%token alias_command unalias_command
%token word new_line quotes semicolon 
%token read_from write_to pipe_token ampersand
%token syntax

%%

/* Start */

line    : command			{;}
		| line command 		{;}
        ;

      	
command : //Simple Commands
		  exit_command		{ exit(0); }
		

		//Built-in Commands
		| printenv_command		{ print_environment_variables(); }
		| change_directory		{;}
		| set_environ_var		{;}
		| unset_environ_var 	{;}
		| aliasing_commands		{;}
		| unaliasing_command	{;}

		//Path Commands
		| path_command			{ eventCount++; builtin = 0; }

		//IO Redirection Commands
		| input 		{;}
		| output 		{;}

		//Piping
		| pipe_token			{ currcmd++; }

		//Errors
		| syntax_error 		{ syntax_error_found(); }
		| error 			{ syntax_error_found(); }
		;

/* Built-in Commands */

change_directory   	: cd_command word  	{ change_directory($2); }
					| cd_command quote_input 	{ change_directory($2); } 
					| cd_command				{ change_directory_home(); } 
        			;

set_environ_var		: setenv_command word word  { set_environment_variable($2, $3); }
					;

unset_environ_var	: unsetenv_command word 			{ unset_environment_variable($2); }
					;

aliasing_commands	: alias_command word word 			{ create_alias($2, $3); }
					| alias_command word quote_input	{ create_alias($2, $3); }
					| alias_command 					{ list_aliases(); }
					;

unaliasing_command	: unalias_command word 				{ remove_alias($2); }
					;

/* Path Commands */

path_command : word				{
											//printf("PATH: word: %s\n", $1);
											comtab[currcmd].comname = $1;
											//comtab[currcmd]->argptr = NIL(ARGTAB);
											comtab[currcmd].nargs = 0;
											comtab[currcmd].argptr->args[0] = $1;
										}
			 | word arguments	{
											//printf("PATH: ARGUMENTS word: %s\n", $1);
		 									comtab[currcmd].comname = $1;		
		 									comtab[currcmd].nargs = currarg;
											comtab[currcmd].argptr->args[0] = $1;
											//comtab[currcmd].argptr->args[++currarg] = $2;
										}				

/* Non-Commands */

arguments 	: word_or_quote				{ 
											//printf("ARG: word: %s\n", $1);							
											currarg = 1; /*1st arg reserved for comm.*/
											comtab[currcmd].argptr->args[currarg++] = $1; 
										}
			| arguments word_or_quote	{ 
											//printf("ARG: ARGUMENTS word: %s\n", $2);
											comtab[currcmd].argptr->args[currarg++] = $2;  
										}
		    ;

word_or_quote : quote_input { $$ = $1; }
			  | word { $$ = $1; }
			  ;

quote_input : quotes { $$ = $1; $$[strlen($$) - 1] = '\0'; $$ = $$ + 1; }
			;

/* Syntax */ 

syntax_error 		: syntax {;}
					;

/* I/O Redirection */

input : read_from word 	{
							//strcpy(srcf , $2);
							//comtab[0].infd = BADFD;
							switch_input($2);
						}
	  ;

output : write_to word 	{
							//strcpy( distf, $2);
							//comtab[currcmd-1].outfd = BADFD;
							switch_output($2);
						}
	   | write_to write_to word 	{	/* Append to EOF */
	   									//strcpy(distf, $3);
	   									//comtab[currcmd-1].outfd = BADFD;
	   									switch_output($3);
	   									append = 1;
	   								}	
	   	;   

%%

int main (void) {

	//TODO: ADD HANDLE FOR CTRL+C ESCAPE (IN shell_init())
	shell_init();
	
	while (1) {	
		handle_new_line();

		/* Save the entire command line */
		char true_buffer[1024];
		preparse(true_buffer);
		parse_string(true_buffer);
		if(PATH_EXECUTABLE)
		{
			// printf("Non built-in command entered.\n");
			// for (i = 0; i<comtab[currcmd].nargs; i++)
			// {
			// 	printf("Command Name: %s | Command Arg %d: %s\n", comtab[currcmd].comname, i, comtab[currcmd].argptr->args[i]);
			// }
			executeIt();
		}
	}
	free_memory();
}




