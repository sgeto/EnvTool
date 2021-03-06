/**
 * \file    ignore.c
 * \ingroup Misc
 * \brief
 *   Support for reading a config-file with things to
 *   ignore at run-time.
 */
#include "envtool.h"
#include "color.h"
#include "smartlist.h"
#include "ignore.h"

const struct search_list sections[] = {
                       { 0, "[Compiler]" },
                       { 1, "[Registry]" },
                       { 2, "[Python]" },
                       { 3, "[PE-resources]" }
                     };

/**\struct ignore_node
 */
struct ignore_node {
       const char *section;  /** The section; one of the above */
       char       *value;
     };

/** A dynamic array of \c "ignore_node".
 */
static smartlist_t *ignore_list;

/**
 * Callback for \c smartlist_read_file():
 *
 * Accepts only strings like "ignore = xx" from the config-file.
 * Add to \c ignore_list with the correct "section".
 *
 * \param[in] sl    the smartlist to add the string-value to.
 * \param[in] line  the prepped string-value from the file opened in
 *                  \c cfg_ignore_init().
 */
static void cfg_parse (smartlist_t *sl, char *line)
{
  char   ignore [256], *p;
  struct ignore_node *node = NULL;
  BOOL   add_it = FALSE;
  static const char *section = NULL;

  strip_nl (line);
  p = str_trim (line);

  if (*p == '[')
  {
    unsigned idx = list_lookup_value (p, sections, DIM(sections));

    if (idx == UINT_MAX)
         WARN ("Ignoring unknown section: %s.\n", p);
    else section = sections[idx].name; /* remember the section for nex line */
    return;
  }

  if (!section)
     return;

  ignore[0] = '\0';

  if (sscanf(p, "ignore = %256s", ignore) == 1 && ignore[0] != '\"')
     add_it = TRUE;

  /**
   * For things to "ignore with spaces", the above \c sscanf() returned with
   * \c "ignore[0] == '\"'" and \b not the full ignore string-value.
   * Therefore use this quite complex \c sscanf() expression to get the whole
   * string-value.
   *   Ref: https://msdn.microsoft.com/en-us/library/xdb9w69d.aspx
   */
  if (ignore[0] == '\"' &&
      sscanf(p, "ignore = \"%256[-_ :()\\/.a-zA-Z0-9^\"]\"", ignore) == 1)
     add_it = TRUE;

  if (add_it)
  {
    node = MALLOC (sizeof(*node));
    node->section = section;
    node->value   = STRDUP (ignore);
    smartlist_add (sl, node);
    DEBUGF (3, "%s: '%s'.\n", section, ignore);
  }
}

/**
 * Try to open and parse a config-file.
 * \param[in] fname  the config-file.
 */
int cfg_ignore_init (const char *fname)
{
  char *file = getenv_expand (fname);

  DEBUGF (3, "file: %s\n", file);
  if (file)
  {
    ignore_list = smartlist_read_file (file, (smartlist_parse_func)cfg_parse);
    FREE (file);
  }
  cfg_ignore_dump();
  return (ignore_list != NULL);
}

/**
 * Lookup a \c value to test for ignore. Compare the \c section too.
 *
 * \param[in] section  Look for the \c value in this section.
 * \param[in] value    The string-value to check.
 *
 * \retval 0 the \c section and \c value was not something to ignore.
 * \retval 1 the \c section and \c value was found in the \c ignore_list.
 */
int cfg_ignore_lookup (const char *section, const char *value)
{
  int i, max;

  if (section[0] != '[' || !ignore_list)
     return (0);

  max = smartlist_len (ignore_list);
  for (i = 0; i < max; i++)
  {
    const struct ignore_node *node = smartlist_get (ignore_list, i);

    if (!stricmp(section, node->section) && !stricmp(value, node->value))
    {
      DEBUGF (3, "Found '%s' in %s.\n", value, section);
      return (1);
    }
  }
  return (0);
}

/**
 * Helper indices for \c cfg_ignore_first() and
 * \c cfg_ignore_next().
 */
static int  next_idx = -1;
static UINT curr_sec = UINT_MAX;

/**
 * Lookup the first ignored \c value in a \c section.
 */
const char *cfg_ignore_first (const char *section)
{
  const struct ignore_node *node;
  int   i, max;
  UINT  idx = list_lookup_value (section, sections, DIM(sections));

  if (idx == UINT_MAX)
  {
    DEBUGF (2, "No such section: %s.\n", section);
    goto not_found;
  }

  max = ignore_list ? smartlist_len (ignore_list) : 0;
  for (i = 0; i < max; i++)
  {
    node = smartlist_get (ignore_list, i);
    if (!stricmp(section, node->section))
    {
      next_idx = i + 1;
      curr_sec = idx;
      return (node->value);
    }
  }

not_found:
  next_idx = -1;
  curr_sec = UINT_MAX;
  return (NULL);
}

/**
 * Lookup the next ignored \c value.
 */
const char *cfg_ignore_next (const char *section)
{
  const struct ignore_node *node;
  int   i, max = smartlist_len (ignore_list);

  /* cfg_ignore_first() not called or no ignorables found
   * by cfg_ignore_first().
   */
  if (next_idx == -1 || curr_sec == UINT_MAX)
     return (NULL);

  for (i = next_idx; i < max; i++)
  {
    node = smartlist_get (ignore_list, i);
    if (!stricmp(sections[curr_sec].name, section) &&
        !stricmp(section, node->section))
    {
      next_idx = i + 1;
      return (node->value);
    }
  }
  next_idx = -1;
  return (NULL);
}

/**
 * Dump all ignored values in all sections.
 */
void cfg_ignore_dump (void)
{
  int i, j;

  for (i = 0; i < DIM(sections); i++)
  {
    const char *ignored, *section = sections[i].name;

    for (j = 0, ignored = cfg_ignore_first(section);
         ignored;
         ignored = cfg_ignore_next(section), j++)
        ;
        DEBUGF (2, "section: %-15s: num: %d.\n", section, j);
  }
}

/**
 * Free the memory allocated in the \c ignore_list smartlist.
 * Called from \c cleanup() in envtool.c.
 */
void cfg_ignore_exit (void)
{
  int i, max;

  if (!ignore_list)
     return;

  max = smartlist_len (ignore_list);
  for (i = 0; i < max; i++)
  {
    struct ignore_node *node = smartlist_get (ignore_list, i);

    DEBUGF (2, "%d: ignore: '%s'\n", i, node->value);
    FREE (node->value);
    FREE (node);
  }
  smartlist_free (ignore_list);
}

