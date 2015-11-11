/*
******************************
* Sergey SHPAK, sergey.shpak *
******************************
*/

#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>  // malloc, exit, strtol
#include <assert.h>  // assert
#include <string.h>  // strlen
#include <stdio.h>  // printf, FILE
#include <unistd.h>  // dir

/* We will use this constant to generate masks later */
# define ADDRESS_BIT_LENGTH 32


/* GLOBAL VARIABLES SECTION */

/* We will be stroing masks after their computation 
    to not recompute them again.  */
unsigned long *set_index_mask = NULL;
unsigned long *block_offset_mask = NULL;
unsigned long *line_tag_mask = NULL;

/* In these variables the results of cache modeling
    will be stored */
int cache_hits = 0;
int cache_misses = 0;
int cache_evictions = 0;

/* END OF GLOBAL VARIABLES SECTION */


/* STRUCTURES DESCRIPTION SECTION */

struct cache_line {
/*
    Structure that represents line in cache sets.
*/
    int valid_bit;
    int tag;
    int age;  // field to implement the LRU cache policy
    long *bytes;  // this field is not currently used
};

struct set {
/*
    Structure that represents cache set.
*/
    long index;
    int number_of_lines;
    struct cache_line *lines;
};

struct cache_model {
/*
    Structure that represents cache.
*/
    struct set *sets;
    int number_of_sets;
};

struct passed_args {
/*
    Structure to store arguments passed to the program.
*/
    int help_flag;
    int verbose_flag;

    /* We are using chars here as we assume 
        that the values are not bigger than 1 byte */
    char set_index_bits_num;
    char associativity_num;
    char block_bits_num;

    char *trace_file;
};

/* This enum is used during trace file parsing.
    It represents the set of operations with data. */
enum MemoryAccessOperation {M, L, S};

struct file_line {
/*
    Sturcture to represent a line in the trace file.
*/
    enum MemoryAccessOperation operation;
    unsigned long address;
};

struct address_separated {
/*
    Structure to represent an address in a convenient way to
    find its cache location.
*/
    long set_index;
    long line_index;
    long block_offset;
};

/* STRUCTURES DESCRIPTION SECTION END */


/* ARGUMENTS PARSING SECTION  */

void print_help () {
    printf("USAGE:\n");
    printf("\t-s <numerical_param>\tNumber of set index bits\n");
    printf("\t-E <numerical_param>\tAssociativity (number of lines per set)\n");
    printf("\t-b <numerical_param>\tNumber of block bits\n");
    printf("\t-t <tracefile>\tName of the valgring trace to replay\n");
    printf("\t-h, --help\tPrint this help (optional)\n");
    printf("\t-v\tVerbose flag that displays trace info (optional)\n");
}

/* Further the functions that print errors in case of "exceptions" are implemented */

void no_argument_passed (int arg) {
    printf("The option \"%c\" should be passed with a numerical argument\n", arg);
    exit(EXIT_SUCCESS);
}

void bad_argument_passed () {
    printf("A bad argument was passed\n\n");
    print_help();
    exit(EXIT_SUCCESS);    
}

void numerical_limits_exceeded () {
    printf("The numerical values must lay in the interval [2, 255]\n");
    exit(EXIT_SUCCESS);
}

/* End of "exceptions handlers" */

/* Functions to procees users input */

void validate_string_atona (char *string_to_validate) {
/*
    Function to check if the passed argument can be converted
    to a numertical representation.
*/
    char next_char = string_to_validate[0];
    int string_len;
    int string_idx = 0;

    /* Checking that only positive values are passed  */
    if ('-' == next_char) {
        printf("The numerical values should be positive.\n");
        exit(EXIT_SUCCESS);
    }

    /* Checking for non-numeric symbols  */
    while ('\0' != next_char) {
        if (next_char < 48 || next_char > 57) {
            printf("Some passed numerical values contain non-numeric symbols.\n");
            printf("The program will terminate now\n");
            exit(EXIT_SUCCESS);
        }
        string_idx++;
        next_char = string_to_validate[string_idx];
    }

    /* Checking if passed numerical values are not too big:
         unsigned int should not be written with more than 5 symbols */
    string_len = strlen(string_to_validate);
    if (string_len > 3) {
        numerical_limits_exceeded();
    }
}

