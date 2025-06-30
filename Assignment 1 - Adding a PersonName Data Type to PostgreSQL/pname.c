/*
 * src/tutorial/pname.c
 *
 ******************************************************************************
  This file contains routines that can be bound to a Postgres backend and
  called by the backend in the process of processing queries.  The calling
  format for these routines is dictated by Postgres architecture.
******************************************************************************/

#include "postgres.h"
#include <string.h>
#include "fmgr.h"
#include "libpq/pqformat.h"		/* needed for send/recv functions */
#include <stdbool.h>
#include <regex.h>
#include "utils/elog.h"       //NECESSARY FOR ELOG FUNCTION		
#include "utils/builtins.h"  // NECESSARY FOR PG_RETURN_TEXT_P


bool checkName(char *string);
char* whitespace_removal(char* str);
bool eq(char* fa1, char* fa2, char* ga1, char* ga2);
bool lt(char* fa1, char* fa2, char* ga1, char* ga2);
bool gt(char* fa1, char* fa2, char* ga1, char* ga2);


PG_MODULE_MAGIC;

typedef struct PersonName
{
	int 		length;
	char		pname[FLEXIBLE_ARRAY_MEMBER]; //This is same as giving name[] 
	// Also, flexible array member MUST be the last member in the data structure.
}PersonName;

bool checkName(char *string){
	// Regular expression pattern for validating names
	char *pattern = "^[A-Z][A-Za-z'-]+([ ][A-Z][A-Za-z'-]+)*,[ ]?[A-Z][A-Za-z'-]+([ ][A-Z][A-Za-z'-]+)*$";

	// Compile the regular expression
	regex_t regex;
	int reti = regcomp(&regex, pattern, REG_EXTENDED);
	if (reti) {
		fprintf(stderr, "Could not compile regex\n");
		return false;
	}

	// Execute the regular expression
	reti = regexec(&regex, string, 0, NULL, 0);
	if (!reti) {
		regfree(&regex);
		return true;
	} else if (reti == REG_NOMATCH) {
		regfree(&regex);
		return false;
	} else {
		char error_message[100];
		regerror(reti, &regex, error_message, sizeof(error_message));
		regfree(&regex);
		return false;
    }
}

/*****************************************************************************
 * Input/Output functions
 *****************************************************************************/

char* whitespace_removal(char* str)
{
	//number of spaces in string can be 1 or 2 or any number depending upon name e.g. "Smith, John Andrew" has 2 spaces. Or "Smith, John Andrew Jinx" has 3 spaces
	
	char *comma_pos = strchr(str, ','); // Find the first comma character
	if (*(comma_pos+1) == ' ')  //Only if there is a space immediately after comma
	{
	char *space_pos = comma_pos + 1; 
    	//int s_length = strlen(str);
	if (space_pos != NULL) {
        	memmove(space_pos, space_pos + 1, strlen(space_pos)); // Shift characters to the left
        	// No need to manually give \0 at end.. as that too will be copied in the previous memmove statement.
		//str[s_length - 1] = "\0"; //Decrease length by 1
	}
	}
	return 0;
}


PG_FUNCTION_INFO_V1(pname_in);

Datum
pname_in(PG_FUNCTION_ARGS)
{
	int strlength;
	PersonName *result;
	char *str = PG_GETARG_CSTRING(0);
	if (!checkName(str))
		ereport(ERROR,(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),errmsg("invalid input syntax for type %s: \"%s\"","PersonName", str)));
	whitespace_removal(str); //Remove first white space
	//elog(LOG,"str: %s", str);
	strlength = strlen(str)+ 1; //null terminated \0 is there in string
	//size of includes \0 in its length calculation. strlen function however does not.
	result = (PersonName *) palloc(VARHDRSZ + strlength);
	// No need to free result pointer later as any pointer of SQL i.e. done by palloc, not malloc or calloc etc. will be freed automatically.
	// i.e. those allocated by palloc function.
	SET_VARSIZE(result, VARHDRSZ + strlength);
	snprintf(result->pname,strlength, "%s", str);
	// Using strcpy is not good here i.e. strcpy(str, result->pname); This is because strcpy doesnt automatically add \0 to string unlike snprintf.
	// THE LENGTH FIELD WILL BE SET BY SET_VARSIZE. SO, donot type the command result->length = strlen(str)
	//This is wrong -> result = (PersonName *) palloc(sizeof(PersonName)+strlen) // size of personname will give size of fixed part of array
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(pname_out);

