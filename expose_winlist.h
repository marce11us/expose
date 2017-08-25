#ifndef E_WINLIST_H
#define E_WINLIST_H

int expose_init(void);
int expose_shutdown(void);

typedef enum _E_Winlist_Filter {
  E_WINLIST_FILTER_NONE = 0,
  E_WINLIST_FILTER_CLASS_WINDOWS = 1, /* all windows from the same class */
  E_WINLIST_FILTER_CLASSES = 2 /* loop through classes (last selected win) */
} E_Winlist_Filter;

typedef enum E_Winlist_Activate_Type
{
   E_WINLIST_ACTIVATE_TYPE_NONE,
   E_WINLIST_ACTIVATE_TYPE_KEY,
   E_WINLIST_ACTIVATE_TYPE_MOUSE,
   E_WINLIST_ACTIVATE_TYPE_EDGE,
} E_Winlist_Activate_Type;

void expose_winlist_hide(void);
int  expose_winlist_expose(E_Zone *zone, E_Winlist_Filter filter);
void e_winlist_modifiers_set(int mod, E_Winlist_Activate_Type type);
void e_winlist_direction_select(E_Zone *zone, int dir);

#endif