void check_result_limits_atona (long unsigned int result) {
/*
    Function to check if the numerical representation of the passed
    argument does not exceed a certain limit.
*/
    if (result > 25) {
        numerical_limits_exceeded();
    }
}

char atona (char *arg_to_parse) {
/*
    Function to convert passed string arguments to their numerical representation.
*/
    int string_length;
    int i;
    char multiplier;
    char current_char;
    int current_char_numeric;
    unsigned int result_internal = 0;
    char result;
    validate_string_atona(arg_to_parse);
    
    /* Convertion starts here */
    string_length = strlen(arg_to_parse);
    multiplier = 1;
    for (i = string_length - 1; i >= 0; i--) {
        current_char = arg_to_parse[i];
        current_char_numeric = current_char - 48;
        result_internal += multiplier * current_char_numeric;
        multiplier *= 10;
    }
    /* Convertion ends here */

    check_result_limits_atona(result_internal);
    result = result_internal;
    return result;
}

void store_char_param (int arg, char *optarg, struct passed_args *args) {
/*
    Function to store a char parameter passed to the program
*/

/* First step: we choose a field in passed_args structure
    that we will modifying by storing its address */
    char *field_to_reference;
    switch (arg) {
        case 's':
            field_to_reference = &(args->set_index_bits_num);
            break;
        case 'E':
            field_to_reference = &(args->associativity_num);
            break;
        case 'b':
            field_to_reference = &(args->block_bits_num);
            break;
        default:
            printf("Default case in store_unsigned_int_param. This should not have happened.\n");
            exit(EXIT_FAILURE);
            break;
    }

/* Second step: we store the numerical representation of the passed argument in the
    corresponding field of the passed_args structure */
    char parsed_arg = atona(optarg);
    *field_to_reference = parsed_arg;
}

void store_string_param (int arg, char *optarg, struct passed_args *args) {
/* 
    Function to store a string parameter passed to the program
*/
    switch (arg) {
        case 't':
            args->trace_file = optarg;
            break;
        default:
            printf("Default case in store_string_param. This should not have happened.\n");
            exit(EXIT_FAILURE);
            break;
    }
}

void check_trace_file_name(char *tracefile) {
/*
    A function to check that the trace file wich name was passsed as an argument
    to the program exists and can be read
*/
    int result = 0;
    result = access(tracefile, F_OK);
    if (0 != result) {
        printf("ERROR: File \"%s\" cannot be found\n", tracefile);
        exit(EXIT_SUCCESS);
    }
    result = access(tracefile, R_OK);
    if (0 != result) {
        printf("ERROR: FILE \"%s\" exists, but cannot be read\n", tracefile);
        exit(EXIT_SUCCESS);
    }
}

void validate_args (struct passed_args *args) {
/*
    A function to validate the arguments, passed to the program.
    If arguments cannot be validated the execution is stopped.
*/

/* The following part is to check that either a help flag was set
    or there is enough arguments to run the program normally */
    if ((1 != args->help_flag) 
        && ((0 == args->set_index_bits_num)
            || (0 == args->associativity_num)
            || (0 == args->block_bits_num)
            || (NULL == args->trace_file))) {
        printf("Not enough parameters are passed.\n\n");
        print_help();
        exit(EXIT_SUCCESS);
    }

    if (NULL != args->trace_file) {
        check_trace_file_name(args->trace_file);
    }
}

struct passed_args *parse_passed_arguments (int argc, char* argv[]) {
/*
    The main function for parsing the arguments passed to the program 
*/
    struct passed_args *args = malloc(sizeof(struct passed_args));
    assert(NULL != args);

    args->help_flag = 0;
    args->verbose_flag = 0;
    args->set_index_bits_num = 0;
    args->associativity_num = 0;
    args->block_bits_num = 0;
    args->trace_file = NULL;
    