Datum
pname_out(PG_FUNCTION_ARGS)
{
	PersonName    *pname = (PersonName *) PG_GETARG_POINTER(0);
	char	   *result;

	result = psprintf("%s", pname->pname);
	PG_RETURN_CSTRING(result);
}

/*****************************************************************************
 * New Operators
 *
 * A practical PersonName datatype would provide much more than this, of course.
 *****************************************************************************/


/*****************************************************************************
 * Operator class for defining B-tree index
 *
 * It's essential that the comparison operators and support function for a
 * B-tree index opclass always agree on the relative ordering of any two
 * data values.  Experience has shown that it's depressingly easy to write
 * unintentionally inconsistent functions.  One way to reduce the odds of
 * making a mistake is to make all the functions simple wrappers around
 * an internal three-way-comparison function, as we do here.
 *****************************************************************************/
bool eq(char* fa1, char* fa2, char* ga1, char* ga2)
{
    //elog(LOG,"a_family: %s b_family: %s a_given: %s b_given: %s",fa1, fa2, ga1, ga2);
    //elog(LOG,"a_family: %d b_family: %d a_given: %d b_given: %d",strlen(fa1), strlen(fa2), strlen(ga1), strlen(ga2));    
    if(strcmp(fa1,fa2) == 0 && strcmp(ga1,ga2) == 0)
        return true;
    else
        return false;
}

bool lt(char* fa1, char* fa2, char* ga1, char* ga2)
{
    if(strcmp(fa1,fa2) < 0)
        return true;
    else if(strcmp(fa1,fa2) == 0 && strcmp(ga1,ga2) < 0 )
        return true;
    else
        return false;
}

bool gt(char* fa1, char* fa2, char* ga1, char* ga2)
{
    if(strcmp(fa1,fa2) > 0)
        return true;
    else if(strcmp(fa1,fa2) == 0 && strcmp(ga1,ga2) > 0 )
        return true;
    else
        return false;
}

static bool
pname_internal(PersonName * a, PersonName *b, int c)  //c is the flag value for the type of unary functions.. i.e. given or family or show
{
    // The original data present at the data structure should not be modified..i.e.the comma should be there inside pname.
	//So, i will store family name and given name etc. in temporary string a_family, a_given etc...
	char *a_given, * b_given, *a_family, *b_family;
	int a_given_pos, b_given_pos;
	int a_length, b_length;
	int i=0, j=0;
	
	a_length = strlen(a->pname);
	b_length = strlen(b->pname);

	for(i = 0; a->pname[i] != '\0'; i++) {
		if (a->pname[i] == ',') {
			a_given_pos = i;  //comma position
			break;
		}
	}

	for(i = 0; b->pname[i] != '\0'; i++) {
		if (b->pname[i] == ',') {
			b_given_pos = i;  //comma position
			break;
		}
	}

	a_family = (char*) palloc ((a_given_pos+1) * sizeof(char));
	for(i = 0; i < a_given_pos; i++)
	{
		a_family[i] = a->pname[i];
	}
	a_family[a_given_pos] = '\0';

	b_family = (char*) palloc ((b_given_pos+1) * sizeof(char));
	for(i = 0; i < b_given_pos; i++)
	{
		b_family[i] = b->pname[i];
	}
	b_family[b_given_pos] = '\0';

	a_given = (char*) palloc ((a_length - a_given_pos) * sizeof(char));
	
	for(i = a_given_pos + 1; i < a_length; i++)
	{
		a_given[j] = a->pname[i];	
		//strncat(a_given, &(a->pname[i]), 1);
		j=j+1;
	}
	a_given[a_length - a_given_pos - 1] = '\0';
	
	j=0;
	b_given = (char*) palloc ((b_length - b_given_pos) * sizeof(char));
	for(i = b_given_pos + 1; i < b_length; i++)
	{
		b_given[j] = b->pname[i];	
		//strncat(b_given, &(b->pname[i]), 1);
		j=j+1;
	}
	b_given[b_length - b_given_pos - 1] = '\0';


	if (c == 1)  //less than
	{
	return lt(a_family, b_family, a_given, b_given);
	}
	else if (c==2)  //less than or equal to
	{
	return (lt(a_family, b_family, a_given, b_given) || eq(a_family, b_family, a_given, b_given));
	}
	else if (c==3)  //equal to
	{
	return eq(a_family, b_family, a_given, b_given);
	}
	else if (c==4)  //greater than or equal to
	{
	return (gt(a_family, b_family, a_given, b_given) || eq(a_family, b_family, a_given, b_given));
	}
	else if (c==5) //greater than
	{
	return gt(a_family, b_family, a_given, b_given);
	}
	else // c==6 
	{
	return !(eq(a_family, b_family, a_given, b_given));
	}
	
	// There is no main function in this c code. If there was, i could have freed all pointers there. But, i think the purpose of integrating postgres sql file with c was that itself.
	// The below frees will never even occur because the function exits at any of the returns above.
	// This is why you must use palloc as Postgres will automatically manage the pointers allocated by palloc. And for palloc, you do #include "postgres.h"
	
	//free(a_given);
	//free(b_given);
	//free(a_family);
	//free(b_family);
}

