/*
 * $Id$
 *
 * by JH <jheinonen@bigfoot.com>
 *
 * Copyright (C) Jaakko Heinonen
 */

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif
#if defined(HAVE_LOCALE_H) && defined(HAVE_SETLOCALE)
#	include <locale.h>
#endif
#include "abook.h"
#include "ui.h"
#include "database.h"
#include "list.h"
#include "filter.h"
#include "edit.h"
#include "misc.h"
#include "options.h"

static void             init_abook();
static void             set_filenames();
static void		free_filenames();
static void             parse_command_line(int argc, char **argv);
static void             show_usage();
static void             mutt_query(char *str);
static void             init_mutt_query();
static void             quit_mutt_query();
static void		convert(char *srcformat, char *srcfile,
				char *dstformat, char *dstfile);

char *datafile = NULL;
char *rcfile = NULL;

static void
init_abook()
{
	set_filenames();
	init_options();

	signal(SIGKILL, quit_abook);
	signal(SIGTERM, quit_abook);
	
	if( init_ui() )
		exit(1);
	
	umask(DEFAULT_UMASK);

	/*
	 * this is very ugly for now
	 */
	/*if( options_get_int("datafile", "autosave") )*/

	if( load_database(datafile) == 2 ) {
		char *tmp = strconcat(getenv("HOME"),
				"/" DATAFILE, NULL);

		if( safe_strcmp(tmp, datafile) ) {
			refresh_screen();
			statusline_msg("Sorry, the specified file does "
				"not appear to be a valid abook addressbook");
			statusline_msg("Will open default addressbook...");
			free(datafile);
			datafile = tmp;
			load_database(datafile);
		} else
			free(tmp);
	}

	refresh_screen();
}

void
quit_abook()
{
	if( options_get_int("autosave") )
		save_database();
	else if( statusline_ask_boolean("Save database", TRUE) )
		save_database();

	close_config();
	close_database();

	close_ui();
	
	exit(0);
}

int
main(int argc, char **argv)
{
#if defined(HAVE_SETLOCALE) && defined(HAVE_LOCALE_H)
	setlocale(LC_ALL, "" );
#endif
		
	parse_command_line(argc, argv);
	
	init_abook();

	get_commands();	
	
	quit_abook();

	return 0;
}

static void
set_filenames()
{
	struct stat s;

	if( (stat(getenv("HOME"), &s)) == -1 || ! S_ISDIR(s.st_mode) ) {
		fprintf(stderr,"%s is not a valid HOME directory\n", getenv("HOME") );
		exit(1);
	}

	if (!datafile)
		datafile = strconcat(getenv("HOME"), "/" DATAFILE, NULL);

	rcfile = strconcat(getenv("HOME"), "/" RCFILE, NULL);

	atexit(free_filenames);
}

static void
free_filenames()
{
	my_free(rcfile);
	my_free(datafile);
}

static void
parse_command_line(int argc, char **argv)
{
	int i;

	for( i = 1; i < argc; i++ ) {
		if( !strcmp(argv[i], "--help") ) {
			show_usage();
			exit(1);
		} else
		if( !strcmp(argv[i], "--mutt-query") )
			mutt_query(argv[i + 1]);
		else
		if( !strcmp(argv[i], "--datafile") ) {
			if (argv[i+1]) {
				if (argv[i+1][0] != '/') {
					char *cwd = my_getcwd();
					datafile = strconcat(cwd, "/", argv[i+1], NULL);
					free(cwd);
				} else {
					datafile = strdup(argv[i+1]);
				}
				i++;
			} else {
				show_usage();
				exit(1);
			}
		} else
		if( !strcmp(argv[i], "--convert") ) {
			if( argc < 5 || argc > 6 ) {
				fprintf(stderr, "incorrect number of argumets to make conversion\n");
				fprintf(stderr, "try %s --help\n", argv[0]);
				exit(1);
			}
			if( argv[i+4] )
				convert(argv[i+1], argv[i+2],
					argv[i+3], argv[i+4]);
			else
				convert(argv[i+1], argv[i+2], argv[i+3], "-");
		} else {
			printf("option %s not recognized\n", argv[i]);
			printf("try %s --help\n", argv[0]);
			exit(1);
		}
	}
}


static void
show_usage()
{
	puts	(PACKAGE " v " VERSION "\n");
	puts	("	--help				show usage");
	puts	("	--datafile	<filename>	use an alternative addressbook file");
	puts	("	--mutt-query	<string>	make a query for mutt");
	puts	("	--convert	<inputformat> <inputfile> "
		 "<outputformat> <outputfile>");
	putchar('\n');
	puts	("available formats for --convert option:");
	print_filters();
#ifdef DEBUG
	puts	("\nWarning: this version compiled with DEBUG flag ON");
#endif
}

extern list_item *database;
extern int items;

static void
muttq_print_item(int item)
{
	char emails[MAX_EMAILS][MAX_EMAIL_LEN];
	int i;

	split_emailstr(item, emails);
	
	for(i = 0; i < (options_get_int("mutt_return_all_emails") ?
			MAX_EMAILS : 1) ; i++)
		if( *emails[i] )
			printf("%s\t%s\t%s\n", emails[i],
				database[item][NAME],
				database[item][NOTES] == NULL ? " " :
					database[item][NOTES]
				);
}