    int c;  // getopt_long stores parsed short options here
    const char *short_opts = "hvs:E:b:t:";
    static int help_flag;
    opterr = 0;  // disable printing error messages by getopt_long 
    while (1) {
        static struct option long_options[] = 
            {
                {"help", no_argument, &help_flag, 1},
                {0, 0, 0, 0}
            };

        int option_index = 0;
        c = getopt_long(argc, argv, 
                            short_opts, long_options, &option_index);
        
        /* Detect the end of the options */
        if (-1 == c) {
            break;
        }
        
        switch (c) {
            /* Case for long options */
            case 0:
                break;
            
            case 'h':
                args->help_flag = 1;
                break;
            
            case 'v':
                args->verbose_flag = 1;
                break;
            
            case 's':
            case 'E':
            case 'b':
                if (NULL == optarg) no_argument_passed(c);
                store_char_param (c, optarg, args);
                break;

            case 't':
                if (NULL == optarg) no_argument_passed (c);
                store_string_param (c, optarg, args);
                break;

            case '?':
            default:
                bad_argument_passed();
                break;
        }
    }
    if (help_flag) {
        args->help_flag = 1;
    }
    
    validate_args(args);

    return args;
}

/* END OF ARGUMENTS PARSING SECTION */


/* PASSED ARGUMENTS HELPERS SECTION */

char get_set_bits_num(struct passed_args *args) {
    char set_bits_num = args->set_index_bits_num;
    return set_bits_num;
}

char get_lines_num(struct passed_args *args) {
    char lines_num = args->associativity_num;
    return lines_num;
}

char get_block_bits_num(struct passed_args *args) {
    char block_bits_num = args->block_bits_num;
    return block_bits_num;
}

char *get_trace_file(struct passed_args *args) {
    char *trace_file = args->trace_file;
    return trace_file;
}

int get_help_flag(struct passed_args *args) {
    int help_flag = args->help_flag;
    return help_flag;
}

int get_verbose_flag(struct passed_args *args) {
    int verbose_flag = args->verbose_flag;
    return verbose_flag;
}

/* END OF PASSED ARGUMENTS HELPERS SECTION */


/* CACHE MODEL SECTION */

int two_to_pow(int pow) {
/*
    Function that returns two to the power of pow.
    It is used for computing the number of cache sets or blocks of sets' lines.
*/
    int result = 1;
    int i = 0;
    for (i = 0; i < pow; i++) {
        result *= 2;
    }
    return result;
}

int get_number_of_sets(char set_bits_num) {
/*
    Function for computing the number of sets.
*/
    int number_of_sets = two_to_pow(set_bits_num);
    return number_of_sets;
}

int get_number_of_blocks(char block_bits_num)
/*
    Function for computing the number of blocks.
*/
{
    int number_of_blocks = two_to_pow(block_bits_num);
    return number_of_blocks;
}

/* The following functions are used for generating the cache model */

void generate_line(struct passed_args *args, struct cache_line *ya_line, long tag) {
/*
    Function to generate and initialize a line of cache set
*/
    char block_bits_num = get_block_bits_num(args);
    int number_of_blocks = get_number_of_blocks(block_bits_num);
    long *bytes = calloc(number_of_blocks, sizeof(long));
    ya_line->bytes = bytes;
    ya_line->valid_bit = 0;
    ya_line->tag = tag;
    ya_line->age = 0;
}

void generate_set(struct passed_args *args, struct set *ya_set, long index) {
/*
    Function to generate a cache set
*/
    char number_of_lines = args->associativity_num;
    struct cache_line *lines = malloc(number_of_lines * sizeof(struct cache_line));
    int i;
    ya_set->index = index;
    ya_set->number_of_lines = number_of_lines;

    /* Generating and initializing lines of cache set  */
    for (i = 0; i < number_of_lines; i++) {
        generate_line(args, &lines[i], i);
    }
    ya_set->lines = lines;
}

struct set *generate_sets(struct passed_args *args) {
/*
    Function to generate all cache sets
*/
    int number_of_sets = get_number_of_sets(args->set_index_bits_num);
    struct set *sets = malloc(number_of_sets * sizeof(struct set));
    int i = 0;
    for (i = 0; i < number_of_sets; i++) {
        generate_set(args, &sets[i], i);
    }
    return sets;
}