static char*
pname_internal_two(PersonName * a, int c)  //c is the flag value for the type of unary functions.. i.e. given or family or show
{
    char *a_given, *a_family, *a_show_given;
	int a_given_pos, a_show_given_pos;  //a_given_pos is the position of comma and a_show_given_pos is the position of first space if the given name has two words etc.
	int a_length;
	int fullname_length;
	char *fullname;
	int i = 0, j=0;

	a_length = strlen(a->pname);

	for(i = 0; a->pname[i] != '\0'; i++) {
		if (a->pname[i] == ',') {
			a_given_pos = i;  //comma position
			break;
		}
	}

	a_family = (char*) palloc ((a_given_pos+1) * sizeof(char));
	for(i = 0; i < a_given_pos; i++)
	{
		a_family[i] = a->pname[i];
	}
	a_family[a_given_pos] = '\0';
	
	a_given = (char*) palloc ((a_length - a_given_pos) * sizeof(char));
	for(i = a_given_pos + 1; i < a_length; i++)
	{
		a_given[j] = a->pname[i];
		//strncat(a_given, &(a->pname[i]), 1);
		j=j+1;
	}
	a_given[a_length - a_given_pos - 1] = '\0';

	// For showing full name, if the given name has more than 1 word or name in it, only first word will be taken.
	// e.g "Smith,John Andrew" i.e. given name will be John Andrew. output of show function i.e. getting fullname , only "John Smith" will be printed.
	
	for(i = 0; i < strlen(a_given); i++)
	{
		if(a_given[i] == ' '){
			a_show_given_pos = i;		
			break;
		}	
	}
	a_show_given = (char*) palloc ((a_show_given_pos + 1) * sizeof(char));
	
	j=0;
	for(i=0; i < a_show_given_pos; i++)
	{
		a_show_given[j] = a_given[i];
		j = j + 1;
	}
	a_show_given[a_show_given_pos] = '\0';

	// To calculate full length needed for the buffer
	fullname_length = strlen(a_show_given) + strlen(a_family) + 2; // 1 for space, 1 for null terminator

	// Create a buffer to hold the full name
	fullname = (char*) palloc (fullname_length * sizeof(char));

	// Copy the first name to the full name buffer
    	strcpy(fullname, a_show_given);

	// Concatenate a space and the last name to the full name buffer
    	strcat(fullname, " "); // Adding space at end of given_name 
    	strcat(fullname, a_family);

	if (c==8) //family
	{
	return a_family;
	}
	else if (c==9) //given
	{
	return a_given;
	}
	else  //c=10 //show
	{
	return fullname;
	}
	
	//It will never reach these free commands. So, I use palloc instead of malloc so, atleast those pointers created using palloc will automatically be freed by Postgres Memory Management System.
	//free(a_given);
	//free(a_family);
	//free(a_show_given);
	//free(fullname);
}

