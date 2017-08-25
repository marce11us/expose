#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#include "expose_winlist.h"

#ifdef HAVE_GETTEXT
      #define _(str) gettext(str)
      #define d_(str, dom) dgettext(PACKAGE dom, str)
      #define P_(str, str_p, n) ngettext(str, str_p, n)
      #define dP_(str, str_p, n, dom) dngettext(PACKAGE dom, str, str_p, n)
  #else

      #define _(str) (str)
      #define d_(str, dom) (str)
      #define P_(str, str_p, n) (str_p)
      #define dP_(str, str_p, n, dom) (str_p)
  #endif

  /* These macros are used to just mark strings for translation, this is useful
   * for string lists which are not dynamically allocated
   */
  #define N_(str) (str)
  #define NP_(str, str_p) str, str_p


FILE* logfile;

extern const char *_expose_act;
extern E_Action *_act_expose;
E_Config_Dialog *expose_int_config(Evas_Object *parent, const char *params EINA_UNUSED);
#endif