struct cache_model *create_cache_model(struct passed_args *args) {
/* Main function of the cache model section. Returns cache model, built according to the
    user's "specifications", given as program arguments. */
    struct cache_model *cache = malloc(sizeof(struct cache_model));
    int number_of_sets = get_number_of_sets(args->set_index_bits_num);
    struct set *sets = generate_sets(args);
    cache->number_of_sets = number_of_sets;
    cache->sets = sets;
    return cache;
}

/* End of the subsection where the functions to generate the cache model
    are described */

/* END OF CACHE MODEL SECTION */


/* FILE PARSING SECTION */

int count_lines_to_regard_internal(FILE *fp) {
    /* Count number of lines that are to be taken into consideration by csim.
        That means that the lines starting with 'I' are not counted. */
    int result = 0;
    int last_eol = 0;  // used to store information that previously observed char was '\n'
    char next_char;

    /* Check if the first line is to be counted */
    if (!feof(fp)) {
        next_char = fgetc(fp);
        if (' ' == next_char) {
            result++;
        }
    }

    /* Processing the rest of file */
    while (!feof(fp)) {
        next_char = fgetc(fp);
        if (1 == last_eol) {
            if (' ' == next_char) {
                result++;
            }
            last_eol = 0;
        }
        if ('\n' == next_char) {
            last_eol = 1;
        }
    }

    rewind(fp);  // rewinding file to restore the original pointer
    return result;
}

void skip_to_new_line(FILE *fp) {
/*
    Function to skip a line of the file
*/
    char next_char = '1';
    while (('\n' != next_char) && (EOF != next_char)) {
        next_char = fgetc(fp);
    }
}

enum MemoryAccessOperation get_line_operation(char operation_char) {
/*
    Function to return the operation with the data described on
    a line of trace file
*/
    enum MemoryAccessOperation operation;
    switch (operation_char) {
        case 'M':
        case 'm':
            operation = M;
            break;
        case 'L':
        case 'l':
            operation = L;
            break;
        case 'S':
        case 's':
            operation = S;
            break;
        default:
            printf("In the file being parsed an operation '%c' cannot be recognized.\n", operation_char);
            exit(EXIT_SUCCESS);
            break;
    }
    return operation;
}

/* Functions in this subsection are aimed to check the correctness of addresses
    given in the trace file and to represent them as numerical values */

void verify_address_character(char char_to_verify) {
/*
    Function to check if the address given in the trace file 
    conains only hex characters.
*/
    if (!(
        (char_to_verify >= 48 && char_to_verify <= 57)  // 0..9
        || (char_to_verify >= 65 && char_to_verify <= 70)  // A..F
        || (char_to_verify >= 97 && char_to_verify <= 102)  // a..f
        )) {
        printf("The file being parsed contains an address that is not a hex value: %c\n", char_to_verify);
        exit(EXIT_SUCCESS);
    } 
}

unsigned long hex_to_ulong(char *hex_str) {
/*
    Function to convert string representation of the address
    to a numerical one
*/
    unsigned long result = strtoul(hex_str, NULL, 16);
    return result;
}

long get_line_address(FILE *fp) {
/*
    Function to get a numeric representation of the address
    given on a file string
*/
    char next_char;
    char address_str[8] = {'\0'};
    int i;
    unsigned long result = 0; 
    next_char = fgetc(fp);
    if (' ' != next_char) {
        printf("The file being parsed is badly formatted: no space between an operation and an address ('%c' found instead).\n",
                next_char);
        exit(EXIT_SUCCESS);
    }
    for (i = 0; i < 8; i++) {
        next_char = fgetc(fp);
        if (',' == next_char) {
            break;
        }
        verify_address_character(next_char);
        address_str[i] = next_char;
    }
    result = hex_to_ulong(address_str);
    return result;
}

/* End of addresses processing subsection */

