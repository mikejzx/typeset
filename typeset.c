#include <alloca.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BOOL int
#define TRUE 1
#define FALSE 0

// A single line
typedef struct line
{
    char *data;
    struct line *next;

    BOOL force_nojust;
    BOOL indent;
} line;

// Buffer of lines
typedef struct line_buf
{
    line *head, *tail;
} line_buf;

static  line *line_add(line_buf*, char*, size_t);
static  void  add_paragraph(line_buf*, char*, BOOL, size_t);
#define add_para_ni(b, p) add_paragraph((b), (p), FALSE, strlen(p))
#define add_para(b, p)    add_paragraph((b), (p), TRUE, strlen(p))
static  FILE *cmdline(int, char**);
static  BOOL is_punctuation(char);

static const int COLUMNS = 80;
static const int INDENT_WIDTH = 2;

static BOOL
    do_hangpunct_right = TRUE,
    do_hangpunct_left = TRUE;

/*
 * Entry-point
 *
 * @return status
 */
int main(int argc, char *argv[])
{
    // Initialise line buffer
    line_buf *buf = malloc(sizeof(line_buf));
    assert(buf);

    // Read command line args, and get input file.
    FILE *file = cmdline(argc, argv);
    if (!file)
    {
        return -1;
    }

    // Read from input file into our buffer
    {
        char *this_line = NULL;
        size_t this_line_len = 0;

        size_t len;
        for(char *line; getline(&line, &len, file) != -1;)
        {
            size_t llen = strlen(line);

            // If this is an empty line, clear the current line, and push it
            // as a paragraph
            if (llen == 1 && line[0] == '\n')
            {
                if (this_line)
                {
                    add_paragraph(buf, this_line, TRUE, this_line_len);
                    free(this_line);
                    this_line = NULL;
                    this_line_len = 0;
                }

                continue;
            }

            if (!this_line)
            {
                this_line_len = llen;
                this_line = malloc((this_line_len + 1) * sizeof(char));
                strcpy(this_line, line);
                this_line[strcspn(this_line, "\n")] = ' ';
                this_line[this_line_len] = '\0';
                this_line_len = strlen(this_line);
            }
            else
            {
                this_line_len += llen;
                this_line = realloc(this_line, (this_line_len + 1) * sizeof(char));
                strcat(this_line, line);
                this_line[strcspn(this_line, "\n")] = ' ';
                this_line[this_line_len] = '\0';
                this_line_len = strlen(this_line);
            }

            //add_paragraph(buf, line, TRUE, strlen(line));
        }

        // Add last paragraph
        if (this_line)
        {
            add_paragraph(buf, this_line, TRUE, this_line_len);
            free(this_line);
            this_line = NULL;
            this_line_len = 0;
        }

        fclose(file);
    }

    // Iterate over each line of our text and justify.
    for (line *l = buf->head; l; l = l->next)
    {
        unsigned j;

        char *line = l->data;
        size_t line_length = strlen(line);

        // Find number of spaces we have in the line.
        int line_space_count = 0;
        for (j = 0; j < line_length; ++j)
        {
            if (line[j] == ' ') ++line_space_count;
        }

        // Spaces needed to fully justify the line.
        int spaces_needed = COLUMNS - (int)line_length;

        // Indent first line in paragraph
        if (l->indent)
        {
            spaces_needed -= INDENT_WIDTH;
            for (j = 0; j < INDENT_WIDTH; printf(" "), ++j);
        }

        // On left-side hangpunct, we add a single space, if the line doesn't
        // begin with a punctuation mark.  If it does, we say that we need one
        // extra space.
        if (do_hangpunct_left)
        {
            if (is_punctuation(line[0]))
            {
                printf("%c", line[0]);

                // Move line address over, as we already printed first char.
                ++line;
                ++spaces_needed;
                --line_length;
            }
            else
            {
                printf(" ");
            }
        }

        // Hanging punctuation on right-side--we just add an additional space.
        if (do_hangpunct_right && is_punctuation(line[line_length - 1]))
        {
            ++spaces_needed;
        }

        // If we have no spaces, or we need too many spaces, just print the
        // line verbatim.
        if (!line_space_count ||
            spaces_needed < 1 ||
            l->force_nojust)
        {
            printf("%s\n", line);
            continue;
        }

        // Number of spaces we can add to every existing space in the line.
        size_t even_spaces_total = spaces_needed - spaces_needed % line_space_count;
        size_t even_spaces = even_spaces_total / line_space_count;

        // Number of spaces we need to evenly distribute across the line.
        double remainder_incr = ((double)line_space_count + even_spaces_total) / spaces_needed;
        if (remainder_incr < 0.0) remainder_incr = 0.0;

        // Store all the spacing values, we use char because spacing should
        // never exceed 255.
        unsigned char *spaces = alloca(sizeof(unsigned char) * line_space_count);
        memset(spaces, 1 + even_spaces, line_space_count);

        int spaces_remaining = (int)spaces_needed - even_spaces * line_space_count;

        // This little trick makes lines with "lone spaces" have their space
        // applied at the end of the line.  This should hopefully reduce the
        // "rivers of spaces" effect a little on left-hand-side.
        if (spaces_remaining == 1)
        {
            ++spaces[line_space_count - 1];
            --spaces_remaining;
        }

        // Round-robin/incremental method for the remaining spaces.
        if (remainder_incr > 0.0)
        {
            int index_old = -1;
            for (double inc = 0.0;
                inc < (double)line_space_count && spaces_remaining > 0;
                inc += remainder_incr)
            {
                int index = (int)ceil(inc);

                if (index != index_old)
                {
                    ++spaces[index];
                    --spaces_remaining;
                    index_old = index;
                }
            }
        }

        // Finally, we print out the line.
        unsigned space_idx = 0;
        for (j = 0; j < line_length; ++j)
        {
            if (line[j] == ' ')
            {
                // Print however many spaces are at this point.
                for (unsigned char k = 0; k < spaces[space_idx]; ++k, printf(" "));
                ++space_idx;
                continue;
            }

            printf("%c", line[j]);
        }
        printf("\n");
    }

    return 0;
}