static int pname_compare_datastructure(PersonName* pname1, PersonName* pname2) //For btree ... necessary for ORDER BY
{
 	// NOW I REALIZE IT WOULD HAVE BEEN FAR EASIER TO CREATE A SEPERATE DATA STRUCTURE TO STORE FAMILY NAME AND GIVEN NAME OF A NAME STRING. AAH, LEAVE IT.	
	// I don't need to malloc for storing the given, family names again.. as those strings are created and malloced for in pname_internal_two. Since, the string is returned, i.e. pointer to start of null terminated string is returned, i just need to store the returned pointer start address in a pointer variable. so, when i print this pointer or use string functions like strcmp, it will parse the string from start to the null terminated part automatically. Pointers are amazing things.
	//Remember, postgres FREES THE "ADDRESS" pointed to by the pointers. Even if a pointer address is copied to another pointer ..i.e. both pointers point to same address, Postgres frees the ADDRESS..
	// what we mean by freeing pointers is freeing the starting address from heap. That is how Postgres deals with memory management, even if you create a 1000 pointers by copying address ie. assigning
	// i.e. char* a = char* b ... i.e. all pointers pointing to same address, the "ADDRESS" is freed.
	char* name1_given = pname_internal_two(pname1, 9);
	char* name2_given = pname_internal_two(pname2, 9);
	char* name1_family = pname_internal_two(pname1, 8);
	char* name2_family = pname_internal_two(pname2, 8);
	// if family names are different then return the strcmp value , else return the strcmp of the given names
    	if (strcmp(name1_family, name2_family) != 0) //if true 
		return strcmp(name1_family, name2_family);     //returns 
	else
		return strcmp(name1_given, name2_given);

}

PG_FUNCTION_INFO_V1(pname_lt);

Datum
pname_lt(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(pname_internal(a, b, 1));
}

PG_FUNCTION_INFO_V1(pname_le);

Datum
pname_le(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(pname_internal(a, b, 2));
}

PG_FUNCTION_INFO_V1(pname_eq);

Datum
pname_eq(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);
	
	PG_RETURN_BOOL(pname_internal(a, b, 3));
}

PG_FUNCTION_INFO_V1(pname_ge);

Datum
pname_ge(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(pname_internal(a, b, 4));
}

PG_FUNCTION_INFO_V1(pname_gt);

Datum
pname_gt(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(pname_internal(a, b, 5));
}

PG_FUNCTION_INFO_V1(pname_neq);

Datum
pname_neq(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(pname_internal(a, b, 6));
}

PG_FUNCTION_INFO_V1(pname_family);

Datum
pname_family(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	
	// I am using PG_RETURN_CSTRING. 
	// PG_RETURN_TEXT is used to return a text value directly.
	// PG_RETURN_TEXT_P is used when returning a text value allocated with palloc or char pointer.
	// Keep it simple.
	
	PG_RETURN_TEXT_P(cstring_to_text(pname_internal_two(a, 8)));
}

PG_FUNCTION_INFO_V1(pname_given);

Datum
pname_given(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);

	PG_RETURN_TEXT_P(cstring_to_text(pname_internal_two(a, 9)));
}

PG_FUNCTION_INFO_V1(pname_show);

Datum
pname_show(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);

	PG_RETURN_TEXT_P(cstring_to_text(pname_internal_two(a, 10)));
}


// Using strcmp in the both pname_"compare..." functions for b-tree indexing

PG_FUNCTION_INFO_V1(pname_btree_datastructure);

Datum pname_btree_datastructure(PG_FUNCTION_ARGS)
{
    PersonName *pname1 = (PersonName *) PG_GETARG_POINTER(0);   // no need to free pname1 or pname2 as i am not allocating memory to them i.e. not doing palloc or malloc or calloc etc.
    PersonName *pname2 = (PersonName *) PG_GETARG_POINTER(1);	// Postgres Manages the memory address internally for arguments passed to function like PG_GETARG_POINTER. So, Postgres will free them. I don't have to.
    
    PG_RETURN_INT32(pname_compare_datastructure(pname1, pname2));
}

// Using pname_hash for hash indexing

// c variable is int below. It will store the address.
PG_FUNCTION_INFO_V1(pname_hash);

Datum
pname_hash(PG_FUNCTION_ARGS)
{
    int hash = 5381;  //Dan Bernstein popularized hash ..York university.. saw in many sites..
    int c;
    int i;
    PersonName *pname = (PersonName *) PG_GETARG_POINTER(0);

    whitespace_removal(pname->pname);
    
    for (i=0; i<strlen(pname->pname); i++) 
	{ 
	c = pname->pname[i];    //The ASCII value of the characters in the name is stored in c in each iteration through the characters.
	// This will include the ascii value of comma also for calculation of hash .. but since names are unique, i guess its not an issue.
	hash = ((hash << 5) + hash) + c;     //Bit wise shift operator .. shifting the binary representation of value stored in variable hash to the left by 5 positions. i.e. right padding by 
	}
    PG_RETURN_INT32(hash);
}