void parse_file_lines(FILE *fp, struct file_line *lines) {
/*
    Function to parse the lines of a trace file and fill
    the passed file_line structure with the results
*/
    char next_char;
    int i = 0;

    while(!feof(fp)) {
        next_char = fgetc(fp);
        if (' ' != next_char) {
        /* If we deal with instruction on the line, 
            we skip to the next one  */
            skip_to_new_line(fp);
            continue;
        }

        /* Reading and storing next line */
        next_char = fgetc(fp);
        lines[i].operation = get_line_operation(next_char);
        lines[i].address = get_line_address(fp);
        i++;

        /* Getting to the next line */
        skip_to_new_line(fp);
    }
    rewind(fp);
}

int count_lines_to_regard(char *file_name) {
/*
    Function to get the number of lines of the trace file
    that will be processed by the program. This function is needed
    to estimate how much memory is to be used to store the trace fle
    representation.
*/
    FILE *fp = fopen(file_name, "r");
    if (NULL == fp) {
        printf("Cannot open file %s\n", file_name);
        exit(EXIT_FAILURE);
    }
    int lines_to_regard_count = count_lines_to_regard_internal(fp);
    fclose(fp);
    return lines_to_regard_count;
}

struct file_line *process_trace_file(char *file_name, int lines_to_regard_count) {
/*
    Main fuction to process a trace file
*/
    FILE *fp = fopen(file_name, "r");
    struct file_line *file_representation;
    if (NULL == fp) {
        printf("Cannot open file %s\n", file_name);
        exit(EXIT_FAILURE);
    }
    file_representation = malloc(lines_to_regard_count * sizeof(struct file_line));
    parse_file_lines(fp, file_representation);
    fclose(fp);
    return file_representation;
}

/* END OF FILE PARSING SECTION */


/* ADDRESS COMPUTING SECTION */

unsigned int generate_addr_mask(int left_offset, int right_offset) {
/* 
    Function for mask generation. 
    left_offset - number of zero most significant bits in the mask.
    right_offset - number of zero least significant bits in the mask. 
*/
    unsigned int right_mask = 1;
    unsigned int mask = 1;
    
    /* Several checkings of variable boundaries.  */
    if (left_offset >= ADDRESS_BIT_LENGTH) {
        mask = 0;  // all 1's 
        return mask;
    }
    if (left_offset <= 0) {
        left_offset = 0;
        mask = 4294967295;
    }
    if (right_offset >= ADDRESS_BIT_LENGTH) {
        mask = 0;
        return mask;
    }
    if (right_offset < 0) {
        right_offset = 0;
    }
    
    /* Case when the mask offsets overlap */
    if (right_offset > (ADDRESS_BIT_LENGTH - left_offset)) return 0;

    right_mask = (right_mask << right_offset) - 1;
    /* Generating mask without considering the right offset */
    if (left_offset != 0) {
        mask = (mask << (ADDRESS_BIT_LENGTH - left_offset)) - 1;
    }
    /* Now we consider the right offset */
    mask = mask - right_mask;
    return mask;
}

unsigned int generate_set_index_mask(struct passed_args *args) {
/* 
    Generate mask for set index extraction.
*/
    char set_bits_num = get_set_bits_num(args);
    char block_bits_num = get_block_bits_num(args);
    char tag_bits_num = ADDRESS_BIT_LENGTH - set_bits_num - block_bits_num;
    unsigned int mask = generate_addr_mask(tag_bits_num, block_bits_num);
    return mask;
}

unsigned int generate_block_offset_mask(struct passed_args *args) {
/*
    Generate mask for block offset value extraction.
*/
    char block_bits_num = get_block_bits_num(args);
    char left_offset_mask = ADDRESS_BIT_LENGTH - block_bits_num;
    unsigned int mask = generate_addr_mask(left_offset_mask, 0);
    return mask;
}

unsigned int generate_tag_mask(struct passed_args *args) {
/*
    Generate mask for line tag extraction.
*/
    char set_bits_num = get_set_bits_num(args);
    char block_bits_num = get_block_bits_num(args);
    unsigned int mask = generate_addr_mask(0, set_bits_num + block_bits_num);
    return mask;
}

