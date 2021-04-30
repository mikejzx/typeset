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

static const int COLUMNS = 80;
static const int INDENT_WIDTH = 2;

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

    // Constant paragraphs for now
    // TODO: read input from files and stdin
    add_para_ni(buf,
            "In old times when wishing still helped one, there lived a "
            "king whose daughters were all beautiful, but the youngest was so "
            "beautiful that the sun itself, which has seen so much, was "
            "astonished whenever it shone in her face.");
    add_para(buf,
            "Close"
            "by the King's castle lay a great dark forest, and under an old "
            "lime-tree in the forest was a well, and when the day was very warm, "
            "the King's child went out into the forest and sat down by the side "
            "of the cool fountain, and when she was dull she took a golden ball, "
            "and threw it up on high and caught it, and this ball was her "
            "favorite plaything.");
    add_para(buf,
            "Now it so happened that on"
            "one occasion the princess's golden ball did not fall into the "
            "little hand which she was holding up for it, but on to the ground "
            "beyond, and rolled straight into the water.");
    add_para(buf,
            "The King's daughter followed it with her eyes, but it vanished, and "
            "the well was deep, so deep that the bottom could not be seen.");
    add_para(buf,
            "On this she began to cry, and cried louder and louder, and "
            "could not be comforted.");
    add_para(buf,
            "And as she thus lamented "
            "some one said to her, ``What ails thee, King's daughter? Thou "
            "weepest so that even a stone would show pity.''");
    add_para(buf,
            "She looked round to the side from whence the voice came, and saw a "
            "frog stretching forth its thick, ugly head from the water.");
    add_para(buf,
            "``Ah! old water-splasher, is it thou?'' said she; ``I am "
            "weeping for my golden ball, which has fallen into the well.''");

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

        // If we have no spaces, or we need too many spaces, just print the
        // line verbatim.
        if (!line_space_count ||
            spaces_needed < 1 ||
            l->force_nojust)
        {
            printf("%s\n", line);
            continue;
        }

        // Hanging punctuation on right-side--we just add an additional space.
        // TODO: clean this up, put in a function, etc.
        // TODO: left-side hanging punct
        // TODO: make toggleable
        if (line[line_length - 1] == ',' ||
            line[line_length - 1] == '.' ||
            line[line_length - 1] == ';' ||
            line[line_length - 1] == ':' ||
            line[line_length - 1] == '"' ||
            line[line_length - 1] == '\'' ||
            line[line_length - 1] == '-')
        {
            ++spaces_needed;
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
}
