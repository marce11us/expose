#include "e.h"
#include "e_mod_main.h"

/* actual module specifics */
static void _expose_mod_action_winlist_cb(E_Object *obj, const char *params);
static Eina_Bool _expose_mod_action_winlist_mouse_cb(E_Object *obj, const char *params, E_Binding_Event_Mouse_Button *ev);
static void _expose_mod_action_winlist_key_cb(E_Object *obj, const char *params, Ecore_Event_Key *ev);
static void _expose_mod_action_winlist_edge_cb(E_Object *obj, const char *params, E_Event_Zone_Edge *ev);
static void _expose_mod_action_winlist_signal_cb(E_Object *obj, const char *params, const char *sig, const char *src);
static void _expose_mod_action_winlist_acpi_cb(E_Object *obj, const char *params, E_Event_Acpi *ev);

static E_Module *conf_module = NULL;
const char *_expose_act = NULL;
E_Action *_act_expose= NULL;

/* module setup */
E_API E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
   "Expose"
};

E_API void *
e_modapi_init(E_Module *m)
{
	logfile=fopen("/home/marcellus/.expose_log", "a+");

   conf_module = m;
   e_configure_registry_category_add("windows", 50, _("Windows"), NULL, "preferences-system-windows");
   e_configure_registry_item_add("windows/expose", 70, _("Expose"), NULL, "preferences-expose", expose_int_config);
   expose_init();
   _expose_act = eina_stringshare_add("expose");
   /* add module supplied action */
   _act_expose = e_action_add(_expose_act);
   if (_act_expose)
     {
        _act_expose->func.go = _expose_mod_action_winlist_cb;
        _act_expose->func.go_mouse = _expose_mod_action_winlist_mouse_cb;
        _act_expose->func.go_key = _expose_mod_action_winlist_key_cb;
        _act_expose->func.go_edge = _expose_mod_action_winlist_edge_cb;
        _act_expose->func.go_signal = _expose_mod_action_winlist_signal_cb;
        _act_expose->func.go_acpi = _expose_mod_action_winlist_acpi_cb;
        e_action_predef_name_set(N_("Expose : List"), N_("Expose"),
                                 "expose", "expose", NULL, 0);
       // e_action_predef_name_set(N_("Expose : List"),
       //                          N_("Next window of same class"), "expose",
       //                          "class-next", NULL, 0);
     }
   e_module_delayed_set(m, 1);
   return m;
}

E_API int
e_modapi_shutdown(E_Module *m EINA_UNUSED)
{
   E_Config_Dialog *cfd;

   /* remove module-supplied action */
   if (_act_expose)
     {
        e_action_predef_name_del("Expose : List", "Expose");
        //e_action_predef_name_del("Expose : List",
        //                         "Next window of same class");
        e_action_del("expose");
        _act_expose = NULL;
     }
   expose_shutdown();

   while ((cfd = e_config_dialog_get("E", "windows/expose")))
     e_object_del(E_OBJECT(cfd));
   e_configure_registry_item_del("windows/expose");
   e_configure_registry_category_del("windows");
   conf_module = NULL;
   eina_stringshare_replace(&_expose_act, NULL);
   return 1;
}

E_API int
e_modapi_save(E_Module *m EINA_UNUSED)
{
   return 1;
}

/* action callback */
static Eina_Bool
_expose_mod_action_winlist_cb_helper(E_Object *obj EINA_UNUSED, const char *params, int modifiers, E_Winlist_Activate_Type type)
{
   fprintf(logfile, "in cb_helper\n");
   fflush(logfile);
   E_Zone *zone = NULL;
   E_Winlist_Filter filter = E_WINLIST_FILTER_NONE;

   zone = e_zone_current_get();
   fprintf(logfile, "before zone\n");
   fflush(logfile);

   if (type)
   e_winlist_modifiers_set(modifiers, type);


   if (!zone) return EINA_FALSE;
   if (params)
     {
        if (!strcmp(params, "expose"))
		{
			expose_winlist_expose(zone, filter);
			return EINA_TRUE;
		}
		else return EINA_FALSE;
     }


   return EINA_TRUE;
}

static void
_expose_mod_action_winlist_cb(E_Object *obj, const char *params)
{
   _expose_mod_action_winlist_cb_helper(obj, params, 0, 0);
}

static Eina_Bool
_expose_mod_action_winlist_mouse_cb(E_Object *obj, const char *params, E_Binding_Event_Mouse_Button *ev)
{
   return _expose_mod_action_winlist_cb_helper(obj, params,
     e_bindings_modifiers_to_ecore_convert(ev->modifiers), E_WINLIST_ACTIVATE_TYPE_MOUSE);
}

static void
_expose_mod_action_winlist_key_cb(E_Object *obj, const char *params, Ecore_Event_Key *ev)
{
   _expose_mod_action_winlist_cb_helper(obj, params, ev->modifiers, E_WINLIST_ACTIVATE_TYPE_KEY);
}

static void
_expose_mod_action_winlist_edge_cb(E_Object *obj, const char *params, E_Event_Zone_Edge *ev)
{
   _expose_mod_action_winlist_cb_helper(obj, params, ev->modifiers, E_WINLIST_ACTIVATE_TYPE_EDGE);
}

static void
_expose_mod_action_winlist_signal_cb(E_Object *obj EINA_UNUSED, const char *params EINA_UNUSED, const char *sig EINA_UNUSED, const char *src EINA_UNUSED)
{
   e_util_dialog_show(_("Expose Error"), _("Expose cannot be activated from a signal binding"));
}

static void
_expose_mod_action_winlist_acpi_cb(E_Object *obj EINA_UNUSED, const char *params EINA_UNUSED, E_Event_Acpi *ev EINA_UNUSED)
{
   e_util_dialog_show(_("Expose Error"), _("Expose cannot be activated from an ACPI binding"));
}