long get_set_index(long address, struct passed_args *args) {
/*
    A function to extract the set index from an address
*/
    long set_index;
    char shift_value = get_block_bits_num(args);
    if (NULL == set_index_mask) {
        set_index_mask = malloc(sizeof(unsigned int));
        *set_index_mask = generate_set_index_mask(args);
    }
    set_index = (address & (*set_index_mask)) >> shift_value;
    return set_index;
}

long get_line_tag(long address, struct passed_args *args) {
/*
    A function to extract the line tag from an address
*/
    int line_tag;
    char block_bits_num = get_block_bits_num(args);
    char set_index_bits = get_set_bits_num(args);
    char shift_value = block_bits_num + set_index_bits;
    if (NULL == line_tag_mask) {
        line_tag_mask = malloc(sizeof(unsigned int));
        *line_tag_mask = generate_tag_mask(args);
    }
    line_tag = (address & (*line_tag_mask)) >> shift_value;
    return line_tag;
}

long get_offset(long address, struct passed_args *args) {
/*
    A function to extract the block offset from an address
*/
    int offset;
    if (NULL == block_offset_mask) {
        block_offset_mask = malloc(sizeof(unsigned int));
        *block_offset_mask = generate_block_offset_mask(args);
    }
    offset = address & (*block_offset_mask);
    return offset;
}

struct address_separated *separate_address(long address, struct passed_args *args) {
/*
    A function to separate an address into a set index, line tag and block offset parts
*/
    struct address_separated *separated_address = malloc(sizeof(struct address_separated));
    separated_address->set_index = get_set_index(address, args);
    separated_address->line_index = get_line_tag(address, args);
    separated_address->block_offset = get_offset(address, args);
    return separated_address;
}

/* END OF ADDRESS COMPUTING SECTION */


/* CACHE MANIPULATION SECTION  */

/* Group of "aging" functions.
    Used to implement the "Least Recently Used" policy. */
void age_line(struct cache_line *line) {
/*
    Function to increment the age of a line of set chache
*/
    line->age += 1;
}

void age_lines(struct set *current_set) {
/*
    A function to increment the ages of all lines in the passed set cache.
    Lines that do are not used for data caching are not considered.
*/
    int number_of_lines = current_set->number_of_lines;
    int i;
    struct cache_line *current_line;
    for (i = 0; i < number_of_lines; i++) {
        current_line = &(current_set->lines[i]);
        /* As the concept of age is used to implement
            the LRU Cache policy, we consider only lines
            that cache data */
        if (1 == current_line->valid_bit) {
            age_line(current_line);
        }
    }
}

void age_cache(struct cache_model *cache) {
/*
    Main function to increment the age of all suitablie lines of the cache
*/
    int number_of_sets = cache->number_of_sets;
    int i;
    for (i = 0; i < number_of_sets; i++) {
        age_lines(&(cache->sets[i]));
    }
}

struct cache_line *get_lru_line(struct cache_line *lines_of_set, int number_of_lines,
                                int line_age[]) {
/*
    Function to choose a line in a set of cache lines according to the
    LRU cache policy
*/
    int max_age = 0;
    int current_age;
    struct cache_line current_line;
    int lru_index = 0;
    int i;
    for(i = 0; i < number_of_lines; i++) {
        current_line = lines_of_set[i];
        current_age = current_line.age;
        if (current_age > max_age) {
            max_age = current_age;
            lru_index = i;
        }
    }
    cache_evictions += 1;  // global variable
    return &(lines_of_set[lru_index]);
}

int is_usable(struct cache_line *line) {
/*
    Function to checl if the passed line is not storing any cache data
    and, therfore, can be used to store some
*/
    if (line->valid_bit) {
    /* If valid bit is set, than this line is occupied,
        hence it my not be used directly */
        return 0;
    }
    return 1;
}

