/*
 * Structure to represent information about a particular file type.
 */
typedef struct file_type {
    char *name;       /* Filename extension for this type. */
    int index;        /* Row/column index into conversion matrix. */
} FILE_TYPE;

/*
 * Structure to represent information about a conversion between types.
 */

typedef struct conversion {
    FILE_TYPE *from;
    FILE_TYPE *to;
    char **cmd_and_args;  /* Command to run to perform the conversion. */
} CONVERSION;

/*
 * Initialize the conversions module.
 *
 * @return 0 if initialization was successful, -1 otherwise.
 */
int conversions_init(void);

/*
 * Finalize the conversions module.
 *
 * @return 0 if finalization was successful, -1 otherwise.
 */
int conversions_fini(void);

/*
 * Define a new file type, with a given name.
 *
 * @param name  The name of the file type (e.g. "PDF")
 * @return a newly created FILE_TYPE object for the specified type name,
 * or NULL, if for some reason (lack of memory or other) a new file type
 * cannot be defined.
 */
FILE_TYPE *define_type(char *name);

/*
 * Lookup a file type by its name.
 *
 * @param name  The name of the file type.
 * @return the FILE_TYPE object for the specified type name, or NULL,
 * if no file type with this name has been defined.
 */
FILE_TYPE *find_type(char *name);

/*
 * Infer the type of a file, by examining its extension.
 *
 * @param filename  The name of a file.
 * @return the FILE_TYPE object for the file, inferred based on any
 * extension (e.g. .pdf) that the filename may have, or NULL if the
 * file type cannot be inferred.
 */
FILE_TYPE *infer_file_type(char *filename);

/*
 * Define a conversion between two file types. If there was previously
 * a conversion between the same two types, it is deleted.
 *
 * @param from  The name of the file type being converted from.
 * @param to  The name of the file type being converted to.
 * @param cmd_and_args  Command for performing the conversion, or NULL,
 * a previously defined conversion is to be deleted.  The command
 * is expected to read data from standard input and write data to
 * standard output, so that it can be used in a pipeline.
 * @return a CONVERSION object that records the information about
 * the conversion.
 */
CONVERSION *define_conversion(char *from, char *to, char *cmd_and_args[]);

/*
 * Search for a conversion path between two file types.
 * Return a NULL-terminated array of CONVERSION objects that describes
 * the conversion path.  It is possible for the array to be empty
 * (i.e. the first entry is NULL); this will occur when the two file
 * types passed are the same, so that no conversion is required.
 * The caller is responsible for freeing the array that is returned
 * by this function, but not the CONVERSION objects pointed to
 * by its entries.
 *
 * @param from  The name of the file type to be converted from.
 * @param to  The name of the file type to be converted to.
 * @return  A pointer to a NULL-terminated array of pointers to
 * CONVERSION objects.
 */
CONVERSION **find_conversion_path(char *from, char *to);