static int
mutt_query_name(char *str)
{
	int i, j;
	char *tmp;

	for(i = 0, j = 0 ; i < items; i++) {
		tmp = strdup(database[i][NAME]);
		if( strstr( strupper(tmp), strupper(str) ) != NULL ) {
			if( !j )
				putchar('\n');
			muttq_print_item(i);
			j++;
		}
		free(tmp);
	}

	return j;
}

static int
mutt_query_email(char *str)
{
	int i, j, k;
	char *tmp, emails[MAX_EMAILS][MAX_EMAIL_LEN];

	for(i = 0, j = 0; i < items; i++) {
		split_emailstr(i, emails);
		for(k = 0; k < MAX_EMAILS; k++) {
			if( *emails[k] ) {
				tmp = strdup( emails[k] );
				if( strstr( strupper(tmp), strupper(str) ) != NULL ) {
					if( !j )
						putchar('\n');
					j++;
					if( options_get_int("mutt_return_all_emails") ) {
						muttq_print_item(i);
						free(tmp);
						break;
					} else
						printf("%s\t%s\n", emails[k],
							database[i][NAME]);
				}
				free(tmp);
			}
		}
	}

	return j;
}

static void
mutt_query(char *str)
{
	int i;
	
	init_mutt_query();

	if( str == NULL || !strcasecmp(str, "all") ) {
		printf("All items\n");
		for(i = 0; i < items; i++)
			muttq_print_item(i);
	} else {
		if( !mutt_query_name(str) && !mutt_query_email(str) ) {
			printf("Not found\n");
			quit_mutt_query(1);
		}
	}

	quit_mutt_query(0);
}

static void
init_mutt_query()
{
	set_filenames();
	init_options();
	
	if( load_database(datafile) ) {
		printf("Cannot open database\n");
		quit_mutt_query(1);
		exit(1);
	}
}

static void
quit_mutt_query(int status)
{
	close_database();
	close_config();

	exit(status);
}


void
launch_mutt(int item)
{
	int i;
	char email[MAX_EMAIL_LEN];
	char *cmd;
	char *tmp = options_get_str("mutt_command");

	if( !is_valid_item(item) )
		return;

	cmd = strconcat(tmp, " '", NULL );

	for(i=0; i < items; i++) {
		if( ! is_selected(i) && i != list_current_item() )
			continue;
		get_first_email(email, i);
		tmp = mkstr("%s \"%s\"", cmd, database[i][NAME]);
		my_free(cmd);
		if( *database[i][EMAIL] ) {
			cmd = mkstr("%s <%s>", tmp, email);
			free(tmp);
			tmp = cmd;
		}
		cmd = strconcat(tmp, " ", NULL);
		free(tmp);
	}

	tmp = mkstr("%s%c", cmd, '\'');
	free(cmd);
	cmd = tmp;
#ifdef DEBUG
	fprintf(stderr, "cmd: %s\n", cmd);
#endif
	system(cmd);	

	free(cmd);
}

void
launch_wwwbrowser(int item)
{
	char *cmd = NULL;

	if( !is_valid_item(item) )
		return;

	if( database[item][URL] )
		cmd = mkstr("%s '%s'",
				options_get_str("www_command"),
				safe_str(database[item][URL]));
	else
		return;

	if ( cmd )
		system(cmd);

	free(cmd);
}

void *
abook_malloc(size_t size)
{
	void *ptr;

	if ( (ptr = malloc(size)) == NULL ) {
		if( is_ui_initialized() )
			quit_abook();
		perror("malloc() failed");
		exit(1);
	}

	return ptr;
}

void *
abook_realloc(void *ptr, size_t size)
{
	ptr = realloc(ptr, size);

	if( size == 0 )
		return NULL;

	if( ptr == NULL ) {
		if( is_ui_initialized() )
			quit_abook();
		perror("realloc() failed");
		exit(1);
	}

	return ptr;
}

FILE *
abook_fopen (const char *path, const char *mode)
{	
	struct stat s;
	
	if( ! strchr(mode, 'r') )
		return fopen(path, mode);
	
	if ( (stat(path, &s)) == -1 )
		return NULL;
	
	return S_ISREG(s.st_mode) ? fopen(path, mode) : NULL;
}



static void
convert(char *srcformat, char *srcfile, char *dstformat, char *dstfile)
{
	int ret=0;

	if( !srcformat || !srcfile || !dstformat || !dstfile ) {
		fprintf(stderr, "too few argumets to make conversion\n");
		fprintf(stderr, "try --help\n");
	}

	strlower(srcformat);
	strlower(dstformat);

	if( !strcmp(srcformat, dstformat) ) {
		printf(	"input and output formats are the same\n"
			"exiting...\n");
		exit(1);
	}

	set_filenames();
	init_options();

	switch( import(srcformat, srcfile) ) {
		case -1:
			printf("input format %s not supported\n", srcformat);
			ret = 1;
		case 1:
			printf("cannot read file %s\n", srcfile);
			ret = 1;
	}

	if(!ret)
		switch( export(dstformat, dstfile) ) {
			case -1:
				printf("output format %s not supported\n",
						dstformat);
				ret = 1;
				break;
			case 1:
				printf("cannot write file %s\n", dstfile);
				ret = 1;
				break;
		}

	close_database();
	close_config();
	exit(ret);
}