struct cache_line *get_line_to_use(struct set *set_to_observe) {
/*
    Main function for choosing a line that will be used for storing
    another portion of data
*/
    int number_of_lines = set_to_observe->number_of_lines;
    int *line_age = calloc(number_of_lines, sizeof(int));
    int i;
    struct cache_line *current_line;
    struct cache_line *result_line;
    for (i = 0; i < number_of_lines; i++) {
        current_line = &(set_to_observe->lines[i]);
        if (is_usable(current_line)) {
            return current_line;
        }
        line_age[i] = current_line->age;
    }
    /* If we are here no usable line was found.
    We will be using the "LRU" cache policy. */
    result_line = get_lru_line(set_to_observe->lines, number_of_lines, line_age);
    free(line_age);
    return result_line;
}

void add_to_set(struct set *set_to_use, struct address_separated *addr_sep, long int address) {
/*
    Function that stimulates the process of adding data stored at the passed address
    to a passed set of the cache
*/
    struct cache_line *line_to_use = get_line_to_use(set_to_use);
    line_to_use->tag = addr_sep->line_index;
    line_to_use->age = 1;
    line_to_use->valid_bit = 1;
}

void add_to_cache(long int address, struct cache_model *cache, struct passed_args *args) {
/*
    A main function to stimulate the process of adding data (stored at the passed address) to cache
*/
    struct address_separated *addr_sep = separate_address(address, args);
    long set_index = addr_sep->set_index;
    struct set *set_to_use = &(cache->sets[set_index]);
    add_to_set(set_to_use, addr_sep, address);
    free(addr_sep);
}

void nullify_age(struct cache_model *cache, long set_index, long line_index) {
/*
    Function to reset the age of a passed line after the data that is "stored"
    at its location was accessed (for the LRU cache policy)
*/
    struct set *set_to_process = &(cache->sets[set_index]);
    struct cache_line *line_to_process = &(set_to_process->lines[line_index]);
    line_to_process->age = 1;
}

int check_validity(struct cache_model *cache, long set_index, long line_tag) {
/*
    Function to check if the data stored at the given address is in the cache
    (interal)
*/
    struct set *set_to_observe = &(cache->sets[set_index]);
    int number_of_lines = set_to_observe->number_of_lines;
    struct cache_line *set_lines = set_to_observe->lines;
    int i;
    int found_line_index = -1;
    long current_line_tag;
    int current_line_valid_bit;
    for (i = 0; i < number_of_lines; i++) {
        current_line_tag = set_lines[i].tag;
        current_line_valid_bit = set_lines[i].valid_bit;
        if (current_line_valid_bit && current_line_tag == line_tag) {
            found_line_index = i;
            break;
        }
    }
    if (-1 == found_line_index) {
        return 0;
    }
    /* We will be nullifying the age of the cache entry here */
    nullify_age(cache, set_index, found_line_index);
    return 1;
}

int is_in_cache(long int address, struct cache_model *cache, struct passed_args *args) {
/*
    Function to check if the data stored at the given address is in the cache
    (external)
*/
    struct address_separated *addr_sep = separate_address(address, args);
    long set_index = addr_sep->set_index;
    long line_tag = addr_sep->line_index;
    int is_valid = check_validity(cache, set_index, line_tag);
    free(addr_sep);
    return is_valid;
}

void make_cache_step(long int address, struct cache_model *cache, struct passed_args *args) {
/*
    A main function to process the lines of the trace file in a sequential way
*/
    age_cache(cache);
    if (is_in_cache(address, cache, args)) {
        cache_hits += 1;
        return;
    }
    cache_misses += 1;
    add_to_cache(address, cache, args);
}

/* END OF CACHE MANIPULATION SECTION  */


int main (int argc, char * argv[])
{
    struct passed_args *args = parse_passed_arguments(argc, argv);
    struct file_line *lines;
    struct cache_model *cache = create_cache_model(args);
    int lines_count;
    int i = 0;

    /* If help flag was passed, print the help message and stop execution */
    if (args->help_flag) {
        print_help();
        return 0;
    }
    lines_count = count_lines_to_regard(args->trace_file);
    lines = process_trace_file(args->trace_file, lines_count);
    for (i = 0; i < lines_count; i++) {
        make_cache_step(lines[i].address, cache, args);
        if (M == lines[i].operation) {
            make_cache_step(lines[i].address, cache, args);
        }
    }
    
    printSummary(cache_hits, cache_misses, cache_evictions);
    return 0;
}
