#include "e.h"
#include "e_mod_main.h"

static void        *_create_data(E_Config_Dialog *cfd);
static void         _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

struct _E_Config_Dialog_Data
{
   int    windows_other_desks;
   int    windows_other_screens;
   int    iconified;
   int    iconified_other_desks;
   int    iconified_other_screens;

   int    focus, raise, uncover;
   int    jump_desk;
   int    move_after_select;

   double scroll_speed;

   struct
   {
      Evas_Object *min_w, *min_h;
   } gui;
};

E_Config_Dialog *
expose_int_config(Evas_Object *parent EINA_UNUSED, const char *params EINA_UNUSED)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "windows/expose")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.create_widgets = _basic_create;
   v->basic.check_changed = _basic_check_changed;

   cfd = e_config_dialog_new(NULL, _("Window Expose Settings"),
                             "E", "windows/expose",
                             "preferences-expose", 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->windows_other_desks =e_config->winlist_list_show_other_desk_windows;
#ifdef _ALL_DESKTOPS_
   cfdata->iconified = e_config->winlist_list_show_iconified;
#endif

}

static void *
_create_data(E_Config_Dialog *cfd EINA_UNUSED)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   if (!cfdata) return NULL;
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd EINA_UNUSED, E_Config_Dialog_Data *cfdata)
{
   free(cfdata);
}

static int
_basic_apply(E_Config_Dialog *cfd EINA_UNUSED, E_Config_Dialog_Data *cfdata)
{
#define DO(_e_config, _cfdata) \
  e_config->winlist_##_e_config = cfdata->_cfdata
   DO(list_show_iconified, iconified);
#ifdef _ALL_DESKTOPS_
   DO(list_show_other_desk_windows, windows_other_desks);
#endif
#undef DO

   e_config_save_queue();

   return 1;
}

static int
_basic_check_changed(E_Config_Dialog *cfd EINA_UNUSED, E_Config_Dialog_Data *cfdata)
{
#define DO(_e_config, _cfdata) \
  if (e_config->winlist_##_e_config != cfdata->_cfdata) return 1
#define DO_DBL(_e_config, _cfdata) \
  if (!EINA_DBL_EQ(e_config->winlist_##_e_config, cfdata->_cfdata)) return 1

   DO(list_show_iconified, iconified);
#ifdef _ALL_DESKTOPS_
   DO(list_show_other_desk_windows, windows_other_desks);
#endif
#undef DO

   return 0;
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd EINA_UNUSED, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *ol, *ob;

   e_dialog_resizable_set(cfd->dia, 1);

   ol = e_widget_list_add(evas, 0, 0);

   ob = e_widget_check_add(evas, _("Selct iconfied windows"), &(cfdata->iconified));
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
#ifdef _ALL_DESKTOPS_
   ob = e_widget_check_add(evas, _("Show windows from all desktops"), &(cfdata->windows_other_desks));
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
#endif
   
   return ol;
}