/*
 * Add new line to line buffer.
 *
 * @param buf   Buffer to add to.
 * @param data  Line data.
 */
static line *line_add(line_buf *buf, char *data, size_t len)
{
    // Allocate
    line *l = malloc(sizeof(line));
    assert(l);

    l->data = malloc(len + 1);
    memcpy(l->data, data, len);
    l->next = 0;

    l->force_nojust = FALSE;
    l->indent = FALSE;

    // Set tail
    if (buf->tail)
        buf->tail->next = l;
    else
        buf->head = buf->tail = l;

    buf->tail = l;

    return l;
}

/*
 * Adds a new paragraph to the line buffer, handling line-breaks and
 * indentation.
 *
 * @param buf  Buffer to add to.
 * @param p    Paragraph text as a single line.
 * @param ind  Whether to indent this paragraph or not.
 * @param s    Length of the paragraph line.
 */
static void add_paragraph(line_buf *buf, char *p, BOOL ind, size_t s)
{
    if (!s) s = strlen(p);
    if (!s) return;
    if (p[0] == '\n')
    {
        return;
    }

    // Wrap lines of paragraph.  First line is INDENT_WIDTH shorter than the
    // rest.
    unsigned last = 0;
    line *l;
    for (unsigned i = 0; i < s; i += COLUMNS - (ind ? INDENT_WIDTH : 0))
    {
        // Find end of line (i.e., end of the last word before overflow)
        for (;p[i] != ' ' && i > 0; --i);

        // And go back further until we get to previous word
        // This strips rest of leading space.
        if (i > 0 && p[i] == ' ')
        {
            for (;p[i] == ' ' && i > 0; --i);
            ++i;
        }

        if (i == last) continue;

        l = line_add(buf, &p[0] + last, i - last);

        last = i;

        if (ind)
        {
            // Indent first line of paragraph
            l->indent = TRUE;
            ind = FALSE;
        }

        // Strip trailing spaces on broken line.
        for (;p[last] == ' ' && last < s; ++last);
    }
    l = line_add(buf, &p[0] + last, s - last);
    l->force_nojust = TRUE;

    // Insert line breaks between paragraphs
    // (TODO: make optional)
    line_add(buf, " ", 1);
}

/*
 * Read and interpret command-line arguments.
 *
 * @param argc  Argument count
 * @param argv  Argument vector.
 *
 * @return file pointer to where input data is stored.  Is either a path that
 *         user gave us, or a temporary file that we create with standard input
 *         in it.
 */
static FILE *cmdline(int argc, char* argv[])
{
    FILE *fp = NULL;

    int fileidx = -1;

    int i = 1;
    for (const char *arg = argv[1]; i < argc; arg = argv[++i])
    {
        // Help info.
        if (strcmp(arg, "-h") == 0||
            strcmp(arg, "--help") == 0)
        {
            printf("Help info (TODO)\n");
            return 0;
        }

        // Assume non-arguments is a file path.
        if (fileidx < 0)
        {
            fileidx = i;
        }
    }

    if (fileidx < 0)
    {
        // If we didn't get a file index, read from standard input
        // into a temporary file
        fp = tmpfile();
        if (!fp)
        {
            fprintf(stderr, "typeset: failed to read from stdin\n");
            exit(-1);
        }
        size_t len;
        for(char *line; getline(&line, &len, stdin) != -1; fprintf(fp, "%s", line));
        fflush(fp);
        rewind(fp);
    }
    else
    {
        // Else read from the file we got.
        fp = fopen(argv[fileidx], "rb");
        if (!fp)
        {
            fprintf(stderr, "typeset: %s: no such file\n", argv[fileidx]);
            exit(-1);
        }
    }

    return fp;
}

/*
 * Check if a character is a punctuation mark suitable
 * for hanging punctuation.
 *
 * @param x  Char to check
 *
 * @return TRUE if character is punctuation.
 */
static BOOL is_punctuation(char x)
{
    return
        x == ',' ||
        x == '.' ||
        x == ';' ||
        x == ':' ||
        x == '"' ||
        x == '\'' ||
        x == '-';
}
